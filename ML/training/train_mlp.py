import argparse
import json
import math
import random
from dataclasses import dataclass, replace
from pathlib import Path
from typing import List, Optional, Sequence, Tuple

import numpy as np
import torch
from torch import nn
from torch.utils.data import DataLoader, TensorDataset


BOARD_W = 8
BOARD_H = 8
CELL_COUNT = BOARD_W * BOARD_H
FEATURE_PLANES = 14
MODEL_SCORE_SCALE = 12.0

TEAM_PLAYER = 0
TEAM_ENEMY = 1

TERRAIN_PLAIN = 0
TERRAIN_WATER = 1
TERRAIN_MOUNTAIN = 2
TERRAIN_OBSTACLE = 3
TERRAIN_HAZARD = 4

PATTERN_ADJACENT = 0
PATTERN_LINE = 1
PATTERN_BLAST = 2

UNIT_VANGUARD = 0
UNIT_ARTILLERY = 1
UNIT_STRIKER = 2
UNIT_BRUISER = 3


@dataclass(frozen=True)
class UnitStats:
    name: str
    max_health: int
    move_range: int
    attack_range: int
    attack_damage: int
    pattern: int
    flying: bool = False


UNIT_STATS = {
    UNIT_VANGUARD: UnitStats("Vanguard", 8, 3, 1, 3, PATTERN_ADJACENT),
    UNIT_ARTILLERY: UnitStats("Artillery", 6, 2, 3, 2, PATTERN_BLAST),
    UNIT_STRIKER: UnitStats("Striker", 6, 4, 3, 2, PATTERN_LINE),
    UNIT_BRUISER: UnitStats("Bruiser", 10, 2, 1, 4, PATTERN_ADJACENT),
}


@dataclass(frozen=True)
class Unit:
    unit_id: int
    team: int
    unit_class: int
    health: int
    position: Tuple[int, int]
    acted: bool = False

    @property
    def stats(self) -> UnitStats:
        return UNIT_STATS[self.unit_class]


@dataclass(frozen=True)
class State:
    board: Tuple[int, ...]
    control: Tuple[int, ...]
    units: Tuple[Unit, ...]
    active_team: int = TEAM_PLAYER
    turn_number: int = 1


def index(cell: Tuple[int, int]) -> int:
    return cell[1] * BOARD_W + cell[0]


def in_bounds(cell: Tuple[int, int]) -> bool:
    return 0 <= cell[0] < BOARD_W and 0 <= cell[1] < BOARD_H


def cardinal() -> Sequence[Tuple[int, int]]:
    return ((1, 0), (-1, 0), (0, 1), (0, -1))


def manhattan(a: Tuple[int, int], b: Tuple[int, int]) -> int:
    return abs(a[0] - b[0]) + abs(a[1] - b[1])


def opposing(team: int) -> int:
    return TEAM_ENEMY if team == TEAM_PLAYER else TEAM_PLAYER


def create_board() -> Tuple[Tuple[int, ...], Tuple[int, ...]]:
    terrain = [TERRAIN_PLAIN] * CELL_COUNT
    control = [1] * CELL_COUNT

    for y in range(BOARD_H):
        for x in range(BOARD_W):
            dist = math.sqrt((x - 3.5) ** 2 + (y - 3.5) ** 2)
            control[index((x, y))] = 4 if dist < 1.6 else (3 if dist < 2.8 else 1)

    for cell in ((2, 2), (2, 3), (5, 4), (5, 5)):
        terrain[index(cell)] = TERRAIN_WATER
    for cell in ((1, 5), (6, 2)):
        terrain[index(cell)] = TERRAIN_MOUNTAIN
    for cell in ((3, 3), (4, 3)):
        terrain[index(cell)] = TERRAIN_OBSTACLE
        control[index(cell)] = 0
    for cell in ((1, 1), (6, 6), (3, 6), (4, 1)):
        terrain[index(cell)] = TERRAIN_HAZARD

    return tuple(terrain), tuple(control)


def initial_state() -> State:
    board, control = create_board()
    units = (
        Unit(0, TEAM_PLAYER, UNIT_VANGUARD, UNIT_STATS[UNIT_VANGUARD].max_health, (1, 6), False),
        Unit(1, TEAM_PLAYER, UNIT_ARTILLERY, UNIT_STATS[UNIT_ARTILLERY].max_health, (2, 7), False),
        Unit(2, TEAM_ENEMY, UNIT_STRIKER, UNIT_STATS[UNIT_STRIKER].max_health, (6, 1), False),
        Unit(3, TEAM_ENEMY, UNIT_BRUISER, UNIT_STATS[UNIT_BRUISER].max_health, (5, 0), False),
    )
    return State(board, control, units, TEAM_PLAYER, 1)


def board_terrain(state: State, cell: Tuple[int, int]) -> int:
    return state.board[index(cell)]


def occupied(state: State, cell: Tuple[int, int], ignore_unit: Optional[int] = None) -> bool:
    for unit in state.units:
        if unit.health <= 0:
            continue
        if ignore_unit is not None and unit.unit_id == ignore_unit:
            continue
        if unit.position == cell:
            return True
    return False


def reachable_tiles(state: State, unit: Unit) -> List[Tuple[int, int]]:
    frontier = [unit.position]
    distances = {unit.position: 0}

    while frontier:
        current = frontier.pop(0)
        for direction in cardinal():
            nxt = (current[0] + direction[0], current[1] + direction[1])
            if not in_bounds(nxt):
                continue
            terrain = board_terrain(state, nxt)
            if terrain in (TERRAIN_MOUNTAIN, TERRAIN_OBSTACLE):
                continue
            if terrain == TERRAIN_WATER and not unit.stats.flying:
                continue
            if occupied(state, nxt, unit.unit_id):
                continue

            cost = distances[current] + 1
            if cost > unit.stats.move_range:
                continue
            if nxt in distances and distances[nxt] <= cost:
                continue
            distances[nxt] = cost
            frontier.append(nxt)

    return sorted(distances.keys())


def affected_tiles(state: State, unit: Unit, origin: Tuple[int, int], target: Tuple[int, int]) -> List[Tuple[int, int]]:
    stats = unit.stats
    if stats.pattern == PATTERN_ADJACENT:
        return [target] if 1 <= manhattan(origin, target) <= stats.attack_range else []

    if stats.pattern == PATTERN_LINE:
        dx = target[0] - origin[0]
        dy = target[1] - origin[1]
        cardinal_move = (dx == 0 and dy != 0) or (dy == 0 and dx != 0)
        if not cardinal_move:
            return []
        distance = max(abs(dx), abs(dy))
        if distance < 1 or distance > stats.attack_range:
            return []
        direction = (0 if dx == 0 else dx // distance, 0 if dy == 0 else dy // distance)
        cells = []
        for step in range(1, stats.attack_range + 1):
            cell = (origin[0] + direction[0] * step, origin[1] + direction[1] * step)
            if not in_bounds(cell):
                break
            if board_terrain(state, cell) in (TERRAIN_MOUNTAIN, TERRAIN_OBSTACLE):
                break
            cells.append(cell)
        return cells

    if stats.pattern == PATTERN_BLAST:
        if not (1 <= manhattan(origin, target) <= stats.attack_range):
            return []
        cells = []
        for y in range(target[1] - 1, target[1] + 2):
            for x in range(target[0] - 1, target[0] + 2):
                cell = (x, y)
                if in_bounds(cell) and manhattan(cell, target) <= 1:
                    cells.append(cell)
        return cells

    return []


def enemies_in_cells(state: State, team: int, cells: Sequence[Tuple[int, int]]) -> bool:
    return any(unit.team != team and unit.health > 0 and unit.position in cells for unit in state.units)


def legal_attack_targets(state: State, unit: Unit, origin: Tuple[int, int]) -> List[Tuple[int, int]]:
    targets = []
    for y in range(BOARD_H):
        for x in range(BOARD_W):
            cell = (x, y)
            if cell == origin:
                continue
            cells = affected_tiles(state, unit, origin, cell)
            if cells and enemies_in_cells(state, unit.team, cells):
                targets.append(cell)
    return targets


def generate_actions_for_unit(state: State, unit: Unit, attacks_only: bool = False) -> List[Tuple[int, Tuple[int, int], Optional[Tuple[int, int]], bool]]:
    actions = []
    for destination in reachable_tiles(state, unit):
        for target in legal_attack_targets(state, unit, destination):
            actions.append((unit.unit_id, destination, target, False))
        if not attacks_only:
            actions.append((unit.unit_id, destination, None, destination == unit.position))
    return actions


def generate_actions(state: State, attacks_only: bool = False) -> List[Tuple[int, Tuple[int, int], Optional[Tuple[int, int]], bool]]:
    actions = []
    for unit in state.units:
        if unit.team != state.active_team or unit.health <= 0 or unit.acted:
            continue
        actions.extend(generate_actions_for_unit(state, unit, attacks_only))
    return actions


def prune_dead(units: Sequence[Unit]) -> Tuple[Unit, ...]:
    return tuple(unit for unit in units if unit.health > 0)


def reset_flags(units: Sequence[Unit], team: int) -> Tuple[Unit, ...]:
    return tuple(replace(unit, acted=False) if unit.team == team else unit for unit in units)


def apply_hazards(state: State) -> State:
    updated = []
    for unit in state.units:
        health = unit.health - 1 if board_terrain(state, unit.position) == TERRAIN_HAZARD else unit.health
        updated.append(replace(unit, health=health))
    return replace(state, units=prune_dead(updated))


def all_acted(state: State, team: int) -> bool:
    living = [unit for unit in state.units if unit.team == team and unit.health > 0]
    return bool(living) and all(unit.acted for unit in living)


def apply_action(state: State, action: Tuple[int, Tuple[int, int], Optional[Tuple[int, int]], bool]) -> State:
    unit_id, destination, attack_target, _ = action
    units = list(state.units)
    actor_index = next((i for i, unit in enumerate(units) if unit.unit_id == unit_id), None)
    if actor_index is None:
        return state

    actor = units[actor_index]
    actor = replace(actor, position=destination, acted=True)
    units[actor_index] = actor

    if attack_target is not None:
        cells = affected_tiles(state, actor, destination, attack_target)
        for i, unit in enumerate(units):
            if unit.team == actor.team or unit.health <= 0:
                continue
            if unit.position in cells:
                units[i] = replace(unit, health=unit.health - actor.stats.attack_damage)

    next_state = replace(state, units=prune_dead(units))
    if terminal(next_state):
        return next_state
    if all_acted(next_state, next_state.active_team):
        next_state = apply_hazards(next_state)
        if terminal(next_state):
            return next_state
        next_team = opposing(next_state.active_team)
        next_state = replace(next_state, active_team=next_team, units=reset_flags(next_state.units, next_team))
        if next_team == TEAM_PLAYER:
            next_state = replace(next_state, turn_number=next_state.turn_number + 1)
    return next_state


def winner(state: State) -> Optional[int]:
    player_alive = any(unit.team == TEAM_PLAYER and unit.health > 0 for unit in state.units)
    enemy_alive = any(unit.team == TEAM_ENEMY and unit.health > 0 for unit in state.units)
    if player_alive and enemy_alive:
        return None
    return TEAM_PLAYER if player_alive else TEAM_ENEMY


def terminal(state: State) -> bool:
    return winner(state) is not None


def mobility(state: State, team: int) -> int:
    return sum(len(reachable_tiles(state, unit)) for unit in state.units if unit.team == team and unit.health > 0)


def threat_score(state: State, team: int) -> float:
    score = 0.0
    for unit in state.units:
        if unit.team != team or unit.health <= 0:
            continue
        best_damage = 0
        for action in generate_actions_for_unit(state, unit, attacks_only=True):
            unit_id, destination, attack_target, _ = action
            if attack_target is None:
                continue
            proxy = replace(unit, position=destination)
            cells = affected_tiles(state, proxy, destination, attack_target)
            damage = sum(proxy.stats.attack_damage for target in state.units if target.team != team and target.position in cells)
            best_damage = max(best_damage, damage)
        score += best_damage
    return score


def positional_potential(state: State, unit: Unit) -> float:
    reachable = reachable_tiles(state, unit)
    best_control = state.control[index(unit.position)]
    for cell in reachable:
        best_control = max(best_control, state.control[index(cell)])
    center_bias = 1.0 - manhattan(unit.position, (3, 3)) / 8.0
    return best_control + center_bias


def evaluate_state(state: State, perspective: int) -> float:
    if terminal(state):
        return 1000.0 if winner(state) == perspective else -1000.0

    health_balance = 0.0
    control_balance = 0.0
    hazard_balance = 0.0
    future_balance = 0.0

    for unit in state.units:
        sign = 1.0 if unit.team == perspective else -1.0
        health_balance += sign * (unit.health / unit.stats.max_health)
        control_balance += sign * state.control[index(unit.position)]
        future_balance += sign * positional_potential(state, unit)
        if board_terrain(state, unit.position) == TERRAIN_HAZARD:
            hazard_balance -= sign

    threat_balance = threat_score(state, perspective) - threat_score(state, opposing(perspective))
    mobility_balance = mobility(state, perspective) - mobility(state, opposing(perspective))

    return (
        health_balance * 4.0
        + control_balance * 0.40
        + threat_balance * 0.65
        + mobility_balance * 0.10
        + future_balance * 0.35
        + hazard_balance * 0.45
    )


def encode_state(state: State, perspective: int) -> np.ndarray:
    features = np.zeros((FEATURE_PLANES, CELL_COUNT), dtype=np.float32)

    for y in range(BOARD_H):
        for x in range(BOARD_W):
            cell = (x, y)
            idx = index(cell)
            terrain = board_terrain(state, cell)
            features[8, idx] = 1.0 if terrain == TERRAIN_WATER else 0.0
            features[9, idx] = 1.0 if terrain == TERRAIN_MOUNTAIN else 0.0
            features[10, idx] = 1.0 if terrain == TERRAIN_OBSTACLE else 0.0
            features[11, idx] = 1.0 if terrain == TERRAIN_HAZARD else 0.0
            features[12, idx] = min(1.0, state.control[idx] / 4.0)
            features[13, idx] = 1.0 if state.active_team == perspective else 0.0

    for unit in state.units:
        idx = index(unit.position)
        friendly = unit.team == perspective
        base = 0 if friendly else 1
        features[base, idx] = unit.health / unit.stats.max_health
        features[2 if friendly else 3, idx] = unit.stats.move_range / 4.0
        features[4 if friendly else 5, idx] = unit.stats.attack_range / 4.0
        features[6 if friendly else 7, idx] = unit.stats.attack_damage / 4.0

    return features.reshape(-1)


def choose_self_play_action(state: State, epsilon: float) -> Tuple[int, Tuple[int, int], Optional[Tuple[int, int]], bool]:
    actions = generate_actions(state)
    if not actions:
        raise RuntimeError("No actions available during self-play.")
    if random.random() < epsilon:
        return random.choice(actions)

    scored = []
    for action in actions:
        next_state = apply_action(state, action)
        score = evaluate_state(next_state, state.active_team)
        scored.append((score, action))
    scored.sort(key=lambda item: item[0], reverse=True)
    return scored[0][1]


def generate_dataset(episodes: int, epsilon: float, max_turns: int) -> Tuple[np.ndarray, np.ndarray]:
    samples: List[np.ndarray] = []
    targets: List[float] = []

    for _ in range(episodes):
        state = initial_state()
        episode_samples = []

        while not terminal(state) and state.turn_number <= max_turns:
            perspective = state.active_team
            heuristic = evaluate_state(state, perspective)
            episode_samples.append((encode_state(state, perspective), perspective, heuristic))
            state = apply_action(state, choose_self_play_action(state, epsilon))

        outcome = winner(state)
        for encoded, perspective, heuristic in episode_samples:
            # Blend a dense heuristic target with the sparse game result so the model learns both tactics and outcome.
            terminal_value = 1.0 if outcome == perspective else -1.0
            heuristic_value = math.tanh(heuristic / MODEL_SCORE_SCALE)
            target = 0.65 * heuristic_value + 0.35 * terminal_value
            samples.append(encoded)
            targets.append(target)

    return np.asarray(samples, dtype=np.float32), np.asarray(targets, dtype=np.float32).reshape(-1, 1)


class TacticalMLP(nn.Module):
    def __init__(self, input_size: int) -> None:
        super().__init__()
        self.network = nn.Sequential(
            nn.Linear(input_size, 256),
            nn.ReLU(),
            nn.Linear(256, 128),
            nn.ReLU(),
            nn.Linear(128, 1),
            nn.Tanh(),
        )

    def forward(self, x: torch.Tensor) -> torch.Tensor:
        return self.network(x)


def train_model(args: argparse.Namespace) -> None:
    random.seed(args.seed)
    np.random.seed(args.seed)
    torch.manual_seed(args.seed)

    features, targets = generate_dataset(args.episodes, args.epsilon, args.max_turns)
    dataset = TensorDataset(torch.from_numpy(features), torch.from_numpy(targets))
    loader = DataLoader(dataset, batch_size=args.batch_size, shuffle=True)

    model = TacticalMLP(features.shape[1])
    optimizer = torch.optim.Adam(model.parameters(), lr=args.learning_rate)
    criterion = nn.MSELoss()

    for epoch in range(1, args.epochs + 1):
        running_loss = 0.0
        for batch_features, batch_targets in loader:
            predictions = model(batch_features)
            loss = criterion(predictions, batch_targets)
            optimizer.zero_grad()
            loss.backward()
            optimizer.step()
            running_loss += loss.item()

        avg_loss = running_loss / max(1, len(loader))
        print(f"epoch={epoch:03d} loss={avg_loss:.6f}")

    output_dir = Path(args.output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)
    onnx_path = output_dir / "tactical_mlp.onnx"
    meta_path = output_dir / "tactical_mlp_meta.json"

    model.eval()
    sample = torch.from_numpy(features[:1])
    torch.onnx.export(
        model,
        (sample,),
        onnx_path.as_posix(),
        input_names=["state"],
        output_names=["value"],
        dynamic_axes={"state": {0: "batch"}, "value": {0: "batch"}},
        opset_version=17,
        dynamo=True,
    )

    metadata = {
        "input_size": int(features.shape[1]),
        "episodes": args.episodes,
        "epochs": args.epochs,
        "epsilon": args.epsilon,
        "max_turns": args.max_turns,
        "learning_rate": args.learning_rate,
        "model_score_scale": MODEL_SCORE_SCALE,
    }
    meta_path.write_text(json.dumps(metadata, indent=2), encoding="utf-8")
    print(f"exported={onnx_path}")
    print(f"metadata={meta_path}")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Train a lightweight tactical MLP and export it to ONNX.")
    parser.add_argument("--episodes", type=int, default=600, help="Number of self-play episodes for dataset generation.")
    parser.add_argument("--epochs", type=int, default=32, help="Training epochs.")
    parser.add_argument("--batch-size", type=int, default=64, help="Batch size.")
    parser.add_argument("--learning-rate", type=float, default=1e-3, help="Adam learning rate.")
    parser.add_argument("--epsilon", type=float, default=0.18, help="Self-play exploration rate.")
    parser.add_argument("--max-turns", type=int, default=18, help="Turn cap for each self-play episode.")
    parser.add_argument("--seed", type=int, default=7, help="Random seed.")
    parser.add_argument("--output-dir", type=str, default="ML/model", help="Output directory for ONNX and metadata.")
    return parser.parse_args()


if __name__ == "__main__":
    train_model(parse_args())
