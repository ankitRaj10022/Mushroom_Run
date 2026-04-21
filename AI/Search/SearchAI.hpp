#pragma once

#include "AI/Heuristics/IEvaluator.hpp"
#include "AI/IAI.hpp"
#include "AI/ML/GameStateEncoder.hpp"
#include "AI/ML/MLModel.hpp"
#include "AI/ML/PlayerInfluenceModel.hpp"
#include "AI/Search/TranspositionTable.hpp"
#include "Utils/Zobrist.hpp"

#include <array>
#include <chrono>
#include <memory>

namespace Game {

class SearchAI final : public IAI {
 public:
  explicit SearchAI(std::shared_ptr<IEvaluator> evaluator);

  bool loadModel(const std::string& modelPath);
  bool modelAvailable() const;
  void clearCaches();
  void setPlayerInfluenceModel(const PlayerInfluenceModel* model);

  std::optional<Action> chooseAction(const GameState& state,
                                     Team team,
                                     const AISettings& settings,
                                     SearchMetrics& metrics) override;
  std::vector<Action> planTurn(GameState state,
                               Team team,
                               const AISettings& settings,
                               SearchMetrics& metrics) override;

 private:
  SearchResult iterativeDeepening(const GameState& state, Team team, const AISettings& settings);
  SearchResult searchRoot(const GameState& state, Team team, int depth, const AISettings& settings);
  SearchResult greedyFallback(const GameState& state, Team team, const AISettings& settings) const;
  float searchNode(GameState& state, int depth, float alpha, float beta, Team maximizingTeam, int ply, const AISettings& settings);
  float quiescence(GameState& state, float alpha, float beta, Team maximizingTeam, int ply, int remainingDepth,
                   const AISettings& settings);
  float evaluateHybrid(const GameState& state, Team perspective, const AISettings& settings) const;
  std::vector<Action> orderActions(const GameState& state,
                                   std::vector<Action> actions,
                                   int ply,
                                   const AISettings& settings,
                                   const Action* ttBest) const;
  int orderingScore(const GameState& state, const Action& action, int ply, const AISettings& settings,
                    const Action* ttBest) const;
  void rememberKiller(int ply, const Action& action);
  bool isKiller(int ply, const Action& action) const;
  bool usePruning(const AISettings& settings) const;
  bool outOfTime() const;

  std::shared_ptr<IEvaluator> evaluator_;
  GameStateEncoder encoder_;
  MLModel model_;
  ZobristHasher hasher_;
  mutable TranspositionTable transpositionTable_;
  mutable std::vector<std::array<Action, 2>> killerMoves_;
  const PlayerInfluenceModel* playerInfluenceModel_{nullptr};
  std::chrono::steady_clock::time_point deadline_{};
  SearchMetrics metrics_{};
  bool timedOut_{false};
};

}  // namespace Game
