#pragma once

#include "Core/GameTypes.hpp"
#include "Entities/Entity.hpp"

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/System/Vector2.hpp>

#include <memory>

namespace Game {

class Unit : public Entity {
 public:
  explicit Unit(UnitRecord record);
  ~Unit() override = default;

  int id() const;
  Team team() const;
  const UnitRecord& record() const;

  void sync(const UnitRecord& record, const sf::Vector2f& targetPosition, bool snapToTarget);
  void pulseAttack();
  void pulseHit();

  void update(float deltaSeconds) override;
  void render(sf::RenderTarget& target) const override;

 protected:
  virtual sf::Color primaryColor() const = 0;

 private:
  UnitRecord record_{};
  sf::Vector2f worldPosition_{};
  sf::Vector2f targetPosition_{};
  float animationTime_{0.0f};
  float movementEnergy_{0.0f};
  float attackPulse_{0.0f};
  float hitPulse_{0.0f};
};

class PlayerUnit final : public Unit {
 public:
  using Unit::Unit;

 protected:
  sf::Color primaryColor() const override;
};

class EnemyUnit final : public Unit {
 public:
  using Unit::Unit;

 protected:
  sf::Color primaryColor() const override;
};

std::unique_ptr<Unit> makeUnitView(const UnitRecord& record);

}  // namespace Game
