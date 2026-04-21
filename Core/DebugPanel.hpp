#pragma once

#include "AI/Search/SearchTypes.hpp"
#include "Core/GameState.hpp"

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RenderTarget.hpp>

namespace Game {

class DebugPanel {
 public:
  DebugPanel();

  bool hasFont() const;
  void render(sf::RenderTarget& target, const SearchMetrics& metrics, const AISettings& settings, const GameState& state) const;

 private:
  sf::Font font_{};
  bool fontLoaded_{false};
};

}  // namespace Game
