#pragma once

#include <array>
#include <cstddef>
#include <cstdlib>

namespace Game {

struct Vec2i {
  int x{0};
  int y{0};
};

inline bool operator==(const Vec2i& lhs, const Vec2i& rhs) {
  return lhs.x == rhs.x && lhs.y == rhs.y;
}

inline bool operator!=(const Vec2i& lhs, const Vec2i& rhs) {
  return !(lhs == rhs);
}

inline bool operator<(const Vec2i& lhs, const Vec2i& rhs) {
  return lhs.x < rhs.x || (lhs.x == rhs.x && lhs.y < rhs.y);
}

inline Vec2i operator+(const Vec2i& lhs, const Vec2i& rhs) {
  return {lhs.x + rhs.x, lhs.y + rhs.y};
}

inline Vec2i operator-(const Vec2i& lhs, const Vec2i& rhs) {
  return {lhs.x - rhs.x, lhs.y - rhs.y};
}

inline int manhattanDistance(const Vec2i& lhs, const Vec2i& rhs) {
  return std::abs(lhs.x - rhs.x) + std::abs(lhs.y - rhs.y);
}

inline std::array<Vec2i, 4> cardinalDirections() {
  return {{{1, 0}, {-1, 0}, {0, 1}, {0, -1}}};
}

struct Vec2iHash {
  std::size_t operator()(const Vec2i& value) const noexcept {
    return (static_cast<std::size_t>(value.x) << 32U) ^ static_cast<std::size_t>(value.y);
  }
};

}  // namespace Game
