#pragma once

#include "Core/GameTypes.hpp"

#include <optional>
#include <vector>

namespace Game {

struct HeroProgress {
  UnitClass heroClass{UnitClass::CannonMech};
  bool unlocked{false};
  int level{1};
  int xp{0};
  int bonusHealth{0};
  int bonusMove{0};
  int bonusDamage{0};
};

class CampaignSystem {
 public:
  CampaignSystem();

  void resetCampaign();

  int credits() const;
  int campaignXp() const;
  int missionsCleared() const;

  const std::vector<HeroProgress>& roster() const;
  const std::vector<UnitClass>& activeSquad() const;

  bool isHeroUnlocked(UnitClass heroClass) const;
  bool canSelectHero(UnitClass heroClass) const;
  void toggleSquadMember(UnitClass heroClass);

  bool isMissionUnlocked(std::size_t scenarioIndex) const;
  void unlockMission(std::size_t scenarioIndex);

  std::vector<UnitRecord> buildSquad(const std::vector<Vec2i>& spawnCells, int startingUnitId = 0) const;
  void applyVictoryRewards(const MissionContext& mission, const std::vector<UnitRecord>& survivingSquad);
  bool spendUpgrade(UnitClass heroClass, UpgradeFocus focus);

  const HeroProgress* heroProgress(UnitClass heroClass) const;

 private:
  HeroProgress* mutableHeroProgress(UnitClass heroClass);
  static int upgradeCost(const HeroProgress& hero, UpgradeFocus focus);
  void unlockRosterByProgress();

  int credits_{0};
  int campaignXp_{0};
  int missionsCleared_{0};
  std::vector<HeroProgress> roster_;
  std::vector<UnitClass> activeSquad_;
  std::vector<bool> unlockedMissions_;
};

}  // namespace Game
