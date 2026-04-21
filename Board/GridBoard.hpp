#pragma once

#include "Board/Tile.hpp"
#include "Utils/Vec2i.hpp"

#include <array>
#include <vector>

namespace Game {

class GridBoard {
 public:
  static constexpr int kWidth = 8;
  static constexpr int kHeight = 8;
  static constexpr int kCellCount = kWidth * kHeight;

  GridBoard();

  bool inBounds(const Vec2i& position) const;
  int index(const Vec2i& position) const;

  Tile& tileAt(const Vec2i& position);
  const Tile& tileAt(const Vec2i& position) const;

  std::vector<Vec2i> allPositions() const;

  static GridBoard createSkirmishBoard();
  static GridBoard createFoundryBoard();
  static GridBoard createHarborBoard();

 private:
  std::array<Tile, kCellCount> tiles_{};
};

}  // namespace Game
