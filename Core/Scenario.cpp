#include "Core/Scenario.hpp"

#include <algorithm>

namespace Game {

namespace {

UnitRecord makeUnit(int id, Team team, UnitClass unitClass, const Vec2i& position, bool objectiveUnit = false) {
  UnitRecord unit;
  unit.id = id;
  unit.team = team;
  unit.unitClass = unitClass;
  unit.position = position;
  unit.health = unit.stats().maxHealth;
  unit.objectiveUnit = objectiveUnit;
  unit.experienceValue = team == Team::Enemy ? 2 : 1;
  return unit;
}

MissionContext makeMission(MissionObjectiveType objective, int credits, int xp) {
  MissionContext mission;
  mission.objective = objective;
  mission.rewardCredits = credits;
  mission.rewardXp = xp;
  return mission;
}

GridBoard makeShadowRelayBoard() {
  GridBoard board = GridBoard::createFoundryBoard();
  board.tileAt({4, 1}) = Tile{TerrainType::Building, 4, 4, false};
  board.tileAt({6, 1}).controlValue = 4;
  board.tileAt({5, 5}).terrain = TerrainType::Forest;
  return board;
}

GridBoard makeOverseerBoard() {
  GridBoard board = GridBoard::createHarborBoard();
  board.tileAt({3, 3}) = Tile{TerrainType::Building, 5, 5, false};
  board.tileAt({2, 6}).terrain = TerrainType::Rock;
  board.tileAt({5, 5}).terrain = TerrainType::Mountain;
  return board;
}

GridBoard makeSurvivalBoard() {
  GridBoard board = GridBoard::createSkirmishBoard();
  board.tileAt({3, 3}).terrain = TerrainType::Building;
  board.tileAt({3, 3}).structureHealth = 4;
  board.tileAt({4, 3}).terrain = TerrainType::Building;
  board.tileAt({4, 3}).structureHealth = 4;
  return board;
}

std::vector<UnitRecord> defaultSquad(const std::vector<Vec2i>& spawns) {
  static const std::array<UnitClass, 3> kDefaultHeroes{
      UnitClass::CannonMech,
      UnitClass::ArtilleryMech,
      UnitClass::GuardMech,
  };

  std::vector<UnitRecord> units;
  for (std::size_t index = 0; index < std::min<std::size_t>(kDefaultHeroes.size(), spawns.size()); ++index) {
    units.push_back(makeUnit(static_cast<int>(index), Team::Player, kDefaultHeroes[index], spawns[index]));
  }
  return units;
}

}  // namespace

const std::vector<ScenarioDefinition>& scenarioCatalog() {
  static const std::vector<ScenarioDefinition> kCatalog = [] {
    std::vector<ScenarioDefinition> scenarios;

    ScenarioDefinition riftBasin;
    riftBasin.id = "rift-basin";
    riftBasin.name = "Rift Basin";
    riftBasin.subtitle = "Clean opener built around tempo, lane control, and focused takedowns.";
    riftBasin.briefing = "Break the first Vek spearhead and secure the basin before the invasion escalates.";
    riftBasin.mode = GameMode::Campaign;
    riftBasin.mission = makeMission(MissionObjectiveType::EliminateAll, 45, 2);
    riftBasin.board = GridBoard::createSkirmishBoard();
    riftBasin.playerSpawnCells = {{0, 5}, {1, 7}, {2, 6}, {0, 4}};
    riftBasin.units = defaultSquad(riftBasin.playerSpawnCells);
    riftBasin.units.push_back(makeUnit(10, Team::Enemy, UnitClass::Scarab, {5, 1}));
    riftBasin.units.push_back(makeUnit(11, Team::Enemy, UnitClass::Beetle, {6, 3}));
    riftBasin.units.push_back(makeUnit(12, Team::Enemy, UnitClass::Firefly, {7, 2}));
    scenarios.push_back(riftBasin);

    ScenarioDefinition foundryPass;
    foundryPass.id = "foundry-pass";
    foundryPass.name = "Foundry Pass";
    foundryPass.subtitle = "Hold the reactor corridor while heavier assault bugs rotate through the seam.";
    foundryPass.briefing = "Defend the foundry spine for five turns. Let the terrain buy time and punish overextensions.";
    foundryPass.mode = GameMode::Campaign;
    foundryPass.mission = makeMission(MissionObjectiveType::DefendBase, 60, 3);
    foundryPass.mission.turnLimit = 5;
    foundryPass.board = GridBoard::createFoundryBoard();
    foundryPass.playerSpawnCells = {{0, 4}, {1, 6}, {2, 7}, {1, 5}};
    foundryPass.units = defaultSquad(foundryPass.playerSpawnCells);
    foundryPass.units.push_back(makeUnit(20, Team::Enemy, UnitClass::Beetle, {5, 0}));
    foundryPass.units.push_back(makeUnit(21, Team::Enemy, UnitClass::Scarab, {6, 1}));
    foundryPass.units.push_back(makeUnit(22, Team::Enemy, UnitClass::Firefly, {7, 4}));
    foundryPass.units.push_back(makeUnit(23, Team::Enemy, UnitClass::Brute, {6, 6}));
    scenarios.push_back(foundryPass);

    ScenarioDefinition harborBreach;
    harborBreach.id = "harbor-breach";
    harborBreach.name = "Harbor Breach";
    harborBreach.subtitle = "Take and hold key platforms while the enemy tries to drown the district.";
    harborBreach.briefing = "Capture two control zones for two rounds. Waterlogged lanes reward smart dashes and artillery zoning.";
    harborBreach.mode = GameMode::Campaign;
    harborBreach.mission = makeMission(MissionObjectiveType::CaptureZones, 70, 4);
    harborBreach.mission.controlZoneCount = 3;
    harborBreach.mission.controlZones = {Vec2i{2, 1}, Vec2i{4, 3}, Vec2i{6, 5}, Vec2i{0, 0}};
    harborBreach.mission.requiredControlZones = 2;
    harborBreach.mission.controlHeldTurnsRequired = 2;
    harborBreach.board = GridBoard::createHarborBoard();
    harborBreach.playerSpawnCells = {{0, 3}, {1, 5}, {2, 6}, {0, 2}};
    harborBreach.units = defaultSquad(harborBreach.playerSpawnCells);
    harborBreach.units.push_back(makeUnit(30, Team::Enemy, UnitClass::Scarab, {5, 3}));
    harborBreach.units.push_back(makeUnit(31, Team::Enemy, UnitClass::Firefly, {6, 2}));
    harborBreach.units.push_back(makeUnit(32, Team::Enemy, UnitClass::Beetle, {7, 5}));
    harborBreach.units.push_back(makeUnit(33, Team::Enemy, UnitClass::Brute, {6, 7}));
    scenarios.push_back(harborBreach);

    ScenarioDefinition shadowRelay;
    shadowRelay.id = "shadow-relay";
    shadowRelay.name = "Shadow Relay";
    shadowRelay.subtitle = "Escort a relay operative across a fractured foundry under pressure.";
    shadowRelay.briefing = "Keep the relay operative alive and move them to the extraction platform before the ambushers collapse the route.";
    shadowRelay.mode = GameMode::Campaign;
    shadowRelay.mission = makeMission(MissionObjectiveType::Escort, 85, 4);
    shadowRelay.mission.turnLimit = 6;
    shadowRelay.mission.escortUnitId = 90;
    shadowRelay.mission.escortDestination = {6, 1};
    shadowRelay.board = makeShadowRelayBoard();
    shadowRelay.playerSpawnCells = {{0, 5}, {1, 6}, {2, 7}, {1, 4}};
    shadowRelay.units = defaultSquad(shadowRelay.playerSpawnCells);
    shadowRelay.units.push_back(makeUnit(90, Team::Player, UnitClass::OraclePrime, {2, 4}, true));
    shadowRelay.units.push_back(makeUnit(40, Team::Enemy, UnitClass::Firefly, {6, 0}));
    shadowRelay.units.push_back(makeUnit(41, Team::Enemy, UnitClass::Scarab, {7, 2}));
    shadowRelay.units.push_back(makeUnit(42, Team::Enemy, UnitClass::Beetle, {5, 5}));
    shadowRelay.units.push_back(makeUnit(43, Team::Enemy, UnitClass::Brute, {7, 6}));
    scenarios.push_back(shadowRelay);

    ScenarioDefinition overseerCore;
    overseerCore.id = "overseer-core";
    overseerCore.name = "Overseer Core";
    overseerCore.subtitle = "A boss strike mission against a flying command organism with layered support.";
    overseerCore.briefing = "Break the escort screen, weather the cataclysm volleys, and bring down the Overseer before the grid collapses.";
    overseerCore.mode = GameMode::Campaign;
    overseerCore.mission = makeMission(MissionObjectiveType::BossFight, 120, 6);
    overseerCore.mission.turnLimit = 8;
    overseerCore.mission.bossUnitId = 99;
    overseerCore.board = makeOverseerBoard();
    overseerCore.playerSpawnCells = {{0, 4}, {1, 6}, {2, 7}, {0, 3}};
    overseerCore.units = defaultSquad(overseerCore.playerSpawnCells);
    overseerCore.units.push_back(makeUnit(99, Team::Enemy, UnitClass::Overseer, {6, 2}));
    overseerCore.units.push_back(makeUnit(50, Team::Enemy, UnitClass::Firefly, {7, 3}));
    overseerCore.units.push_back(makeUnit(51, Team::Enemy, UnitClass::Scarab, {5, 5}));
    overseerCore.units.push_back(makeUnit(52, Team::Enemy, UnitClass::Brute, {6, 6}));
    scenarios.push_back(overseerCore);

    ScenarioDefinition survivalRun;
    survivalRun.id = "vek-onslaught";
    survivalRun.name = "Vek Onslaught";
    survivalRun.subtitle = "Endless-pressure simulation for survival tuning and AI benchmarking.";
    survivalRun.briefing = "Survive six rounds against repeated pressure. Use it to test builds, heuristics, and late-game positioning.";
    survivalRun.mode = GameMode::Survival;
    survivalRun.mission = makeMission(MissionObjectiveType::Survival, 90, 5);
    survivalRun.mission.survivalTargetTurns = 6;
    survivalRun.board = makeSurvivalBoard();
    survivalRun.playerSpawnCells = {{0, 5}, {1, 6}, {2, 7}, {0, 4}};
    survivalRun.units = defaultSquad(survivalRun.playerSpawnCells);
    survivalRun.units.push_back(makeUnit(60, Team::Enemy, UnitClass::Scarab, {6, 1}));
    survivalRun.units.push_back(makeUnit(61, Team::Enemy, UnitClass::Firefly, {7, 2}));
    survivalRun.units.push_back(makeUnit(62, Team::Enemy, UnitClass::Beetle, {6, 5}));
    survivalRun.units.push_back(makeUnit(63, Team::Enemy, UnitClass::Brute, {7, 6}));
    scenarios.push_back(survivalRun);

    return scenarios;
  }();

  return kCatalog;
}

const ScenarioDefinition& defaultScenarioDefinition() {
  return scenarioCatalog().front();
}

GameState createScenarioState(const ScenarioDefinition& scenario, const std::vector<UnitRecord>& squad) {
  std::vector<UnitRecord> units;
  units.reserve(scenario.units.size() + squad.size());

  for (const UnitRecord& unit : scenario.units) {
    if (unit.team == Team::Enemy || unit.objectiveUnit) {
      units.push_back(unit);
    }
  }

  if (squad.empty()) {
    for (const UnitRecord& unit : scenario.units) {
      if (unit.team == Team::Player && !unit.objectiveUnit) {
        units.push_back(unit);
      }
    }
  } else {
    for (std::size_t index = 0; index < squad.size(); ++index) {
      UnitRecord hero = squad[index];
      hero.position = index < scenario.playerSpawnCells.size() ? scenario.playerSpawnCells[index] : hero.position;
      hero.team = Team::Player;
      hero.health = hero.stats().maxHealth;
      hero.acted = false;
      hero.shield = 0;
      hero.cooldowns.fill(0);
      hero.statusDurations.fill(0);
      hero.statusMagnitudes.fill(0);
      units.push_back(hero);
    }
  }

  std::sort(units.begin(), units.end(), [](const UnitRecord& lhs, const UnitRecord& rhs) { return lhs.id < rhs.id; });
  return GameState(scenario.board, units, Team::Player, scenario.mission);
}

GameState createDefaultScenario() {
  return createScenarioState(defaultScenarioDefinition());
}

}  // namespace Game
