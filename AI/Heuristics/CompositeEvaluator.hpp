#pragma once

#include "AI/Heuristics/IEvaluator.hpp"

namespace Game {

class CompositeEvaluator final : public IEvaluator {
 public:
  float evaluate(const GameState& state, Team perspective) const override;

 private:
  float positionalPotential(const GameState& state, const UnitRecord& unit) const;
};

}  // namespace Game
