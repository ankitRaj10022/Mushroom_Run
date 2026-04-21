#pragma once

#include "Core/GameState.hpp"

#include <vector>

namespace Game {

class GameStateEncoder {
 public:
  static constexpr int kFeaturePlanes = 14;
  static constexpr int kInputSize = GridBoard::kCellCount * kFeaturePlanes;

  std::vector<float> encode(const GameState& state, Team perspective) const;
};

}  // namespace Game
