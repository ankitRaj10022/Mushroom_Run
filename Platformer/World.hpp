#pragma once

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/View.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Keyboard.hpp>

#include <string>
#include <vector>

namespace Game {

enum class WorldAudioEvent {
  Start,
  Jump,
  Coin,
  Block,
  Stomp,
  PowerUp,
  Hurt,
  ExtraLife,
  LevelClear,
  GameOver
};

class PlatformerWorld {
 public:
  PlatformerWorld();

  void setFont(const sf::Font* font, bool hasFont);
  void handleKeyPressed(sf::Keyboard::Key key);
  void handleKeyReleased(sf::Keyboard::Key key);
  void update(float deltaSeconds);
  void render(sf::RenderWindow& window) const;

  std::vector<WorldAudioEvent> consumeAudioEvents();

 private:
  enum class TileType {
    Empty,
    Ground,
    Brick,
    QuestionCoin,
    QuestionMushroom,
    UsedBlock,
    PipeLeft,
    PipeRight,
    PipeCapLeft,
    PipeCapRight,
    Stone
  };

  enum class ScreenState {
    Title,
    Playing,
    LevelClear,
    GameOver
  };

  struct Player {
    sf::Vector2f position{};
    sf::Vector2f previousPosition{};
    sf::Vector2f velocity{};
    sf::Vector2f size{34.0f, 46.0f};
    bool onGround{false};
    bool superForm{false};
    bool invulnerable{false};
    float invulnerableTimer{0.0f};
    bool facingRight{true};
    int coins{0};
    int lives{3};
    int score{0};
  };

  struct Enemy {
    sf::Vector2f position{};
    sf::Vector2f velocity{-90.0f, 0.0f};
    sf::Vector2f size{36.0f, 32.0f};
    bool onGround{false};
    bool alive{true};
    bool squashed{false};
    float squashTimer{0.0f};
  };

  struct Coin {
    sf::Vector2f center{};
    bool collected{false};
  };

  struct Mushroom {
    sf::Vector2f position{};
    sf::Vector2f velocity{88.0f, 0.0f};
    sf::Vector2f size{34.0f, 34.0f};
    bool active{true};
    bool emerging{true};
    float emergeDistance{0.0f};
  };

  void buildLevel();
  void resetRun();
  void resetLife(bool keepProgress);
  void startGame();
  void updatePlaying(float deltaSeconds);
  void updatePlayer(float deltaSeconds);
  void updateEnemies(float deltaSeconds);
  void updateMushrooms(float deltaSeconds);
  void updateCamera(float deltaSeconds);
  void resolvePlayerEnemyInteractions();
  void resolvePlayerCollectibles();
  void handlePlayerDamage();
  void makePlayerSuper();
  void collectCoin(int amount, int scoreValue);
  void bumpBlock(int tileX, int tileY);
  void spawnMushroom(int tileX, int tileY);
  void completeLevel();
  void pushAudio(WorldAudioEvent event);

  bool inBounds(int tileX, int tileY) const;
  TileType tileAt(int tileX, int tileY) const;
  void setTile(int tileX, int tileY, TileType tile);
  bool isSolid(TileType tile) const;
  bool isBreakable(TileType tile) const;
  sf::FloatRect tileBounds(int tileX, int tileY) const;
  sf::FloatRect playerBounds() const;
  sf::FloatRect enemyBounds(const Enemy& enemy) const;
  sf::FloatRect mushroomBounds(const Mushroom& mushroom) const;
  void resolveHorizontalCollision(sf::Vector2f& position, sf::Vector2f& velocity, const sf::Vector2f& size, bool reverseOnHit);
  bool resolveVerticalCollision(sf::Vector2f& position,
                                sf::Vector2f& velocity,
                                const sf::Vector2f& size,
                                bool& onGround,
                                bool triggerBlocks);
  void drawBackground(sf::RenderWindow& window) const;
  void drawTiles(sf::RenderWindow& window) const;
  void drawCoins(sf::RenderWindow& window) const;
  void drawMushrooms(sf::RenderWindow& window) const;
  void drawEnemies(sf::RenderWindow& window) const;
  void drawPlayer(sf::RenderWindow& window) const;
  void drawHud(sf::RenderWindow& window) const;
  void drawOverlay(sf::RenderWindow& window) const;
  void drawText(sf::RenderTarget& target,
                const std::string& text,
                unsigned int size,
                const sf::Vector2f& position,
                const sf::Color& color,
                bool centered = false) const;

  const sf::Font* uiFont_{nullptr};
  bool hasFont_{false};
  ScreenState screenState_{ScreenState::Title};
  Player player_{};
  std::vector<TileType> tiles_{};
  std::vector<Coin> coins_{};
  std::vector<Enemy> enemies_{};
  std::vector<Mushroom> mushrooms_{};
  std::vector<WorldAudioEvent> pendingAudio_{};
  sf::View worldView_{};
  int width_{0};
  int height_{0};
  float cameraX_{0.0f};
  float timeRemaining_{400.0f};
  float overlayTimer_{0.0f};
  float titlePulse_{0.0f};
  int flagColumn_{0};
  bool moveLeftHeld_{false};
  bool moveRightHeld_{false};
  bool runHeld_{false};
  bool jumpHeld_{false};
  bool jumpQueued_{false};
};

}  // namespace Game
