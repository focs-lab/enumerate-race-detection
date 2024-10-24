#pragma once

#include <cstdint>
#include <iostream>
#include <unordered_map>
#include <unordered_set>

#include "event.cpp"

class Trace;

struct TraceHash {
  size_t operator()(const Trace &trace) const;
};

class Trace {
  std::unordered_map<uint32_t, long long> mmap;
  std::unordered_set<uint32_t> lockset;
  std::unordered_set<EventIndex> events; // indices into allEvents vector

public:
  Trace(std::vector<Event> &allEvents,
        std::unordered_map<EventIndex, EventIndex> &po) {
    std::unordered_set<EventIndex> tmp;
    for (const auto &pair : po) {
      tmp.insert(pair.second);
    }

    // Insert first event of each thread into events
    for (EventIndex i = 0; i < allEvents.size(); ++i) {
      if (tmp.find(i) == tmp.end()) {
        events.insert(i);
      }
    }
  }

  Trace(const Trace &) = default;
  Trace(Trace &&) = default;
  Trace &operator=(const Trace &) = default;
  Trace &operator=(Trace &&) = default;
  friend struct TraceHash;

  Trace appendEvent(std::vector<Event> &allEvents, EventIndex idx,
                    std::unordered_map<EventIndex, EventIndex> &po) {
    Trace t(*this);
    t.events.erase(idx);

    if (po.find(idx) != po.end())
      t.events.insert(po[idx]);

    Event &event = allEvents[idx];
    switch (event.getEventType()) {
    case EventType::Acquire:
      t.lockset.insert(event.getVarId());
      break;
    case EventType::Release:
      t.lockset.erase(event.getVarId());
      break;
    case EventType::Write:
      t.mmap[event.getVarId()] = event.getVarValue();
      break;
    case EventType::Read:
      // Do nothing
      break;
    default:
      // Nothing should come here
      std::cerr << "Error generating new trace node: unknown event type - "
                << event.getEventType() << std::endl;
      break;
    }

    return t;
  }

  std::vector<EventIndex>
  getExecutableEvents(std::vector<Event> &allEvents,
                      std::unordered_map<EventIndex, uint32_t> &goodWrites) {
    std::vector<EventIndex> executables;
    for (const auto idx : events) {
      Event event = allEvents[idx];
      switch (event.getEventType()) {
      case EventType::Acquire:
        if (lockset.find(event.getVarId()) == lockset.end())
          executables.push_back(idx);
        break;
      case EventType::Release:
        if (lockset.find(event.getVarId()) != lockset.end())
          executables.push_back(idx);
        break;
      case EventType::Write:
        executables.push_back(idx);
        break;
      case EventType::Read:
        // Do nothing
        if (mmap[event.getVarId()] == goodWrites[idx])
          executables.push_back(idx);
        break;
      default:
        // Nothing should come here
        std::cerr << "Error generating new trace node: unknown event type - "
                  << event.getEventType() << std::endl;
        break;
      }
    }

    return executables;
  }

  bool isFinished() { return events.empty(); }

  bool operator==(const Trace &other) const {
    if (mmap != other.mmap)
      return false;

    if (events.size() != other.events.size())
      return false;

    for (const auto e : events) {
      if (other.events.find(e) == other.events.end())
        return false;
    }

    return true;
  }

  std::unordered_set<EventIndex> &getEventIds() { return events; }
};

size_t TraceHash::operator()(const Trace &trace) const {
  size_t prime = 31;
  size_t hash = 1;

  size_t mmapHash = trace.mmap.size();
  for (const auto &[key, value] : trace.mmap) {
    mmapHash ^= std::hash<uint8_t>()(key) ^
                std::hash<long long>()(value) + 0x9e3779b9 + (mmapHash << 6) +
                    (mmapHash >> 2); // Combine hashes
  }

  size_t eventHash = trace.events.size();
  for (const auto idx : trace.events) {
    eventHash ^= std::hash<EventIndex>()(idx) + 0x9e3779b9 + (eventHash << 6) +
                 (eventHash >> 2); // Combine hashes
  }

  hash = prime * hash + mmapHash;
  hash = prime * hash + eventHash;

  return hash;
};
