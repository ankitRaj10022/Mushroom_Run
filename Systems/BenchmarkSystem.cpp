#include "Systems/BenchmarkSystem.hpp"

#include <algorithm>

namespace Game {

BenchmarkSystem::BenchmarkSystem(std::shared_ptr<IEvaluator> evaluator) : searchAI_(evaluator), mctsAI_(evaluator) {}

void BenchmarkSystem::loadModel(const std::string& modelPath) {
  searchAI_.loadModel(modelPath);
  mctsAI_.loadModel(modelPath);
}

BenchmarkResult BenchmarkSystem::runBenchmark(const ScenarioDefinition& scenario,
                                              AIMode firstMode,
                                              AIMode secondMode,
                                              int games,
                                              const AISettings& settings) {
  BenchmarkResult result;
  result.firstMode = firstMode;
  result.secondMode = secondMode;
  result.games = std::max(1, games);

  std::uint64_t accumulatedNodes = 0;
  double firstTimeTotal = 0.0;
  double secondTimeTotal = 0.0;
  int firstDecisions = 0;
  int secondDecisions = 0;

  for (int gameIndex = 0; gameIndex < result.games; ++gameIndex) {
    GameState state = createScenarioState(scenario);
    int safety = 0;

    while (!state.isTerminal() && safety++ < 64) {
      SearchMetrics metrics{};
      const AIMode currentMode = state.activeTeam() == Team::Player ? firstMode : secondMode;
      const Team actingTeam = state.activeTeam();
      const std::optional<Action> action = chooseActionForMode(state, state.activeTeam(), currentMode, settings, metrics);
      if (!action) {
        state.forceEndTurn();
      } else {
        state.applyAction(*action);
      }

      accumulatedNodes += metrics.nodes;
      if (actingTeam == Team::Player) {
        firstTimeTotal += metrics.elapsedMs;
        ++firstDecisions;
      } else {
        secondTimeTotal += metrics.elapsedMs;
        ++secondDecisions;
      }
    }

    const std::optional<Team> winner = state.winner();
    if (winner && *winner == Team::Player) {
      ++result.firstWins;
    } else {
      ++result.secondWins;
    }
  }

  result.averageFirstDecisionMs = firstDecisions > 0 ? firstTimeTotal / static_cast<double>(firstDecisions) : 0.0;
  result.averageSecondDecisionMs = secondDecisions > 0 ? secondTimeTotal / static_cast<double>(secondDecisions) : 0.0;
  result.averageNodes =
      result.games > 0 ? accumulatedNodes / static_cast<std::uint64_t>(result.games) : 0;
  return result;
}

std::optional<Action> BenchmarkSystem::chooseActionForMode(const GameState& state,
                                                           Team team,
                                                           AIMode mode,
                                                           const AISettings& settings,
                                                           SearchMetrics& metrics) {
  AISettings effective = settings;
  effective.mode = mode;

  if (mode == AIMode::MCTS) {
    return mctsAI_.chooseAction(state, team, effective, metrics);
  }
  return searchAI_.chooseAction(state, team, effective, metrics);
}

}  // namespace Game
