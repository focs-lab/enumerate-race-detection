#pragma once

#include <cstdint>
#include <functional>
#include <unordered_map>

// Hash function for std::unordered_map<uint32_t, uint32_t>
struct MapHasher {
  std::size_t
  operator()(const std::unordered_map<uint32_t, uint32_t> &m) const {
    std::size_t hash = 0;
    for (const auto &[key, value] : m) {
      hash ^= std::hash<uint32_t>()(key) ^ std::hash<uint32_t>()(value) +
                                               0x9e3779b9 + (hash << 6) +
                                               (hash >> 2); // Combine hashes
    }
    return hash;
  }
};

// Equality function for std::unordered_map<uint32_t, uint32_t>
struct MapEqual {
  bool operator()(const std::unordered_map<uint32_t, uint32_t> &lhs,
                  const std::unordered_map<uint32_t, uint32_t> &rhs) const {
    return lhs == rhs; // Use built-in equality operator
  }
};
