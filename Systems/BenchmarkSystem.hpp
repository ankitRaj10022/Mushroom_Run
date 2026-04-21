#pragma once

#include "AI/Heuristics/IEvaluator.hpp"
#include "AI/MCTS/MCTSAI.hpp"
#include "AI/Search/SearchAI.hpp"
#include "Core/Scenario.hpp"

#include <memory>
#include <vector>

namespace Game {

struct BenchmarkResult {
  AIMode firstMode{AIMode::AlphaBeta};
  AIMode secondMode{AIMode::MCTS};
  int games{0};
  int firstWins{0};
  int secondWins{0};
  double averageFirstDecisionMs{0.0};
  double averageSecondDecisionMs{0.0};
  std::uint64_t averageNodes{0};
};

class BenchmarkSystem {
 public:
  explicit BenchmarkSystem(std::shared_ptr<IEvaluator> evaluator);

  void loadModel(const std::string& modelPath);
  BenchmarkResult runBenchmark(const ScenarioDefinition& scenario,
                               AIMode firstMode,
                               AIMode secondMode,
                               int games,
                               const AISettings& settings);

 private:
  std::optional<Action> chooseActionForMode(const GameState& state,
                                            Team team,
                                            AIMode mode,
                                            const AISettings& settings,
                                            SearchMetrics& metrics);

  SearchAI searchAI_;
  MCTSAI mctsAI_;
};

}  // namespace Game
