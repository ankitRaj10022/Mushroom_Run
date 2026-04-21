#pragma once

#include "Moves/Action.hpp"

#include <cstdint>
#include <string>

namespace Game {

enum class AIMode {
  Minimax,
  AlphaBeta,
  Hybrid,
  MCTS
};

inline std::string toString(AIMode mode) {
  switch (mode) {
    case AIMode::Minimax:
      return "Minimax";
    case AIMode::AlphaBeta:
      return "Alpha-Beta";
    case AIMode::Hybrid:
      return "Hybrid ML + Search";
    case AIMode::MCTS:
      return "MCTS + Value Net";
  }
  return "Unknown";
}

struct AISettings {
  AIMode mode{AIMode::Hybrid};
  int maxDepth{4};
  int quiescenceDepth{2};
  int timeBudgetMs{280};
  int simulationDepth{8};
  bool enablePruning{true};
  bool enableMoveOrdering{true};
  bool enableQuiescence{true};
  bool enableKillerHeuristic{true};
  bool enableTranspositionTable{true};
  bool enableMLInfluence{true};
  float classicalWeight{0.75f};
  float mlWeight{0.25f};
  float mctsExploration{1.25f};
};

struct SearchMetrics {
  AIMode mode{AIMode::Hybrid};
  int requestedDepth{0};
  int completedDepth{0};
  std::uint64_t nodes{0};
  std::uint64_t quiescenceNodes{0};
  std::uint64_t prunes{0};
  std::uint64_t ttHits{0};
  double elapsedMs{0.0};
  float bestScore{0.0f};
  float classicalScore{0.0f};
  float mlScore{0.0f};
  float combinedRootScore{0.0f};
  std::uint64_t mctsIterations{0};
  bool modelAvailable{false};
};

struct SearchResult {
  Action action{};
  float score{0.0f};
  bool completed{false};
};

}  // namespace Game
