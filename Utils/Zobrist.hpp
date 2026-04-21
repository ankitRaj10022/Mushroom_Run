#pragma once

#include "Core/GameState.hpp"

#include <array>
#include <cstdint>

namespace Game {

class ZobristHasher {
 public:
  ZobristHasher();

  std::uint64_t hash(const GameState& state) const;

 private:
  static constexpr int kMaxUnits = 32;
  static constexpr int kMaxHealthBucket = 16;

  std::array<std::array<std::uint64_t, GridBoard::kCellCount>, kMaxUnits> positionKeys_{};
  std::array<std::array<std::uint64_t, kMaxHealthBucket>, kMaxUnits> healthKeys_{};
  std::array<std::array<std::uint64_t, 2>, kMaxUnits> actedKeys_{};
  std::array<std::uint64_t, 2> activeTeamKeys_{};
};

}  // namespace Game
