#include <numeric>
#include <unordered_set>

#include "event.hpp"
#include "preprocesser.hpp"

PreprocessResult
preprocess(std::vector<std::vector<Event>> &events,
           std::unordered_map<uint32_t, tid_t> &thread_to_tid_map) {
  std::vector<EventId> writes;
  std::vector<EventId> reads;
  std::vector<EventId> joins;
  std::vector<EventId> forks;
  std::unordered_map<EventId, std::unordered_set<vid_t>> event_to_lock_map;

  CommonArg arg = initialize(events, writes, reads, joins, forks,
                             event_to_lock_map, thread_to_tid_map);
  std::unordered_set<std::pair<EventId, EventId>> cops =
      generateCOPs(events, writes, reads, event_to_lock_map, arg.closure);

  return {arg, cops};
}

CommonArg initialize(
    std::vector<std::vector<Event>> &events, std::vector<EventId> &writes,
    std::vector<EventId> &reads, std::vector<EventId> &joins,
    std::vector<EventId> &forks,
    std::unordered_map<EventId, std::unordered_set<vid_t>> &event_to_lock_map,
    std::unordered_map<uint32_t, tid_t> &thread_to_tid_map) {
  std::unordered_map<vid_t, std::unordered_map<uint32_t, std::vector<EventId>>>
      var_to_write_map;
  std::unordered_map<vid_t, std::unordered_map<uint32_t, std::vector<EventId>>>
      var_to_read_map;
  std::unordered_map<tid_t, EventId> begin_fork_map;

  std::unordered_map<EventId, EventId> acq_rel_map;

  size_t totalSize =
      std::accumulate(events.begin(), events.end(), 0,
                      [](size_t sum, const std::vector<Event> &thread) {
                        return sum + thread.size();
                      });
  std::vector<Event> inputTrace(totalSize, Event{});

  for (tid_t i = 0; i < events.size(); ++i) {
    std::unordered_map<vid_t, EventId> acquiredLocks;

    for (eid_t j = 0; j < events[i].size(); ++j) {
      Event &e = events[i][j];
      EventId id{i, j};
      switch (e.getEventType()) {
      case EventType::Acquire:
        acquiredLocks[e.getVarId()] = id;
        break;
      case EventType::Release: {
        vid_t l = e.getVarId();
        acq_rel_map[acquiredLocks[l]] = id;
        acquiredLocks.erase(l);
        break;
      }
      case EventType::Read: {
        var_to_read_map[e.getVarId()][e.getVarValue()].push_back(id);
        reads.push_back(id);
        for (auto [l, _] : acquiredLocks) {
          event_to_lock_map[id].insert(l);
        }
        break;
      }
      case EventType::Write: {
        writes.push_back(id);
        var_to_write_map[e.getVarId()][e.getVarValue()].push_back(id);
        for (auto [l, _] : acquiredLocks) {
          event_to_lock_map[id].insert(l);
        }
        break;
      }
      case EventType::Fork: {
        forks.push_back({i, j});
        tid_t tid =
            thread_to_tid_map.find(e.getVarId()) == thread_to_tid_map.end()
                ? e.getVarId()
                : thread_to_tid_map[e.getVarId()];
        begin_fork_map[tid] = id;
        break;
      }
      case EventType::Join:
        joins.push_back({i, j});
        break;
      case EventType::Begin:
      case EventType::End:
      default:
        break;
      }
      inputTrace[e.getEventNum()] = e;
    }
  }

  Closure clj =
      buildClosure(events, inputTrace, var_to_write_map, var_to_read_map,
                   writes, reads, joins, forks, thread_to_tid_map);

  return CommonArg{events,
                   var_to_write_map,
                   var_to_read_map,
                   thread_to_tid_map,
                   acq_rel_map,
                   begin_fork_map,
                   clj};
}

Closure buildClosure(
    std::vector<std::vector<Event>> &events, std::vector<Event> &inputTrace,
    std::unordered_map<vid_t,
                       std::unordered_map<uint32_t, std::vector<EventId>>>
        &var_to_write_map,
    std::unordered_map<vid_t,
                       std::unordered_map<uint32_t, std::vector<EventId>>>
        &var_to_read_map,
    std::vector<EventId> &writes, std::vector<EventId> &reads,
    std::vector<EventId> &joins, std::vector<EventId> &forks,
    std::unordered_map<uint32_t, tid_t> &thread_to_tid_map) {
  Closure::Builder cb{static_cast<uint32_t>(events.size())};

  // Add fork-begin partial ordering
  for (auto f : forks) {
    Event e = getEvent(events, f);
    tid_t tid = thread_to_tid_map.find(e.getVarId()) == thread_to_tid_map.end()
                    ? e.getVarId()
                    : thread_to_tid_map[e.getVarId()];
    EventId beg{tid, 0};
    cb.addRelation(beg, f);
  }

  // Add end-join partial ordering
  for (auto j : joins) {
    Event e = getEvent(events, j);
    tid_t tid = thread_to_tid_map.find(e.getVarId()) == thread_to_tid_map.end()
                    ? e.getVarId()
                    : thread_to_tid_map[e.getVarId()];
    EventId end{tid, events[tid].size() - 1};
    cb.addRelation(j, end);
  }

  // Add write-read partial ordering for sole writers
  for (auto w : writes) {
    Event e = getEvent(events, w);
    if (isSoleWriter(e, var_to_write_map)) {
      auto reads = var_to_read_map[e.getVarId()][e.getVarValue()];
      for (auto r : reads) {
        cb.addRelation(r, w);
      }
    }
  }

  return cb.build(inputTrace, events);
}

std::unordered_set<std::pair<EventId, EventId>> generateCOPs(
    std::vector<std::vector<Event>> &events, std::vector<EventId> &writes,
    std::vector<EventId> &reads,
    std::unordered_map<EventId, std::unordered_set<vid_t>> &event_to_lock_map,
    Closure &clj) {
  std::unordered_set<std::pair<EventId, EventId>> cops;

  // Generate COP pairs
  for (size_t i = 0; i < writes.size(); ++i) {
    auto w = writes[i];

    // // Add <write, write> COP pairs
    for (size_t j = 1; j < writes.size(); ++j) {
      auto w2 = writes[j];
      if (isCandidateRace(w, w2, events, event_to_lock_map, clj))
        cops.insert(makeCOP(events, w, w2));
    }

    // Add <write, read> COP pairs
    for (auto r : reads) {
      if (isCandidateRace(w, r, events, event_to_lock_map, clj))
        cops.insert(makeCOP(events, w, r));
    }
  }

  return cops;
}
