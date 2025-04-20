#pragma once

#include "event.hpp"
#include "preprocesser.hpp"
#include <vector>

/* Constructs and generates include set for each candidate race.
 *
 * Returns a vector of eid_t that indicates last event that participates for
 * predicting pair (e1, e2). Any events in each thread after eid_t for each
 * thread is not needed for predicting (e1, e2). */
class IncludeSet {
private:
  struct Range {
    tid_t tid;
    eid_t start;
    eid_t end;
  };

  /* Adds range of events to explore next to stack */
  void addRange(EventId e, std::vector<eid_t> &iset, std::stack<Range> &s);

  /* Adds ranges of all good writes of read r to stack */
  void addGoodWrites(EventId e, std::vector<eid_t> &iset, std::stack<Range> &s,
                     std::vector<std::vector<Event>> &events, CommonArg &arg);

  /* Ensures all acquires are closed */
  void matchAcquires(tid_t thread, std::vector<eid_t> &iset,
                     std::stack<Range> &s,
                     std::unordered_map<tid_t, std::unordered_set<vid_t>> &acq,
                     std::unordered_map<tid_t, std::unordered_set<vid_t>> &rel,
                     std::vector<std::vector<Event>> &events, CommonArg &arg);

  /* Adds ranges of all dependencies of e to stack */
  void includeEvent(EventId e, std::vector<eid_t> &iset, std::stack<Range> &s,
                    std::unordered_map<tid_t, std::unordered_set<vid_t>> &acq,
                    std::unordered_map<tid_t, std::unordered_set<vid_t>> &rel,
                    std::vector<std::vector<Event>> &events, CommonArg &arg);

public:
  IncludeSet() = default;

  /* Returns include set for e1, e2 */
  std::vector<eid_t> find(EventId e1, EventId e2,
                          std::vector<std::vector<Event>> &events,
                          CommonArg &arg);
};

inline std::vector<eid_t> getIncludeSet(EventId e1, EventId e2,
                                        std::vector<std::vector<Event>> &events,
                                        CommonArg &arg) {
  IncludeSet finder{};
  return finder.find(e1, e2, events, arg);
}
