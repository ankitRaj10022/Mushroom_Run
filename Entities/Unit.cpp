#include "Entities/Unit.hpp"

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/ConvexShape.hpp>
#include <SFML/Graphics/RectangleShape.hpp>

#include <algorithm>
#include <cmath>

namespace Game {

namespace {

sf::Color darken(const sf::Color& color, int amount) {
  return sf::Color(
      static_cast<sf::Uint8>(std::max(0, color.r - amount)),
      static_cast<sf::Uint8>(std::max(0, color.g - amount)),
      static_cast<sf::Uint8>(std::max(0, color.b - amount)),
      color.a);
}

}  // namespace

Unit::Unit(UnitRecord record) : record_(record) {}

int Unit::id() const {
  return record_.id;
}

Team Unit::team() const {
  return record_.team;
}

const UnitRecord& Unit::record() const {
  return record_;
}

void Unit::sync(const UnitRecord& record, const sf::Vector2f& targetPosition, bool snapToTarget) {
  record_ = record;
  targetPosition_ = targetPosition;
  if (snapToTarget) {
    worldPosition_ = targetPosition_;
  }
}

void Unit::pulseAttack() {
  attackPulse_ = 0.22f;
}

void Unit::pulseHit() {
  hitPulse_ = 0.28f;
}

void Unit::update(float deltaSeconds) {
  const sf::Vector2f delta = targetPosition_ - worldPosition_;
  const float distance = std::sqrt(delta.x * delta.x + delta.y * delta.y);
  worldPosition_ += delta * std::min(1.0f, deltaSeconds * 10.0f);
  animationTime_ += deltaSeconds;
  movementEnergy_ = std::clamp(movementEnergy_ * (distance > 1.0f ? 0.96f : 0.88f) + distance * 0.012f, 0.0f, 1.0f);
  attackPulse_ = std::max(0.0f, attackPulse_ - deltaSeconds);
  hitPulse_ = std::max(0.0f, hitPulse_ - deltaSeconds);
}

void Unit::render(sf::RenderTarget& target) const {
  const float pulseScale = attackPulse_ > 0.0f ? 1.12f : 1.0f;
  const sf::Color base = hitPulse_ > 0.0f ? sf::Color(255, 235, 120) : primaryColor();
  const float hover = std::sin(animationTime_ * 4.0f + static_cast<float>(record_.id)) * 1.6f + movementEnergy_ * 3.4f;
  const sf::Vector2f renderPosition = worldPosition_ + sf::Vector2f(0.0f, -hover);

  sf::CircleShape shadow(20.0f);
  shadow.setOrigin(20.0f, 10.0f);
  shadow.setScale(1.3f + movementEnergy_ * 0.08f, 0.45f - movementEnergy_ * 0.05f);
  shadow.setPosition(worldPosition_ + sf::Vector2f(0.0f, 18.0f));
  shadow.setFillColor(sf::Color(8, 10, 15, 100));
  target.draw(shadow);

  if (record_.team == Team::Player) {
    sf::ConvexShape body(6);
    body.setPoint(0, {-18.0f, -10.0f});
    body.setPoint(1, {0.0f, -22.0f});
    body.setPoint(2, {18.0f, -10.0f});
    body.setPoint(3, {18.0f, 10.0f});
    body.setPoint(4, {0.0f, 20.0f});
    body.setPoint(5, {-18.0f, 10.0f});
    body.setPosition(renderPosition);
    body.setScale(pulseScale, pulseScale);
    body.setFillColor(base);
    body.setOutlineThickness(2.0f);
    body.setOutlineColor(darken(base, 95));
    target.draw(body);

    sf::RectangleShape cockpit({20.0f, 10.0f});
    cockpit.setOrigin(10.0f, 5.0f);
    cockpit.setPosition(renderPosition + sf::Vector2f(0.0f, -3.0f));
    cockpit.setFillColor(sf::Color(219, 229, 241));
    target.draw(cockpit);

    sf::RectangleShape cannon({26.0f, 6.0f});
    cannon.setOrigin(4.0f, 3.0f);
    cannon.setPosition(renderPosition + sf::Vector2f(2.0f, -8.0f));
    cannon.setRotation(record_.unitClass == UnitClass::ArtilleryMech ? -42.0f : -16.0f);
    cannon.setFillColor(darken(base, 40));
    target.draw(cannon);
  } else {
    sf::ConvexShape thorax(8);
    thorax.setPoint(0, {-14.0f, -14.0f});
    thorax.setPoint(1, {0.0f, -20.0f});
    thorax.setPoint(2, {14.0f, -14.0f});
    thorax.setPoint(3, {20.0f, 0.0f});
    thorax.setPoint(4, {14.0f, 14.0f});
    thorax.setPoint(5, {0.0f, 18.0f});
    thorax.setPoint(6, {-14.0f, 14.0f});
    thorax.setPoint(7, {-20.0f, 0.0f});
    thorax.setPosition(renderPosition);
    thorax.setScale(pulseScale, pulseScale);
    thorax.setFillColor(base);
    thorax.setOutlineThickness(2.0f);
    thorax.setOutlineColor(darken(base, 95));
    target.draw(thorax);

    for (int side = -1; side <= 1; side += 2) {
      for (int segment = 0; segment < 3; ++segment) {
        sf::RectangleShape leg({18.0f - static_cast<float>(segment) * 2.0f, 3.0f});
        leg.setOrigin(2.0f, 1.5f);
        leg.setPosition(renderPosition +
                        sf::Vector2f(static_cast<float>(side) * 8.0f, -4.0f + static_cast<float>(segment) * 9.0f));
        leg.setRotation(static_cast<float>(side) * (35.0f + static_cast<float>(segment) * 12.0f));
        leg.setFillColor(darken(base, 35));
        target.draw(leg);
      }
    }
  }

  const float healthRatio = static_cast<float>(record_.health) / static_cast<float>(record_.stats().maxHealth);
  sf::RectangleShape healthBack({46.0f, 6.0f});
  healthBack.setOrigin(23.0f, 32.0f);
  healthBack.setPosition(renderPosition);
  healthBack.setFillColor(sf::Color(12, 16, 22, 210));
  target.draw(healthBack);

  sf::RectangleShape healthFill({46.0f * std::max(0.0f, healthRatio), 6.0f});
  healthFill.setOrigin(23.0f, 32.0f);
  healthFill.setPosition(renderPosition);
  healthFill.setFillColor(record_.team == Team::Player ? sf::Color(92, 222, 187) : sf::Color(255, 132, 112));
  target.draw(healthFill);
}

sf::Color PlayerUnit::primaryColor() const {
  return sf::Color(84, 182, 255);
}

sf::Color EnemyUnit::primaryColor() const {
  return sf::Color(209, 114, 92);
}

std::unique_ptr<Unit> makeUnitView(const UnitRecord& record) {
  if (record.team == Team::Player) {
    return std::make_unique<PlayerUnit>(record);
  }
  return std::make_unique<EnemyUnit>(record);
}

}  // namespace Game
