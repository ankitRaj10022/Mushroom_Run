#include "Entities/Ability/Ability.hpp"

#include "Core/GameState.hpp"

#include <algorithm>
#include <memory>

namespace Game {

namespace {

enum class TargetAffinity {
  Enemy,
  Ally,
  EmptyCell
};

class SingleTargetAbility final : public Ability {
 public:
  SingleTargetAbility(AbilityId id,
                      int range,
                      int cooldown,
                      int damage,
                      int heal,
                      int shield,
                      int pushForce,
                      TargetAffinity affinity,
                      StatusEffectType status,
                      int statusDuration,
                      int statusMagnitude,
                      bool allowStructures)
      : id_(id),
        range_(range),
        cooldown_(cooldown),
        damage_(damage),
        heal_(heal),
        shield_(shield),
        pushForce_(pushForce),
        affinity_(affinity),
        status_(status),
        statusDuration_(statusDuration),
        statusMagnitude_(statusMagnitude),
        allowStructures_(allowStructures) {}

  AbilityId id() const override {
    return id_;
  }

  int cooldown() const override {
    return cooldown_;
  }

  std::vector<Vec2i> validTargets(const GameState& state, const UnitRecord& unit, const Vec2i& from) const override {
    std::vector<Vec2i> targets;
    for (const Vec2i& cell : state.board().allPositions()) {
      if (manhattanDistance(from, cell) < 1 || manhattanDistance(from, cell) > range_) {
        continue;
      }

      const UnitRecord* occupant = state.findUnitAt(cell);
      if (affinity_ == TargetAffinity::Enemy) {
        if (occupant != nullptr && occupant->team != unit.team) {
          targets.push_back(cell);
          continue;
        }
        if (allowStructures_ && unit.team == Team::Enemy && isStructure(state.board().tileAt(cell).terrain) &&
            state.board().tileAt(cell).structureHealth > 0) {
          targets.push_back(cell);
        }
      } else if (affinity_ == TargetAffinity::Ally) {
        if (occupant != nullptr && occupant->team == unit.team) {
          targets.push_back(cell);
        }
      } else if (affinity_ == TargetAffinity::EmptyCell) {
        if (!state.isOccupied(cell) && !blocksMovement(state.board().tileAt(cell).terrain, unit.stats().flying)) {
          targets.push_back(cell);
        }
      }
    }
    return targets;
  }

  std::vector<Vec2i> affectedCells(const GameState& state,
                                   const UnitRecord& unit,
                                   const Vec2i& from,
                                   const Vec2i& target) const override {
    (void)state;
    (void)unit;
    (void)from;
    return {target};
  }

  void resolve(GameState& state, UnitRecord& actor, const Vec2i& from, const Vec2i& target) const override {
    (void)from;
    if (damage_ > 0) {
      state.damageCell(target, damage_, actor.team, true);
    }
    if (heal_ > 0) {
      state.healCell(target, heal_);
    }
    if (shield_ > 0) {
      state.grantShield(target, shield_);
    }
    if (pushForce_ > 0) {
      state.pushCell(target, target - actor.position, pushForce_, actor.team);
    }
    if (statusDuration_ > 0) {
      state.applyStatus(target, status_, statusDuration_, statusMagnitude_);
    }
  }

 private:
  AbilityId id_{AbilityId::BreachShot};
  int range_{1};
  int cooldown_{0};
  int damage_{0};
  int heal_{0};
  int shield_{0};
  int pushForce_{0};
  TargetAffinity affinity_{TargetAffinity::Enemy};
  StatusEffectType status_{StatusEffectType::Burn};
  int statusDuration_{0};
  int statusMagnitude_{0};
  bool allowStructures_{false};
};

class BlastAbility final : public Ability {
 public:
  BlastAbility(AbilityId id,
               int range,
               int cooldown,
               int damage,
               int splashRadius,
               StatusEffectType status,
               int statusDuration,
               int statusMagnitude,
               bool affectAllies = false)
      : id_(id),
        range_(range),
        cooldown_(cooldown),
        damage_(damage),
        splashRadius_(splashRadius),
        status_(status),
        statusDuration_(statusDuration),
        statusMagnitude_(statusMagnitude),
        affectAllies_(affectAllies) {}

  AbilityId id() const override {
    return id_;
  }

  int cooldown() const override {
    return cooldown_;
  }

  std::vector<Vec2i> validTargets(const GameState& state, const UnitRecord& unit, const Vec2i& from) const override {
    std::vector<Vec2i> targets;
    for (const Vec2i& cell : state.board().allPositions()) {
      if (manhattanDistance(from, cell) < 1 || manhattanDistance(from, cell) > range_) {
        continue;
      }

      const auto affected = affectedCells(state, unit, from, cell);
      if (state.hasEnemyInCells(unit.team, affected) || (unit.team == Team::Enemy && state.hasStructureInCells(affected)) ||
          (affectAllies_ && state.hasFriendlyInCells(unit.team, affected))) {
        targets.push_back(cell);
      }
    }
    return targets;
  }

  std::vector<Vec2i> affectedCells(const GameState& state,
                                   const UnitRecord& unit,
                                   const Vec2i& from,
                                   const Vec2i& target) const override {
    (void)unit;
    (void)from;
    std::vector<Vec2i> cells;
    for (int y = target.y - splashRadius_; y <= target.y + splashRadius_; ++y) {
      for (int x = target.x - splashRadius_; x <= target.x + splashRadius_; ++x) {
        const Vec2i cell{x, y};
        if (state.board().inBounds(cell) && manhattanDistance(target, cell) <= splashRadius_) {
          cells.push_back(cell);
        }
      }
    }
    return cells;
  }

  void resolve(GameState& state, UnitRecord& actor, const Vec2i& from, const Vec2i& target) const override {
    (void)from;
    for (const Vec2i& cell : affectedCells(state, actor, actor.position, target)) {
      if (!affectAllies_ && state.isFriendlyAt(actor.team, cell)) {
        continue;
      }
      state.damageCell(cell, damage_, actor.team, true);
      if (statusDuration_ > 0) {
        state.applyStatus(cell, status_, statusDuration_, statusMagnitude_);
      }
    }
  }

 private:
  AbilityId id_{AbilityId::MeteorRain};
  int range_{1};
  int cooldown_{0};
  int damage_{1};
  int splashRadius_{1};
  StatusEffectType status_{StatusEffectType::Burn};
  int statusDuration_{0};
  int statusMagnitude_{0};
  bool affectAllies_{false};
};

class BeamAbility final : public Ability {
 public:
  BeamAbility(AbilityId id, int range, int cooldown, int damage, StatusEffectType status, int statusDuration, int statusMagnitude)
      : id_(id),
        range_(range),
        cooldown_(cooldown),
        damage_(damage),
        status_(status),
        statusDuration_(statusDuration),
        statusMagnitude_(statusMagnitude) {}

  AbilityId id() const override {
    return id_;
  }

  int cooldown() const override {
    return cooldown_;
  }

  std::vector<Vec2i> validTargets(const GameState& state, const UnitRecord& unit, const Vec2i& from) const override {
    std::vector<Vec2i> targets;
    for (const Vec2i& direction : cardinalDirections()) {
      for (int step = 1; step <= range_; ++step) {
        const Vec2i cell = from + Vec2i{direction.x * step, direction.y * step};
        if (!state.board().inBounds(cell)) {
          break;
        }
        if (blocksAttack(state.board().tileAt(cell).terrain)) {
          break;
        }
        if (const UnitRecord* target = state.findUnitAt(cell); target != nullptr && target->team != unit.team) {
          targets.push_back(cell);
          break;
        }
        if (unit.team == Team::Enemy && isStructure(state.board().tileAt(cell).terrain) &&
            state.board().tileAt(cell).structureHealth > 0) {
          targets.push_back(cell);
          break;
        }
      }
    }
    return targets;
  }

  std::vector<Vec2i> affectedCells(const GameState& state,
                                   const UnitRecord& unit,
                                   const Vec2i& from,
                                   const Vec2i& target) const override {
    (void)unit;
    std::vector<Vec2i> cells;
    const Vec2i delta = target - from;
    const bool cardinal = (delta.x == 0 && delta.y != 0) || (delta.y == 0 && delta.x != 0);
    if (!cardinal) {
      return cells;
    }
    const int distance = std::max(std::abs(delta.x), std::abs(delta.y));
    const Vec2i direction{delta.x == 0 ? 0 : delta.x / distance, delta.y == 0 ? 0 : delta.y / distance};
    for (int step = 1; step <= range_; ++step) {
      const Vec2i cell = from + Vec2i{direction.x * step, direction.y * step};
      if (!state.board().inBounds(cell)) {
        break;
      }
      if (blocksAttack(state.board().tileAt(cell).terrain)) {
        break;
      }
      cells.push_back(cell);
    }
    return cells;
  }

  void resolve(GameState& state, UnitRecord& actor, const Vec2i& from, const Vec2i& target) const override {
    for (const Vec2i& cell : affectedCells(state, actor, from, target)) {
      if (state.isFriendlyAt(actor.team, cell)) {
        continue;
      }
      state.damageCell(cell, damage_, actor.team, true);
      if (state.findUnitAt(cell) != nullptr && statusDuration_ > 0) {
        state.applyStatus(cell, status_, statusDuration_, statusMagnitude_);
      }
      if (state.findUnitAt(cell) != nullptr || state.hasStructureInCells({cell})) {
        break;
      }
    }
  }

 private:
  AbilityId id_{AbilityId::FireflyBeam};
  int range_{1};
  int cooldown_{0};
  int damage_{1};
  StatusEffectType status_{StatusEffectType::Burn};
  int statusDuration_{0};
  int statusMagnitude_{0};
};

class DashAbility final : public Ability {
 public:
  DashAbility(AbilityId id, int range, int cooldown, int landingDamage, StatusEffectType status, int statusDuration, int statusMagnitude)
      : id_(id),
        range_(range),
        cooldown_(cooldown),
        landingDamage_(landingDamage),
        status_(status),
        statusDuration_(statusDuration),
        statusMagnitude_(statusMagnitude) {}

  AbilityId id() const override {
    return id_;
  }

  int cooldown() const override {
    return cooldown_;
  }

  std::vector<Vec2i> validTargets(const GameState& state, const UnitRecord& unit, const Vec2i& from) const override {
    std::vector<Vec2i> targets;
    for (const Vec2i& cell : state.board().allPositions()) {
      if (manhattanDistance(from, cell) < 1 || manhattanDistance(from, cell) > range_) {
        continue;
      }
      if (state.isOccupied(cell) || blocksMovement(state.board().tileAt(cell).terrain, unit.stats().flying)) {
        continue;
      }
      const auto affected = affectedCells(state, unit, from, cell);
      if (state.hasEnemyInCells(unit.team, affected)) {
        targets.push_back(cell);
      }
    }
    return targets;
  }

  std::vector<Vec2i> affectedCells(const GameState& state,
                                   const UnitRecord& unit,
                                   const Vec2i& from,
                                   const Vec2i& target) const override {
    (void)state;
    (void)unit;
    (void)from;
    std::vector<Vec2i> cells;
    cells.push_back(target);
    for (const Vec2i& direction : cardinalDirections()) {
      cells.push_back(target + direction);
    }
    return cells;
  }

  void resolve(GameState& state, UnitRecord& actor, const Vec2i& from, const Vec2i& target) const override {
    (void)from;
    actor.position = target;
    for (const Vec2i& cell : affectedCells(state, actor, target, target)) {
      if (state.isFriendlyAt(actor.team, cell)) {
        continue;
      }
      state.damageCell(cell, landingDamage_, actor.team, false);
      if (state.findUnitAt(cell) != nullptr && statusDuration_ > 0) {
        state.applyStatus(cell, status_, statusDuration_, statusMagnitude_);
      }
    }
  }

 private:
  AbilityId id_{AbilityId::Shadowstep};
  int range_{1};
  int cooldown_{0};
  int landingDamage_{1};
  StatusEffectType status_{StatusEffectType::Slow};
  int statusDuration_{0};
  int statusMagnitude_{0};
};

const SingleTargetAbility kBreachShot{AbilityId::BreachShot, 2, 0, 2, 0, 0, 1, TargetAffinity::Enemy, StatusEffectType::Slow, 0, 0,
                                      true};
const BlastAbility kMeteorRain{AbilityId::MeteorRain, 4, 3, 2, 1, StatusEffectType::Burn, 2, 1};
const SingleTargetAbility kGuardianPulse{AbilityId::GuardianPulse, 3, 3, 0, 1, 2, 0, TargetAffinity::Ally, StatusEffectType::Burn, 0,
                                         0, false};
const DashAbility kShadowstep{AbilityId::Shadowstep, 3, 3, 2, StatusEffectType::Slow, 2, 1};
const SingleTargetAbility kOracleWard{AbilityId::OracleWard, 4, 3, 0, 3, 2, 0, TargetAffinity::Ally, StatusEffectType::Burn, 0, 0,
                                      false};
const BlastAbility kScarabMortar{AbilityId::ScarabMortar, 4, 1, 2, 1, StatusEffectType::Burn, 1, 1};
const SingleTargetAbility kBeetleCharge{AbilityId::BeetleCharge, 2, 2, 3, 0, 0, 1, TargetAffinity::Enemy, StatusEffectType::Stun, 1,
                                        1, true};
const BeamAbility kFireflyBeam{AbilityId::FireflyBeam, 4, 1, 2, StatusEffectType::Burn, 1, 1};
const BlastAbility kVenomBurst{AbilityId::VenomBurst, 3, 2, 1, 1, StatusEffectType::Slow, 2, 1};
const BlastAbility kOverseerCataclysm{AbilityId::OverseerCataclysm, 5, 3, 3, 2, StatusEffectType::Burn, 2, 1};

}  // namespace

std::string Ability::name() const {
  return toString(id());
}

const Ability& abilityFor(AbilityId id) {
  switch (id) {
    case AbilityId::BreachShot:
      return kBreachShot;
    case AbilityId::MeteorRain:
      return kMeteorRain;
    case AbilityId::GuardianPulse:
      return kGuardianPulse;
    case AbilityId::Shadowstep:
      return kShadowstep;
    case AbilityId::OracleWard:
      return kOracleWard;
    case AbilityId::ScarabMortar:
      return kScarabMortar;
    case AbilityId::BeetleCharge:
      return kBeetleCharge;
    case AbilityId::FireflyBeam:
      return kFireflyBeam;
    case AbilityId::VenomBurst:
      return kVenomBurst;
    case AbilityId::OverseerCataclysm:
      return kOverseerCataclysm;
  }

  return kBreachShot;
}

const Ability& abilityFor(const UnitRecord& unit, int abilitySlot) {
  const auto stats = unit.stats();
  const int clampedSlot = std::clamp(abilitySlot, 0, kAbilitySlotCount - 1);
  return abilityFor(stats.abilities[static_cast<std::size_t>(clampedSlot)]);
}

}  // namespace Game
