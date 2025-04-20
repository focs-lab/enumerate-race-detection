#pragma once

#include <format>
#include <stdexcept>

class EncodingError : public std::runtime_error {
public:
  explicit EncodingError(uint32_t i, uint64_t rawEvent)
      : std::runtime_error(std::format(
            "Error parsing input at event {}, raw event: {}", i, rawEvent)) {}
};
