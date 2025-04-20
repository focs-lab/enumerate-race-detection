#include "trace.hpp"
#include "event.hpp"
#include <memory>

bool Trace::isExecutable(CommonArg &arg, std::vector<eid_t> &iset, EventId id,
                         EventId e1, EventId e2) {
  if (id.getEid() >= COMPLETED || !isIncluded(id, iset))
    return false;

  for (auto mhb : arg.closure.getHappensBefore(id))
    if (!isExecuted(mhb))
      return false;

  Event event = getEvent(arg.events, id);

  switch (event.getEventType()) {
  case EventType::Acquire:
    if (locks.find(event.getVarId()) == locks.end())
      return true;
    break;
  case EventType::Release:
    if (locks.find(event.getVarId()) != locks.end())
      return true;
    break;
  case EventType::Write: {
    if (!isHeldVar(arg, iset, id, e1, e2))
      return true;
    // return true;
    break;
  }
  case EventType::Read:
    if (mmap[event.getVarId()] == event.getVarValue())
      return true;
    break;
  case EventType::Fork:
    return true;
  case EventType::Join: {
    tid_t tid = arg.tid_map.find(event.getVarId()) == arg.tid_map.end()
                    ? event.getVarId()
                    : arg.tid_map[event.getVarId()];
    EventId endEvent = {tid, arg.events[event.getVarId()].size() - 1};
    if (isExecuted(endEvent))
      return true;
    break;
  }
  case EventType::Begin:
    return true;
  case EventType::End:
    return true;
  default:
    // Nothing should come here
    std::cerr << "Error generating new trace node: unknown event type - "
              << event.getEventType() << std::endl;
    return false;
  }

  return false;
}

std::vector<EventId> Trace::getExecutableEvents(CommonArg &arg,
                                                std::vector<eid_t> &iset,
                                                EventId e1, EventId e2) {
  std::vector<EventId> executables;

  for (tid_t i = 0; i < events.size(); ++i) {
    const eid_t eid = events[i];
    EventId id = {i, eid};
    if (isExecutable(arg, iset, id, e1, e2) && id != e1 && id != e2) {
      executables.push_back(id);
    }
  }

  return executables;
}

void Trace::advanceReads(CommonArg &arg, std::vector<eid_t> &iset, EventId e1,
                         EventId e2) {
  for (tid_t i = 0; i < events.size(); ++i) {
    tid_t tid = i;
    while (true) {
      EventId id = {tid, events[tid]};
      if (events[tid] >= COMPLETED || !isIncluded(id, iset))
        break;

      if ((tid == e1.getTid() && events[tid] == e1.getEid()) ||
          (tid == e2.getTid() && events[tid] == e2.getEid()))
        break;

      for (auto mhb : arg.closure.getHappensBefore(id))
        if (!isExecuted(mhb))
          break;

      Event event = getEvent(arg.events, id);
      if (!(event.getEventType() == EventType::Read &&
            mmap[event.getVarId()] == event.getVarValue())) {
        break;
      }

      ++events[tid];
    }
  }
}

std::shared_ptr<Trace> Trace::appendEvent(CommonArg &arg,
                                          std::vector<eid_t> &iset, EventId id,
                                          EventId e1, EventId e2) {
  std::shared_ptr t = std::make_shared<Trace>(*this);
  t->events[id.getTid()] += 1;

  Event event = getEvent(arg.events, id);
  switch (event.getEventType()) {
  case EventType::Acquire:
    t->locks[event.getVarId()] = id;
    break;
  case EventType::Release:
    t->locks.erase(event.getVarId());
    break;
  case EventType::Write:
    t->mmap[event.getVarId()] = event.getVarValue();
    break;
  case EventType::Read:
    // Do nothing
    break;
  case EventType::Fork: {
    tid_t tid = arg.tid_map.find(event.getVarId()) == arg.tid_map.end()
                    ? event.getVarId()
                    : arg.tid_map[event.getVarId()];
    t->events[tid] = FIRST_EVENT;
    break;
  }
  case EventType::Join:
    // Do nothing
    break;
  case EventType::Begin:
    // Do nothing
    break;
  case EventType::End:
    t->events[id.getTid()] = COMPLETED;
    break;
  default:
    // Nothing should come here
    std::cerr << "Error generating new trace node: unknown event type - "
              << event.getEventType() << std::endl;
    break;
  }

  t->priority = t->computePriority(arg, iset, e1, e2);
  t->prev = this;
  t->advanceReads(arg, iset, e1, e2);

  return t;
}

bool Trace::isHeldVar(CommonArg &arg, std::vector<eid_t> &iset, EventId write,
                      EventId e1, EventId e2) {
  Event e = getEvent(arg.events, write);
  vid_t var = e.getVarId();
  uint32_t currVal = mmap[var];

  // 1. Check if write writes the same val
  if (e.getVarValue() == currVal)
    return false;

  // 2. Check any other write that can write the current value
  for (auto w : arg.var_to_write_map[var][currVal]) {
    if (isIncluded(w, iset) && !isExecuted(w))
      return false;
  }

  // 3. Check any other read waiting to read this val
  for (auto r : arg.var_to_read_map[var][currVal]) {
    if (isIncluded(r, iset) && !isExecuted(r) &&
        (r.getTid() == e1.getTid() ||
         r.getTid() ==
             e2.getTid())) // Only if there is a read in thread t1 & t2
      return true;
  }

  return false;
}

const uint32_t X1 = 100;
const uint32_t X2 = 20;
const uint32_t X3 = 2;
const uint32_t X4 = 1;
const uint32_t THRESHOLD = 80;

uint32_t Trace::computePriority(CommonArg &arg, std::vector<eid_t> &iset,
                                EventId e1, EventId e2) {
  // 1. Distance to COP
  uint32_t distToCOP =
      (computeDistance(arg.events, e1) + computeDistance(arg.events, e2)) * X1;

  uint32_t unblockCost = 0;

  // 2. If next event in t1/t2 not executable, compute cost to unblock
  EventId t1Event = {e1.getTid(), events[e1.getTid()]};
  if (t1Event != e1 && !isExecutable(arg, iset, t1Event, e1, e2)) {
    unblockCost += computeUnblockCost(arg, iset, e1);
  }

  EventId t2Event = {e2.getTid(), events[e2.getTid()]};
  if (t2Event != e2 && !isExecutable(arg, iset, t2Event, e1, e2)) {
    unblockCost += computeUnblockCost(arg, iset, e1);
  }

  // h(x) = distToE1 * X1 + distToE2 * X1 + unblockT1Cost + unblockT2Cost
  return distToCOP + unblockCost;
}

uint32_t Trace::computeDistance(std::vector<std::vector<Event>> &allEvents,
                                EventId e) {
  if (events[e.getTid()] == TO_BE_FORKED)
    return e.getEid();

  if (events[e.getTid()] == COMPLETED || events[e.getTid()] >= e.getEid())
    return 0;

  return e.getTid() - events[e.getTid()];
}

uint32_t Trace::computeUnblockCost(CommonArg &arg, std::vector<eid_t> &iset,
                                   EventId e) {
  uint32_t numMHB = 0;
  for (auto hb : arg.closure.getHappensBefore(e))
    if (!isExecuted(hb))
      ++numMHB;

  uint32_t cost = numMHB * X2;
  Event event = getEvent(arg.events, e);

  switch (event.getEventType()) {
  case EventType::Acquire: {
    EventId currAcq = locks[event.getVarId()];
    cost += computeDistance(arg.events, arg.acq_rel_map[currAcq]) * X3;
    break;
  }
  case EventType::Read: {
    uint32_t totalDist = 0;
    uint32_t numGoodWrites = 0;

    for (auto w : arg.var_to_write_map[event.getVarId()][event.getVarValue()]) {
      if (isIncluded(w, iset) && !isExecuted(w) &&
          w.getTid() !=
              e.getTid()) { // Check if w and e in same thread (implies e < w)
        totalDist += computeDistance(arg.events, w);
        ++numGoodWrites;
      }
    }

    // Compute cost based on avg dist to good writes
    cost += totalDist / numGoodWrites * X4;
    break;
  }
  case EventType::Join: {
    tid_t tid = arg.tid_map.find(event.getVarId()) == arg.tid_map.end()
                    ? event.getVarId()
                    : arg.tid_map[event.getVarId()];
    uint32_t distToEnd = computeDistance(
        arg.events, {tid, arg.events[event.getVarId()].size() - 1});
    cost += distToEnd * X3;
    break;
  }
  case EventType::Begin: {
    cost += computeDistance(arg.events, arg.begin_fork_map[e.getTid()]) * X3;
    break;
  }
  case EventType::Write:
    break; // Write in t1/t2 blocked only if held var (that is, held by
           // read in t2/t1)
  // case EventType::Release:
  // case EventType::Fork:
  // case EventType::End:
  default:
    break;
  }

  return cost >= THRESHOLD ? THRESHOLD : cost;
}

void Trace::printTrace() {
  std::cout << "Trace: [";
  for (auto e : events) {
    std::cout << e << ", ";
  }
  std::cout << "]" << std::endl;
}
