#pragma once

#include "Core/GameState.hpp"

#include <string>
#include <vector>

namespace Game {

struct ScenarioDefinition {
  std::string id;
  std::string name;
  std::string subtitle;
  std::string briefing;
  GameMode mode{GameMode::Campaign};
  MissionContext mission{};
  GridBoard board;
  std::vector<UnitRecord> units;
  std::vector<Vec2i> playerSpawnCells;
  int recommendedSquadSize{3};
};

const std::vector<ScenarioDefinition>& scenarioCatalog();
const ScenarioDefinition& defaultScenarioDefinition();
GameState createScenarioState(const ScenarioDefinition& scenario, const std::vector<UnitRecord>& squad = {});
GameState createDefaultScenario();

}  // namespace Game
