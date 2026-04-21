#pragma once

#include "Core/GameState.hpp"

#include <vector>

namespace Game {

class MoveGenerator {
 public:
  static std::vector<Vec2i> reachableTiles(const GameState& state, const UnitRecord& unit);
  static std::vector<Action> generateActions(const GameState& state);
  static std::vector<Action> generateActionsForTeam(const GameState& state, Team team, bool onlyAttacks = false);
  static std::vector<Action> generateActionsForUnit(const GameState& state,
                                                    const UnitRecord& unit,
                                                    bool onlyAttacks = false);
  static std::vector<Vec2i> legalAttackTargets(const GameState& state,
                                               const UnitRecord& unit,
                                               const Vec2i& from,
                                               int abilitySlot = 0);
  static std::vector<Vec2i> affectedTiles(const GameState& state,
                                          const UnitRecord& unit,
                                          const Vec2i& from,
                                          const Vec2i& target,
                                          int abilitySlot = 0);
  static int estimateActionDamage(const GameState& state, const Action& action);
  static int mobilityCount(const GameState& state, Team team);
  static float threatScore(const GameState& state, Team team);
  static std::vector<Action> generateNoisyActions(const GameState& state);

 private:
  static bool hasHostileTargetInCells(const GameState& state, Team team, const std::vector<Vec2i>& cells);
};

}  // namespace Game
