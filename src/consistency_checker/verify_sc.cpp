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

constexpr size_t LOCKS_OFFSET = 10;

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
  std::unordered_set<Trace, TraceHash> seen(allEvents.size());
  bool res = false;

#ifdef DEBUG
  int i = 1;
#endif

  for (auto prevMmap : prevMmaps) {
    std::stack<Trace> stack;

    Trace init{allEvents, po, prevLocks, prevMmap};
    stack.push(init);

    while (!stack.empty()) {
      Trace reordering = stack.top();
      stack.pop();

#ifdef DEBUG
      std::cout << "Node " << i << std::endl;
      for (auto i : reordering.getEventIds()) {
        std::cout << allEvents[i] << std::endl;
      }
      std::cout << std::endl;
      ++i;
#endif

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
        }
      }
    }
  }

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

  allEvents.reserve(windowSize + LOCKS_OFFSET);
  std::unordered_map<uint32_t, uint32_t> initMmap;
  prevMmaps.insert(initMmap);

  int window = 0;
  while (!file.eof()) {
    if (verbose)
      std::cout << "Window " << window << std::endl;

    std::unordered_map<uint32_t, uint32_t> currLocks = nextLocks;

    parseBinaryFile(file, allEvents, po, goodWrites, nextLocks, window,
                    windowSize);

    printParsingDebug(allEvents, po, goodWrites, nextLocks, prevMmaps);

    bool res = verify_sc(allEvents, po, goodWrites, currLocks, prevMmaps);

#ifdef DEBUG
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
    std::cout << "[Lock " << p.first << ", thread " << p.second << "]"
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
