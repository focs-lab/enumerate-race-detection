#include <cstdint>
#include <fstream>
#include <iostream>
#include <stack>
#include <unordered_map>
#include <vector>

#include "../common/event.cpp"

constexpr uint32_t NO_DATA = -1;

void parseBinaryFile(std::ifstream &file, std::vector<Event> &events,
                     std::unordered_map<EventIndex, EventIndex> &po,
                     std::unordered_map<EventIndex, uint32_t> &goodWrites,
                     std::unordered_map<uint32_t, uint32_t> &locks,
                     uint32_t window, size_t windowSize) {
  std::unordered_map<uint32_t, std::stack<EventIndex>> threadStacks;

  // Reset state
  po.clear();
  events.clear();
  goodWrites.clear();

  uint32_t i = 0;
  while (i < windowSize) {
    uint64_t rawEvent;

    file.read(reinterpret_cast<char *>(&rawEvent), sizeof(rawEvent));
    if (file.eof())
      break;

    if (file.fail()) {
      std::cerr << "Error parsing input file at event "
                << window * windowSize + i << std::endl;
      std::cerr << "Raw Event is: " << rawEvent << std::endl;
      break;
    }

    // Parsing logic here
    Event e = Event(rawEvent);

    switch (e.getEventType()) {
    case EventType::Read:
      // Update GoodWrites
      goodWrites[events.size()] = e.getVarValue();
      break;
    case EventType::Acquire:
      locks[e.getVarId()] = e.getThreadId();
      break;
    case EventType::Release:
      locks.erase(e.getVarId());
      break;
    case EventType::Write:
      break;
    case EventType::Fork:
    case EventType::Begin:
    case EventType::End:
    case EventType::Join:
      // Ignore these currently
      continue;
    default:
      break;
    }

    events.push_back(e);
    threadStacks[e.getThreadId()].push(events.size() - 1);

    ++i;
  }

  // 2. Handle unreleased locks
  for (auto p : locks) {
    uint64_t raw_event =
        Event::createRawEvent(EventType::Release, p.second, p.first, NO_DATA);
    Event e = Event(raw_event);
    events.push_back(e);
    threadStacks[p.second].push(events.size() - 1);
  }

  // 3. Construct PO
  for (auto &pair : threadStacks) {
    std::stack<EventIndex> &stack = pair.second;
    EventIndex next = stack.top();
    stack.pop();

    while (!stack.empty()) {
      EventIndex curr = stack.top();
      stack.pop();
      po.insert(std::make_pair(curr, next));
      next = curr;
    }
  }
}
