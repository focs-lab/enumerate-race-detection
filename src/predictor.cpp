#include "predictor.hpp"
#include "config.hpp"
#include "event.hpp"
#include "iset.hpp"
#include "trace.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <numeric>
#include <queue>
#include <vector>

std::pair<bool, uint32_t> isDataRace(EventId e1, EventId e2, CommonArg &arg,
                                     Option &opts) {
  std::vector<eid_t> includeSet = getIncludeSet(e1, e2, arg.events, arg);
  return verifySC(e1, e2, arg, includeSet, opts);
}

std::pair<bool, uint32_t> verifySC(EventId e1, EventId e2, CommonArg &arg,
                                   std::vector<eid_t> &includeSet,
                                   Option &opt) {
  size_t totalSize =
      std::accumulate(arg.events.begin(), arg.events.end(), 0,
                      [](size_t sum, const std::vector<Event> &thread) {
                        return sum + thread.size();
                      });
  uint64_t i = 1; // Track number of nodes explored

  std::unordered_set<std::shared_ptr<Trace>> seen(totalSize);
  std::priority_queue<std::shared_ptr<Trace>,
                      std::vector<std::shared_ptr<Trace>>, TracePtrCmp>
      pq;

  // 1. Initialize empty trace
  std::shared_ptr<Trace> init = std::make_shared<Trace>(arg, includeSet);
  pq.push(init);

  while (!pq.empty()) {
    std::shared_ptr<Trace> reordering = pq.top();
    pq.pop();
    ++i;

    // 2. Check curr reordering is witness
    if (reordering->isWitness(e1, e2)) {
      if (opt.witness) {
        generateWitness(arg.events, reordering, e1, e2, opt);
      }
      return {true, i};
    }

    // 3. Execute all executable events
    for (auto i : reordering->getExecutableEvents(arg, includeSet, e1, e2)) {
      std::shared_ptr<Trace> nextReordering =
          reordering->appendEvent(arg, includeSet, i, e1, e2);

      if (seen.find(nextReordering) == seen.end()) {
        pq.push(nextReordering);
        seen.insert(nextReordering);
      }
    }
  }

  return {false, i};
}

/* Generate and writes witness to output file dir */
void generateWitness(std::vector<std::vector<Event>> &events,
                     std::shared_ptr<Trace> t, EventId e1, EventId e2,
                     Option &opts) {
  std::vector<Event> witness = t->getWitness(events);
  // std::cout << "Witness received, now writing..." << std::endl;

  // Construct output file name
  Event event1 = getEvent(events, e1);
  Event event2 = getEvent(events, e2);
  std::string filename = std::to_string(event1.getEventNum()) + "_" +
                         std::to_string(event2.getEventNum()) + ".txt";

  // std::cout << "generating witness!" << std::endl;
  // std::cout << filename << std::endl;

  std::string outputDir = opts.outputDir.value_or("witness");
  if (!std::filesystem::exists(outputDir)) {
    std::filesystem::create_directory(outputDir);
  }

  std::filesystem::path outputFilePath =
      std::filesystem::path(outputDir) / filename;

  // std::ofstream outputFile(outputFilePath, std::ios::binary);
  std::ofstream outputFile{outputFilePath};

  if (!outputFile.is_open()) {
    std::cerr << "Error opening file to generate witness" << std::endl;
    return;
  }

  for (auto e : witness) {
    try {
      outputFile << e << std::endl;
      // // For binary file output
      // outputFile.write(reinterpret_cast<const char *>(&rawEvent),
      //                  sizeof(rawEvent));
    } catch (const std::runtime_error &e) {
      std::cerr << e.what() << std::endl;
      outputFile.close();
      return;
    }
  }

  outputFile.close();
}

std::vector<Event>
Trace::getWitness(std::vector<std::vector<Event>> &allEvents) {
  std::vector<Event> witness;

  Trace *curr = this;
  while (curr->prev != nullptr) {
    for (tid_t i = events.size() - 1; i >= 0 && i < static_cast<tid_t>(-1);
         --i) {
      for (eid_t j = curr->events[i] - 1; j >= prev->events[i] && j < COMPLETED;
           --j) {
        witness.push_back(getEvent(allEvents, {i, j}));
      }
    }

    curr = curr->prev;
  }

  return witness;
}
