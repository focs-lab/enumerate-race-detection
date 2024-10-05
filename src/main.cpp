#include <chrono>
#include <cstdint>
#include <iostream>
#include <unordered_map>

#include "common/event.cpp"
#include "consistency_checker/verify_sc.cpp"
#include "parsing/parser.cpp"

void printParsingDebug(std::vector<Event> &allEvents,
                       std::unordered_map<EventIndex, EventIndex> &po,
                       std::unordered_map<EventIndex, uint32_t> &goodWrites);

auto main(int argc, char *argv[]) -> int {
  if (argc < 2) {
    std::cout << "Usage: <input_file>" << std::endl;
    return 0;
  }

  std::string filename = argv[1];

  auto start = std::chrono::high_resolution_clock::now();

  std::vector<Event> allEvents;
  std::unordered_map<EventIndex, EventIndex> po;
  std::unordered_map<EventIndex, uint32_t> goodWrites;

  parseBinaryFile(filename, allEvents, po, goodWrites);

  auto parsingEnd = std::chrono::high_resolution_clock::now();
  std::cout << "Parsing took: "
            << std::chrono::duration_cast<std::chrono::milliseconds>(
                   parsingEnd - start)
                       .count() /
                   1000.0
            << "s" << std::endl
            << std::endl;

  printParsingDebug(allEvents, po, goodWrites);

  bool isConsistent = verify_sc(allEvents, po, goodWrites);

#ifdef DEBUG
  std::cout << std::endl;
#endif

  std::cout << std::boolalpha << "Is sequentially consistent: " << isConsistent
            << std::endl;
  auto end = std::chrono::high_resolution_clock::now();
  std::cout << "Verify_SC took: "
            << std::chrono::duration_cast<std::chrono::milliseconds>(end -
                                                                     parsingEnd)
                       .count() /
                   1000.0
            << "s" << std::endl;
  return 0;
}

void printParsingDebug(std::vector<Event> &allEvents,
                       std::unordered_map<EventIndex, EventIndex> &po,
                       std::unordered_map<EventIndex, uint32_t> &goodWrites) {
#ifdef DEBUG
  std::cout << "Debug enabled" << std::endl;

  std::cout << "Events: " << std::endl;
  for (auto p : allEvents) {
    std::cout << p << std::endl;
  }
  std::cout << std::endl;

  std::cout << "PO: " << std::endl;
  for (auto p : po) {
    std::cout << "[" << allEvents[p.first].prettyString() << ", "
              << allEvents[p.second].prettyString() << "]" << std::endl;
  }
  std::cout << std::endl;

  std::cout << "GW: " << std::endl;
  for (auto p : goodWrites) {
    std::cout << "[" << allEvents[p.first].prettyString() << ", " << p.second
              << "]" << std::endl;
  }
  std::cout << std::endl;
#endif
}
