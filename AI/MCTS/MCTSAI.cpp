#include "AI/MCTS/MCTSAI.hpp"

#include "Moves/MoveGenerator.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>

namespace Game {

namespace {

constexpr float kModelScoreScale = 12.0f;

}  // namespace

MCTSAI::MCTSAI(std::shared_ptr<IEvaluator> evaluator) : evaluator_(std::move(evaluator)) {}

bool MCTSAI::loadModel(const std::string& modelPath) {
  return model_.load(modelPath);
}

bool MCTSAI::modelAvailable() const {
  return model_.available();
}

std::optional<Action> MCTSAI::chooseAction(const GameState& state,
                                           Team team,
                                           const AISettings& settings,
                                           SearchMetrics& metrics) {
  metrics = {};
  metrics.mode = AIMode::MCTS;
  metrics.requestedDepth = settings.maxDepth;
  metrics.modelAvailable = model_.available();

  if (state.isTerminal() || state.activeTeam() != team) {
    return std::nullopt;
  }

  nodes_.clear();
  nodes_.push_back(Node{state, -1, {}, {}, orderedActions(state), 0, 0.0f});
  if (nodes_.front().unexpandedActions.empty()) {
    return std::nullopt;
  }

  const auto started = std::chrono::steady_clock::now();
  const auto deadline = started + std::chrono::milliseconds(settings.timeBudgetMs);

  while (std::chrono::steady_clock::now() < deadline) {
    const int leaf = selectAndExpand(team, settings);
    const float value = rollout(nodes_[static_cast<std::size_t>(leaf)].state, team, settings);

    for (int nodeIndex = leaf; nodeIndex >= 0; nodeIndex = nodes_[static_cast<std::size_t>(nodeIndex)].parent) {
      Node& node = nodes_[static_cast<std::size_t>(nodeIndex)];
      ++node.visits;
      node.totalValue += value;
    }

    ++metrics.mctsIterations;
  }

  const auto ended = std::chrono::steady_clock::now();
  metrics.elapsedMs = std::chrono::duration<double, std::milli>(ended - started).count();
  metrics.nodes = static_cast<std::uint64_t>(nodes_.size());

  const Node& root = nodes_.front();
  Action bestAction = Action::invalid();
  float bestValue = -1.0e9f;
  int bestVisits = -1;

  for (const int childIndex : root.children) {
    const Node& child = nodes_[static_cast<std::size_t>(childIndex)];
    if (child.visits == 0) {
      continue;
    }

    const float meanValue = child.totalValue / static_cast<float>(child.visits);
    if (child.visits > bestVisits || (child.visits == bestVisits && meanValue > bestValue)) {
      bestVisits = child.visits;
      bestValue = meanValue;
      bestAction = child.action;
    }
  }

  metrics.bestScore = bestValue;
  metrics.classicalScore = evaluator_->evaluate(state, team);
  metrics.mlScore = model_.available() ? model_.predict(encoder_.encode(state, team)) * kModelScoreScale : 0.0f;
  metrics.combinedRootScore = evaluateHybrid(state, team, settings);

  return bestAction.isValid() ? std::optional<Action>{bestAction} : std::nullopt;
}

std::vector<Action> MCTSAI::planTurn(GameState state,
                                     Team team,
                                     const AISettings& settings,
                                     SearchMetrics& metrics) {
  std::vector<Action> plan;
  SearchMetrics latest{};
  int guard = 0;
  while (!state.isTerminal() && state.activeTeam() == team && guard++ < 16) {
    const std::optional<Action> action = chooseAction(state, team, settings, latest);
    if (!action) {
      break;
    }
    plan.push_back(*action);
    state.applyAction(*action);
  }
  metrics = latest;
  return plan;
}

int MCTSAI::selectAndExpand(Team rootTeam, const AISettings& settings) {
  (void)rootTeam;
  int nodeIndex = 0;
  while (true) {
    Node& node = nodes_[static_cast<std::size_t>(nodeIndex)];
    if (node.state.isTerminal()) {
      return nodeIndex;
    }

    if (!node.unexpandedActions.empty()) {
      Action action = node.unexpandedActions.back();
      node.unexpandedActions.pop_back();

      GameState next = node.state;
      next.applyAction(action);

      Node child;
      child.state = std::move(next);
      child.parent = nodeIndex;
      child.action = action;
      child.unexpandedActions = orderedActions(child.state);

      const int childIndex = static_cast<int>(nodes_.size());
      nodes_.push_back(std::move(child));
      nodes_[static_cast<std::size_t>(nodeIndex)].children.push_back(childIndex);
      return childIndex;
    }

    if (node.children.empty()) {
      return nodeIndex;
    }

    nodeIndex = bestChildUcb(nodeIndex, settings);
    if (nodeIndex < 0) {
      return 0;
    }
  }
}

int MCTSAI::bestChildUcb(int nodeIndex, const AISettings& settings) const {
  const Node& node = nodes_[static_cast<std::size_t>(nodeIndex)];
  int bestChild = -1;
  float bestScore = -1.0e9f;
  const float parentVisits = static_cast<float>(std::max(1, node.visits));

  for (const int childIndex : node.children) {
    const Node& child = nodes_[static_cast<std::size_t>(childIndex)];
    if (child.visits == 0) {
      return childIndex;
    }

    const float exploitation = child.totalValue / static_cast<float>(child.visits);
    const float exploration =
        settings.mctsExploration * std::sqrt(std::log(parentVisits + 1.0f) / static_cast<float>(child.visits));
    const float score = exploitation + exploration;
    if (score > bestScore) {
      bestScore = score;
      bestChild = childIndex;
    }
  }

  return bestChild;
}

float MCTSAI::rollout(GameState state, Team perspective, const AISettings& settings) {
  for (int depth = 0; depth < settings.simulationDepth && !state.isTerminal(); ++depth) {
    std::vector<Action> actions = orderedActions(state);
    if (actions.empty()) {
      state.forceEndTurn();
      continue;
    }

    const int sampleCount = std::min<int>(3, static_cast<int>(actions.size()));
    std::uniform_int_distribution<int> distribution(0, sampleCount - 1);
    state.applyAction(actions[static_cast<std::size_t>(distribution(rng_))]);
  }

  return evaluateHybrid(state, perspective, settings);
}

float MCTSAI::evaluateHybrid(const GameState& state, Team perspective, const AISettings& settings) const {
  const float classicalScore = evaluator_->evaluate(state, perspective);
  if (!settings.enableMLInfluence || !model_.available()) {
    return classicalScore;
  }

  const float mlScore = model_.predict(encoder_.encode(state, perspective)) * kModelScoreScale;
  return settings.classicalWeight * classicalScore + settings.mlWeight * mlScore;
}

std::vector<Action> MCTSAI::orderedActions(const GameState& state) const {
  std::vector<Action> actions = MoveGenerator::generateActions(state);
  std::sort(actions.begin(), actions.end(), [&](const Action& lhs, const Action& rhs) {
    const int lhsDamage = MoveGenerator::estimateActionDamage(state, lhs);
    const int rhsDamage = MoveGenerator::estimateActionDamage(state, rhs);
    if (lhsDamage != rhsDamage) {
      return lhsDamage > rhsDamage;
    }
    return lhs.wait < rhs.wait;
  });
  return actions;
}

}  // namespace Game
