#include <cstdint>
#include <fstream>
#include <iostream>
#include <stack>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "../common/event.cpp"

constexpr uint32_t NO_DATA = -1;

// Assumption is locks are acq and rel in pairs
// So regardless of how one window executes, only the init prev locks matter
// execution does not affect the state of the next locks
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

#ifdef DEBUG
  std::unordered_set<uint32_t> vars;
  std::unordered_set<uint32_t> lockVars;
#endif

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
      locks[e.getVarId()]++;
      break;
    case EventType::Release:
      locks[e.getVarId()]--;
      if (locks[e.getVarId()] <= 0)
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

#ifdef DEBUG
    if (e.getEventType() == EventType::Acquire ||
        e.getEventType() == EventType::Release)
      lockVars.insert(e.getVarId());
    else
      vars.insert(e.getVarId());
#endif

    events.push_back(e);
    threadStacks[e.getThreadId()].push(events.size() - 1);

    ++i;
  }

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

#ifdef DEBUG
  std::cout << "Num threads: " << threadStacks.size() << std::endl;
  std::cout << "Num variables: " << vars.size() << std::endl;
  std::cout << "Num locks : " << lockVars.size() << std::endl;

  int gwCnt = 0;
  std::unordered_set<uint32_t> tmp;
  for (auto p : goodWrites) {
    if (tmp.find(p.second) == tmp.end()) {
      tmp.insert(p.second);
      ++gwCnt;
    }
  }
  std::cout << "Num unique good writes: " << gwCnt << std::endl;
#endif
}
