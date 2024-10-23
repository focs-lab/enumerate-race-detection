#pragma once

#include <cstdint>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "event.cpp"

class Trace;

struct TraceHash {
  size_t operator()(const Trace &trace) const;
};

class Trace {
  std::unordered_map<uint32_t, uint32_t> mmap;
  std::unordered_set<uint32_t> lockset;
  std::unordered_set<EventId, EventIdHasher, EventIdEqual>
      events; // indices into threadEvents
  uint32_t depth = 0;

public:
  Trace(std::unordered_map<uint32_t, std::vector<Event>> &threadEvents) {
    for (auto p : threadEvents) {
      events.insert(std::make_pair(p.first, 0));
    }
  }

  Trace(const Trace &) = default;
  Trace(Trace &&) = default;
  Trace &operator=(const Trace &) = default;
  Trace &operator=(Trace &&) = default;
  friend struct TraceHash;

  Trace
  appendEvent(std::unordered_map<uint32_t, std::vector<Event>> &threadEvents,
              EventId id) {
    Trace t(*this);
    t.events.erase(id);

    size_t s = threadEvents[id.first].size();
    if (id.second + 1 != s)
      t.events.insert(std::make_pair(id.first, id.second + 1));
    else
      t.events.insert(std::make_pair(id.first, -1));

    Event &event = threadEvents[id.first][id.second];
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

    ++t.depth;
    return t;
  }

  std::vector<EventId> getExecutableEvents(
      std::unordered_map<uint32_t, std::vector<Event>> &threadEvents,
      std::unordered_map<EventId, uint32_t, EventIdHasher, EventIdEqual>
          &goodWrites) {
    std::vector<EventId> executables;
    for (const auto id : events) {
      if (id.second == -1)
        continue;

      Event event = threadEvents[id.first][id.second];
      switch (event.getEventType()) {
      case EventType::Acquire:
        if (lockset.find(event.getVarId()) == lockset.end())
          executables.push_back(id);
        break;
      case EventType::Release:
        if (lockset.find(event.getVarId()) != lockset.end())
          executables.push_back(id);
        break;
      case EventType::Write:
        executables.push_back(id);
        break;
      case EventType::Read:
        // Do nothing
        if (mmap[event.getVarId()] == goodWrites[id])
          executables.push_back(id);
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

  std::unordered_map<uint32_t, uint32_t> getMmap() { return mmap; }

  bool isTerminated() {
    for (auto p : events) {
      if (p.second != -1)
        return false;
    }
    return true;
  }

  bool isFinished(size_t windowSize) { return depth == windowSize; }

  void resetDepth() { depth = 0; }

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

  std::unordered_set<EventId, EventIdHasher, EventIdEqual> &getEventIds() {
    return events;
  }

  uint32_t getDepth() {
    return depth;
  }
};

size_t TraceHash::operator()(const Trace &trace) const {
  size_t prime = 31;
  size_t hash = 1;

  size_t mmapHash = trace.mmap.size();
  for (const auto &[key, value] : trace.mmap) {
    mmapHash ^= std::hash<uint32_t>()(key) ^
                std::hash<uint32_t>()(value) + 0x9e3779b9 + (mmapHash << 6) +
                    (mmapHash >> 2); // Combine hashes
  }

  size_t eventHash = trace.events.size();
  for (const auto idx : trace.events) {
    eventHash ^= EventIdHasher()(idx) + 0x9e3779b9 + (eventHash << 6) +
                 (eventHash >> 2); // Combine hashes
  }

  hash = prime * hash + mmapHash;
  hash = prime * hash + eventHash;

  return hash;
};
