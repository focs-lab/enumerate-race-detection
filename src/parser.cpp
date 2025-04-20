#include <cstdint>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <unordered_map>
#include <utility>

#include "errors.hpp"
#include "parser.hpp"

ParseResult parse(const std::string &filename) {
  std::ifstream file{filename, std::ios::binary};

  if (!file.is_open()) {
    std::cerr << "Error opening file: " << filename << std::endl;
    throw std::runtime_error{"Failed to open file" + filename};
  }

  std::vector<std::vector<Event>> events;
  std::unordered_map<uint32_t, tid_t>
      thread_to_tid_map; // map of thread_id to tid (ensures tid is serial)

  uint32_t i = 0;
  while (true) {
    uint64_t rawEvent = 0;

    file.read(reinterpret_cast<char *>(&rawEvent), sizeof(rawEvent));
    if (file.eof())
      break;

    if (file.fail()) {
      throw EncodingError{i, rawEvent};
    }

    // Parsing logic here
    Event e = Event{rawEvent, i++};
    // std::cout << "Event: " << e << std::endl;

    auto [it, isInserted] = thread_to_tid_map.try_emplace(
        e.getThreadId(), thread_to_tid_map.size());
    if (isInserted)
      events.push_back(std::vector<Event>());

    tid_t tid = it->second;
    e.setThreadId(tid); // overwrite existing thread id based on tid_t
    events[tid].push_back(e);
  }

  file.close();

  // for (auto p : thread_to_tid_map) {
  //   std::cout << p.first << ", " << p.second << std::endl;
  // }

  return {events, thread_to_tid_map};
}
