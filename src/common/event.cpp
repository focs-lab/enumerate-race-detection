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
  Fork = 6,
  Join = 7
};

class Event {
private:
  // 4 bits event identifier, 8 bits thread identifier, 20 bits variable
  // identifer, 32 bits variable value
  uint64_t raw_event;

public:
  Event() : raw_event(0) {}

  Event(uint64_t raw_event_) : raw_event(raw_event_) {}

  EventType getEventType() const {
    return static_cast<EventType>((raw_event >> 60) & 0xF);
  }

  uint32_t getThreadId() const { return (raw_event >> 52) & 0xFF; }

  uint32_t getVarId() const { return (raw_event >> 32) & 0xFFFFF; }

  uint32_t getVarValue() const { return raw_event & 0xFFFFFFFF; }

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
        << getVarValue();
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

std::ostream &operator<<(std::ostream &os, const Event &event) {
  return os << event.prettyString();
}

typedef std::vector<Event>::size_type EventIndex;
