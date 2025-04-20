#include "iset.hpp"
#include "event.hpp"
#include <stack>
#include <unordered_map>
#include <unordered_set>
#include <vector>

std::vector<eid_t> IncludeSet::find(EventId e1, EventId e2,
                                    std::vector<std::vector<Event>> &events,
                                    CommonArg &arg) {
  std::vector<eid_t> iset(events.size(), UNUSED);
  std::stack<Range> s;
  std::unordered_map<tid_t, std::unordered_set<vid_t>> acq;
  std::unordered_map<tid_t, std::unordered_set<vid_t>> rel;

  s.push(Range{e1.getTid(), 0, e1.getEid()});
  s.push(Range{e2.getTid(), 0, e2.getEid()});
  iset[e1.getTid()] = e1.getEid();
  iset[e2.getTid()] = e2.getEid();

  while (!s.empty()) {
    Range range = s.top();
    s.pop();

    for (auto i = range.end; i >= range.start && i < COMPLETED; --i) {
      // 1. Add dependencies
      EventId e = {range.tid, i};
      includeEvent(e, iset, s, acq, rel, events, arg);
    }
  }

  return iset;
}

void IncludeSet::addRange(EventId e, std::vector<eid_t> &iset,
                          std::stack<Range> &s) {
  if (iset[e.getTid()] != UNUSED && iset[e.getTid()] >= e.getEid())
    return;

  eid_t nextStart = iset[e.getTid()] == UNUSED ? 0 : iset[e.getTid()] + 1;
  Range next = Range{e.getTid(), nextStart, e.getEid()};

  iset[e.getTid()] = e.getEid();
  s.push(next);
}

void IncludeSet::addGoodWrites(EventId e, std::vector<eid_t> &iset,
                               std::stack<Range> &s,
                               std::vector<std::vector<Event>> &events,
                               CommonArg &arg) {
  Event evt = getEvent(events, e);
  std::vector<EventId> &writes =
      arg.var_to_write_map[evt.getVarId()][evt.getVarValue()];

  for (auto w : writes) {
    if (w.getTid() == e.getTid() && w.getEid() > e.getTid())
      continue; // w happens after e, ignore

    addRange(e, iset, s);
  }
}

void IncludeSet::matchAcquires(
    tid_t thread, std::vector<eid_t> &iset, std::stack<Range> &s,
    std::unordered_map<tid_t, std::unordered_set<vid_t>> &acq,
    std::unordered_map<tid_t, std::unordered_set<vid_t>> &rel,
    std::vector<std::vector<Event>> &events, CommonArg &arg) {

  while (!acq[thread].empty() && iset[thread] + 1 < events[thread].size()) {
    EventId e = {thread, iset[thread] + 1};
    ++iset[thread];
    includeEvent(e, iset, s, acq, rel, events, arg);
  }
}

void IncludeSet::includeEvent(
    EventId e, std::vector<eid_t> &iset, std::stack<Range> &s,
    std::unordered_map<tid_t, std::unordered_set<vid_t>> &acq,
    std::unordered_map<tid_t, std::unordered_set<vid_t>> &rel,
    std::vector<std::vector<Event>> &events, CommonArg &arg) {
  // 1. Add direct deps
  for (auto hb : arg.closure.getHappensBefore(e)) {
    addRange(hb, iset, s);
  }

  Event evt = getEvent(events, e);

  // 2. Handle writes, acquire and releases
  switch (evt.getEventType()) {
  case EventType::Read:
    addGoodWrites(e, iset, s, events, arg);
    break;
  case EventType::Acquire: {
    std::unordered_set<vid_t> &releases = rel[e.getTid()];
    if (releases.find(evt.getVarId()) == releases.end()) {
      // Corresponding release not found yet
      acq[e.getTid()].insert(evt.getVarId());
      matchAcquires(e.getTid(), iset, s, acq, rel, events, arg);
    } else {
      releases.erase(evt.getVarId());
    }

    break;
  }
  case EventType::Release: {
    std::unordered_set<vid_t> &acquires = acq[e.getTid()];
    if (acquires.find(evt.getVarId()) == acquires.end()) {
      rel[e.getTid()].insert(evt.getVarId());
    } else {
      acquires.erase(evt.getVarId());
    }
    break;
  }

  default:
    break;
  }
}
