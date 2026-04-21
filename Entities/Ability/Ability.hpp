#pragma once

#include "Core/GameTypes.hpp"

#include <memory>
#include <string>
#include <vector>

namespace Game {

class GameState;

class Ability {
 public:
  virtual ~Ability() = default;

  virtual AbilityId id() const = 0;
  virtual int cooldown() const = 0;
  virtual std::string name() const;
  virtual std::vector<Vec2i> validTargets(const GameState& state, const UnitRecord& unit, const Vec2i& from) const = 0;
  virtual std::vector<Vec2i> affectedCells(const GameState& state, const UnitRecord& unit, const Vec2i& from,
                                           const Vec2i& target) const = 0;
  virtual void resolve(GameState& state, UnitRecord& actor, const Vec2i& from, const Vec2i& target) const = 0;
};

const Ability& abilityFor(AbilityId id);
const Ability& abilityFor(const UnitRecord& unit, int abilitySlot);

}  // namespace Game
