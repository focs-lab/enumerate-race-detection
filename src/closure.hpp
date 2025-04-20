#pragma once

#include "event.hpp"
#include <algorithm>
#include <cassert>
#include <unordered_map>
#include <vector>

class Closure {
private:
  /* Vector Clock representation */
  class Clock {
  private:
    std::vector<uint32_t> ts;

  public:
    Clock(uint32_t numThreads) {
      for (auto i = 0; i < numThreads; ++i)
        ts.push_back(0);
    }

    Clock(Clock &&) = default;
    Clock(const Clock &other) : ts{other.ts} {}

    Clock &operator=(const Clock &other) {
      if (this != &other) {
        ts = other.ts;
      }
      return *this;
    }

    Clock &operator=(Clock &&other) {
      if (this != &other) {
        ts = std::move(other.ts);
      }
      return *this;
    }

    bool operator==(const Clock &other) const { return ts == other.ts; }

    uint32_t &operator[](size_t index) { return ts[index]; }

    /* Clocks c1 < c2 for event e1, e2 iff e1 is ordered before e2 by Closure */
    bool operator<(const Clock &other) const {
      for (auto i = 0; i < ts.size(); ++i)
        if (ts[i] > other.ts[i])
          return false;

      return true;
    }

    Clock join(const Clock &other) {
      Clock joined{static_cast<uint32_t>(ts.size())};
      for (auto i = 0; i < ts.size(); ++i) {
        joined[i] = std::max(ts[i], other.ts[i]);
      }

      return joined;
    }

    Clock incr(const tid_t tid) {
      Clock newClock{*this};
      ++newClock[tid];
      return newClock;
    }

    const std::vector<uint32_t> getTimestamps() { return ts; }
  };

  std::unordered_map<EventId, Clock> event_clocks;
  std::unordered_map<EventId, std::vector<EventId>> transitive_reduction;

public:
  Closure() = default;
  Closure(
      std::unordered_map<EventId, Clock> clocks_,
      std::unordered_map<EventId, std::vector<EventId>> transitive_reduction_)
      : event_clocks{clocks_}, transitive_reduction{transitive_reduction_} {}
  Closure(const Closure &) = default;
  Closure(Closure &&) = default;
  Closure &operator=(const Closure &) = default;
  Closure &operator=(Closure &&) = default;

  /* Returns if e1 < e2 */
  bool happensBefore(const EventId &e1, const EventId &e2) {
    return event_clocks.at(e1) < event_clocks.at(e2);
  }

  /* Returns transitive reduction of Closure for event e. I.e., direct
   * "dependencies" that must happen before e. */
  std::vector<EventId> &getHappensBefore(const EventId &e) {
    return transitive_reduction[e];
  }

  class Builder {
  private:
    std::unordered_map<EventId, std::vector<EventId>> transitive_reduction;
    uint32_t numThreads;

  public:
    Builder(uint32_t numThreads_) : numThreads{numThreads_} {}

    /* Add partial ordering in which e2 happens before e1 */
    void addRelation(const EventId &e1, const EventId &e2) {
      transitive_reduction[e1].push_back(e2);
    }

    /* Vector clock algorithm to compute Closure */
    Closure build(std::vector<Event> &events,
                  std::vector<std::vector<Event>> &allEvents) {
      std::unordered_map<EventId, Clock> clocks;
      std::vector<Clock> threadClocks;
      std::vector<eid_t> eids;

      for (auto i = 0; i < numThreads; ++i) {
        eids.push_back(0);
        threadClocks.push_back(Clock{numThreads});
      }

      for (auto e : events) {
        tid_t tid = e.getThreadId();
        eid_t eid = eids[tid];

        Clock c = threadClocks[tid];
        c = c.incr(tid);
        threadClocks[tid] = c;

        // Join clocks
        for (auto prevEvent : transitive_reduction[EventId{tid, eid}]) {
          assert(clocks.find(prevEvent) != clocks.end());
          c = c.join(clocks.at(prevEvent));
        }

        clocks.emplace(EventId{tid, eid}, c);

        ++eids[tid];
      }

      return Closure{clocks, transitive_reduction};
    }
  };
};
