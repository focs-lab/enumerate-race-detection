#include <cstddef>
#include <cstdint>
#include <stack>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "../common/event.cpp"
#include "../common/trace.cpp"
#include "../parsing/parser.cpp"
#include "../utils/map_hash.cpp"

void printParsingDebug(
    std::unordered_map<uint32_t, std::vector<Event>> &threadEvents,
    std::unordered_map<EventId, uint32_t, EventIdHasher, EventIdEqual>
        &goodWrites);

bool verify_sc(std::unordered_map<uint32_t, std::vector<Event>> &threadEvents,
               std::unordered_map<EventId, uint32_t, EventIdHasher,
                                  EventIdEqual> &goodWrites,
               bool &terminated, uint32_t window, size_t windowSize,
               std::unordered_set<Trace, TraceHash> &windowState) {
  std::unordered_set<Trace, TraceHash> nextWindowState;
  std::unordered_set<Trace, TraceHash> seen(windowSize);
  bool res = false;

#ifdef DEBUG
  int i = 1;
#endif

  for (auto prevTrace : windowState) {
    std::stack<Trace> stack;

    // #ifdef DEBUG
    //     std::cout << "Prev Trace: " << std::endl;
    //     for (auto id : prevTrace.getEventIds()) {
    //       std::cout << id.first << ", " << id.second << std::endl;
    //     }
    //     std::cout << std::endl;
    // #endif

    Trace init{prevTrace};
    init.resetDepth();
    stack.push(init);

    while (!stack.empty()) {
      Trace reordering = stack.top();
      stack.pop();

      // #ifdef DEBUG
      //       std::cout << "Node " << i << std::endl;
      //       for (auto id : reordering.getEventIds()) {
      //         std::cout << id.first << ", " << id.second << std::endl;
      //       }
      //       std::cout << std::endl;
      //       ++i;
      // #endif

      if (reordering.isTerminated()) {
        terminated = true;
        return true;
      }

      if (reordering.isFinished(windowSize)) {
        res = true;
        if (nextWindowState.find(reordering) == nextWindowState.end())
          nextWindowState.insert(std::move(reordering));
      }

      for (auto i : reordering.getExecutableEvents(threadEvents, goodWrites)) {
        Trace nextReordering = reordering.appendEvent(threadEvents, i);
        if (seen.find(nextReordering) == seen.end()) {
          stack.push(nextReordering);
          seen.insert(nextReordering);
        }
      }
    }
  }

  windowState = std::move(nextWindowState);
  return res;
}

bool windowing(std::string &filename, size_t windowSize, bool verbose) {
  std::unordered_map<uint32_t, std::vector<Event>> threadEvents;
  std::unordered_map<EventId, uint32_t, EventIdHasher, EventIdEqual> goodWrites;

  parseBinaryFile(filename, threadEvents, goodWrites);

  // Window state
  // Set of event ids + mmap + lockset
  std::unordered_set<Trace, TraceHash> windowState;
  Trace init(threadEvents);
  windowState.insert(init);

  printParsingDebug(threadEvents, goodWrites);

  int window = 0;
  bool terminated = false;

  while (!terminated) {
    if (verbose)
      std::cout << "Window " << window << std::endl;

    bool res = verify_sc(threadEvents, goodWrites, terminated, window,
                         windowSize, windowState);

#ifdef DEBUG
    std::cout << std::boolalpha << "Curr window result: " << res << std::endl
              << std::endl;
#endif

    if (!res) {
      return false;
    }

    ++window;
  }

  return true;
}

void printParsingDebug(
    std::unordered_map<uint32_t, std::vector<Event>> &threadEvents,
    std::unordered_map<EventId, uint32_t, EventIdHasher, EventIdEqual>
        &goodWrites) {
#ifdef DEBUG
  std::cout << "Thread Events: " << std::endl;
  for (auto p : threadEvents) {
    std::cout << "Thread " << p.first << std::endl;
    for (auto e : p.second) {
      std::cout << e << std::endl;
    }
    std::cout << std::endl;
  }
  std::cout << std::endl;
#endif
}
