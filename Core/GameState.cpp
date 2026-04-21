#include "Core/GameState.hpp"

#include "Entities/Ability/Ability.hpp"

#include <algorithm>

namespace Game {

namespace {

Vec2i normalizedStep(const Vec2i& delta) {
  return {(delta.x > 0) - (delta.x < 0), (delta.y > 0) - (delta.y < 0)};
}

bool isBlockingTerrainForPush(TerrainType terrain, bool flying) {
  if (isLethalTerrain(terrain, flying)) {
    return false;
  }
  return terrain == TerrainType::Mountain || terrain == TerrainType::Rock || terrain == TerrainType::Building;
}

int applyShield(UnitRecord& unit, int damage) {
  if (damage <= 0) {
    return 0;
  }

  const int absorbed = std::min(unit.shield, damage);
  unit.shield -= absorbed;
  return damage - absorbed;
}

}  // namespace

GameState::GameState(GridBoard board, std::vector<UnitRecord> units, Team activeTeam, MissionContext mission)
    : board_(std::move(board)), mission_(mission), units_(std::move(units)), activeTeam_(activeTeam) {}

GridBoard& GameState::board() {
  return board_;
}

const GridBoard& GameState::board() const {
  return board_;
}

MissionContext& GameState::mission() {
  return mission_;
}

const MissionContext& GameState::mission() const {
  return mission_;
}

std::vector<UnitRecord>& GameState::units() {
  return units_;
}

const std::vector<UnitRecord>& GameState::units() const {
  return units_;
}

Team GameState::activeTeam() const {
  return activeTeam_;
}

void GameState::setActiveTeam(Team team) {
  activeTeam_ = team;
}

int GameState::turnNumber() const {
  return turnNumber_;
}

const UnitRecord* GameState::findUnit(int id) const {
  const auto iterator = std::find_if(units_.begin(), units_.end(), [id](const UnitRecord& unit) { return unit.id == id; });
  return iterator == units_.end() ? nullptr : &(*iterator);
}

UnitRecord* GameState::findUnit(int id) {
  const auto iterator = std::find_if(units_.begin(), units_.end(), [id](const UnitRecord& unit) { return unit.id == id; });
  return iterator == units_.end() ? nullptr : &(*iterator);
}

const UnitRecord* GameState::findUnitAt(const Vec2i& position) const {
  const auto iterator = std::find_if(units_.begin(), units_.end(), [&](const UnitRecord& unit) {
    return unit.alive() && unit.position == position;
  });
  return iterator == units_.end() ? nullptr : &(*iterator);
}

UnitRecord* GameState::findUnitAt(const Vec2i& position) {
  const auto iterator = std::find_if(units_.begin(), units_.end(), [&](const UnitRecord& unit) {
    return unit.alive() && unit.position == position;
  });
  return iterator == units_.end() ? nullptr : &(*iterator);
}

bool GameState::isOccupied(const Vec2i& position, std::optional<int> ignoreUnitId) const {
  return std::any_of(units_.begin(), units_.end(), [&](const UnitRecord& unit) {
    return unit.alive() && unit.position == position && (!ignoreUnitId || unit.id != *ignoreUnitId);
  });
}

bool GameState::isFriendlyAt(Team team, const Vec2i& position) const {
  const UnitRecord* unit = findUnitAt(position);
  return unit != nullptr && unit->team == team;
}

bool GameState::hasEnemyInCells(Team friendlyTeam, const std::vector<Vec2i>& cells) const {
  for (const Vec2i& cell : cells) {
    const UnitRecord* unit = findUnitAt(cell);
    if (unit != nullptr && unit->team != friendlyTeam) {
      return true;
    }
  }
  return false;
}

bool GameState::hasFriendlyInCells(Team team, const std::vector<Vec2i>& cells) const {
  for (const Vec2i& cell : cells) {
    const UnitRecord* unit = findUnitAt(cell);
    if (unit != nullptr && unit->team == team) {
      return true;
    }
  }
  return false;
}

bool GameState::hasStructureInCells(const std::vector<Vec2i>& cells) const {
  for (const Vec2i& cell : cells) {
    if (!board_.inBounds(cell)) {
      continue;
    }
    const Tile& tile = board_.tileAt(cell);
    if (isStructure(tile.terrain) && tile.structureHealth > 0) {
      return true;
    }
  }
  return false;
}

std::vector<const UnitRecord*> GameState::unitsForTeam(Team team) const {
  std::vector<const UnitRecord*> result;
  for (const auto& unit : units_) {
    if (unit.team == team && unit.alive()) {
      result.push_back(&unit);
    }
  }
  return result;
}

std::vector<UnitRecord*> GameState::unitsForTeam(Team team) {
  std::vector<UnitRecord*> result;
  for (auto& unit : units_) {
    if (unit.team == team && unit.alive()) {
      result.push_back(&unit);
    }
  }
  return result;
}

bool GameState::allUnitsActed(Team team) const {
  bool foundAlive = false;
  for (const auto& unit : units_) {
    if (unit.team != team || !unit.alive()) {
      continue;
    }
    foundAlive = true;
    if (!unit.acted) {
      return false;
    }
  }
  return foundAlive;
}

void GameState::resetActedFlags(Team team) {
  for (auto& unit : units_) {
    if (unit.team == team && unit.alive()) {
      unit.acted = false;
    }
  }
}

int GameState::gridPower() const {
  int total = 0;
  for (const Vec2i& cell : board_.allPositions()) {
    const Tile& tile = board_.tileAt(cell);
    if (isStructure(tile.terrain) && tile.structureHealth > 0) {
      total += tile.structureHealth;
    }
  }
  return total;
}

int GameState::survivingStructures() const {
  int count = 0;
  for (const Vec2i& cell : board_.allPositions()) {
    const Tile& tile = board_.tileAt(cell);
    if (isStructure(tile.terrain) && tile.structureHealth > 0) {
      ++count;
    }
  }
  return count;
}

int GameState::enemyCount() const {
  return static_cast<int>(std::count_if(units_.begin(), units_.end(), [](const UnitRecord& unit) {
    return unit.team == Team::Enemy && unit.alive();
  }));
}

int GameState::playerCount() const {
  return static_cast<int>(std::count_if(units_.begin(), units_.end(), [](const UnitRecord& unit) {
    return unit.team == Team::Player && unit.alive();
  }));
}

int GameState::controlledZones(Team team) const {
  int controlled = 0;
  for (int i = 0; i < mission_.controlZoneCount; ++i) {
    if (isFriendlyAt(team, mission_.controlZones[static_cast<std::size_t>(i)])) {
      ++controlled;
    }
  }
  return controlled;
}

bool GameState::noLivingUnits(Team team) const {
  return std::none_of(units_.begin(), units_.end(), [&](const UnitRecord& unit) {
    return unit.team == team && unit.alive();
  });
}

bool GameState::missionSucceeded() const {
  switch (mission_.objective) {
    case MissionObjectiveType::EliminateAll:
      return enemyCount() == 0;
    case MissionObjectiveType::DefendBase:
      return mission_.turnLimit > 0 ? turnNumber_ > mission_.turnLimit : enemyCount() == 0;
    case MissionObjectiveType::Escort: {
      const UnitRecord* escort = findUnit(mission_.escortUnitId);
      return escort != nullptr && escort->alive() && escort->position == mission_.escortDestination;
    }
    case MissionObjectiveType::CaptureZones:
      return mission_.requiredControlZones > 0 &&
             mission_.controlHeldTurns >= std::max(1, mission_.controlHeldTurnsRequired);
    case MissionObjectiveType::BossFight: {
      if (mission_.bossUnitId >= 0) {
        const UnitRecord* boss = findUnit(mission_.bossUnitId);
        return boss == nullptr || !boss->alive();
      }
      return enemyCount() == 0;
    }
    case MissionObjectiveType::Survival:
      return mission_.survivalTargetTurns > 0 ? turnNumber_ > mission_.survivalTargetTurns : enemyCount() == 0;
  }
  return false;
}

bool GameState::missionFailed() const {
  if (noLivingUnits(Team::Player) || gridPower() <= 0) {
    return true;
  }

  switch (mission_.objective) {
    case MissionObjectiveType::Escort: {
      if (mission_.escortUnitId >= 0) {
        const UnitRecord* escort = findUnit(mission_.escortUnitId);
        if (escort == nullptr || !escort->alive()) {
          return true;
        }
      }
      [[fallthrough]];
    }
    case MissionObjectiveType::DefendBase:
    case MissionObjectiveType::CaptureZones:
    case MissionObjectiveType::BossFight:
      return mission_.turnLimit > 0 && turnNumber_ > mission_.turnLimit && !missionSucceeded();
    case MissionObjectiveType::EliminateAll:
    case MissionObjectiveType::Survival:
      return false;
  }
  return false;
}

bool GameState::isTerminal() const {
  return missionSucceeded() || missionFailed();
}

std::optional<Team> GameState::winner() const {
  if (!isTerminal()) {
    return std::nullopt;
  }
  return missionSucceeded() ? std::optional<Team>{Team::Player} : std::optional<Team>{Team::Enemy};
}

void GameState::damageCell(const Vec2i& cell, int damage, Team sourceTeam, bool damageStructures) {
  (void)sourceTeam;
  if (!board_.inBounds(cell) || damage <= 0) {
    return;
  }

  if (UnitRecord* unit = findUnitAt(cell)) {
    const int remaining = applyShield(*unit, damage);
    unit->health -= remaining;
  }

  Tile& tile = board_.tileAt(cell);
  if (damageStructures && isStructure(tile.terrain) && tile.structureHealth > 0) {
    tile.structureHealth -= damage;
    if (tile.structureHealth <= 0) {
      collapseStructure(cell);
    }
  }
}

void GameState::healCell(const Vec2i& cell, int amount) {
  if (amount <= 0) {
    return;
  }

  UnitRecord* unit = findUnitAt(cell);
  if (unit == nullptr || !unit->alive()) {
    return;
  }

  unit->health = std::min(unit->stats().maxHealth, unit->health + amount);
}

void GameState::grantShield(const Vec2i& cell, int amount) {
  if (amount <= 0) {
    return;
  }

  UnitRecord* unit = findUnitAt(cell);
  if (unit == nullptr || !unit->alive()) {
    return;
  }

  unit->shield += amount;
}

void GameState::applyStatus(const Vec2i& cell, StatusEffectType type, int duration, int magnitude) {
  UnitRecord* unit = findUnitAt(cell);
  if (unit == nullptr || !unit->alive() || duration <= 0) {
    return;
  }

  const std::size_t index = static_cast<std::size_t>(statusIndex(type));
  unit->statusDurations[index] = std::max(unit->statusDurations[index], duration);
  unit->statusMagnitudes[index] = std::max(unit->statusMagnitudes[index], magnitude);
}

void GameState::pushCell(const Vec2i& cell, const Vec2i& direction, int force, Team sourceTeam) {
  if (force <= 0) {
    return;
  }

  UnitRecord* unit = findUnitAt(cell);
  if (unit == nullptr) {
    return;
  }

  const Vec2i step = normalizedStep(direction);
  if (step == Vec2i{0, 0}) {
    return;
  }

  for (int i = 0; i < force && unit->alive(); ++i) {
    const Vec2i next = unit->position + step;
    if (!board_.inBounds(next)) {
      unit->health = 0;
      return;
    }

    const Tile& tile = board_.tileAt(next);
    if (isLethalTerrain(tile.terrain, unit->stats().flying)) {
      unit->position = next;
      unit->health = 0;
      return;
    }

    if (isOccupied(next) || isBlockingTerrainForPush(tile.terrain, unit->stats().flying)) {
      unit->health -= 1;
      if (UnitRecord* collided = findUnitAt(next)) {
        collided->health -= 1;
      }
      if (isStructure(tile.terrain)) {
        damageCell(next, 1, sourceTeam, true);
      }
      return;
    }

    unit->position = next;
  }
}

void GameState::collapseStructure(const Vec2i& cell) {
  Tile& tile = board_.tileAt(cell);
  tile.terrain = TerrainType::Ground;
  tile.structureHealth = 0;
  tile.controlValue = 1;
}

void GameState::tickPersistentEffects(Team team) {
  for (auto& unit : units_) {
    if (unit.team != team || !unit.alive()) {
      continue;
    }

    if (unit.hasStatus(StatusEffectType::Stun)) {
      unit.acted = true;
    }

    if (unit.hasStatus(StatusEffectType::Burn)) {
      const int damage = std::max(1, unit.statusMagnitude(StatusEffectType::Burn));
      const int remaining = applyShield(unit, damage);
      unit.health -= remaining;
    }

    for (int& cooldown : unit.cooldowns) {
      if (cooldown > 0) {
        --cooldown;
      }
    }

    for (int index = 0; index < kStatusEffectCount; ++index) {
      if (unit.statusDurations[static_cast<std::size_t>(index)] <= 0) {
        unit.statusDurations[static_cast<std::size_t>(index)] = 0;
        unit.statusMagnitudes[static_cast<std::size_t>(index)] = 0;
        continue;
      }

      --unit.statusDurations[static_cast<std::size_t>(index)];
      if (unit.statusDurations[static_cast<std::size_t>(index)] == 0) {
        unit.statusMagnitudes[static_cast<std::size_t>(index)] = 0;
      }
    }
  }

  pruneDeadUnits();
}

void GameState::beginTeamPhase(Team team) {
  resetActedFlags(team);
  tickPersistentEffects(team);
}

void GameState::updateMissionProgress() {
  if (mission_.objective == MissionObjectiveType::CaptureZones) {
    if (controlledZones(Team::Player) >= mission_.requiredControlZones) {
      ++mission_.controlHeldTurns;
    } else {
      mission_.controlHeldTurns = 0;
    }
  }
}

void GameState::applyAction(const Action& action) {
  UnitRecord* actor = findUnit(action.unitId);
  if (actor == nullptr || !actor->alive() || actor->team != activeTeam_ || actor->acted) {
    return;
  }
  if (!board_.inBounds(action.moveTarget) || isOccupied(action.moveTarget, actor->id)) {
    return;
  }

  const Vec2i from = actor->position;
  actor->position = action.moveTarget;

  if (action.isAttack()) {
    const int slot = std::clamp(action.abilitySlot, 0, kAbilitySlotCount - 1);
    if (actor->cooldowns[static_cast<std::size_t>(slot)] <= 0) {
      const Ability& ability = abilityFor(*actor, slot);
      ability.resolve(*this, *actor, from, *action.attackTarget);
      actor = findUnit(action.unitId);
      if (actor != nullptr) {
        actor->cooldowns[static_cast<std::size_t>(slot)] = ability.cooldown();
      }
    }
  }

  actor = findUnit(action.unitId);
  if (actor != nullptr) {
    actor->acted = true;
  }

  pruneDeadUnits();
  advanceTurnIfNeeded();
}

void GameState::applyHazardTick() {
  // Lethal terrain resolves immediately on displacement. Persistent damage is handled as status processing.
}

void GameState::pruneDeadUnits() {
  units_.erase(std::remove_if(units_.begin(), units_.end(), [](const UnitRecord& unit) { return !unit.alive(); }), units_.end());
}

void GameState::forceEndTurn() {
  for (auto& unit : units_) {
    if (unit.team == activeTeam_ && unit.alive()) {
      unit.acted = true;
    }
  }
  advanceTurnIfNeeded();
}

void GameState::advanceTurnIfNeeded() {
  if (isTerminal() || !allUnitsActed(activeTeam_)) {
    return;
  }

  if (activeTeam_ == Team::Player) {
    activeTeam_ = Team::Enemy;
    beginTeamPhase(Team::Enemy);
    if (!isTerminal() && allUnitsActed(Team::Enemy)) {
      advanceTurnIfNeeded();
    }
    return;
  }

  applyHazardTick();
  updateMissionProgress();
  if (isTerminal()) {
    return;
  }

  activeTeam_ = Team::Player;
  ++turnNumber_;
  beginTeamPhase(Team::Player);
  if (!isTerminal() && allUnitsActed(Team::Player)) {
    advanceTurnIfNeeded();
  }
}

}  // namespace Game
