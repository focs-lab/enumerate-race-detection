#include <cstdint>
#include <stack>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "../common/event.cpp"
#include "../common/trace.cpp"
#include "../utils/map_hash.cpp"
#include "../utils/random.cpp"

const uint32_t NUM_ATTEMPTS = 100;
const uint32_t NODE_COST = 1;
const uint32_t FLATTEN_DEPTH = 5;

uint64_t random_walk(std::vector<Event> &allEvents,
                     std::unordered_map<EventIndex, EventIndex> &po,
                     std::unordered_map<EventIndex, uint32_t> &goodWrites,
                     std::unordered_map<uint32_t, uint32_t> &prevLocks,
                     std::unordered_set<std::unordered_map<uint32_t, uint32_t>,
                                        MapHasher, MapEqual> &prevMmaps) {
  // Num of nodes explored
  std::vector<uint32_t> depths;
  std::vector<uint32_t> degrees;

  uint64_t cost = 1;
  uint64_t degree = 1;
  uint32_t depth = 0;

  uint32_t t = 0;

  // Extra check here to guard against locks not acq/rel in pairs
  for (auto p : prevLocks) {
    if (p.second > 1) {
      return cost;
    }
  }

  std::vector<std::unordered_map<uint32_t, uint32_t>> mmaps(prevMmaps.begin(),
                                                            prevMmaps.end());
  std::unordered_map<uint32_t, uint32_t> mmap =
      mmaps[get_random_int(0, mmaps.size())];
  Trace curr{allEvents, po, prevLocks, mmap};

  // std::cout << std::endl << 'a' << std::endl;
  // for (auto p : curr.getEventIds()) {
  //   std::cout << p << ',';
  // }
  // std::cout << std::endl;

  int i = 0;
  while (!curr.isFinished()) {
    ++i;
    // std::cout << i << '-';
    std::stack<std::pair<Trace, uint32_t>> stack;
    std::unordered_set<Trace, TraceHash> seen;
    uint32_t currCost = 0;
    uint32_t currDegree = 0;
    std::vector<Trace> nextTraces;

    stack.push(std::make_pair<>(curr, 0));

    while (!stack.empty()) {
      auto currNode = stack.top();
      Trace reordering = currNode.first;
      uint32_t depth = currNode.second;
      stack.pop();

      if (reordering.isFinished()) {
        return cost + currCost;
      }
      ++currCost;

      std::vector<EventIndex> executableEvents =
          reordering.getExecutableEvents(allEvents, goodWrites);
      if (executableEvents.empty())
        continue;

      EventIndex nextEvent =
          executableEvents[get_random_int(0, executableEvents.size())];
      Trace nextReordering = reordering.appendEvent(allEvents, nextEvent, po);

      if (depth == FLATTEN_DEPTH) {
        currDegree += executableEvents.size();
        nextTraces.push_back(nextReordering);
      } else {
        if (seen.find(nextReordering) == seen.end()) {
          seen.insert(nextReordering);
          stack.push(std::make_pair(nextReordering, depth + 1));
        }
      }
    }

    degree *= currDegree;
    cost += degree * currCost;
    if (nextTraces.size() <= 0)
      return cost;

    curr = nextTraces[get_random_int(0, nextTraces.size())];
    // std::cout << "c" << std::endl;
    // for (auto p : curr.getEventIds()) {
    //   std::cout << p << ',';
    // }
    // std::cout << std::endl;
    // std::cout << "isFin: " << curr.isFinished() << std::endl;
  }

  // std::cout << "HELLO" << std::endl;
  // std::vector<EventIndex> executableEvents =
  //     curr.getExecutableEvents(allEvents, goodWrites);

  // while (executableEvents.size() > 0) {
  //   degrees.push_back(executableEvents.size());
  //   int currDegree = executableEvents.size();

  //   degree *= currDegree;
  //   cost += (degree * NODE_COST);
  //   ++depth;

  //   EventIndex nextEvent =
  //       executableEvents[get_random_int(0, executableEvents.size())];
  //   Trace nextReordering = curr.appendEvent(allEvents, nextEvent, po);

  //   if (nextReordering.isFinished())
  //     return cost;

  //   curr = std::move(nextReordering);
  //   executableEvents =
  //       std::move(curr.getExecutableEvents(allEvents, goodWrites));
  // }

  // analyse_vector(degrees);
  // std::cout << depth << std::endl;
  // std::cout << "Random Walk: (" << cost << ", " << depth << ',' << t << ")"
  //           << std::endl;
  // std::cout << depth << ",";
  return cost;
}

double estimate_cost(std::vector<Event> &allEvents,
                     std::unordered_map<EventIndex, EventIndex> &po,
                     std::unordered_map<EventIndex, uint32_t> &goodWrites,
                     std::unordered_map<uint32_t, uint32_t> &prevLocks,
                     std::unordered_set<std::unordered_map<uint32_t, uint32_t>,
                                        MapHasher, MapEqual> &prevMmaps) {
  uint64_t total = 0;

  // std::cout << std::endl;
  // std::cout << std::endl << "Random Walk: (";
  // std::cout << std::endl;
  for (auto i = 0; i < NUM_ATTEMPTS; ++i) {
    total += random_walk(allEvents, po, goodWrites, prevLocks, prevMmaps);
  }

  // std::cout << ")" << std::endl;
  // std::cout << std::endl;

  return total / (double)NUM_ATTEMPTS * prevMmaps.size();
}

// uint64_t random_walk(std::vector<Event> &allEvents,
//                      std::unordered_map<EventIndex, EventIndex> &po,
//                      std::unordered_map<EventIndex, uint32_t> &goodWrites,
//                      std::unordered_map<uint32_t, uint32_t> &prevLocks,
//                      std::unordered_set<std::unordered_map<uint32_t,
//                      uint32_t>,
//                                         MapHasher, MapEqual> &prevMmaps) {
//   // Num of nodes explored
//   std::vector<uint32_t> depths;
//   std::vector<uint32_t> degrees;

//   uint64_t cost = 1;
//   uint64_t degree = 1;
//   uint32_t depth = 0;

//   uint32_t t = 0;

//   // Extra check here to guard against locks not acq/rel in pairs
//   for (auto p : prevLocks) {
//     if (p.second > 1) {
//       return cost;
//     }
//   }

//   std::vector<std::unordered_map<uint32_t, uint32_t>>
//   mmaps(prevMmaps.begin(),
//                                                             prevMmaps.end());
//   std::unordered_map<uint32_t, uint32_t> mmap =
//       mmaps[get_random_int(0, mmaps.size())];

//   std::stack<Trace> stack;
//   Trace curr{allEvents, po, prevLocks, mmap};
//   std::vector<EventIndex> executableEvents =
//       curr.getExecutableEvents(allEvents, goodWrites);

//   while (executableEvents.size() > 0) {
//     degrees.push_back(executableEvents.size());
//     int currDegree = executableEvents.size();

//     degree *= currDegree;
//     cost += (degree * NODE_COST);
//     ++depth;

//     EventIndex nextEvent =
//         executableEvents[get_random_int(0, executableEvents.size())];
//     Trace nextReordering = curr.appendEvent(allEvents, nextEvent, po);

//     curr = std::move(nextReordering);
//     executableEvents =
//         std::move(curr.getExecutableEvents(allEvents, goodWrites));
//   }

//   // analyse_vector(degrees);
//   // std::cout << depth << std::endl;
//   // std::cout << "Random Walk: (" << cost << ", " << depth << ',' << t <<
//   ")"
//   //           << std::endl;
//   // std::cout << depth << ",";
//   return cost;
// }

// uint64_t random_walk(std::vector<Event> &allEvents,
//                      std::unordered_map<EventIndex, EventIndex> &po,
//                      std::unordered_map<EventIndex, uint32_t> &goodWrites,
//                      std::unordered_map<uint32_t, uint32_t> &prevLocks,
//                      std::unordered_set<std::unordered_map<uint32_t,
//                      uint32_t>,
//                                         MapHasher, MapEqual> &prevMmaps,
//                      std::unordered_set<Trace, TraceHash> &seen) {
//   // Num of nodes explored
//   std::vector<uint32_t> depths;
//   std::vector<uint32_t> degrees;

//   uint64_t cost = 1;
//   uint64_t degree = 1;
//   uint32_t depth = 0;

//   uint32_t t = 0;

//   // Extra check here to guard against locks not acq/rel in pairs
//   for (auto p : prevLocks) {
//     if (p.second > 1) {
//       return cost;
//     }
//   }

//   std::vector<std::unordered_map<uint32_t, uint32_t>>
//   mmaps(prevMmaps.begin(),
//                                                             prevMmaps.end());
//   std::unordered_map<uint32_t, uint32_t> mmap =
//       mmaps[get_random_int(0, mmaps.size())];

//   Trace curr{allEvents, po, prevLocks, mmap};
//   std::vector<EventIndex> executableEvents =
//       curr.getExecutableEvents(allEvents, goodWrites);
//   // std::vector<EventIndex> executableEvents =
//   //     curr.getPOExecutableEvents(allEvents, goodWrites);

//   while (executableEvents.size() > 0) {
//     degrees.push_back(executableEvents.size());
//     int currDegree = executableEvents.size();
//     // int currDegree = executableEvents.size() / 2;
//     // if (currDegree == 0)
//     //   currDegree = 1;
//     degree *= currDegree;
//     cost += (degree * NODE_COST);
//     ++depth;
//     // std::cout << "Walk 1 node: (" << executableEvents.size() << ", " <<
//     // degree
//     //           << ")" << std::endl;

//     EventIndex nextEvent =
//         executableEvents[get_random_int(0, executableEvents.size())];
//     // std::vector<EventIndex> exe =
//     //     curr.getExecutableEvents(allEvents, goodWrites);
//     // i29.656f (std::find(exe.begin(), exe.end(), nextEvent) == exe.end()) {
//     //   std::cout << depth << ",";
//     //   return cost;
//     // }
//     Trace nextReordering = curr.appendEvent(allEvents, nextEvent, po);
//     if (seen.find(nextReordering) == seen.end()) {
//       seen.insert(nextReordering);
//     } else {
//       ++t;
//       return cost;
//     }

//     curr = std::move(nextReordering);
//     executableEvents =
//         std::move(curr.getExecutableEvents(allEvents, goodWrites));
//   }

//   // analyse_vector(degrees);
//   // std::cout << depth << std::endl;
//   // std::cout << "Random Walk: (" << cost << ", " << depth << ',' << t <<
//   ")"
//   //           << std::endl;
//   // std::cout << depth << ",";
//   return cost;
// }

// double estimate_cost(std::vector<Event> &allEvents,
//                      std::unordered_map<EventIndex, EventIndex> &po,
//                      std::unordered_map<EventIndex, uint32_t> &goodWrites,
//                      std::unordered_map<uint32_t, uint32_t> &prevLocks,
//                      std::unordered_set<std::unordered_map<uint32_t,
//                      uint32_t>,
//                                         MapHasher, MapEqual> &prevMmaps) {
//   uint64_t total = 0;
//   std::unordered_set<Trace, TraceHash> seen(allEvents.size());

//   // std::cout << std::endl;
//   // std::cout << std::endl << "Random Walk: (";
//   // std::cout << std::endl;
//   for (auto i = 0; i < NUM_ATTEMPTS; ++i) {
//     total += random_walk(allEvents, po, goodWrites, prevLocks, prevMmaps,
//     seen);
//   }

//   // std::cout << ")" << std::endl;
//   // std::cout << std::endl;

//   return total / (double)NUM_ATTEMPTS;
// }
