#include "AI/Heuristics/CompositeEvaluator.hpp"

#include "Moves/MoveGenerator.hpp"

#include <algorithm>

namespace Game {

float CompositeEvaluator::evaluate(const GameState& state, Team perspective) const {
  if (state.isTerminal()) {
    const auto winner = state.winner();
    return winner && *winner == perspective ? 1000.0f : -1000.0f;
  }

  float healthBalance = 0.0f;
  float controlBalance = 0.0f;
  float futureBalance = 0.0f;
  float structureBalance = (perspective == Team::Player ? 1.0f : -1.0f) * static_cast<float>(state.gridPower());

  for (const auto& unit : state.units()) {
    if (!unit.alive()) {
      continue;
    }
    const float sign = unit.team == perspective ? 1.0f : -1.0f;
    healthBalance += sign * (static_cast<float>(unit.health) / static_cast<float>(unit.stats().maxHealth));
    controlBalance += sign * static_cast<float>(state.board().tileAt(unit.position).controlValue);
    futureBalance += sign * positionalPotential(state, unit);
  }

  const float threatBalance =
      MoveGenerator::threatScore(state, perspective) - MoveGenerator::threatScore(state, opposingTeam(perspective));
  const float mobilityBalance = static_cast<float>(MoveGenerator::mobilityCount(state, perspective) -
                                                   MoveGenerator::mobilityCount(state, opposingTeam(perspective)));

  return healthBalance * 4.2f + controlBalance * 0.35f + threatBalance * 0.70f + mobilityBalance * 0.10f +
         futureBalance * 0.30f + structureBalance * 0.55f;
}

float CompositeEvaluator::positionalPotential(const GameState& state, const UnitRecord& unit) const {
  const std::vector<Vec2i> reachable = MoveGenerator::reachableTiles(state, unit);
  int bestControl = state.board().tileAt(unit.position).controlValue;
  for (const Vec2i& cell : reachable) {
    bestControl = std::max(bestControl, state.board().tileAt(cell).controlValue);
  }
  const float centerBias = 1.0f - static_cast<float>(manhattanDistance(unit.position, {3, 3})) / 8.0f;
  return static_cast<float>(bestControl) + centerBias;
}

}  // namespace Game
