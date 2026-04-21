#include "Core/Game.hpp"

#include <exception>
#include <iostream>

int main() {
  try {
    Game::Game game;
    game.run();
  } catch (const std::exception& error) {
    std::cerr << "Fatal error: " << error.what() << '\n';
    return 1;
  }

  return 0;
}
