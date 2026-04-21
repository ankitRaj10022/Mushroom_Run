# Tactical Hybrid AI

A compact C++17 turn-based tactics project with three interchangeable enemy controllers:

- `Minimax`
- `Alpha-Beta` with iterative deepening, move ordering, quiescence, killer moves, and a transposition table
- `Hybrid ML + Search`, which blends the classical evaluator with an ONNX-loaded MLP

The gameplay loop is intentionally small and inspectable: an 8x8 tactical board, terrain, hazards, directional and area attacks, player-vs-enemy turns, SFML visualization, and a live analytics panel for AI behavior.

## Architecture

### Runtime Layer

- `Core/Game.*`: main loop, input, turn flow, AI scheduling, rendering orchestration
- `Core/GameState.*`: copyable authoritative state used by both gameplay and search
- `Core/Scenario.*`: initial skirmish layout
- `Entities/Unit.*`: polymorphic renderable unit hierarchy
- `Board/*`: grid board and terrain definitions
- `Moves/*`: move generation, attack geometry, mobility and threat helpers

### AI Layer

- `AI/Heuristics/CompositeEvaluator.*`: health, control, threat, mobility, hazard, and future-position scoring
- `AI/Search/SearchAI.*`: iterative deepening, alpha-beta, quiescence, killer moves, move ordering
- `AI/Search/TranspositionTable.*`: TT entries and replacement policy
- `Utils/Zobrist.*`: board hashing for TT reuse
- `AI/ML/GameStateEncoder.*`: fixed-size board/unit feature encoder
- `AI/ML/MLModel.*`: ONNX Runtime wrapper for MLP inference

### ML Layer

- `ML/training/train_mlp.py`: self-play dataset generation, supervised MLP training, ONNX export
- `ML/model/`: expected output location for the exported model and metadata

## Project Structure

```text
.
├── AI
│   ├── Heuristics
│   ├── ML
│   └── Search
├── Board
├── Core
├── Entities
├── ML
│   ├── model
│   └── training
├── Moves
├── Utils
├── CMake
├── CMakeLists.txt
├── README.md
└── main.cpp
```

## Search Stack

The enemy controller evaluates action-level plies across the full turn system:

1. Iterative deepening increases search depth until the target depth or time budget is hit.
2. Root actions are ordered using damage, control-value gain, center pressure, TT best move, and killer moves.
3. Alpha-beta pruning is applied when enabled.
4. The transposition table stores exact and bound scores keyed by Zobrist hashes.
5. Quiescence search extends leaf nodes through tactical attack sequences to reduce horizon effects.

The hybrid evaluator blends:

```text
final_score = classical_weight * heuristic_score + ml_weight * ml_score
```

The Python training script learns `ml_score` targets from self-play states and exports the model to ONNX for C++ inference.

## Build

### Prerequisites

- CMake 3.24+
- A C++17 compiler
- Git, if `TACTICAL_FETCH_SFML=ON`
- SFML 2.6.x, or let CMake fetch it automatically
- Optional: ONNX Runtime for native inference

### Configure

```powershell
cmake -S . -B build
```

If you already installed ONNX Runtime locally, point CMake at it:

```powershell
$env:ONNXRUNTIME_ROOT="C:\path\to\onnxruntime"
cmake -S . -B build -DTACTICAL_ENABLE_ONNX=ON
```

To build without ML inference first:

```powershell
cmake -S . -B build -DTACTICAL_ENABLE_ONNX=OFF
```

### Build

```powershell
cmake --build build --config Release
```

### Run

```powershell
.\build\Release\tactical_hybrid_ai.exe
```

## Controls

- `Left Click`: select a player unit, choose a destination, then choose an attack target
- `Space`: commit move-only or wait
- `Right Click` or `Backspace`: cancel the current selection stage
- `1`, `2`, `3`: Minimax, Alpha-Beta, Hybrid
- `[`, `]`: decrease/increase depth
- `P`: pruning
- `M`: ML influence
- `O`: move ordering
- `Q`: quiescence search
- `K`: killer moves
- `T`: transposition table
- `-`, `=`: decrease/increase ML blend weight
- `R`: reset the scenario

## Train And Export The MLP

Install the Python dependencies:

```powershell
python -m pip install -r ML/training/requirements.txt
```

Generate self-play data, train, and export ONNX:

```powershell
python ML/training/train_mlp.py --episodes 800 --epochs 40
```

This writes:

- `ML/model/tactical_mlp.onnx`
- `ML/model/tactical_mlp_meta.json`

The C++ game tries to load `ML/model/tactical_mlp.onnx` automatically on startup.

## Notes

- The C++ and Python evaluators intentionally mirror each other so the model learns against the same tactical signals used by search.
- The runtime board rendering is shape-based to keep the repository asset-light and easy to build from scratch.
- ONNX support is integrated at the CMake and C++ levels, but the project still builds without it for gameplay-only testing.
