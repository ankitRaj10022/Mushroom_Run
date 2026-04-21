#pragma once

#include "Board/GridBoard.hpp"
#include "Core/GameTypes.hpp"
#include "Moves/Action.hpp"

#include <optional>
#include <vector>

namespace Game {

class GameState {
 public:
  GameState() = default;
  GameState(GridBoard board, std::vector<UnitRecord> units, Team activeTeam, MissionContext mission = {});

  GridBoard& board();
  const GridBoard& board() const;

  MissionContext& mission();
  const MissionContext& mission() const;

  std::vector<UnitRecord>& units();
  const std::vector<UnitRecord>& units() const;

  Team activeTeam() const;
  void setActiveTeam(Team team);

  int turnNumber() const;

  const UnitRecord* findUnit(int id) const;
  UnitRecord* findUnit(int id);
  const UnitRecord* findUnitAt(const Vec2i& position) const;
  UnitRecord* findUnitAt(const Vec2i& position);

  bool isOccupied(const Vec2i& position, std::optional<int> ignoreUnitId = std::nullopt) const;
  bool isFriendlyAt(Team team, const Vec2i& position) const;
  bool hasEnemyInCells(Team friendlyTeam, const std::vector<Vec2i>& cells) const;
  bool hasFriendlyInCells(Team team, const std::vector<Vec2i>& cells) const;
  bool hasStructureInCells(const std::vector<Vec2i>& cells) const;
  std::vector<const UnitRecord*> unitsForTeam(Team team) const;
  std::vector<UnitRecord*> unitsForTeam(Team team);

  bool allUnitsActed(Team team) const;
  void resetActedFlags(Team team);

  bool isTerminal() const;
  std::optional<Team> winner() const;
  int gridPower() const;
  int survivingStructures() const;
  int enemyCount() const;
  int playerCount() const;
  int controlledZones(Team team) const;

  void applyAction(const Action& action);
  void applyHazardTick();
  void pruneDeadUnits();
  void forceEndTurn();
  void advanceTurnIfNeeded();

  void damageCell(const Vec2i& cell, int damage, Team sourceTeam, bool damageStructures = true);
  void healCell(const Vec2i& cell, int amount);
  void grantShield(const Vec2i& cell, int amount);
  void pushCell(const Vec2i& cell, const Vec2i& direction, int force, Team sourceTeam);
  void applyStatus(const Vec2i& cell, StatusEffectType type, int duration, int magnitude);

 private:
  void beginTeamPhase(Team team);
  void tickPersistentEffects(Team team);
  void updateMissionProgress();
  void collapseStructure(const Vec2i& cell);
  bool noLivingUnits(Team team) const;
  bool missionSucceeded() const;
  bool missionFailed() const;

  GridBoard board_{};
  MissionContext mission_{};
  std::vector<UnitRecord> units_{};
  Team activeTeam_{Team::Player};
  int turnNumber_{1};
};

}  // namespace Game
