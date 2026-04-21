#pragma once

#include "Utils/Vec2i.hpp"

#include <array>
#include <stdexcept>
#include <string>

namespace Game {

enum class Team {
  Player,
  Enemy
};

inline Team opposingTeam(Team team) {
  return team == Team::Player ? Team::Enemy : Team::Player;
}

enum class GameMode {
  Campaign,
  Survival,
  Sandbox
};

enum class HeroRole {
  Tank,
  Assassin,
  Mage,
  Support,
  Bruiser,
  Siege,
  Boss
};

enum class AttackPattern {
  Adjacent,
  Line,
  Blast
};

enum class AbilityId {
  BreachShot,
  MeteorRain,
  GuardianPulse,
  Shadowstep,
  OracleWard,
  ScarabMortar,
  BeetleCharge,
  FireflyBeam,
  VenomBurst,
  OverseerCataclysm
};

enum class StatusEffectType {
  Burn,
  Slow,
  Stun,
  Count
};

enum class MissionObjectiveType {
  EliminateAll,
  DefendBase,
  Escort,
  CaptureZones,
  BossFight,
  Survival
};

enum class UpgradeFocus {
  Vitality,
  Power,
  Mobility
};

enum class UnitClass {
  CannonMech,
  ArtilleryMech,
  GuardMech,
  ShadowBlade,
  OraclePrime,
  Scarab,
  Beetle,
  Firefly,
  Brute,
  Overseer
};

constexpr int kAbilitySlotCount = 3;
constexpr int kStatusEffectCount = static_cast<int>(StatusEffectType::Count);

inline int statusIndex(StatusEffectType type) {
  return static_cast<int>(type);
}

inline std::string toString(Team team) {
  return team == Team::Player ? "Player" : "Enemy";
}

inline std::string toString(GameMode mode) {
  switch (mode) {
    case GameMode::Campaign:
      return "Campaign";
    case GameMode::Survival:
      return "Survival";
    case GameMode::Sandbox:
      return "AI Sandbox";
  }
  return "Unknown";
}

inline std::string toString(HeroRole role) {
  switch (role) {
    case HeroRole::Tank:
      return "Tank";
    case HeroRole::Assassin:
      return "Assassin";
    case HeroRole::Mage:
      return "Mage";
    case HeroRole::Support:
      return "Support";
    case HeroRole::Bruiser:
      return "Bruiser";
    case HeroRole::Siege:
      return "Siege";
    case HeroRole::Boss:
      return "Boss";
  }
  return "Unknown";
}

inline std::string toString(AttackPattern pattern) {
  switch (pattern) {
    case AttackPattern::Adjacent:
      return "Adjacent";
    case AttackPattern::Line:
      return "Line";
    case AttackPattern::Blast:
      return "Blast";
  }
  return "Unknown";
}

inline std::string toString(AbilityId ability) {
  switch (ability) {
    case AbilityId::BreachShot:
      return "Breach Shot";
    case AbilityId::MeteorRain:
      return "Meteor Rain";
    case AbilityId::GuardianPulse:
      return "Guardian Pulse";
    case AbilityId::Shadowstep:
      return "Shadowstep";
    case AbilityId::OracleWard:
      return "Oracle Ward";
    case AbilityId::ScarabMortar:
      return "Scarab Mortar";
    case AbilityId::BeetleCharge:
      return "Beetle Charge";
    case AbilityId::FireflyBeam:
      return "Firefly Beam";
    case AbilityId::VenomBurst:
      return "Venom Burst";
    case AbilityId::OverseerCataclysm:
      return "Overseer Cataclysm";
  }
  return "Unknown";
}

inline std::string toString(StatusEffectType status) {
  switch (status) {
    case StatusEffectType::Burn:
      return "Burn";
    case StatusEffectType::Slow:
      return "Slow";
    case StatusEffectType::Stun:
      return "Stun";
    case StatusEffectType::Count:
      break;
  }
  return "Unknown";
}

inline std::string toString(MissionObjectiveType objective) {
  switch (objective) {
    case MissionObjectiveType::EliminateAll:
      return "Eliminate all enemies";
    case MissionObjectiveType::DefendBase:
      return "Defend the base";
    case MissionObjectiveType::Escort:
      return "Escort the payload";
    case MissionObjectiveType::CaptureZones:
      return "Capture control zones";
    case MissionObjectiveType::BossFight:
      return "Defeat the boss";
    case MissionObjectiveType::Survival:
      return "Survive the assault";
  }
  return "Unknown";
}

inline std::string toString(UpgradeFocus focus) {
  switch (focus) {
    case UpgradeFocus::Vitality:
      return "Vitality";
    case UpgradeFocus::Power:
      return "Power";
    case UpgradeFocus::Mobility:
      return "Mobility";
  }
  return "Unknown";
}

inline std::string toString(UnitClass unitClass) {
  switch (unitClass) {
    case UnitClass::CannonMech:
      return "Vanguard";
    case UnitClass::ArtilleryMech:
      return "Arcanist";
    case UnitClass::GuardMech:
      return "Bulwark";
    case UnitClass::ShadowBlade:
      return "Shadow Blade";
    case UnitClass::OraclePrime:
      return "Oracle Prime";
    case UnitClass::Scarab:
      return "Scarab";
    case UnitClass::Beetle:
      return "Beetle";
    case UnitClass::Firefly:
      return "Firefly";
    case UnitClass::Brute:
      return "Brute";
    case UnitClass::Overseer:
      return "Overseer";
  }
  return "Unknown";
}

struct UnitStats {
  std::string name;
  HeroRole role{HeroRole::Bruiser};
  int maxHealth{1};
  int moveRange{1};
  int attackRange{1};
  int attackDamage{1};
  AttackPattern pattern{AttackPattern::Adjacent};
  int pushForce{0};
  bool flying{false};
  std::array<AbilityId, kAbilitySlotCount> abilities{
      AbilityId::BreachShot,
      AbilityId::BreachShot,
      AbilityId::BreachShot,
  };
};

inline const UnitStats& statsFor(UnitClass unitClass) {
  static const UnitStats kCannonMech{"Vanguard", HeroRole::Tank, 7, 3, 2, 2, AttackPattern::Adjacent, 1, false,
                                     {AbilityId::BreachShot, AbilityId::GuardianPulse, AbilityId::BreachShot}};
  static const UnitStats kArtilleryMech{"Arcanist", HeroRole::Mage, 5, 3, 4, 2, AttackPattern::Blast, 0, false,
                                        {AbilityId::MeteorRain, AbilityId::BreachShot, AbilityId::MeteorRain}};
  static const UnitStats kGuardMech{"Bulwark", HeroRole::Tank, 8, 2, 2, 2, AttackPattern::Line, 1, false,
                                    {AbilityId::GuardianPulse, AbilityId::BreachShot, AbilityId::OracleWard}};
  static const UnitStats kShadowBlade{"Shadow Blade", HeroRole::Assassin, 5, 4, 3, 3, AttackPattern::Adjacent, 0, false,
                                      {AbilityId::Shadowstep, AbilityId::VenomBurst, AbilityId::BreachShot}};
  static const UnitStats kOraclePrime{"Oracle Prime", HeroRole::Support, 6, 3, 3, 1, AttackPattern::Line, 0, false,
                                      {AbilityId::OracleWard, AbilityId::GuardianPulse, AbilityId::BreachShot}};
  static const UnitStats kScarab{"Scarab", HeroRole::Siege, 4, 3, 4, 2, AttackPattern::Blast, 0, false,
                                 {AbilityId::ScarabMortar, AbilityId::VenomBurst, AbilityId::ScarabMortar}};
  static const UnitStats kBeetle{"Beetle", HeroRole::Bruiser, 7, 3, 1, 3, AttackPattern::Adjacent, 1, false,
                                 {AbilityId::BeetleCharge, AbilityId::BreachShot, AbilityId::BeetleCharge}};
  static const UnitStats kFirefly{"Firefly", HeroRole::Siege, 4, 4, 4, 2, AttackPattern::Line, 0, true,
                                  {AbilityId::FireflyBeam, AbilityId::VenomBurst, AbilityId::FireflyBeam}};
  static const UnitStats kBrute{"Brute", HeroRole::Bruiser, 9, 2, 1, 4, AttackPattern::Adjacent, 1, false,
                                {AbilityId::BeetleCharge, AbilityId::GuardianPulse, AbilityId::VenomBurst}};
  static const UnitStats kOverseer{"Overseer", HeroRole::Boss, 16, 3, 5, 4, AttackPattern::Blast, 1, true,
                                   {AbilityId::OverseerCataclysm, AbilityId::FireflyBeam, AbilityId::GuardianPulse}};

  switch (unitClass) {
    case UnitClass::CannonMech:
      return kCannonMech;
    case UnitClass::ArtilleryMech:
      return kArtilleryMech;
    case UnitClass::GuardMech:
      return kGuardMech;
    case UnitClass::ShadowBlade:
      return kShadowBlade;
    case UnitClass::OraclePrime:
      return kOraclePrime;
    case UnitClass::Scarab:
      return kScarab;
    case UnitClass::Beetle:
      return kBeetle;
    case UnitClass::Firefly:
      return kFirefly;
    case UnitClass::Brute:
      return kBrute;
    case UnitClass::Overseer:
      return kOverseer;
  }

  throw std::runtime_error("Unsupported unit class");
}

struct MissionContext {
  MissionObjectiveType objective{MissionObjectiveType::EliminateAll};
  int turnLimit{0};
  int controlZoneCount{0};
  std::array<Vec2i, 4> controlZones{};
  int requiredControlZones{0};
  int controlHeldTurns{0};
  int controlHeldTurnsRequired{1};
  int escortUnitId{-1};
  Vec2i escortDestination{};
  int bossUnitId{-1};
  int rewardCredits{40};
  int rewardXp{2};
  int survivalTargetTurns{0};
};

struct UnitRecord {
  int id{-1};
  Team team{Team::Player};
  UnitClass unitClass{UnitClass::CannonMech};
  int health{1};
  Vec2i position{};
  bool acted{false};
  int shield{0};
  int level{1};
  int experienceValue{1};
  int bonusHealth{0};
  int bonusMove{0};
  int bonusDamage{0};
  bool objectiveUnit{false};
  std::array<int, kAbilitySlotCount> cooldowns{};
  std::array<int, kStatusEffectCount> statusDurations{};
  std::array<int, kStatusEffectCount> statusMagnitudes{};

  UnitStats stats() const {
    UnitStats adjusted = statsFor(unitClass);
    adjusted.maxHealth += bonusHealth;
    adjusted.moveRange = std::max(1, adjusted.moveRange + bonusMove - statusMagnitude(StatusEffectType::Slow));
    adjusted.attackDamage += bonusDamage;
    return adjusted;
  }

  bool alive() const {
    return health > 0;
  }

  bool hasStatus(StatusEffectType type) const {
    return statusDurations[static_cast<std::size_t>(statusIndex(type))] > 0;
  }

  int statusMagnitude(StatusEffectType type) const {
    return statusMagnitudes[static_cast<std::size_t>(statusIndex(type))];
  }
};

}  // namespace Game
