#pragma once

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <utility>

#include "event.hpp"
#include "preprocesser.hpp"

class Trace {
  std::unordered_map<vid_t, uint32_t> mmap;
  std::unordered_map<vid_t, EventId> locks;
  std::vector<eid_t>
      events; // indices into std::vector<std::vector<Event>> events
  Trace *prev = nullptr;
  uint32_t priority = 0;

  Trace() = default;

  /* Helper functions */
  bool isHeldVar(CommonArg &arg, std::vector<eid_t> &iset, EventId write,
                 EventId e1, EventId e2);

  /* Returns if event e is included in iset. If false, event e does not
   * participate in prediction for race candidate (e1, e2) */
  inline bool isIncluded(EventId e, std::vector<eid_t> &iset) {
    return e.getEid() <= iset[e.getTid()];
  }

  /* Returns if event e has been executed in current Trace */
  inline bool isExecuted(EventId e) {
    return (events[e.getTid()] < COMPLETED && events[e.getTid()] >= e.getEid());
  }

  bool isExecutable(CommonArg &arg, std::vector<eid_t> &iset, EventId id,
                    EventId e1, EventId e2);

  inline bool isEnabled(EventId e) {
    if (e.getEid() != 0 && events[e.getTid()] != e.getEid())
      return false;

    return true;
  }

  /* Methods to compute priority */
  uint32_t computePriority(CommonArg &arg, std::vector<eid_t> &iset, EventId e1,
                           EventId e2);
  uint32_t computeDistance(std::vector<std::vector<Event>> &allEvents,
                           EventId e);
  uint32_t computeUnblockCost(CommonArg &arg, std::vector<eid_t> &iset,
                              EventId e);

public:
  Trace(CommonArg &arg, std::vector<eid_t> &iset) {
    events = std::vector(arg.events.size(), FIRST_EVENT);
    for (tid_t i = 0; i < events.size(); ++i) {
      if (iset[i] == UNUSED) {
        events[i] = UNUSED;
        continue;
      }

      if (arg.begin_fork_map.find(i) != arg.begin_fork_map.end()) {
        events[i] = TO_BE_FORKED;
        continue;
      }
    }
  }

  Trace(const Trace &) = default;
  Trace(Trace &&) = default;

  Trace &operator=(const Trace &other) {
    if (this != &other) {
      mmap = other.mmap;
      locks = other.locks;
      events = other.events;
      prev = other.prev;
    }
    return *this;
  };

  Trace &operator=(Trace &&other) {
    if (this != &other) {
      mmap = std::move(other.mmap);
      locks = std::move(other.locks);
      events = std::move(other.events);
      prev = other.prev;
    }
    return *this;
  }

  /* Returns ptr to next reordering to be explored */
  std::shared_ptr<Trace> appendEvent(CommonArg &arg, std::vector<eid_t> &iset,
                                     EventId id, EventId e1, EventId e2);

  /* Returns list of events executable in Trace */
  std::vector<EventId> getExecutableEvents(CommonArg &arg,
                                           std::vector<eid_t> &iset, EventId e1,
                                           EventId e2);

  /* Squash Reads to reduce redundant interleavings */
  void advanceReads(CommonArg &arg, std::vector<eid_t> &iset, EventId e1,
                    EventId e2);

  /* Returns the sequence of events executed in the current trace */
  std::vector<Event> getWitness(std::vector<std::vector<Event>> &event);

  inline bool isWitness(EventId e1, EventId e2) {
    if (!isEnabled(e1) || !isEnabled(e2))
      return false;

    return true;
  }

  void printTrace();

  bool operator==(const Trace &other) const {
    if (mmap != other.mmap)
      return false;

    if (events != other.events) {
      return false;
    }

    return true;
  }

  friend struct std::hash<Trace>;
  friend struct TracePtrCmp;
};

namespace std {
template <> struct hash<Trace> {
  std::size_t operator()(const Trace &trace) const {
    size_t prime = 31;
    size_t hash = 1;

    size_t mmapHash = trace.mmap.size();
    for (const auto &[key, value] : trace.mmap) {
      mmapHash ^= std::hash<uint32_t>()(key) ^
                  std::hash<uint32_t>()(value) + 0x9e3779b9 + (mmapHash << 6) +
                      (mmapHash >> 2); // Combine hashes
    }

    size_t eventHash = trace.events.size();
    for (tid_t i = 0; i < trace.events.size(); ++i) {
      auto idx = trace.events[i];
      eventHash ^= std::hash<EventId>()({i, idx}) + 0x9e3779b9 +
                   (eventHash << 6) + (eventHash >> 2); // Combine hashes
    }

    hash = prime * hash + mmapHash;
    hash = prime * hash + eventHash;

    return hash;
  }
};

template <> struct std::hash<std::shared_ptr<Trace>> {
  size_t operator()(const std::shared_ptr<Trace> &ptr) const {
    return std::hash<Trace>()(*ptr);
  }
};

template <> struct equal_to<std::shared_ptr<Trace>> {
  bool operator()(const std::shared_ptr<Trace> &lhs,
                  const std::shared_ptr<Trace> &rhs) const {
    return *lhs == *rhs; // Compare the values inside unique_ptr
  }
};
// template <> struct std::hash<std::unique_ptr<Trace>> {
//   size_t operator()(const std::unique_ptr<Trace> &ptr) const {
//     return std::hash<Trace>()(*ptr);
//   }
// };

// template <> struct equal_to<std::unique_ptr<Trace>> {
//   bool operator()(const std::unique_ptr<Trace> &lhs,
//                   const std::unique_ptr<Trace> &rhs) const {
//     return *lhs == *rhs; // Compare the values inside unique_ptr
//   }
// };
} // namespace std

struct TracePtrCmp {
  bool operator()(const std::shared_ptr<Trace> &ptr1,
                  const std::shared_ptr<Trace> &ptr2) {
    // Min-heap: higher priority comes first
    return ptr1->priority > ptr2->priority;
  }
};
