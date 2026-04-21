#pragma once

#include "AI/Search/SearchTypes.hpp"
#include "Core/GameState.hpp"

#include <optional>
#include <vector>

namespace Game {

class IAI {
 public:
  virtual ~IAI() = default;
  virtual std::optional<Action> chooseAction(const GameState& state,
                                             Team team,
                                             const AISettings& settings,
                                             SearchMetrics& metrics) = 0;
  virtual std::vector<Action> planTurn(GameState state,
                                       Team team,
                                       const AISettings& settings,
                                       SearchMetrics& metrics) = 0;
};

}  // namespace Game
