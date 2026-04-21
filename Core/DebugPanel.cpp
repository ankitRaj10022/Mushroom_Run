#include "Core/DebugPanel.hpp"

#include "Utils/FontLocator.hpp"

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Text.hpp>

#include <iomanip>
#include <sstream>

namespace Game {

DebugPanel::DebugPanel() {
  const std::string fontPath = locateFontFile();
  fontLoaded_ = !fontPath.empty() && font_.loadFromFile(fontPath);
}

bool DebugPanel::hasFont() const {
  return fontLoaded_;
}

void DebugPanel::render(sf::RenderTarget& target,
                        const SearchMetrics& metrics,
                        const AISettings& settings,
                        const GameState& state) const {
  sf::RectangleShape background({340.0f, 640.0f});
  background.setPosition({620.0f, 24.0f});
  background.setFillColor(sf::Color(17, 22, 30, 230));
  background.setOutlineThickness(2.0f);
  background.setOutlineColor(sf::Color(64, 80, 102));
  target.draw(background);

  if (!fontLoaded_) {
    return;
  }

  std::ostringstream lines;
  lines << "Tactical Hybrid AI\n";
  lines << "-----------------------------\n";
  lines << "Active Team: " << toString(state.activeTeam()) << "\n";
  lines << "Turn: " << state.turnNumber() << "\n";
  lines << "Mode: " << toString(settings.mode) << "\n";
  lines << "Depth: " << metrics.completedDepth << " / " << settings.maxDepth << "\n";
  lines << "Nodes: " << metrics.nodes << "\n";
  lines << "Q Nodes: " << metrics.quiescenceNodes << "\n";
  lines << "Prunes: " << metrics.prunes << "\n";
  lines << "TT Hits: " << metrics.ttHits << "\n";
  lines << std::fixed << std::setprecision(2);
  lines << "Time: " << metrics.elapsedMs << " ms\n";
  lines << "Best Score: " << metrics.bestScore << "\n";
  lines << "Heuristic: " << metrics.classicalScore << "\n";
  lines << "ML Score: " << metrics.mlScore << "\n";
  lines << "Root Blend: " << metrics.combinedRootScore << "\n";
  lines << "Model: " << (metrics.modelAvailable ? "Loaded" : "Unavailable") << "\n";
  lines << "\nControls\n";
  lines << "1 2 3  : Switch AI mode\n";
  lines << "[ ]    : Change search depth\n";
  lines << "P      : Toggle pruning\n";
  lines << "M      : Toggle ML influence\n";
  lines << "O/Q/K/T: Ordering, quiescence,\n";
  lines << "         killer moves, TT\n";
  lines << "- / =  : Adjust ML weight\n";
  lines << "R      : Reset scenario\n";
  lines << "Space  : Commit move/wait\n";
  lines << "Right  : Cancel selection stage\n";

  sf::Text text;
  text.setFont(font_);
  text.setCharacterSize(18U);
  text.setFillColor(sf::Color(224, 231, 239));
  text.setPosition({640.0f, 40.0f});
  text.setString(lines.str());
  target.draw(text);
}

}  // namespace Game
