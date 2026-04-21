#pragma once

#include <SFML/Graphics/RenderTarget.hpp>

namespace Game {

class Entity {
 public:
  virtual ~Entity() = default;
  virtual void update(float deltaSeconds) = 0;
  virtual void render(sf::RenderTarget& target) const = 0;
};

}  // namespace Game
