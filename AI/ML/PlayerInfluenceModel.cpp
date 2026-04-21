#include "AI/ML/PlayerInfluenceModel.hpp"

#include "Moves/MoveGenerator.hpp"

#include <algorithm>

namespace Game {

void PlayerInfluenceModel::reset() {
  moveHeat_.fill(0.0f);
  attackHeat_.fill(0.0f);
}

void PlayerInfluenceModel::decay(float factor) {
  for (float& value : moveHeat_) {
    value *= factor;
  }
  for (float& value : attackHeat_) {
    value *= factor;
  }
}

void PlayerInfluenceModel::observePlayerAction(const GameState& stateBeforeAction, const Action& action) {
  decay(0.93f);

  if (stateBeforeAction.board().inBounds(action.moveTarget)) {
    moveHeat_[stateBeforeAction.board().index(action.moveTarget)] += action.wait ? 0.35f : 1.0f;
  }

  const UnitRecord* actor = stateBeforeAction.findUnit(action.unitId);
  if (actor == nullptr || !action.attackTarget) {
    return;
  }

  UnitRecord proxy = *actor;
  proxy.position = action.moveTarget;
  for (const Vec2i& cell : MoveGenerator::affectedTiles(stateBeforeAction, proxy, action.moveTarget, *action.attackTarget)) {
    if (!stateBeforeAction.board().inBounds(cell)) {
      continue;
    }
    attackHeat_[stateBeforeAction.board().index(cell)] += 1.25f;
  }
}

float PlayerInfluenceModel::dangerAt(const GameState& state, const Vec2i& cell) const {
  if (!state.board().inBounds(cell)) {
    return 0.0f;
  }
  const int index = state.board().index(cell);
  return moveHeat_[index] * 0.8f + attackHeat_[index] * 1.35f;
}

float PlayerInfluenceModel::actionBias(const GameState& state, const Action& action) const {
  if (!state.board().inBounds(action.moveTarget)) {
    return 0.0f;
  }

  float risk = dangerAt(state, action.moveTarget);
  float pressure = 0.0f;

  const UnitRecord* actor = state.findUnit(action.unitId);
  if (actor != nullptr && action.attackTarget) {
    UnitRecord proxy = *actor;
    proxy.position = action.moveTarget;
    for (const Vec2i& cell : MoveGenerator::affectedTiles(state, proxy, action.moveTarget, *action.attackTarget)) {
      if (!state.board().inBounds(cell)) {
        continue;
      }
      const int index = state.board().index(cell);
      pressure += moveHeat_[index] * 0.75f + attackHeat_[index] * 0.55f;
    }
  }

  if (action.wait) {
    risk += 0.25f;
  }

  return pressure - risk;
}

}  // namespace Game
