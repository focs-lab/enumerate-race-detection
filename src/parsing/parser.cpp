#include <cstdint>
#include <fstream>
#include <ios>
#include <iostream>
#include <stack>
#include <unordered_map>
#include <vector>

#include "../common/event.cpp"

void parseBinaryFile(const std::string &filename, std::vector<Event> &events,
                     std::unordered_map<EventIndex, EventIndex> &po,
                     std::unordered_map<EventIndex, uint32_t> &goodWrites) {
  std::unordered_map<uint32_t, std::stack<EventIndex>> threadStacks;
  std::ifstream file(filename, std::ios::binary);

  if (!file.is_open()) {
    std::cerr << "Error opening file: " << filename << std::endl;
    return;
  }

  uint32_t i = 0;
  while (true) {
    uint64_t rawEvent;

    file.read(reinterpret_cast<char *>(&rawEvent), sizeof(rawEvent));
    if (file.eof())
      break;

    if (file.fail()) {
      std::cerr << "Error parsing input file at event " << i << std::endl;
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
    case EventType::Write:
    case EventType::Acquire:
    case EventType::Release:
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
  }

  file.close();

  // 2. Construct PO
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

  ++i;
}
