#include <cstdint>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <utility>
#include <vector>

#include "../common/event.cpp"

void parseBinaryFile(
    const std::string &filename,
    std::unordered_map<uint32_t, std::vector<Event>> &threadEvents,
    std::unordered_map<EventId, uint32_t, EventIdHasher, EventIdEqual>
        &goodWrites) {
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
    case EventType::Read: {
      // Update GoodWrites
      EventId id =
          std::make_pair(e.getThreadId(), threadEvents[e.getThreadId()].size());
      goodWrites[id] = e.getVarValue();
      break;
    }
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

    threadEvents[e.getThreadId()].push_back(e);

    ++i;
  }

  file.close();
}
