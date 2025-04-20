#pragma once

#include "event.hpp"
#include <vector>

struct ParseResult {
  /* 2D vector consisting for events for each thread */
  std::vector<std::vector<Event>> events;

  /* Map of input thread id to tid_t. Optional, this is only used if thread id
   * in input trace is not serial starting from 0. Pass an empty map if not
   * in use. */
  std::unordered_map<uint32_t, tid_t> thread_to_tid_map;
};

/* Parses input trace from given path */
ParseResult parse(const std::string &filename);
