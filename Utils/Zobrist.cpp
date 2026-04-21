#include "Utils/Zobrist.hpp"

#include <algorithm>
#include <random>

namespace Game {

ZobristHasher::ZobristHasher() {
  std::mt19937_64 rng(0xC0FFEE1234ULL);
  for (auto& unitKeys : positionKeys_) {
    for (auto& value : unitKeys) {
      value = rng();
    }
  }
  for (auto& unitKeys : healthKeys_) {
    for (auto& value : unitKeys) {
      value = rng();
    }
  }
  for (auto& unitKeys : actedKeys_) {
    for (auto& value : unitKeys) {
      value = rng();
    }
  }
  for (auto& value : activeTeamKeys_) {
    value = rng();
  }
}

std::uint64_t ZobristHasher::hash(const GameState& state) const {
  // A fast incremental-friendly hash is what makes the transposition table practical at interactive frame budgets.
  std::uint64_t key = activeTeamKeys_[state.activeTeam() == Team::Player ? 0 : 1];
  for (const UnitRecord& unit : state.units()) {
    if (!unit.alive()) {
      continue;
    }
    const int unitIndex = std::clamp(unit.id, 0, kMaxUnits - 1);
    key ^= positionKeys_[unitIndex][state.board().index(unit.position)];
    key ^= healthKeys_[unitIndex][std::clamp(unit.health, 0, kMaxHealthBucket - 1)];
    key ^= actedKeys_[unitIndex][unit.acted ? 1 : 0];
  }
  return key;
}

}  // namespace Game
