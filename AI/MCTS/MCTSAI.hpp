#pragma once

#include "AI/Heuristics/IEvaluator.hpp"
#include "AI/IAI.hpp"
#include "AI/ML/GameStateEncoder.hpp"
#include "AI/ML/MLModel.hpp"

#include <memory>
#include <random>

namespace Game {

class MCTSAI final : public IAI {
 public:
  explicit MCTSAI(std::shared_ptr<IEvaluator> evaluator);

  bool loadModel(const std::string& modelPath);
  bool modelAvailable() const;

  std::optional<Action> chooseAction(const GameState& state,
                                     Team team,
                                     const AISettings& settings,
                                     SearchMetrics& metrics) override;
  std::vector<Action> planTurn(GameState state,
                               Team team,
                               const AISettings& settings,
                               SearchMetrics& metrics) override;

 private:
  struct Node {
    GameState state;
    int parent{-1};
    Action action{};
    std::vector<int> children;
    std::vector<Action> unexpandedActions;
    int visits{0};
    float totalValue{0.0f};
  };

  int selectAndExpand(Team rootTeam, const AISettings& settings);
  int bestChildUcb(int nodeIndex, const AISettings& settings) const;
  float rollout(GameState state, Team perspective, const AISettings& settings);
  float evaluateHybrid(const GameState& state, Team perspective, const AISettings& settings) const;
  std::vector<Action> orderedActions(const GameState& state) const;

  std::shared_ptr<IEvaluator> evaluator_;
  GameStateEncoder encoder_;
  MLModel model_;
  std::vector<Node> nodes_;
  std::mt19937 rng_{0xBAD5EEDu};
};

}  // namespace Game
