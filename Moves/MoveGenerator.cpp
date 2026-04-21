#include "Moves/MoveGenerator.hpp"

#include "Entities/Ability/Ability.hpp"

#include <algorithm>
#include <queue>
#include <unordered_map>

namespace Game {

namespace {

int abilityDamageEstimate(AbilityId abilityId) {
  switch (abilityId) {
    case AbilityId::BreachShot:
    case AbilityId::MeteorRain:
    case AbilityId::ScarabMortar:
    case AbilityId::FireflyBeam:
      return 2;
    case AbilityId::Shadowstep:
    case AbilityId::GuardianPulse:
    case AbilityId::OracleWard:
    case AbilityId::VenomBurst:
      return 1;
    case AbilityId::BeetleCharge:
      return 3;
    case AbilityId::OverseerCataclysm:
      return 3;
  }
  return 1;
}

int supportValueEstimate(AbilityId abilityId) {
  switch (abilityId) {
    case AbilityId::GuardianPulse:
      return 5;
    case AbilityId::OracleWard:
      return 7;
    case AbilityId::Shadowstep:
      return 2;
    case AbilityId::VenomBurst:
      return 2;
    case AbilityId::BreachShot:
    case AbilityId::MeteorRain:
    case AbilityId::ScarabMortar:
    case AbilityId::BeetleCharge:
    case AbilityId::FireflyBeam:
    case AbilityId::OverseerCataclysm:
      return 0;
  }
  return 0;
}

bool actionLess(const Action& lhs, const Action& rhs) {
  if (lhs.unitId != rhs.unitId) {
    return lhs.unitId < rhs.unitId;
  }
  if (lhs.moveTarget != rhs.moveTarget) {
    return lhs.moveTarget < rhs.moveTarget;
  }
  if (lhs.attackTarget != rhs.attackTarget) {
    if (!lhs.attackTarget) {
      return true;
    }
    if (!rhs.attackTarget) {
      return false;
    }
    return *lhs.attackTarget < *rhs.attackTarget;
  }
  if (lhs.abilitySlot != rhs.abilitySlot) {
    return lhs.abilitySlot < rhs.abilitySlot;
  }
  return lhs.wait < rhs.wait;
}

}  // namespace

std::vector<Vec2i> MoveGenerator::reachableTiles(const GameState& state, const UnitRecord& unit) {
  std::queue<Vec2i> frontier;
  std::unordered_map<Vec2i, int, Vec2iHash> costByCell;

  frontier.push(unit.position);
  costByCell[unit.position] = 0;

  while (!frontier.empty()) {
    const Vec2i current = frontier.front();
    frontier.pop();

    for (const Vec2i direction : cardinalDirections()) {
      const Vec2i next = current + direction;
      if (!state.board().inBounds(next)) {
        continue;
      }
      if (blocksMovement(state.board().tileAt(next).terrain, unit.stats().flying)) {
        continue;
      }
      if (state.isOccupied(next, unit.id)) {
        continue;
      }

      const int newCost = costByCell[current] + 1;
      if (newCost > unit.stats().moveRange) {
        continue;
      }

      const auto iterator = costByCell.find(next);
      if (iterator != costByCell.end() && iterator->second <= newCost) {
        continue;
      }

      costByCell[next] = newCost;
      frontier.push(next);
    }
  }

  std::vector<Vec2i> reachable;
  reachable.reserve(costByCell.size());
  for (const auto& [cell, _] : costByCell) {
    reachable.push_back(cell);
  }
  std::sort(reachable.begin(), reachable.end());
  return reachable;
}

std::vector<Action> MoveGenerator::generateActions(const GameState& state) {
  return generateActionsForTeam(state, state.activeTeam(), false);
}

std::vector<Action> MoveGenerator::generateActionsForTeam(const GameState& state, Team team, bool onlyAttacks) {
  std::vector<Action> actions;
  for (const UnitRecord* unit : state.unitsForTeam(team)) {
    if (team == state.activeTeam() && unit->acted) {
      continue;
    }
    std::vector<Action> unitActions = generateActionsForUnit(state, *unit, onlyAttacks);
    actions.insert(actions.end(), unitActions.begin(), unitActions.end());
  }

  std::sort(actions.begin(), actions.end(), actionLess);
  actions.erase(std::unique(actions.begin(), actions.end()), actions.end());
  return actions;
}

std::vector<Action> MoveGenerator::generateActionsForUnit(const GameState& state,
                                                          const UnitRecord& unit,
                                                          bool onlyAttacks) {
  std::vector<Action> actions;
  if (!unit.alive() || unit.acted) {
    return actions;
  }

  const std::vector<Vec2i> reachable = reachableTiles(state, unit);
  for (const Vec2i& destination : reachable) {
    GameState movedState = state;
    UnitRecord* movedUnit = movedState.findUnit(unit.id);
    if (movedUnit == nullptr) {
      continue;
    }
    movedUnit->position = destination;

    for (int abilitySlot = 0; abilitySlot < kAbilitySlotCount; ++abilitySlot) {
      if (movedUnit->cooldowns[static_cast<std::size_t>(abilitySlot)] > 0) {
        continue;
      }

      const std::vector<Vec2i> attackTargets = legalAttackTargets(movedState, *movedUnit, destination, abilitySlot);
      for (const Vec2i& target : attackTargets) {
        actions.push_back(Action{unit.id, destination, target, abilitySlot, false});
      }
    }

    if (!onlyAttacks) {
      actions.push_back(Action{unit.id, destination, std::nullopt, -1, destination == unit.position});
    }
  }

  std::sort(actions.begin(), actions.end(), actionLess);
  actions.erase(std::unique(actions.begin(), actions.end()), actions.end());
  return actions;
}

std::vector<Vec2i> MoveGenerator::legalAttackTargets(const GameState& state,
                                                     const UnitRecord& unit,
                                                     const Vec2i& from,
                                                     int abilitySlot) {
  return abilityFor(unit, abilitySlot).validTargets(state, unit, from);
}

std::vector<Vec2i> MoveGenerator::affectedTiles(const GameState& state,
                                                const UnitRecord& unit,
                                                const Vec2i& from,
                                                const Vec2i& target,
                                                int abilitySlot) {
  return abilityFor(unit, abilitySlot).affectedCells(state, unit, from, target);
}

int MoveGenerator::estimateActionDamage(const GameState& state, const Action& action) {
  const UnitRecord* unit = state.findUnit(action.unitId);
  if (unit == nullptr) {
    return 0;
  }
  if (!action.attackTarget || action.abilitySlot < 0) {
    return action.wait ? 0 : state.board().tileAt(action.moveTarget).controlValue;
  }

  GameState movedState = state;
  UnitRecord* movedUnit = movedState.findUnit(action.unitId);
  if (movedUnit == nullptr) {
    return 0;
  }
  movedUnit->position = action.moveTarget;

  const AbilityId abilityId = movedUnit->stats().abilities[static_cast<std::size_t>(action.abilitySlot)];
  const std::vector<Vec2i> affected =
      affectedTiles(movedState, *movedUnit, action.moveTarget, *action.attackTarget, action.abilitySlot);

  const int baseDamage = abilityDamageEstimate(abilityId);
  const int supportValue = supportValueEstimate(abilityId);
  int totalScore = 0;

  for (const auto& target : movedState.units()) {
    if (!target.alive()) {
      continue;
    }

    if (std::find(affected.begin(), affected.end(), target.position) == affected.end()) {
      continue;
    }

    if (target.team != movedUnit->team) {
      totalScore += baseDamage + 1;
    } else if (supportValue > 0) {
      totalScore += std::min(supportValue, target.stats().maxHealth - target.health + 2);
    } else {
      totalScore -= baseDamage;
    }
  }

  if (movedUnit->team == Team::Enemy) {
    for (const Vec2i& cell : affected) {
      if (!movedState.board().inBounds(cell)) {
        continue;
      }
      const Tile& tile = movedState.board().tileAt(cell);
      if (isStructure(tile.terrain) && tile.structureHealth > 0) {
        totalScore += baseDamage + 2;
      }
    }
  }

  if (abilityId == AbilityId::Shadowstep) {
    totalScore += 2;
  }
  if (abilityId == AbilityId::GuardianPulse || abilityId == AbilityId::OracleWard) {
    totalScore += supportValue;
  }

  return totalScore;
}

int MoveGenerator::mobilityCount(const GameState& state, Team team) {
  int total = 0;
  for (const UnitRecord* unit : state.unitsForTeam(team)) {
    total += static_cast<int>(reachableTiles(state, *unit).size());
  }
  return total;
}

float MoveGenerator::threatScore(const GameState& state, Team team) {
  float threat = 0.0f;
  for (const UnitRecord* unit : state.unitsForTeam(team)) {
    const std::vector<Action> actions = generateActionsForUnit(state, *unit, true);
    int bestDamage = 0;
    for (const Action& action : actions) {
      bestDamage = std::max(bestDamage, estimateActionDamage(state, action));
    }
    threat += static_cast<float>(bestDamage);
  }
  return threat;
}

std::vector<Action> MoveGenerator::generateNoisyActions(const GameState& state) {
  return generateActionsForTeam(state, state.activeTeam(), true);
}

bool MoveGenerator::hasHostileTargetInCells(const GameState& state, Team team, const std::vector<Vec2i>& cells) {
  for (const auto& unit : state.units()) {
    if (!unit.alive() || unit.team == team) {
      continue;
    }
    if (std::find(cells.begin(), cells.end(), unit.position) != cells.end()) {
      return true;
    }
  }
  if (team == Team::Enemy) {
    for (const Vec2i& cell : cells) {
      if (state.board().inBounds(cell)) {
        const Tile& tile = state.board().tileAt(cell);
        if (isStructure(tile.terrain) && tile.structureHealth > 0) {
          return true;
        }
      }
    }
  }
  return false;
}

}  // namespace Game
