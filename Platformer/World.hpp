#pragma once

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Texture.hpp>
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
  enum class EnemyType {
    Walker,
    Hopper,
    Spiky,
    Flyer
  };

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
    PipeBodyLeft,
    PipeBodyRight,
    PipeCapLeft,
    PipeCapRight,
    Stone
  };

  enum class ScreenState {
    Title,
    Overworld,
    LevelIntro,
    Playing,
    LevelClear,
    GameOver
  };

  struct Player {
    sf::Vector2f position{};
    sf::Vector2f previousPosition{};
    sf::Vector2f velocity{};
    sf::Vector2f size{36.0f, 46.0f};
    bool onGround{false};
    bool superForm{false};
    bool invulnerable{false};
    float invulnerableTimer{0.0f};
    bool facingRight{true};
    int coins{0};
    int lives{4};
    int score{0};
  };

  struct Enemy {
    EnemyType type{EnemyType::Walker};
    sf::Vector2f position{};
    sf::Vector2f previousPosition{};
    sf::Vector2f velocity{-92.0f, 0.0f};
    sf::Vector2f size{36.0f, 32.0f};
    bool onGround{false};
    bool alive{true};
    bool squashed{false};
    float squashTimer{0.0f};
    float behaviorTimer{0.0f};
    float phase{0.0f};
    float baseY{0.0f};
    bool facingRight{false};
  };

  struct Coin {
    sf::Vector2f center{};
    bool collected{false};
    float phase{0.0f};
  };

  struct Mushroom {
    sf::Vector2f position{};
    sf::Vector2f velocity{92.0f, 0.0f};
    sf::Vector2f size{34.0f, 34.0f};
    bool active{true};
    bool emerging{true};
    float emergeDistance{0.0f};
  };

  struct MovingPlatform {
    sf::Vector2f basePosition{};
    sf::Vector2f previousPosition{};
    sf::Vector2f position{};
    sf::Vector2f size{72.0f, 18.0f};
    sf::Vector2f travel{};
    float speed{1.0f};
    float phase{0.0f};
  };

  void initializeAtlas();
  void startNewCampaign();
  void beginLevel(int levelIndex);
  void resetCurrentLevel(bool preserveProgress);
  void buildLevel(int levelIndex);
  void buildVerdantRun();
  void buildFoundryNight();
  void buildSkyBridge();

  void fillTiles(int left, int top, int right, int bottom, TileType tile);
  void placePipe(int column, int heightTiles);
  void addCoin(int tileX, int tileY);
  void addCoinLine(int left, int right, int tileY);
  void addCoinArc(int centerX, int baseTileY, int radiusTiles);
  void spawnEnemy(EnemyType type, int tileX, int tileY, float direction = -1.0f);
  void addMovingPlatform(const sf::Vector2f& position,
                         const sf::Vector2f& size,
                         const sf::Vector2f& travel,
                         float speed,
                         float phase);

  void updateIntro(float deltaSeconds);
  void updatePlaying(float deltaSeconds);
  void updatePlayer(float deltaSeconds);
  void updateEnemies(float deltaSeconds);
  void updatePlatforms(float deltaSeconds);
  void updateMushrooms(float deltaSeconds);
  void updateCamera(float deltaSeconds);
  void resolvePlayerEnemyInteractions();
  void resolvePlayerCollectibles();
  void resolvePlayerPlatformCollisions();
  void handlePlayerDamage();
  void makePlayerSuper();
  void collectCoin(int amount, int scoreValue);
  void bumpBlock(int tileX, int tileY);
  void spawnMushroom(int tileX, int tileY);
  void completeLevel();
  void enterOverworld();
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

  void drawMenuBackground(sf::RenderWindow& window) const;
  void drawBackground(sf::RenderWindow& window) const;
  void drawTiles(sf::RenderWindow& window) const;
  void drawPlatforms(sf::RenderWindow& window) const;
  void drawCoins(sf::RenderWindow& window) const;
  void drawMushrooms(sf::RenderWindow& window) const;
  void drawEnemies(sf::RenderWindow& window) const;
  void drawPlayer(sf::RenderWindow& window) const;
  void drawHud(sf::RenderWindow& window) const;
  void drawTitleScreen(sf::RenderWindow& window) const;
  void drawOverworld(sf::RenderWindow& window) const;
  void drawOverlay(sf::RenderWindow& window) const;
  void drawSprite(sf::RenderTarget& target,
                  const sf::IntRect& textureRect,
                  const sf::Vector2f& position,
                  const sf::Vector2f& size,
                  bool flipX = false,
                  const sf::Color& tint = sf::Color::White) const;
  void drawText(sf::RenderTarget& target,
                const std::string& text,
                unsigned int size,
                const sf::Vector2f& position,
                const sf::Color& color,
                bool centered = false) const;

  const sf::Font* uiFont_{nullptr};
  bool hasFont_{false};
  bool hasAtlas_{false};
  ScreenState screenState_{ScreenState::Title};
  Player player_{};
  std::vector<TileType> tiles_{};
  std::vector<Coin> coins_{};
  std::vector<Enemy> enemies_{};
  std::vector<Mushroom> mushrooms_{};
  std::vector<MovingPlatform> platforms_{};
  std::vector<WorldAudioEvent> pendingAudio_{};
  sf::Texture atlasTexture_{};
  sf::View worldView_{};
  sf::Vector2f playerSpawn_{};
  sf::Color skyTop_{};
  sf::Color skyBottom_{};
  sf::Color hillNear_{};
  sf::Color hillFar_{};
  std::string currentLevelCode_{};
  std::string currentLevelName_{};
  std::string currentLevelSubtitle_{};
  int width_{0};
  int height_{0};
  int flagColumn_{0};
  int selectedLevelIndex_{0};
  int currentLevelIndex_{0};
  int unlockedLevelCount_{1};
  int standingPlatformIndex_{-1};
  float cameraX_{0.0f};
  float timeRemaining_{0.0f};
  float titlePulse_{0.0f};
  float animationTimer_{0.0f};
  float levelIntroTimer_{0.0f};
  bool campaignCleared_{false};
  bool moveLeftHeld_{false};
  bool moveRightHeld_{false};
  bool runHeld_{false};
  bool jumpHeld_{false};
  bool jumpQueued_{false};
};

}  // namespace Game
