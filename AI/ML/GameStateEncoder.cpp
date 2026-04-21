#include "AI/ML/GameStateEncoder.hpp"

#include <algorithm>

namespace Game {

std::vector<float> GameStateEncoder::encode(const GameState& state, Team perspective) const {
  std::vector<float> features(kInputSize, 0.0f);

  auto writePlane = [&](int plane, const Vec2i& cell, float value) {
    const int offset = plane * GridBoard::kCellCount + state.board().index(cell);
    features[offset] = value;
  };

  for (const Vec2i& cell : state.board().allPositions()) {
    const Tile& tile = state.board().tileAt(cell);
    writePlane(8, cell, tile.terrain == TerrainType::Water ? 1.0f : 0.0f);
    writePlane(9, cell, tile.terrain == TerrainType::Mountain ? 1.0f : 0.0f);
    writePlane(10, cell, (tile.terrain == TerrainType::Rock || tile.terrain == TerrainType::Building) ? 1.0f : 0.0f);
    writePlane(11, cell, (tile.terrain == TerrainType::Chasm || tile.terrain == TerrainType::Forest) ? 1.0f : 0.0f);
    writePlane(12, cell, std::min(1.0f, normalizedControlValue(tile.controlValue) + tile.structureHealth / 6.0f));
    writePlane(13, cell, state.activeTeam() == perspective ? 1.0f : 0.0f);
  }

  for (const UnitRecord& unit : state.units()) {
    if (!unit.alive()) {
      continue;
    }

    const bool friendly = unit.team == perspective;
    const int basePlane = friendly ? 0 : 1;
    writePlane(basePlane, unit.position, static_cast<float>(unit.health) / static_cast<float>(unit.stats().maxHealth));
    writePlane(friendly ? 2 : 3, unit.position, static_cast<float>(unit.stats().moveRange) / 4.0f);
    writePlane(friendly ? 4 : 5, unit.position, static_cast<float>(unit.stats().attackRange) / 4.0f);
    writePlane(friendly ? 6 : 7, unit.position, static_cast<float>(unit.stats().attackDamage) / 4.0f);
  }

  return features;
}

}  // namespace Game
