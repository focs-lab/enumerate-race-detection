#pragma once

#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

enum EventType : uint8_t {
  Read = 0,
  Write = 1,
  Acquire = 2,
  Release = 3,
  Begin = 4,
  End = 5,
  Fork = 6, // Var id holds the tid of the thread to be forked
  Join = 7  // Var Id holds the tid of the thread to wait/join on
};

/* Thread Id of event */
typedef uint32_t tid_t;

/* Variable Id of event */
typedef uint32_t vid_t;

class Event {
private:
  // 4 bits event identifier, 8 bits thread identifier, 20 bits variable
  // identifer, 32 bits variable value
  uint64_t raw_event;
  uint32_t event_num; // ith event in entire input trace, corresponding to ith
                      // line in input trace

public:
  Event() : raw_event(0), event_num(0) {}

  Event(uint64_t raw_event_, uint32_t event_num_)
      : raw_event(raw_event_), event_num(event_num_) {}

  EventType getEventType() const {
    return static_cast<EventType>((raw_event >> 60) & 0xF);
  }

  tid_t getThreadId() const { return (raw_event >> 52) & 0xFF; }
  void setThreadId(tid_t tid) {
    raw_event = (raw_event & ~(0xFFULL << 52)) |
                ((static_cast<uint64_t>(tid) & 0xFF) << 52);
  }

  vid_t getVarId() const { return (raw_event >> 32) & 0xFFFFF; }

  uint32_t getVarValue() const { return raw_event & 0xFFFFFFFF; }

  uint32_t getEventNum() const { return event_num; }

  std::string prettyString() const {
    std::ostringstream oss;
    std::string event_type;
    switch (getEventType()) {
    case EventType::Read:
      event_type = "Read";
      break;
    case EventType::Write:
      event_type = "Write";
      break;
    case EventType::Acquire:
      event_type = "Acquire";
      break;
    case EventType::Release:
      event_type = "Release";
      break;
    case EventType::Begin:
      event_type = "Begin";
      break;
    case EventType::End:
      event_type = "End";
      break;
    case EventType::Fork:
      event_type = "Fork";
      break;
    case EventType::Join:
      event_type = "Join";
      break;
    }

    oss << event_type << " " << getThreadId() << " " << getVarId() << " "
        << getVarValue() << " @" << getEventNum();
    return oss.str();
  }

  static uint64_t createRawEvent(EventType eventType, uint32_t threadId,
                                 uint32_t varId, uint32_t varValue) {
    eventType = static_cast<EventType>(eventType & 0xF);
    threadId = threadId & 0xFF;
    varId = varId & 0xFFFFF;
    varValue = varValue & 0xFFFFFFFF;

    return (static_cast<uint64_t>(eventType) << 60) |
           (static_cast<uint64_t>(threadId) << 52) |
           (static_cast<uint64_t>(varId) << 32) | varValue;
  }
};

std::ostream &operator<<(std::ostream &os, const Event &event);

/* The ith event of a thread in the input trace */
typedef std::vector<Event>::size_type eid_t;

class EventId {
private:
  std::pair<tid_t, eid_t> id;

public:
  EventId() : id{-1, -1} {}
  EventId(tid_t tid_, eid_t eid_) : id{tid_, eid_} {}

  bool operator==(const EventId &other) const { return id == other.id; }
  tid_t getTid() const { return id.first; }
  eid_t getEid() const { return id.second; }
};

/*
** Hash for EventId and candidate races
*/

namespace std {
template <> struct hash<EventId> {
  std::size_t operator()(const EventId &id) const {
    std::size_t h1 = std::hash<tid_t>()(id.getTid());
    std::size_t h2 = std::hash<eid_t>()(id.getEid());
    return h1 ^ (h2 << 1); // Combine hashes of tid and eid
  }
};

template <> struct hash<std::pair<EventId, EventId>> {
  std::size_t operator()(const std::pair<EventId, EventId> &cop) const {
    std::size_t h1 = std::hash<EventId>()(cop.first);
    std::size_t h2 = std::hash<EventId>()(cop.second);
    return h1 ^ (h2 << 1); // Combine hashes of tid and eid
  }
};
} // namespace std

/*
** Utility functions
*/

inline Event getEvent(const std::vector<std::vector<Event>> &events,
                      EventId id) {
  return events[id.getTid()][id.getEid()];
}

const eid_t FIRST_EVENT = 0;
const eid_t UNUSED = -1;
const eid_t TO_BE_FORKED = -2;
const eid_t COMPLETED = -3;

inline bool isSoleWriter(
    Event &w,
    const std::unordered_map<vid_t,
                             std::unordered_map<uint32_t, std::vector<EventId>>>
        &var_to_write_map) {
  return var_to_write_map.at(w.getVarId()).at(w.getVarValue()).size() == 1;
}

/* Creates and orders candidate race e1 and e2 based on order of appearance in
 * input trace */
inline std::pair<EventId, EventId>
makeCOP(std::vector<std::vector<Event>> &events, EventId e1, EventId e2) {
  Event evt1 = getEvent(events, e1);
  Event evt2 = getEvent(events, e2);

  if (evt1.getEventNum() < evt2.getEventNum())
    return {e1, e2};

  return {e2, e1};
}
