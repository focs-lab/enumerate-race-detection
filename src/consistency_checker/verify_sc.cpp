#include <chrono>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <ios>
#include <stack>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "../common/event.cpp"
#include "../common/trace.cpp"
#include "../parsing/parser.cpp"
#include "../utils/map_hash.cpp"
#include "estimate.cpp"

void printParsingDebug(
    std::vector<Event> &allEvents,
    std::unordered_map<EventIndex, EventIndex> &po,
    std::unordered_map<EventIndex, uint32_t> &goodWrites,
    std::unordered_map<uint32_t, uint32_t> &prevLocks,
    std::unordered_set<std::unordered_map<uint32_t, uint32_t>, MapHasher,
                       MapEqual> &prevMmaps);

bool verify_sc(std::vector<Event> &allEvents,
               std::unordered_map<EventIndex, EventIndex> &po,
               std::unordered_map<EventIndex, uint32_t> &goodWrites,
               std::unordered_map<uint32_t, uint32_t> &prevLocks,
               std::unordered_set<std::unordered_map<uint32_t, uint32_t>,
                                  MapHasher, MapEqual> &prevMmaps) {
  std::unordered_set<std::unordered_map<uint32_t, uint32_t>, MapHasher,
                     MapEqual>
      nextMmaps;
  bool res = false;

  uint64_t j = 0;
  uint64_t k = 0;
#ifdef DEBUG
  uint64_t i = 0;
  int seenSize = 0;
  int stackSize = 0;

  std::cout << "Num mmaps: " << prevMmaps.size() << std::endl;
#endif

  // Extra check here to guard against locks not acq/rel in pairs
  for (auto p : prevLocks) {
    if (p.second > 1) {
#ifdef DEBUG
      std::cout << "Max stack size in window: " << stackSize << std::endl;
      std::cout << "Max seen size in window: " << seenSize << std::endl;
#endif
      return false;
    }
  }

  for (auto prevMmap : prevMmaps) {
    uint64_t x = 0;
    uint64_t y = 0;
    uint64_t z = 0;
    std::stack<Trace> stack;
    std::unordered_set<Trace, TraceHash> seen(allEvents.size());

    Trace init{allEvents, po, prevLocks, prevMmap};
    stack.push(init);

    while (!stack.empty()) {
#ifdef DEBUG
      ++i;
      stackSize = stack.size() > stackSize ? stack.size() : stackSize;
      seenSize = seen.size() > seenSize ? seen.size() : seenSize;
#endif
      Trace reordering = stack.top();
      stack.pop();

      // #ifdef DEBUG
      //       std::cout << "Node " << i << std::endl;
      //       for (auto i : reordering.getEventIds()) {
      //         std::cout << allEvents[i] << std::endl;
      //       }
      //       std::cout << std::endl;
      //       ++i;
      // #endif

      if (reordering.isFinished()) {
        res = true;
        if (nextMmaps.find(reordering.getMmap()) == nextMmaps.end())
          nextMmaps.insert(reordering.getMmap());
      }

      for (auto i : reordering.getExecutableEvents(allEvents, goodWrites)) {
        Trace nextReordering = reordering.appendEvent(allEvents, i, po);
        if (seen.find(nextReordering) == seen.end()) {
          stack.push(nextReordering);
          seen.insert(nextReordering);
        } else {
          ++y;
        }
        // ++z;
        // stack.push(nextReordering);
      }
    }

#ifdef DEBUG
    // i = x > i ? x : i;
    j = y > j ? y : j;
    k = z > k ? z : k;
#endif
  }

#ifdef DEBUG
  std::cout << "Max stack size in window: " << stackSize << std::endl;
  std::cout << "Max seen size in window: " << seenSize << std::endl;
  std::cout << "Node explored: " << i << std::endl;
  std::cout << "Seen before: " << j << std::endl;
  // std::cout << "z: " << k << std::endl;
#endif

  prevMmaps = std::move(nextMmaps);
  return res;
}

bool windowing(std::string &filename, size_t windowSize, bool verbose) {
  std::vector<Event> allEvents;
  std::unordered_map<EventIndex, EventIndex> po;
  std::unordered_map<EventIndex, uint32_t> goodWrites;
  std::unordered_map<uint32_t, uint32_t> nextLocks;
  std::unordered_set<std::unordered_map<uint32_t, uint32_t>, MapHasher,
                     MapEqual>
      prevMmaps;

  std::ifstream file(filename, std::ios::binary);
  if (!file.is_open()) {
    std::cerr << "Error opening file: " << filename << std::endl;
    return 1;
  }

  allEvents.reserve(windowSize);
  std::unordered_map<uint32_t, uint32_t> initMmap;
  prevMmaps.insert(std::move(initMmap));

  int window = 0;
  while (!file.eof()) {
#ifdef DEBUG
    auto start = std::chrono::high_resolution_clock::now();
#endif

    if (verbose)
      std::cout << "Window " << window << std::endl;

    std::unordered_map<uint32_t, uint32_t> currLocks = nextLocks;

    parseBinaryFile(file, allEvents, po, goodWrites, nextLocks, window,
                    windowSize);

    // printParsingDebug(allEvents, po, goodWrites, nextLocks, prevMmaps);

#ifdef DEBUG
    std::cout << "Estimate Cost: "
              << estimate_cost(allEvents, po, goodWrites, currLocks, prevMmaps)
              << std::endl;
    std::cout << "Estimate Time Taken: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(
                     std::chrono::high_resolution_clock::now() - start)
                         .count() /
                     1000.0
              << std::endl;
#endif

    bool res = verify_sc(allEvents, po, goodWrites, currLocks, prevMmaps);

#ifdef DEBUG
    auto end = std::chrono::high_resolution_clock::now();
    // std::cout << "Num mmaps: " << prevMmaps.size() << std::endl;
    std::cout << "Time taken: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end -
                                                                       start)
                         .count() /
                     1000.0
              << std::endl;
    std::cout << std::boolalpha << "Curr window result: " << res << std::endl
              << std::endl;
#endif

    if (!res) {
      file.close();
      return false;
    }

    ++window;
  }

  file.close();
  return true;
}

void printParsingDebug(
    std::vector<Event> &allEvents,
    std::unordered_map<EventIndex, EventIndex> &po,
    std::unordered_map<EventIndex, uint32_t> &goodWrites,
    std::unordered_map<uint32_t, uint32_t> &nextLocks,
    std::unordered_set<std::unordered_map<uint32_t, uint32_t>, MapHasher,
                       MapEqual> &prevMmaps) {
#ifdef DEBUG
  std::cout << "Events: " << std::endl;
  for (auto p : allEvents) {
    std::cout << p << std::endl;
  }
  std::cout << std::endl;

  std::cout << "PO: " << std::endl;
  for (auto p : po) {
    // std::cout << "[" << p.first << ", " << p.second << "]" << std::endl;
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

  std::cout << "Next locks: " << std::endl;
  for (auto p : nextLocks) {
    std::cout << "[Lock " << p.first << ", count " << p.second << "]"
              << std::endl;
  }
  std::cout << std::endl;

  std::cout << "Prev Mmaps: " << std::endl;
  for (auto m : prevMmaps) {
    std::cout << "[";
    for (auto p : m) {
      std::cout << "(Var: " << p.first << ", data: " << p.second << ")"
                << std::endl;
    }

    std::cout << "]" << std::endl;
  }
#endif
}
