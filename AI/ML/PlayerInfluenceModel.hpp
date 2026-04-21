#pragma once

#include "Core/GameState.hpp"
#include "Moves/Action.hpp"

#include <array>

namespace Game {

class PlayerInfluenceModel {
 public:
  void reset();
  void observePlayerAction(const GameState& stateBeforeAction, const Action& action);

  float dangerAt(const GameState& state, const Vec2i& cell) const;
  float actionBias(const GameState& state, const Action& action) const;

 private:
  void decay(float factor);

  std::array<float, GridBoard::kCellCount> moveHeat_{};
  std::array<float, GridBoard::kCellCount> attackHeat_{};
};

}  // namespace Game
