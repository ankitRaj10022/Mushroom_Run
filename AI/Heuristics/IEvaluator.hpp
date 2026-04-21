#pragma once

#include "Core/GameState.hpp"

namespace Game {

class IEvaluator {
 public:
  virtual ~IEvaluator() = default;
  virtual float evaluate(const GameState& state, Team perspective) const = 0;
};

}  // namespace Game
