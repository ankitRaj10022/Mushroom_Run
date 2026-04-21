#include "Systems/CampaignSystem.hpp"

#include <algorithm>

namespace Game {

namespace {

std::vector<HeroProgress> defaultRoster() {
  return {
      {UnitClass::CannonMech, true, 1, 0, 0, 0, 0},
      {UnitClass::ArtilleryMech, true, 1, 0, 0, 0, 0},
      {UnitClass::GuardMech, true, 1, 0, 0, 0, 0},
      {UnitClass::ShadowBlade, false, 1, 0, 0, 0, 0},
      {UnitClass::OraclePrime, false, 1, 0, 0, 0, 0},
  };
}

int levelThreshold(int level) {
  return 3 + level * 2;
}

}  // namespace

CampaignSystem::CampaignSystem() {
  resetCampaign();
}

void CampaignSystem::resetCampaign() {
  credits_ = 90;
  campaignXp_ = 0;
  missionsCleared_ = 0;
  roster_ = defaultRoster();
  activeSquad_ = {UnitClass::CannonMech, UnitClass::ArtilleryMech, UnitClass::GuardMech};
  unlockedMissions_.assign(8, false);
  unlockMission(0);
}

int CampaignSystem::credits() const {
  return credits_;
}

int CampaignSystem::campaignXp() const {
  return campaignXp_;
}

int CampaignSystem::missionsCleared() const {
  return missionsCleared_;
}

const std::vector<HeroProgress>& CampaignSystem::roster() const {
  return roster_;
}

const std::vector<UnitClass>& CampaignSystem::activeSquad() const {
  return activeSquad_;
}

bool CampaignSystem::isHeroUnlocked(UnitClass heroClass) const {
  const HeroProgress* hero = heroProgress(heroClass);
  return hero != nullptr && hero->unlocked;
}

bool CampaignSystem::canSelectHero(UnitClass heroClass) const {
  return isHeroUnlocked(heroClass) &&
         (std::find(activeSquad_.begin(), activeSquad_.end(), heroClass) != activeSquad_.end() || activeSquad_.size() < 4);
}

void CampaignSystem::toggleSquadMember(UnitClass heroClass) {
  if (!isHeroUnlocked(heroClass)) {
    return;
  }

  const auto iterator = std::find(activeSquad_.begin(), activeSquad_.end(), heroClass);
  if (iterator != activeSquad_.end()) {
    if (activeSquad_.size() > 2) {
      activeSquad_.erase(iterator);
    }
    return;
  }

  if (activeSquad_.size() < 4) {
    activeSquad_.push_back(heroClass);
  }
}

bool CampaignSystem::isMissionUnlocked(std::size_t scenarioIndex) const {
  return scenarioIndex < unlockedMissions_.size() && unlockedMissions_[scenarioIndex];
}

void CampaignSystem::unlockMission(std::size_t scenarioIndex) {
  if (scenarioIndex >= unlockedMissions_.size()) {
    unlockedMissions_.resize(scenarioIndex + 1, false);
  }
  unlockedMissions_[scenarioIndex] = true;
}

std::vector<UnitRecord> CampaignSystem::buildSquad(const std::vector<Vec2i>& spawnCells, int startingUnitId) const {
  std::vector<UnitRecord> squad;
  const std::size_t count = std::min(activeSquad_.size(), spawnCells.size());
  squad.reserve(count);

  for (std::size_t index = 0; index < count; ++index) {
    const UnitClass heroClass = activeSquad_[index];
    const HeroProgress* hero = heroProgress(heroClass);
    if (hero == nullptr || !hero->unlocked) {
      continue;
    }

    UnitRecord unit;
    unit.id = startingUnitId + static_cast<int>(index);
    unit.team = Team::Player;
    unit.unitClass = heroClass;
    unit.position = spawnCells[index];
    unit.level = hero->level;
    unit.bonusHealth = hero->bonusHealth;
    unit.bonusMove = hero->bonusMove;
    unit.bonusDamage = hero->bonusDamage;
    unit.health = unit.stats().maxHealth;
    unit.experienceValue = hero->level;
    squad.push_back(unit);
  }

  return squad;
}

void CampaignSystem::applyVictoryRewards(const MissionContext& mission, const std::vector<UnitRecord>& survivingSquad) {
  credits_ += mission.rewardCredits;
  campaignXp_ += mission.rewardXp;
  ++missionsCleared_;

  for (const UnitRecord& unit : survivingSquad) {
    if (unit.team != Team::Player || !unit.alive()) {
      continue;
    }

    HeroProgress* hero = mutableHeroProgress(unit.unitClass);
    if (hero == nullptr) {
      continue;
    }

    hero->xp += mission.rewardXp;
    while (hero->xp >= levelThreshold(hero->level)) {
      hero->xp -= levelThreshold(hero->level);
      ++hero->level;
      ++hero->bonusHealth;
      hero->bonusDamage += hero->level % 2 == 0 ? 1 : 0;
    }
  }

  unlockMission(static_cast<std::size_t>(missionsCleared_));
  unlockRosterByProgress();
}

bool CampaignSystem::spendUpgrade(UnitClass heroClass, UpgradeFocus focus) {
  HeroProgress* hero = mutableHeroProgress(heroClass);
  if (hero == nullptr || !hero->unlocked) {
    return false;
  }

  const int cost = upgradeCost(*hero, focus);
  if (credits_ < cost) {
    return false;
  }

  credits_ -= cost;
  switch (focus) {
    case UpgradeFocus::Vitality:
      ++hero->bonusHealth;
      break;
    case UpgradeFocus::Power:
      ++hero->bonusDamage;
      break;
    case UpgradeFocus::Mobility:
      ++hero->bonusMove;
      break;
  }
  return true;
}

const HeroProgress* CampaignSystem::heroProgress(UnitClass heroClass) const {
  const auto iterator = std::find_if(roster_.begin(), roster_.end(), [&](const HeroProgress& hero) {
    return hero.heroClass == heroClass;
  });
  return iterator == roster_.end() ? nullptr : &(*iterator);
}

HeroProgress* CampaignSystem::mutableHeroProgress(UnitClass heroClass) {
  const auto iterator = std::find_if(roster_.begin(), roster_.end(), [&](const HeroProgress& hero) {
    return hero.heroClass == heroClass;
  });
  return iterator == roster_.end() ? nullptr : &(*iterator);
}

int CampaignSystem::upgradeCost(const HeroProgress& hero, UpgradeFocus focus) {
  switch (focus) {
    case UpgradeFocus::Vitality:
      return 30 + hero.bonusHealth * 10;
    case UpgradeFocus::Power:
      return 35 + hero.bonusDamage * 12;
    case UpgradeFocus::Mobility:
      return 40 + hero.bonusMove * 15;
  }
  return 40;
}

void CampaignSystem::unlockRosterByProgress() {
  if (missionsCleared_ >= 1) {
    if (HeroProgress* hero = mutableHeroProgress(UnitClass::ShadowBlade)) {
      hero->unlocked = true;
    }
  }

  if (missionsCleared_ >= 2) {
    if (HeroProgress* hero = mutableHeroProgress(UnitClass::OraclePrime)) {
      hero->unlocked = true;
    }
  }
}

}  // namespace Game
