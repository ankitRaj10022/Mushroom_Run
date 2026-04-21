#include "AI/Search/SearchAI.hpp"

#include "Moves/MoveGenerator.hpp"

#include <algorithm>
#include <limits>

namespace Game {

namespace {

constexpr float kInfinity = 1.0e9f;
constexpr float kModelScoreScale = 12.0f;

}  // namespace

SearchAI::SearchAI(std::shared_ptr<IEvaluator> evaluator) : evaluator_(std::move(evaluator)) {}

bool SearchAI::loadModel(const std::string& modelPath) {
  return model_.load(modelPath);
}

bool SearchAI::modelAvailable() const {
  return model_.available();
}

void SearchAI::clearCaches() {
  transpositionTable_.clear();
}

void SearchAI::setPlayerInfluenceModel(const PlayerInfluenceModel* model) {
  playerInfluenceModel_ = model;
}

std::optional<Action> SearchAI::chooseAction(const GameState& state,
                                             Team team,
                                             const AISettings& settings,
                                             SearchMetrics& metrics) {
  if (state.isTerminal() || state.activeTeam() != team) {
    return std::nullopt;
  }

  metrics_ = {};
  metrics_.mode = settings.mode;
  metrics_.requestedDepth = settings.maxDepth;
  metrics_.modelAvailable = model_.available();
  metrics_.classicalScore = evaluator_->evaluate(state, team);
  metrics_.mlScore =
      model_.available() ? model_.predict(encoder_.encode(state, team)) * kModelScoreScale : 0.0f;
  metrics_.combinedRootScore =
      settings.enableMLInfluence && settings.mode == AIMode::Hybrid && model_.available()
          ? settings.classicalWeight * metrics_.classicalScore + settings.mlWeight * metrics_.mlScore
          : metrics_.classicalScore;

  killerMoves_.assign(static_cast<std::size_t>(settings.maxDepth + settings.quiescenceDepth + 8),
                      {Action::invalid(), Action::invalid()});
  timedOut_ = false;
  deadline_ = std::chrono::steady_clock::now() + std::chrono::milliseconds(settings.timeBudgetMs);

  const auto started = std::chrono::steady_clock::now();
  const SearchResult result = iterativeDeepening(state, team, settings);
  const auto ended = std::chrono::steady_clock::now();
  metrics_.elapsedMs = std::chrono::duration<double, std::milli>(ended - started).count();
  metrics_.bestScore = result.score;

  SearchResult finalResult = result;
  if (!finalResult.action.isValid()) {
    finalResult = greedyFallback(state, team, settings);
    metrics_.bestScore = finalResult.score;
  }

  metrics = metrics_;
  if (!finalResult.action.isValid()) {
    return std::nullopt;
  }

  return finalResult.action;
}

std::vector<Action> SearchAI::planTurn(GameState state,
                                       Team team,
                                       const AISettings& settings,
                                       SearchMetrics& metrics) {
  std::vector<Action> plan;
  SearchMetrics latest{};
  int guard = 0;
  while (!state.isTerminal() && state.activeTeam() == team && guard++ < 12) {
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

SearchResult SearchAI::iterativeDeepening(const GameState& state, Team team, const AISettings& settings) {
  SearchResult best{};
  best.score = evaluator_->evaluate(state, team);

  for (int depth = 1; depth <= settings.maxDepth; ++depth) {
    SearchResult candidate = searchRoot(state, team, depth, settings);
    if (timedOut_) {
      break;
    }
    if (candidate.action.isValid()) {
      best = candidate;
      metrics_.completedDepth = depth;
    }
  }

  return best;
}

SearchResult SearchAI::searchRoot(const GameState& state, Team team, int depth, const AISettings& settings) {
  SearchResult best{};
  float bestScore = -kInfinity;
  float alpha = -kInfinity;
  float beta = kInfinity;
  std::vector<Action> actions = MoveGenerator::generateActions(state);
  actions = orderActions(state, std::move(actions), 0, settings, nullptr);

  for (const Action& action : actions) {
    if (outOfTime()) {
      timedOut_ = true;
      break;
    }

    GameState next = state;
    next.applyAction(action);
    const float score = searchNode(next, depth - 1, alpha, beta, team, 1, settings);
    if (timedOut_) {
      break;
    }

    if (!best.action.isValid() || score > bestScore) {
      bestScore = score;
      best = {action, score, true};
    }

    if (usePruning(settings)) {
      alpha = std::max(alpha, bestScore);
    }
  }

  return best;
}

SearchResult SearchAI::greedyFallback(const GameState& state, Team team, const AISettings& settings) const {
  std::vector<Action> actions = MoveGenerator::generateActions(state);
  if (actions.empty()) {
    return {};
  }

  Action bestAction = actions.front();
  float bestScore = -kInfinity;
  for (const Action& action : actions) {
    GameState next = state;
    next.applyAction(action);
    float score = evaluateHybrid(next, team, settings);
    score += static_cast<float>(orderingScore(state, action, 0, settings, nullptr)) * 0.0015f;
    if (!bestAction.isValid() || score > bestScore) {
      bestAction = action;
      bestScore = score;
    }
  }

  return {bestAction, bestScore, bestAction.isValid()};
}

float SearchAI::searchNode(GameState& state,
                           int depth,
                           float alpha,
                           float beta,
                           Team maximizingTeam,
                           int ply,
                           const AISettings& settings) {
  if (outOfTime()) {
    timedOut_ = true;
    return evaluateHybrid(state, maximizingTeam, settings);
  }

  ++metrics_.nodes;

  if (state.isTerminal()) {
    const auto winner = state.winner();
    return winner && *winner == maximizingTeam ? 1000.0f - static_cast<float>(ply)
                                               : -1000.0f + static_cast<float>(ply);
  }

  if (depth <= 0) {
    if (settings.enableQuiescence) {
      return quiescence(state, alpha, beta, maximizingTeam, ply, settings.quiescenceDepth, settings);
    }
    return evaluateHybrid(state, maximizingTeam, settings);
  }

  const bool maximizing = state.activeTeam() == maximizingTeam;
  const bool pruning = usePruning(settings);
  const float originalAlpha = alpha;
  const float originalBeta = beta;

  const std::uint64_t key = hasher_.hash(state);
  Action ttBestAction = Action::invalid();
  if (settings.enableTranspositionTable) {
    // The transposition table lets sibling branches reuse work when different move orders reach the same state.
    if (const TTEntry* entry = transpositionTable_.probe(key)) {
      ttBestAction = entry->bestAction;
      if (entry->depth >= depth) {
        ++metrics_.ttHits;
        if (entry->bound == BoundType::Exact) {
          return entry->score;
        }
        if (pruning) {
          if (entry->bound == BoundType::Lower) {
            alpha = std::max(alpha, entry->score);
          } else if (entry->bound == BoundType::Upper) {
            beta = std::min(beta, entry->score);
          }
          if (alpha >= beta) {
            return entry->score;
          }
        }
      }
    }
  }

  std::vector<Action> actions = MoveGenerator::generateActions(state);
  if (actions.empty()) {
    return evaluateHybrid(state, maximizingTeam, settings);
  }

  actions = orderActions(state, std::move(actions), ply, settings, ttBestAction.isValid() ? &ttBestAction : nullptr);

  float bestScore = maximizing ? -kInfinity : kInfinity;
  Action bestAction = Action::invalid();

  for (const Action& action : actions) {
    GameState next = state;
    next.applyAction(action);
    const float score = searchNode(next, depth - 1, alpha, beta, maximizingTeam, ply + 1, settings);
    if (timedOut_) {
      return score;
    }

    if (maximizing) {
      if (!bestAction.isValid() || score > bestScore) {
        bestScore = score;
        bestAction = action;
      }
      alpha = pruning ? std::max(alpha, bestScore) : alpha;
    } else {
      if (!bestAction.isValid() || score < bestScore) {
        bestScore = score;
        bestAction = action;
      }
      beta = pruning ? std::min(beta, bestScore) : beta;
    }

    if (pruning && alpha >= beta) {
      ++metrics_.prunes;
      if (settings.enableKillerHeuristic) {
        // Non-capture cutoffs are strong move-ordering candidates at the same depth in future branches.
        rememberKiller(ply, action);
      }
      break;
    }
  }

  if (settings.enableTranspositionTable && bestAction.isValid()) {
    BoundType bound = BoundType::Exact;
    if (bestScore <= originalAlpha) {
      bound = BoundType::Upper;
    } else if (bestScore >= originalBeta) {
      bound = BoundType::Lower;
    }
    transpositionTable_.store(TTEntry{key, depth, bestScore, bound, bestAction});
  }

  return bestScore;
}

float SearchAI::quiescence(GameState& state,
                           float alpha,
                           float beta,
                           Team maximizingTeam,
                           int ply,
                           int remainingDepth,
                           const AISettings& settings) {
  ++metrics_.quiescenceNodes;

  // Quiescence only extends tactically noisy positions so the leaf score is not dominated by an unsearched attack.
  const bool maximizing = state.activeTeam() == maximizingTeam;
  const float standPat = evaluateHybrid(state, maximizingTeam, settings);
  if (remainingDepth <= 0) {
    return standPat;
  }

  if (usePruning(settings)) {
    if (maximizing) {
      if (standPat >= beta) {
        return standPat;
      }
      alpha = std::max(alpha, standPat);
    } else {
      if (standPat <= alpha) {
        return standPat;
      }
      beta = std::min(beta, standPat);
    }
  }

  std::vector<Action> noisyActions = MoveGenerator::generateNoisyActions(state);
  if (noisyActions.empty()) {
    return standPat;
  }

  noisyActions = orderActions(state, std::move(noisyActions), ply, settings, nullptr);
  float bestScore = standPat;

  for (const Action& action : noisyActions) {
    GameState next = state;
    next.applyAction(action);
    const float score = quiescence(next, alpha, beta, maximizingTeam, ply + 1, remainingDepth - 1, settings);

    if (maximizing) {
      bestScore = std::max(bestScore, score);
      if (usePruning(settings)) {
        alpha = std::max(alpha, bestScore);
        if (alpha >= beta) {
          ++metrics_.prunes;
          break;
        }
      }
    } else {
      bestScore = std::min(bestScore, score);
      if (usePruning(settings)) {
        beta = std::min(beta, bestScore);
        if (alpha >= beta) {
          ++metrics_.prunes;
          break;
        }
      }
    }
  }

  return bestScore;
}

float SearchAI::evaluateHybrid(const GameState& state, Team perspective, const AISettings& settings) const {
  const float classicalScore = evaluator_->evaluate(state, perspective);
  if (settings.mode != AIMode::Hybrid || !settings.enableMLInfluence || !model_.available()) {
    return classicalScore;
  }

  const float mlScore = model_.predict(encoder_.encode(state, perspective)) * kModelScoreScale;
  return settings.classicalWeight * classicalScore + settings.mlWeight * mlScore;
}

std::vector<Action> SearchAI::orderActions(const GameState& state,
                                           std::vector<Action> actions,
                                           int ply,
                                           const AISettings& settings,
                                           const Action* ttBest) const {
  if (!settings.enableMoveOrdering) {
    return actions;
  }

  std::sort(actions.begin(), actions.end(), [&](const Action& lhs, const Action& rhs) {
    return orderingScore(state, lhs, ply, settings, ttBest) > orderingScore(state, rhs, ply, settings, ttBest);
  });
  return actions;
}

int SearchAI::orderingScore(const GameState& state,
                            const Action& action,
                            int ply,
                            const AISettings& settings,
                            const Action* ttBest) const {
  int score = 0;
  if (ttBest != nullptr && action == *ttBest) {
    score += 25000;
  }
  if (settings.enableKillerHeuristic && isKiller(ply, action)) {
    score += 5000;
  }

  score += MoveGenerator::estimateActionDamage(state, action) * 700;
  score += state.board().tileAt(action.moveTarget).controlValue * 25;
  score -= isHazard(state.board().tileAt(action.moveTarget).terrain) ? 60 : 0;
  score -= manhattanDistance(action.moveTarget, {3, 3}) * 4;
  if (playerInfluenceModel_ != nullptr && state.activeTeam() == Team::Enemy) {
    score += static_cast<int>(playerInfluenceModel_->actionBias(state, action) * 90.0f);
  }
  if (!action.isAttack() && action.wait) {
    score -= 12;
  }
  return score;
}

void SearchAI::rememberKiller(int ply, const Action& action) {
  if (ply < 0 || static_cast<std::size_t>(ply) >= killerMoves_.size()) {
    return;
  }
  if (killerMoves_[static_cast<std::size_t>(ply)][0] == action) {
    return;
  }
  killerMoves_[static_cast<std::size_t>(ply)][1] = killerMoves_[static_cast<std::size_t>(ply)][0];
  killerMoves_[static_cast<std::size_t>(ply)][0] = action;
}

bool SearchAI::isKiller(int ply, const Action& action) const {
  if (ply < 0 || static_cast<std::size_t>(ply) >= killerMoves_.size()) {
    return false;
  }
  return killerMoves_[static_cast<std::size_t>(ply)][0] == action ||
         killerMoves_[static_cast<std::size_t>(ply)][1] == action;
}

bool SearchAI::usePruning(const AISettings& settings) const {
  return settings.mode != AIMode::Minimax && settings.enablePruning;
}

bool SearchAI::outOfTime() const {
  return std::chrono::steady_clock::now() >= deadline_;
}

}  // namespace Game
