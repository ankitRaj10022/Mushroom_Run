#pragma once

#include <algorithm>

namespace Game {

enum class TerrainType {
  Ground,
  Water,
  Forest,
  Mountain,
  Rock,
  Building,
  Chasm
};

struct Tile {
  TerrainType terrain{TerrainType::Ground};
  int controlValue{1};
  int structureHealth{0};
  bool spawnPoint{false};
};

inline bool blocksMovement(TerrainType terrain, bool flying = false) {
  switch (terrain) {
    case TerrainType::Water:
      return !flying;
    case TerrainType::Mountain:
    case TerrainType::Rock:
    case TerrainType::Building:
    case TerrainType::Chasm:
      return true;
    case TerrainType::Ground:
    case TerrainType::Forest:
      return false;
  }
  return false;
}

inline bool blocksAttack(TerrainType terrain) {
  return terrain == TerrainType::Mountain || terrain == TerrainType::Rock;
}

inline bool isHazard(TerrainType terrain) {
  return terrain == TerrainType::Water || terrain == TerrainType::Chasm;
}

inline bool isStructure(TerrainType terrain) {
  return terrain == TerrainType::Building;
}

inline bool isLethalTerrain(TerrainType terrain, bool flying = false) {
  return terrain == TerrainType::Chasm || (terrain == TerrainType::Water && !flying);
}

inline float normalizedControlValue(int value) {
  return std::clamp(static_cast<float>(value) / 4.0f, 0.0f, 1.0f);
}

}  // namespace Game
