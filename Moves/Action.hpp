#pragma once

#include "Utils/Vec2i.hpp"

#include <optional>
#include <sstream>
#include <string>

namespace Game {

struct Action {
  int unitId{-1};
  Vec2i moveTarget{};
  std::optional<Vec2i> attackTarget;
  int abilitySlot{-1};
  bool wait{false};

  static Action invalid() {
    return {};
  }

  bool isValid() const {
    return unitId >= 0;
  }

  bool isAttack() const {
    return attackTarget.has_value() && abilitySlot >= 0;
  }
};

inline bool operator==(const Action& lhs, const Action& rhs) {
  return lhs.unitId == rhs.unitId && lhs.moveTarget == rhs.moveTarget && lhs.attackTarget == rhs.attackTarget &&
         lhs.abilitySlot == rhs.abilitySlot && lhs.wait == rhs.wait;
}

struct ActionHash {
  std::size_t operator()(const Action& action) const noexcept {
    const std::size_t unitHash = static_cast<std::size_t>(action.unitId + 1);
    const std::size_t moveHash =
        (static_cast<std::size_t>(action.moveTarget.x) << 16U) ^ static_cast<std::size_t>(action.moveTarget.y);
    const std::size_t attackHash = action.attackTarget
                                       ? ((static_cast<std::size_t>(action.attackTarget->x) << 20U) ^
                                          static_cast<std::size_t>(action.attackTarget->y))
                                       : 0U;
    const std::size_t abilityHash = static_cast<std::size_t>(action.abilitySlot + 2);
    return unitHash ^ (moveHash << 1U) ^ (attackHash << 2U) ^ (abilityHash << 3U) ^ (action.wait ? 0x9E3779B9U : 0U);
  }
};

inline std::string actionToString(const Action& action) {
  std::ostringstream stream;
  stream << "Unit " << action.unitId << " -> (" << action.moveTarget.x << ", " << action.moveTarget.y << ")";
  if (action.attackTarget) {
    stream << " ability[" << action.abilitySlot << "] (" << action.attackTarget->x << ", " << action.attackTarget->y << ")";
  } else if (action.wait) {
    stream << " wait";
  }
  return stream.str();
}

}  // namespace Game
