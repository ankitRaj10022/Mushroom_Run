#include "Board/GridBoard.hpp"

#include <array>
#include <cmath>

namespace Game {

namespace {

void paintCells(GridBoard& board, const std::vector<Vec2i>& cells, TerrainType terrain, int controlValue, int structureHealth = 0) {
  for (const Vec2i& cell : cells) {
    board.tileAt(cell).terrain = terrain;
    board.tileAt(cell).controlValue = controlValue;
    board.tileAt(cell).structureHealth = structureHealth;
  }
}

void markSpawnPoints(GridBoard& board, const std::vector<Vec2i>& cells) {
  for (const Vec2i& cell : cells) {
    board.tileAt(cell).spawnPoint = true;
  }
}

}  // namespace

GridBoard::GridBoard() {
  for (int y = 0; y < kHeight; ++y) {
    for (int x = 0; x < kWidth; ++x) {
      const Vec2i cell{x, y};
      const float dx = static_cast<float>(x) - 3.5f;
      const float dy = static_cast<float>(y) - 3.5f;
      const float distance = std::sqrt(dx * dx + dy * dy);
      tileAt(cell) = Tile{TerrainType::Ground, distance < 1.6f ? 4 : (distance < 2.8f ? 3 : 1), 0, false};
    }
  }
}

bool GridBoard::inBounds(const Vec2i& position) const {
  return position.x >= 0 && position.x < kWidth && position.y >= 0 && position.y < kHeight;
}

int GridBoard::index(const Vec2i& position) const {
  return position.y * kWidth + position.x;
}

Tile& GridBoard::tileAt(const Vec2i& position) {
  return tiles_[index(position)];
}

const Tile& GridBoard::tileAt(const Vec2i& position) const {
  return tiles_[index(position)];
}

std::vector<Vec2i> GridBoard::allPositions() const {
  std::vector<Vec2i> positions;
  positions.reserve(kCellCount);
  for (int y = 0; y < kHeight; ++y) {
    for (int x = 0; x < kWidth; ++x) {
      positions.push_back({x, y});
    }
  }
  return positions;
}

GridBoard GridBoard::createSkirmishBoard() {
  GridBoard board;

  paintCells(board, {{2, 4}, {3, 4}, {4, 4}, {5, 4}, {2, 5}, {3, 5}}, TerrainType::Water, 0);
  paintCells(board, {{0, 6}, {0, 7}, {6, 0}, {7, 0}, {7, 1}}, TerrainType::Mountain, 0);
  paintCells(board, {{1, 6}, {4, 2}, {6, 2}}, TerrainType::Rock, 0);
  paintCells(board, {{1, 3}, {5, 1}, {6, 5}}, TerrainType::Forest, 2);
  paintCells(board, {{1, 2}, {2, 2}, {5, 5}, {6, 4}}, TerrainType::Building, 5, 3);
  paintCells(board, {{4, 6}, {5, 6}, {4, 7}}, TerrainType::Chasm, 0);
  markSpawnPoints(board, {{0, 4}, {7, 3}});

  return board;
}

GridBoard GridBoard::createFoundryBoard() {
  GridBoard board;

  paintCells(board, {{0, 0}, {1, 0}, {6, 7}, {7, 7}, {7, 6}}, TerrainType::Mountain, 0);
  paintCells(board, {{3, 1}, {3, 2}, {3, 3}, {4, 4}, {4, 5}, {4, 6}}, TerrainType::Chasm, 0);
  paintCells(board, {{2, 5}, {5, 2}, {6, 2}}, TerrainType::Rock, 0);
  paintCells(board, {{1, 3}, {1, 4}, {6, 4}}, TerrainType::Forest, 2);
  paintCells(board, {{2, 1}, {5, 1}, {2, 6}, {5, 6}}, TerrainType::Building, 5, 3);
  paintCells(board, {{0, 5}, {1, 5}, {2, 4}, {5, 3}, {6, 3}, {7, 3}}, TerrainType::Water, 0);
  markSpawnPoints(board, {{0, 3}, {7, 4}});

  return board;
}

GridBoard GridBoard::createHarborBoard() {
  GridBoard board;

  paintCells(board, {{0, 7}, {1, 7}, {6, 0}, {7, 0}, {7, 1}}, TerrainType::Mountain, 0);
  paintCells(board, {{1, 2}, {2, 2}, {3, 2}, {4, 2}, {4, 3}, {4, 4}, {5, 4}, {6, 4}}, TerrainType::Water, 0);
  paintCells(board, {{2, 5}, {3, 5}, {4, 6}}, TerrainType::Chasm, 0);
  paintCells(board, {{0, 4}, {1, 4}, {5, 1}, {6, 1}}, TerrainType::Forest, 2);
  paintCells(board, {{2, 1}, {3, 1}, {6, 5}, {6, 6}}, TerrainType::Building, 5, 3);
  paintCells(board, {{0, 5}, {5, 2}, {5, 6}}, TerrainType::Rock, 0);
  markSpawnPoints(board, {{0, 2}, {7, 5}});

  return board;
}

}  // namespace Game
