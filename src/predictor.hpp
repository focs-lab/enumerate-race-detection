#pragma once

#include "config.hpp"
#include "event.hpp"
#include "parser.hpp"
#include "preprocesser.hpp"
#include "trace.hpp"
#include <atomic>
#include <chrono>
#include <format>
#include <memory>
#include <ratio>
#include <thread>
#include <unordered_map>
#include <vector>

/*
** Functions for data race prediction
*/

/* Returns if a given pair is a data race, and the number of nodes explored */
std::pair<bool, uint32_t> verifySC(EventId e1, EventId e2, CommonArg &arg,
                                   std::vector<eid_t> &includeSet,
                                   Option &opts);

/* Wrapper function - generates an IncludeSet for e1, e2 before calling verifySC
 */
std::pair<bool, uint32_t> isDataRace(EventId e1, EventId e2, CommonArg &arg,
                                     Option &opts);

void generateWitness(std::vector<std::vector<Event>> &events,
                     std::shared_ptr<Trace> t, EventId e1, EventId e2,
                     Option &opts);

/* Returns num_thread to concurrently execute race prediction */
inline size_t getNumThreads(std::vector<std::pair<EventId, EventId>> &cops,
                            Option &opts) {
  if (opts.num_threads.has_value())
    return opts.num_threads.value();

  auto MAX_THREADS = 64; // TODO: Tweak accordingly
  auto min_per_thread = 1;
  auto max_threads = cops.size() / min_per_thread;
  max_threads = max_threads >= MAX_THREADS ? MAX_THREADS : max_threads;

  auto hardware_threads = std::thread::hardware_concurrency();
  hardware_threads = hardware_threads == 0 ? MAX_THREADS : hardware_threads;

  size_t num_threads =
      hardware_threads > max_threads ? max_threads : hardware_threads;

  return num_threads;
}

class Predictor {
private:
  std::vector<std::pair<EventId, EventId>> races;
  std::vector<std::vector<Event>> events;
  std::unordered_map<uint32_t, tid_t> thread_to_tid_map;
  Option opts;

  void predictPar(CommonArg &arg,
                  std::vector<std::pair<EventId, EventId>> &cops,
                  Option &opts) {
    std::mutex race_mutex;
    std::mutex io_mutex; // Mutex for ensuring serial output to cout
    std::atomic<size_t> idx{0};

    std::vector<std::thread> workers;
    size_t num_threads = getNumThreads(cops, opts);

    auto worker = [&]() {
      while (true) {
        size_t i = idx.fetch_add(1, std::memory_order_relaxed);

        if (i >= cops.size())
          return; // Stop if out of bounds

        std::chrono::time_point<
            std::chrono::steady_clock,
            std::chrono::duration<long long, std::ratio<1LL, 1000000000LL>>>
            start;

        if (opts.verbose)
          start = std::chrono::high_resolution_clock::now();

        auto [isRace, nodesExplored] =
            isDataRace(cops[i].first, cops[i].second, arg, opts);

        if (isRace) {
          std::lock_guard<std::mutex> lock{race_mutex};
          races.push_back(cops[i]);
        }

        if (opts.verbose) {
          auto duration =
              std::chrono::duration_cast<std::chrono::milliseconds>(
                  std::chrono::high_resolution_clock::now() - start) /
              1000.0;
          std::string msg = std::format(
              "Pair {}\nNodes explored: {}\nTime taken ({}, {}): {}\n\n", i,
              nodesExplored, getEvent(events, cops[i].first).getEventNum(),
              getEvent(events, cops[i].second).getEventNum(), duration.count());

          std::lock_guard<std::mutex> lock{io_mutex};
          std::cout << msg;
        }
      }
    };

    for (size_t i = 0; i < num_threads; ++i) {
      workers.emplace_back(worker);
    }

    for (auto &t : workers) {
      t.join();
    }
  }

public:
  Predictor(ParseResult &pr, Option &opts_)
      : events{pr.events}, thread_to_tid_map{pr.thread_to_tid_map},
        opts{opts_} {}

  void predict() {
    PreprocessResult pr = preprocess(events, thread_to_tid_map);

    // auto i = 0;
    if (opts.verbose) {
      std::cout << "Preprocessing complete, num cops: " << pr.cops.size()
                << std::endl;
      std::cout << "Starting race prediction..." << std::endl << std::endl;
    }

    auto cops = std::vector<std::pair<EventId, EventId>>(pr.cops.begin(),
                                                         pr.cops.end());
    predictPar(pr.arg, cops, opts);
  }

  void reportRaces(Option &opt) {
    std::cout << "Num races: " << races.size() << std::endl;

    if (opt.verbose) {
      std::cout
          << "------------------------------------------------------------"
          << std::endl;
      for (auto p : races) {
        Event e1 = getEvent(events, p.first);
        Event e2 = getEvent(events, p.second);
        std::cout << '(' << e1.getEventNum() << ", " << e2.getEventNum() << ')'
                  << std::endl;
      }
    }
  }
};
