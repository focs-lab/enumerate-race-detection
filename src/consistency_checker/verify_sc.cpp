#include <cstdint>
#include <stack>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "../common/event.cpp"
#include "../common/trace.cpp"

bool verify_sc(std::vector<Event> &allEvents,
               std::unordered_map<EventIndex, EventIndex> &po,
               std::unordered_map<EventIndex, uint32_t> &goodWrites) {

  std::stack<Trace> stack;
  std::unordered_set<Trace, TraceHash> seen(allEvents.size());

  Trace init{allEvents, po};
  stack.push(init);

#ifdef DEBUG
  int i = 1;
#endif

  while (!stack.empty()) {
    Trace reordering = stack.top();
    stack.pop();

#ifdef DEBUG
    std::cout << "Node " << i << std::endl;
    ++i;
#endif

    if (reordering.isFinished()) {
      return true;
    }

    for (auto i : reordering.getExecutableEvents(allEvents, goodWrites)) {
      Trace nextReordering = reordering.appendEvent(allEvents, i, po);
      if (seen.find(nextReordering) == seen.end()) {
        stack.push(nextReordering);
        seen.insert(nextReordering);
      }
    }
  }

  return false;
}
