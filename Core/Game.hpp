#pragma once

#include "Platformer/World.hpp"

#if TACTICAL_HAS_AUDIO
#include <SFML/Audio/Sound.hpp>
#include <SFML/Audio/SoundBuffer.hpp>
#endif
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RenderWindow.hpp>

#include <vector>

namespace Game {

class Game {
 public:
  Game();
  void run();

 private:
  void handleEvents();
  void update(float deltaSeconds);
  void render();
  void initializeAudio();
  void flushAudioEvents();

  sf::RenderWindow window_;
  sf::Font uiFont_{};
  bool hasUIFont_{false};
  PlatformerWorld world_;

#if TACTICAL_HAS_AUDIO
  sf::SoundBuffer startBuffer_{};
  sf::SoundBuffer jumpBuffer_{};
  sf::SoundBuffer coinBuffer_{};
  sf::SoundBuffer blockBuffer_{};
  sf::SoundBuffer stompBuffer_{};
  sf::SoundBuffer powerUpBuffer_{};
  sf::SoundBuffer hurtBuffer_{};
  sf::SoundBuffer extraLifeBuffer_{};
  sf::SoundBuffer clearBuffer_{};
  sf::SoundBuffer gameOverBuffer_{};
  std::vector<sf::Sound> activeSounds_{};
#endif
};

}  // namespace Game
