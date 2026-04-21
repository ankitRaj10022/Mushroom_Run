#include "Core/Game.hpp"

#include "Utils/FontLocator.hpp"

#if TACTICAL_HAS_AUDIO
#include <SFML/Audio/SoundBuffer.hpp>
#endif
#include <SFML/Graphics/Color.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/Window/Event.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

namespace Game {

namespace {

#if TACTICAL_HAS_AUDIO
sf::SoundBuffer makeToneBuffer(const std::vector<float>& frequencies,
                               float durationSeconds,
                               float amplitudeScale,
                               float attackSeconds,
                               float releaseSeconds) {
  constexpr unsigned int kSampleRate = 44100;
  const std::size_t sampleCount = static_cast<std::size_t>(durationSeconds * static_cast<float>(kSampleRate));
  std::vector<sf::Int16> samples(sampleCount, 0);

  for (std::size_t index = 0; index < sampleCount; ++index) {
    const float time = static_cast<float>(index) / static_cast<float>(kSampleRate);
    float amplitude = 0.0f;
    for (float frequency : frequencies) {
      amplitude += std::sin(2.0f * 3.1415926535f * frequency * time);
    }
    amplitude /= std::max(1.0f, static_cast<float>(frequencies.size()));

    float envelope = 1.0f;
    if (time < attackSeconds) {
      envelope = time / std::max(0.001f, attackSeconds);
    } else if (time > durationSeconds - releaseSeconds) {
      envelope = (durationSeconds - time) / std::max(0.001f, releaseSeconds);
    }
    envelope = std::clamp(envelope, 0.0f, 1.0f);

    samples[index] = static_cast<sf::Int16>(amplitude * envelope * amplitudeScale * 32767.0f);
  }

  sf::SoundBuffer buffer;
  buffer.loadFromSamples(samples.data(), static_cast<sf::Uint64>(samples.size()), 1, kSampleRate);
  return buffer;
}
#endif

}  // namespace

Game::Game() : window_(sf::VideoMode(1280U, 720U), "Mushroom Run") {
  window_.setVerticalSyncEnabled(true);
  window_.setFramerateLimit(60U);

  const std::string fontPath = locateFontFile();
  hasUIFont_ = !fontPath.empty() && uiFont_.loadFromFile(fontPath);
  world_.setFont(&uiFont_, hasUIFont_);
  initializeAudio();
}

void Game::run() {
  sf::Clock clock;
  while (window_.isOpen()) {
    handleEvents();
    update(clock.restart().asSeconds());
    render();
  }
}

void Game::handleEvents() {
  sf::Event event;
  while (window_.pollEvent(event)) {
    switch (event.type) {
      case sf::Event::Closed:
        window_.close();
        break;
      case sf::Event::KeyPressed:
        world_.handleKeyPressed(event.key.code);
        break;
      case sf::Event::KeyReleased:
        world_.handleKeyReleased(event.key.code);
        break;
      default:
        break;
    }
  }
}

void Game::update(float deltaSeconds) {
  world_.update(deltaSeconds);
  flushAudioEvents();

#if TACTICAL_HAS_AUDIO
  activeSounds_.erase(std::remove_if(activeSounds_.begin(), activeSounds_.end(),
                                     [](const sf::Sound& sound) { return sound.getStatus() == sf::Sound::Stopped; }),
                      activeSounds_.end());
#endif
}

void Game::render() {
  window_.clear(sf::Color(102, 188, 255));
  world_.render(window_);
  window_.display();
}

void Game::initializeAudio() {
#if TACTICAL_HAS_AUDIO
  startBuffer_ = makeToneBuffer({261.63f, 329.63f, 392.0f}, 0.20f, 0.26f, 0.01f, 0.08f);
  jumpBuffer_ = makeToneBuffer({392.0f, 523.25f}, 0.12f, 0.26f, 0.005f, 0.05f);
  coinBuffer_ = makeToneBuffer({987.77f, 1318.51f}, 0.10f, 0.24f, 0.002f, 0.05f);
  blockBuffer_ = makeToneBuffer({220.0f, 164.81f}, 0.09f, 0.24f, 0.002f, 0.04f);
  stompBuffer_ = makeToneBuffer({146.83f, 196.0f}, 0.10f, 0.24f, 0.002f, 0.05f);
  powerUpBuffer_ = makeToneBuffer({392.0f, 523.25f, 659.25f}, 0.32f, 0.22f, 0.01f, 0.12f);
  hurtBuffer_ = makeToneBuffer({196.0f, 146.83f}, 0.18f, 0.26f, 0.005f, 0.08f);
  extraLifeBuffer_ = makeToneBuffer({523.25f, 659.25f, 783.99f}, 0.22f, 0.22f, 0.01f, 0.10f);
  clearBuffer_ = makeToneBuffer({523.25f, 659.25f, 783.99f, 1046.5f}, 0.42f, 0.24f, 0.01f, 0.16f);
  gameOverBuffer_ = makeToneBuffer({293.66f, 246.94f, 196.0f}, 0.46f, 0.22f, 0.01f, 0.18f);
#endif
}

void Game::flushAudioEvents() {
#if TACTICAL_HAS_AUDIO
  for (WorldAudioEvent event : world_.consumeAudioEvents()) {
    sf::Sound sound;
    sound.setVolume(60.0f);

    switch (event) {
      case WorldAudioEvent::Start:
        sound.setBuffer(startBuffer_);
        break;
      case WorldAudioEvent::Jump:
        sound.setBuffer(jumpBuffer_);
        break;
      case WorldAudioEvent::Coin:
        sound.setBuffer(coinBuffer_);
        break;
      case WorldAudioEvent::Block:
        sound.setBuffer(blockBuffer_);
        break;
      case WorldAudioEvent::Stomp:
        sound.setBuffer(stompBuffer_);
        break;
      case WorldAudioEvent::PowerUp:
        sound.setBuffer(powerUpBuffer_);
        break;
      case WorldAudioEvent::Hurt:
        sound.setBuffer(hurtBuffer_);
        break;
      case WorldAudioEvent::ExtraLife:
        sound.setBuffer(extraLifeBuffer_);
        break;
      case WorldAudioEvent::LevelClear:
        sound.setBuffer(clearBuffer_);
        break;
      case WorldAudioEvent::GameOver:
        sound.setBuffer(gameOverBuffer_);
        break;
    }

    sound.play();
    activeSounds_.push_back(std::move(sound));
  }
#else
  (void)world_.consumeAudioEvents();
#endif
}

}  // namespace Game
