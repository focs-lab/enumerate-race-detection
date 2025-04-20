#pragma once

#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "closure.hpp"
#include "event.hpp"

struct CommonArg {
  /* Vector of each threads' events, ordered by program order */
  std::vector<std::vector<Event>> events;

  /* Map of variables to values to write events, notion of GoodWrites */
  std::unordered_map<vid_t, std::unordered_map<uint32_t, std::vector<EventId>>>
      var_to_write_map;

  /* Map of variables to values to read events, the reverse mapping of reads to
   * the writes they can read from */
  std::unordered_map<vid_t, std::unordered_map<uint32_t, std::vector<EventId>>>
      var_to_read_map;

  /* Map of thread name to tid. Optional, this is only used if the thread id
   * from input trace is not serial starting from 0. Pass empty map if not in
   * use. */
  std::unordered_map<uint32_t, tid_t> tid_map;

  /* Map of acquire to matching rel */
  std::unordered_map<EventId, EventId> acq_rel_map;

  /* Map of thread to matching fork */
  std::unordered_map<tid_t, EventId> begin_fork_map;

  /* Transitive, reflexive closure of PO, Fork-Begin, End-Join, RF for
   * sole-writers */
  Closure closure;
};

struct PreprocessResult {
  CommonArg arg;
  std::unordered_set<std::pair<EventId, EventId>> cops;
};

/**
 *  Helper functions for preprocessing
 */

inline bool isSameThread(EventId e1, EventId e2) {
  return e1.getTid() == e2.getTid();
}

inline bool isSameVar(EventId e1, EventId e2,
                      std::vector<std::vector<Event>> &events) {
  return getEvent(events, e1).getVarId() == getEvent(events, e2).getVarId();
}

inline bool hasCommonLock(
    EventId e1, EventId e2,
    std::unordered_map<EventId, std::unordered_set<vid_t>> &event_to_lock_map) {
  if (!event_to_lock_map.contains(e1) || !event_to_lock_map.contains(e2))
    return false;

  for (auto l : event_to_lock_map[e1])
    if (event_to_lock_map[e2].contains(l))
      return true;

  return false;
}

/* Returns if e1 and e2 is ordered by Closure */
inline bool hasHappensBeforeOrdering(EventId e1, EventId e2, Closure &clj) {
  return clj.happensBefore(e1, e2) || clj.happensBefore(e2, e1);
}

/* Filters cop pair (e1, e2), returns false if they are not an actual race */
inline bool isCandidateRace(
    EventId e1, EventId e2, std::vector<std::vector<Event>> &events,
    std::unordered_map<EventId, std::unordered_set<vid_t>> &event_to_lock_map,
    Closure &clj) {
  return !isSameThread(e1, e2) && isSameVar(e1, e2, events) &&
         !hasCommonLock(e1, e2, event_to_lock_map) &&
         !hasHappensBeforeOrdering(e1, e2, clj);
}

/**
 * Declarations for preprocessing functions
 */

/* Preprocesses input traces and extracts relevant information */
PreprocessResult
preprocess(std::vector<std::vector<Event>> &events,
           std::unordered_map<uint32_t, tid_t> &thread_to_tid_map);

/* Transforms and extracts relevant information for preprocessing from input
 * trace
 */
CommonArg initialize(
    std::vector<std::vector<Event>> &events, std::vector<EventId> &writes,
    std::vector<EventId> &reads, std::vector<EventId> &joins,
    std::vector<EventId> &forks,
    std::unordered_map<EventId, std::unordered_set<vid_t>> &event_to_lock_map,
    std::unordered_map<uint32_t, tid_t> &thread_to_tid_map);

/* Generates a set of candidate data races */
std::unordered_set<std::pair<EventId, EventId>> generateCOPs(
    std::vector<std::vector<Event>> &events, std::vector<EventId> &writes,
    std::vector<EventId> &reads,
    std::unordered_map<EventId, std::unordered_set<vid_t>> &event_to_lock_map,
    Closure &clj);

/* Builds Closure based on a vector clock algorithm */
Closure buildClosure(
    std::vector<std::vector<Event>> &events, std::vector<Event> &inputTrace,
    std::unordered_map<vid_t,
                       std::unordered_map<uint32_t, std::vector<EventId>>>
        &var_to_write_map,
    std::unordered_map<vid_t,
                       std::unordered_map<uint32_t, std::vector<EventId>>>
        &var_to_read_map,
    std::vector<EventId> &writes, std::vector<EventId> &reads,
    std::vector<EventId> &joins, std::vector<EventId> &forks,
    std::unordered_map<uint32_t, tid_t> &thread_to_tid_map);
