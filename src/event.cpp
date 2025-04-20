#include "event.hpp"

std::ostream &operator<<(std::ostream &os, const Event &event) {
  return os << event.prettyString();
}
