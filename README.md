# Mushroom Run

`Mushroom Run` is a C++17 side-scrolling platformer built with SFML. The current main build is no longer the old tactical AI prototype; it is now a single-player campaign platformer with a title screen, overworld flow, multiple handcrafted stages, moving platforms, enemy variety, power-ups, coins, lives, timer pressure, and sprite-sheet rendering.

## Current Game

The main gameplay loop is:

`Title -> Overworld -> Level Intro -> Course -> Clear / Game Over`

Current features:

- Three campaign stages with distinct terrain and pacing
- Overworld route select between unlocked stages
- Enemy roster:
  - Walker
  - Hopper
  - Spiky
  - Flyer
- Horizontal and vertical moving platforms
- Coins, brick blocks, question blocks, mushrooms, lives, score, and time limits
- Sprite-sheet rendering for tiles, player, enemies, collectibles, and UI scene art
- Procedural audio tones for jump, stomp, coin, power-up, clear, and game-over feedback

## Active Code Structure

The main build uses these runtime files:

- `main.cpp`: process entry point
- `Core/Game.*`: window loop, input forwarding, and synthesized audio playback
- `Platformer/World.*`: campaign flow, level building, physics, collisions, enemies, platforms, rendering, and UI
- `Utils/FontLocator.*`: finds a usable system font on Windows for HUD and overlays
- `CMakeLists.txt`: build configuration for the `mushroom_run` target

The repository still contains older tactical/AI folders from previous iterations, but they are not part of the current executable target.

## Build

### Prerequisites

- CMake 3.24+
- A C++17 compiler
- Git, if `PLATFORMER_FETCH_SFML=ON`
- SFML 2.6.x, or let CMake fetch it automatically

### Configure

```powershell
cmake -S . -B build
```

Optional flags:

```powershell
cmake -S . -B build -DPLATFORMER_ENABLE_AUDIO=ON
cmake -S . -B build -DPLATFORMER_FETCH_SFML=ON
```

### Build

```powershell
cmake --build build --config Release
```

### Run

Single-config generators like Ninja:

```powershell
.\build\mushroom_run.exe
```

Multi-config generators like Visual Studio:

```powershell
.\build\Release\mushroom_run.exe
```

## Controls

### Title / Menus

- `Enter` / `Space`: start campaign or confirm
- `Esc`: return to title from overworld or game-over flow

### Overworld

- `Left` / `Right` or `A` / `D`: change selected stage
- `Enter` / `Space`: launch selected stage

### In Level

- `Left` / `Right` or `A` / `D`: move
- `Space`, `Up`, or `W`: jump
- `Shift` or `Z`: run
- `R`: restart current stage
- `Esc`: return to overworld

## Level Content

The campaign currently includes:

- `1-1 Verdant Run`: introductory ground course with early movers and mixed enemy pressure
- `1-2 Foundry Night`: denser stone layouts, tighter pits, and lift-heavy traversal
- `1-3 Sky Bridge`: larger gaps, more flyers, and chained platform movement

## Rendering Notes

The sprite art is generated into an internal atlas at runtime inside `Platformer/World.cpp`, so the project does not depend on an external sprite sheet asset yet. The renderer still uses textured sprites, not raw SFML rectangles for the active gameplay objects.

## Audio Notes

Audio is generated procedurally in `Core/Game.cpp`. On Windows, CMake copies `OpenAL32.dll` next to the executable automatically when audio is enabled.

## Status

What is implemented now:

- Full platformer main build
- Title screen and overworld flow
- Multiple levels
- More enemy types
- Moving platforms
- Sprite-sheet based rendering
- Buildable `mushroom_run` executable

What remains as good next steps:

- External PNG sprite atlas instead of code-generated atlas
- Additional enemy behaviors and bosses
- Particle effects for impacts and block hits
- Save data for stage progress and best scores
