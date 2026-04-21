#include "Platformer/World.hpp"

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/Image.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/VertexArray.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace Game {

namespace {

constexpr float kTileSize = 48.0f;
constexpr float kScreenWidth = 1280.0f;
constexpr float kScreenHeight = 720.0f;
constexpr float kGravity = 1850.0f;
constexpr float kJumpVelocity = -670.0f;
constexpr float kMaxFallSpeed = 940.0f;
constexpr float kWalkAcceleration = 1950.0f;
constexpr float kGroundDrag = 2200.0f;
constexpr float kAirDrag = 860.0f;
constexpr float kWalkSpeed = 255.0f;
constexpr float kRunSpeed = 352.0f;
constexpr float kEnemySpeed = 92.0f;
constexpr float kPlatformCarryTolerance = 10.0f;
constexpr int kTileSpriteSize = 16;
constexpr int kLevelCount = 3;

enum class SpriteId {
  Ground,
  Brick,
  Question,
  UsedBlock,
  Stone,
  PipeCapLeft,
  PipeCapRight,
  PipeBodyLeft,
  PipeBodyRight,
  Platform,
  CoinA,
  CoinB,
  Flag,
  PlayerIdle,
  PlayerRunA,
  PlayerRunB,
  PlayerJump,
  PlayerPowerIdle,
  PlayerPowerRunA,
  PlayerPowerRunB,
  PlayerPowerJump,
  WalkerA,
  WalkerB,
  HopperA,
  HopperB,
  SpikyA,
  SpikyB,
  FlyerA,
  FlyerB,
  Mushroom
};

struct LevelDescriptor {
  const char* code;
  const char* name;
  const char* subtitle;
  sf::Color skyTop;
  sf::Color skyBottom;
  sf::Color hillFar;
  sf::Color hillNear;
  float timeLimit;
};

const std::array<LevelDescriptor, kLevelCount> kLevels{{
    {"1-1", "Verdant Run", "A bright opener with ground threats and the first moving decks.",
     sf::Color(121, 203, 255), sf::Color(193, 238, 255), sf::Color(110, 196, 123), sf::Color(79, 169, 96), 380.0f},
    {"1-2", "Foundry Night", "Stone shafts, lift stacks, and tighter enemy pressure.",
     sf::Color(44, 61, 98), sf::Color(102, 126, 176), sf::Color(73, 82, 115), sf::Color(53, 60, 83), 360.0f},
    {"1-3", "Sky Bridge", "Cloud gaps, flyers, and chained moving platforms to the crown.",
     sf::Color(248, 184, 116), sf::Color(255, 224, 181), sf::Color(166, 171, 222), sf::Color(127, 142, 214), 340.0f},
}};

sf::Color rgb(int r, int g, int b, int a = 255) {
  return sf::Color(static_cast<sf::Uint8>(r), static_cast<sf::Uint8>(g), static_cast<sf::Uint8>(b),
                   static_cast<sf::Uint8>(a));
}

float clampf(float value, float low, float high) {
  return std::clamp(value, low, high);
}

bool intersects(const sf::FloatRect& lhs, const sf::FloatRect& rhs) {
  return lhs.intersects(rhs);
}

bool overlapsOnX(const sf::FloatRect& lhs, const sf::FloatRect& rhs) {
  return lhs.left < rhs.left + rhs.width && lhs.left + lhs.width > rhs.left;
}

bool isOnScreen(float cameraX, const sf::Vector2f& position, const sf::Vector2f& size) {
  return position.x + size.x >= cameraX - 96.0f && position.x <= cameraX + kScreenWidth + 96.0f;
}

sf::IntRect spriteRect(SpriteId id) {
  switch (id) {
    case SpriteId::Ground:
      return {0, 0, 16, 16};
    case SpriteId::Brick:
      return {16, 0, 16, 16};
    case SpriteId::Question:
      return {32, 0, 16, 16};
    case SpriteId::UsedBlock:
      return {48, 0, 16, 16};
    case SpriteId::Stone:
      return {64, 0, 16, 16};
    case SpriteId::PipeCapLeft:
      return {80, 0, 16, 16};
    case SpriteId::PipeCapRight:
      return {96, 0, 16, 16};
    case SpriteId::PipeBodyLeft:
      return {112, 0, 16, 16};
    case SpriteId::PipeBodyRight:
      return {128, 0, 16, 16};
    case SpriteId::Platform:
      return {144, 0, 16, 16};
    case SpriteId::CoinA:
      return {160, 0, 16, 16};
    case SpriteId::CoinB:
      return {176, 0, 16, 16};
    case SpriteId::Flag:
      return {192, 0, 16, 16};
    case SpriteId::PlayerIdle:
      return {0, 16, 16, 16};
    case SpriteId::PlayerRunA:
      return {16, 16, 16, 16};
    case SpriteId::PlayerRunB:
      return {32, 16, 16, 16};
    case SpriteId::PlayerJump:
      return {48, 16, 16, 16};
    case SpriteId::PlayerPowerIdle:
      return {64, 16, 16, 16};
    case SpriteId::PlayerPowerRunA:
      return {80, 16, 16, 16};
    case SpriteId::PlayerPowerRunB:
      return {96, 16, 16, 16};
    case SpriteId::PlayerPowerJump:
      return {112, 16, 16, 16};
    case SpriteId::WalkerA:
      return {0, 32, 16, 16};
    case SpriteId::WalkerB:
      return {16, 32, 16, 16};
    case SpriteId::HopperA:
      return {32, 32, 16, 16};
    case SpriteId::HopperB:
      return {48, 32, 16, 16};
    case SpriteId::SpikyA:
      return {64, 32, 16, 16};
    case SpriteId::SpikyB:
      return {80, 32, 16, 16};
    case SpriteId::FlyerA:
      return {96, 32, 16, 16};
    case SpriteId::FlyerB:
      return {112, 32, 16, 16};
    case SpriteId::Mushroom:
      return {128, 32, 16, 16};
  }
  return {0, 0, 16, 16};
}

void setPixelSafe(sf::Image& image, int x, int y, const sf::Color& color) {
  if (x < 0 || y < 0) {
    return;
  }

  const sf::Vector2u size = image.getSize();
  if (static_cast<unsigned int>(x) >= size.x || static_cast<unsigned int>(y) >= size.y) {
    return;
  }

  image.setPixel(static_cast<unsigned int>(x), static_cast<unsigned int>(y), color);
}

void fillRect(sf::Image& image, int x, int y, int width, int height, const sf::Color& color) {
  for (int dy = 0; dy < height; ++dy) {
    for (int dx = 0; dx < width; ++dx) {
      setPixelSafe(image, x + dx, y + dy, color);
    }
  }
}

void paintPattern(sf::Image& image,
                  int originX,
                  int originY,
                  const std::vector<std::string>& pattern,
                  const std::map<char, sf::Color>& palette,
                  int scale = 2) {
  for (std::size_t row = 0; row < pattern.size(); ++row) {
    for (std::size_t column = 0; column < pattern[row].size(); ++column) {
      const auto it = palette.find(pattern[row][column]);
      if (it == palette.end()) {
        continue;
      }

      fillRect(image, originX + static_cast<int>(column) * scale, originY + static_cast<int>(row) * scale, scale, scale, it->second);
    }
  }
}

sf::Image buildAtlasImage() {
  sf::Image image;
  image.create(208U, 48U, sf::Color(0, 0, 0, 0));

  fillRect(image, 0, 0, 16, 16, rgb(152, 95, 46));
  fillRect(image, 0, 0, 16, 4, rgb(73, 178, 74));
  for (int x = 0; x < 16; x += 4) {
    fillRect(image, x, 6 + (x % 8 == 0 ? 0 : 2), 2, 2, rgb(116, 70, 34));
  }

  fillRect(image, 16, 0, 16, 16, rgb(194, 112, 58));
  for (int x = 16; x < 32; x += 8) {
    fillRect(image, x, 5, 16, 1, rgb(138, 78, 42));
    fillRect(image, x, 10, 16, 1, rgb(138, 78, 42));
  }
  fillRect(image, 20, 2, 2, 4, rgb(143, 83, 40));
  fillRect(image, 28, 8, 2, 4, rgb(143, 83, 40));

  fillRect(image, 32, 0, 16, 16, rgb(246, 186, 58));
  fillRect(image, 33, 1, 14, 14, rgb(232, 158, 44));
  fillRect(image, 38, 4, 4, 2, rgb(255, 230, 166));
  fillRect(image, 36, 8, 8, 2, rgb(255, 230, 166));
  fillRect(image, 40, 10, 2, 2, rgb(255, 230, 166));

  fillRect(image, 48, 0, 16, 16, rgb(148, 148, 150));
  fillRect(image, 49, 1, 14, 14, rgb(124, 124, 128));
  fillRect(image, 52, 4, 8, 2, rgb(168, 168, 170));
  fillRect(image, 54, 9, 4, 2, rgb(168, 168, 170));

  fillRect(image, 64, 0, 16, 16, rgb(120, 124, 132));
  fillRect(image, 65, 1, 14, 14, rgb(98, 102, 110));
  fillRect(image, 66, 5, 4, 3, rgb(132, 136, 142));
  fillRect(image, 72, 9, 4, 3, rgb(132, 136, 142));

  fillRect(image, 80, 0, 16, 16, rgb(47, 178, 76));
  fillRect(image, 81, 1, 14, 14, rgb(34, 146, 60));
  fillRect(image, 83, 2, 4, 12, rgb(91, 224, 120, 150));
  fillRect(image, 80, 0, 16, 3, rgb(62, 204, 94));
  fillRect(image, 80, 12, 16, 4, rgb(31, 128, 54));

  fillRect(image, 96, 0, 16, 16, rgb(47, 178, 76));
  fillRect(image, 97, 1, 14, 14, rgb(34, 146, 60));
  fillRect(image, 101, 2, 4, 12, rgb(91, 224, 120, 150));
  fillRect(image, 96, 0, 16, 3, rgb(62, 204, 94));
  fillRect(image, 96, 12, 16, 4, rgb(31, 128, 54));

  fillRect(image, 112, 0, 16, 16, rgb(42, 160, 69));
  fillRect(image, 113, 1, 14, 14, rgb(31, 128, 54));
  fillRect(image, 116, 2, 4, 12, rgb(91, 224, 120, 120));

  fillRect(image, 128, 0, 16, 16, rgb(42, 160, 69));
  fillRect(image, 129, 1, 14, 14, rgb(31, 128, 54));
  fillRect(image, 133, 2, 4, 12, rgb(91, 224, 120, 120));

  fillRect(image, 144, 0, 16, 16, rgb(113, 89, 168));
  fillRect(image, 144, 5, 16, 6, rgb(143, 120, 210));
  fillRect(image, 146, 3, 12, 2, rgb(192, 175, 248));
  fillRect(image, 146, 11, 12, 2, rgb(73, 58, 108));

  fillRect(image, 160, 0, 16, 16, sf::Color(0, 0, 0, 0));
  fillRect(image, 166, 2, 4, 12, rgb(255, 221, 72));
  fillRect(image, 165, 4, 6, 8, rgb(237, 175, 50));

  fillRect(image, 176, 0, 16, 16, sf::Color(0, 0, 0, 0));
  fillRect(image, 181, 2, 6, 12, rgb(255, 221, 72));
  fillRect(image, 182, 4, 4, 8, rgb(237, 175, 50));

  fillRect(image, 192, 0, 16, 16, sf::Color(0, 0, 0, 0));
  fillRect(image, 194, 0, 2, 16, rgb(232, 243, 232));
  fillRect(image, 196, 2, 8, 6, rgb(78, 200, 110));

  const std::map<char, sf::Color> playerPalette{
      {'R', rgb(210, 48, 48)}, {'B', rgb(45, 112, 212)}, {'S', rgb(255, 218, 182)},
      {'K', rgb(96, 58, 28)},  {'Y', rgb(255, 232, 178)}, {'W', rgb(255, 255, 255)}};
  const std::map<char, sf::Color> powerPalette{
      {'R', rgb(71, 180, 97)}, {'B', rgb(255, 224, 172)}, {'S', rgb(255, 218, 182)},
      {'K', rgb(96, 58, 28)},  {'Y', rgb(255, 247, 212)}, {'W', rgb(255, 255, 255)}};
  const std::map<char, sf::Color> walkerPalette{{'B', rgb(148, 95, 45)}, {'D', rgb(97, 57, 22)}, {'W', rgb(255, 248, 234)}};
  const std::map<char, sf::Color> hopperPalette{{'B', rgb(91, 180, 212)}, {'D', rgb(54, 100, 123)}, {'W', rgb(255, 248, 234)}};
  const std::map<char, sf::Color> spikyPalette{{'B', rgb(202, 85, 70)}, {'D', rgb(130, 42, 30)}, {'S', rgb(235, 221, 210)}};
  const std::map<char, sf::Color> flyerPalette{{'B', rgb(123, 92, 210)}, {'D', rgb(70, 50, 130)}, {'W', rgb(255, 248, 234)}};
  const std::map<char, sf::Color> mushroomPalette{
      {'R', rgb(228, 54, 54)}, {'W', rgb(255, 246, 236)}, {'S', rgb(255, 227, 190)}, {'K', rgb(180, 130, 60)}};

  paintPattern(image, 0, 16,
               {"..RRRR..", ".RRRRRR.", ".SSSSSS.", "..KSSK..", "..BBBB..", ".RBBBBR.", ".B.KK.B.", "KK....KK"},
               playerPalette);
  paintPattern(image, 16, 16,
               {"..RRRR..", ".RRRRRR.", ".SSSSSS.", "...KSK..", "..BBBB..", ".RBBBBR.", ".BK.KB..", "KK...KK."},
               playerPalette);
  paintPattern(image, 32, 16,
               {"..RRRR..", ".RRRRRR.", ".SSSSSS.", "..KSS...", "..BBBBR.", ".RBBBB..", "..BK.KB.", ".KK...KK"},
               playerPalette);
  paintPattern(image, 48, 16,
               {"..RRRR..", ".RRRRRR.", ".SSSSSS.", "..KSSK..", "..BBBB..", ".RBBBBR.", ".B....B.", "KK....KK"},
               playerPalette);
  paintPattern(image, 64, 16,
               {"..RRRR..", ".RRRRRR.", ".SSSSSS.", "..KSSK..", "..BBBB..", ".YBBBBY.", ".BKKKKB.", "KK....KK"},
               powerPalette);
  paintPattern(image, 80, 16,
               {"..RRRR..", ".RRRRRR.", ".SSSSSS.", "...KSK..", "..BBBB..", ".YBBBBY.", ".BK.KB..", "KK...KK."},
               powerPalette);
  paintPattern(image, 96, 16,
               {"..RRRR..", ".RRRRRR.", ".SSSSSS.", "..KSS...", "..BBBBY.", ".YBBBB..", "..BK.KB.", ".KK...KK"},
               powerPalette);
  paintPattern(image, 112, 16,
               {"..RRRR..", ".RRRRRR.", ".SSSSSS.", "..KSSK..", "..BBBB..", ".YBBBBY.", ".B....B.", "KK....KK"},
               powerPalette);

  paintPattern(image, 0, 32,
               {"........", "..BBBB..", ".BBBBBB.", ".WW..WW.", ".BBBBBB.", ".BBBBBB.", "..DDDD..", ".D....D."},
               walkerPalette);
  paintPattern(image, 16, 32,
               {"........", "..BBBB..", ".BBBBBB.", ".WW..WW.", ".BBBBBB.", ".BBBBBB.", ".DD..DD.", "D......D"},
               walkerPalette);
  paintPattern(image, 32, 32,
               {"........", "..BBBB..", ".BBBBBB.", ".WW..WW.", ".BBBBBB.", ".BBDDDB.", "..DDDD..", ".D....D."},
               hopperPalette);
  paintPattern(image, 48, 32,
               {"........", "..BBBB..", ".BBBBBB.", ".WW..WW.", ".BBDBBB.", ".BBBBBB.", ".DD..DD.", "D......D"},
               hopperPalette);
  paintPattern(image, 64, 32,
               {"..S..S..", ".SSSSSS.", ".BBBBBB.", "SBBBBBBS", ".BWWWWB.", ".BBBBBB.", "..DDDD..", ".D....D."},
               spikyPalette);
  paintPattern(image, 80, 32,
               {"..S..S..", ".SSSSSS.", ".BBBBBB.", "SBBBBBBS", ".BWWWWB.", ".BBBBBB.", ".DD..DD.", "D......D"},
               spikyPalette);
  paintPattern(image, 96, 32,
               {"........", "..BBBB..", ".BBBBBB.", "BBBBBBBB", ".WW..WW.", ".BBBBBB.", "..DDDD..", ".D....D."},
               flyerPalette);
  paintPattern(image, 112, 32,
               {"........", "..BBBB..", ".BBBBBB.", "BBBBBBBB", ".WW..WW.", ".BBBBBB.", ".DD..DD.", "D......D"},
               flyerPalette);
  paintPattern(image, 128, 32,
               {"..RRRR..", ".RWWWWR.", "RRRWWRRR", "..SSSS..", "..SKKS..", "...KK...", "..K..K..", ".K....K."},
               mushroomPalette);

  return image;
}

SpriteId playerSprite(bool superForm, float velocityX, float velocityY, float animationTimer) {
  if (velocityY < -40.0f || velocityY > 80.0f) {
    return superForm ? SpriteId::PlayerPowerJump : SpriteId::PlayerJump;
  }

  if (std::abs(velocityX) > 40.0f) {
    const bool alternate = static_cast<int>(animationTimer * 10.0f) % 2 == 0;
    if (superForm) {
      return alternate ? SpriteId::PlayerPowerRunA : SpriteId::PlayerPowerRunB;
    }
    return alternate ? SpriteId::PlayerRunA : SpriteId::PlayerRunB;
  }

  return superForm ? SpriteId::PlayerPowerIdle : SpriteId::PlayerIdle;
}

SpriteId enemySprite(PlatformerWorld::EnemyType type, float animationTimer) {
  const bool alternate = static_cast<int>(animationTimer * 8.0f) % 2 == 0;
  switch (type) {
    case PlatformerWorld::EnemyType::Walker:
      return alternate ? SpriteId::WalkerA : SpriteId::WalkerB;
    case PlatformerWorld::EnemyType::Hopper:
      return alternate ? SpriteId::HopperA : SpriteId::HopperB;
    case PlatformerWorld::EnemyType::Spiky:
      return alternate ? SpriteId::SpikyA : SpriteId::SpikyB;
    case PlatformerWorld::EnemyType::Flyer:
      return alternate ? SpriteId::FlyerA : SpriteId::FlyerB;
  }
  return SpriteId::WalkerA;
}

}  // namespace

PlatformerWorld::PlatformerWorld() {
  worldView_.setSize(kScreenWidth, kScreenHeight);
  worldView_.setCenter(kScreenWidth * 0.5f, kScreenHeight * 0.5f);
  initializeAtlas();
}

void PlatformerWorld::setFont(const sf::Font* font, bool hasFont) {
  uiFont_ = font;
  hasFont_ = hasFont;
}

void PlatformerWorld::handleKeyPressed(sf::Keyboard::Key key) {
  switch (screenState_) {
    case ScreenState::Title:
      if (key == sf::Keyboard::Enter || key == sf::Keyboard::Space) {
        startNewCampaign();
      }
      return;
    case ScreenState::Overworld:
      if (key == sf::Keyboard::Left || key == sf::Keyboard::A) {
        selectedLevelIndex_ = std::max(0, selectedLevelIndex_ - 1);
      } else if (key == sf::Keyboard::Right || key == sf::Keyboard::D) {
        selectedLevelIndex_ = std::min(unlockedLevelCount_ - 1, selectedLevelIndex_ + 1);
      } else if (key == sf::Keyboard::Enter || key == sf::Keyboard::Space) {
        beginLevel(selectedLevelIndex_);
      } else if (key == sf::Keyboard::Escape) {
        screenState_ = ScreenState::Title;
      }
      return;
    case ScreenState::LevelIntro:
      return;
    case ScreenState::LevelClear:
      if (key == sf::Keyboard::Enter || key == sf::Keyboard::Space) {
        if (campaignCleared_) {
          screenState_ = ScreenState::Title;
        } else {
          selectedLevelIndex_ = std::min(unlockedLevelCount_ - 1, currentLevelIndex_ + 1);
          enterOverworld();
        }
      }
      return;
    case ScreenState::GameOver:
      if (key == sf::Keyboard::Enter || key == sf::Keyboard::Space) {
        startNewCampaign();
      } else if (key == sf::Keyboard::Escape) {
        screenState_ = ScreenState::Title;
      }
      return;
    case ScreenState::Playing:
      break;
  }

  switch (key) {
    case sf::Keyboard::Left:
    case sf::Keyboard::A:
      moveLeftHeld_ = true;
      break;
    case sf::Keyboard::Right:
    case sf::Keyboard::D:
      moveRightHeld_ = true;
      break;
    case sf::Keyboard::LShift:
    case sf::Keyboard::RShift:
    case sf::Keyboard::Z:
      runHeld_ = true;
      break;
    case sf::Keyboard::Up:
    case sf::Keyboard::W:
    case sf::Keyboard::Space:
      if (!jumpHeld_) {
        jumpQueued_ = true;
      }
      jumpHeld_ = true;
      break;
    case sf::Keyboard::R:
      resetCurrentLevel(true);
      break;
    case sf::Keyboard::Escape:
      enterOverworld();
      break;
    default:
      break;
  }
}

void PlatformerWorld::handleKeyReleased(sf::Keyboard::Key key) {
  if (screenState_ != ScreenState::Playing) {
    return;
  }

  switch (key) {
    case sf::Keyboard::Left:
    case sf::Keyboard::A:
      moveLeftHeld_ = false;
      break;
    case sf::Keyboard::Right:
    case sf::Keyboard::D:
      moveRightHeld_ = false;
      break;
    case sf::Keyboard::LShift:
    case sf::Keyboard::RShift:
    case sf::Keyboard::Z:
      runHeld_ = false;
      break;
    case sf::Keyboard::Up:
    case sf::Keyboard::W:
    case sf::Keyboard::Space:
      jumpHeld_ = false;
      if (player_.velocity.y < -260.0f) {
        player_.velocity.y = -260.0f;
      }
      break;
    default:
      break;
  }
}

void PlatformerWorld::update(float deltaSeconds) {
  titlePulse_ += deltaSeconds;
  animationTimer_ += deltaSeconds;

  switch (screenState_) {
    case ScreenState::Title:
    case ScreenState::Overworld:
    case ScreenState::LevelClear:
    case ScreenState::GameOver:
      return;
    case ScreenState::LevelIntro:
      updateIntro(deltaSeconds);
      return;
    case ScreenState::Playing:
      updatePlaying(deltaSeconds);
      return;
  }
}

void PlatformerWorld::render(sf::RenderWindow& window) const {
  window.setView(window.getDefaultView());

  if (screenState_ == ScreenState::Title) {
    drawMenuBackground(window);
    drawTitleScreen(window);
    return;
  }

  if (screenState_ == ScreenState::Overworld) {
    drawMenuBackground(window);
    drawOverworld(window);
    return;
  }

  sf::View worldView = worldView_;
  worldView.setCenter(cameraX_ + kScreenWidth * 0.5f, kScreenHeight * 0.5f);
  window.setView(worldView);

  drawBackground(window);
  drawTiles(window);
  drawPlatforms(window);
  drawCoins(window);
  drawMushrooms(window);
  drawEnemies(window);
  drawPlayer(window);

  window.setView(window.getDefaultView());
  drawHud(window);
  drawOverlay(window);
}

std::vector<WorldAudioEvent> PlatformerWorld::consumeAudioEvents() {
  std::vector<WorldAudioEvent> events = pendingAudio_;
  pendingAudio_.clear();
  return events;
}

void PlatformerWorld::initializeAtlas() {
  const sf::Image atlas = buildAtlasImage();
  hasAtlas_ = atlasTexture_.loadFromImage(atlas);
  if (hasAtlas_) {
    atlasTexture_.setSmooth(false);
  }
}

void PlatformerWorld::startNewCampaign() {
  player_ = Player{};
  unlockedLevelCount_ = 1;
  selectedLevelIndex_ = 0;
  currentLevelIndex_ = 0;
  campaignCleared_ = false;
  enterOverworld();
}

void PlatformerWorld::beginLevel(int levelIndex) {
  buildLevel(levelIndex);
  screenState_ = ScreenState::LevelIntro;
  levelIntroTimer_ = 1.45f;
  pushAudio(WorldAudioEvent::Start);
}

void PlatformerWorld::resetCurrentLevel(bool preserveProgress) {
  const int coins = preserveProgress ? player_.coins : 0;
  const int lives = preserveProgress ? player_.lives : 4;
  const int score = preserveProgress ? player_.score : 0;

  player_.superForm = false;
  player_.size = {36.0f, 46.0f};
  player_.invulnerable = false;
  player_.invulnerableTimer = 0.0f;
  player_.coins = coins;
  player_.lives = lives;
  player_.score = score;
  beginLevel(currentLevelIndex_);
}

void PlatformerWorld::buildLevel(int levelIndex) {
  currentLevelIndex_ = std::clamp(levelIndex, 0, kLevelCount - 1);
  const LevelDescriptor& descriptor = kLevels[static_cast<std::size_t>(currentLevelIndex_)];
  currentLevelCode_ = descriptor.code;
  currentLevelName_ = descriptor.name;
  currentLevelSubtitle_ = descriptor.subtitle;
  skyTop_ = descriptor.skyTop;
  skyBottom_ = descriptor.skyBottom;
  hillFar_ = descriptor.hillFar;
  hillNear_ = descriptor.hillNear;
  timeRemaining_ = descriptor.timeLimit;
  cameraX_ = 0.0f;
  standingPlatformIndex_ = -1;
  moveLeftHeld_ = false;
  moveRightHeld_ = false;
  runHeld_ = false;
  jumpHeld_ = false;
  jumpQueued_ = false;
  enemies_.clear();
  coins_.clear();
  mushrooms_.clear();
  platforms_.clear();

  switch (currentLevelIndex_) {
    case 0:
      buildVerdantRun();
      break;
    case 1:
      buildFoundryNight();
      break;
    case 2:
      buildSkyBridge();
      break;
    default:
      buildVerdantRun();
      break;
  }

  player_.position = playerSpawn_;
  player_.previousPosition = playerSpawn_;
  player_.velocity = {};
  player_.onGround = false;
  player_.facingRight = true;
}

void PlatformerWorld::buildVerdantRun() {
  width_ = 180;
  height_ = 15;
  flagColumn_ = 172;
  playerSpawn_ = {2.0f * kTileSize, 13.0f * kTileSize - player_.size.y};
  tiles_.assign(static_cast<std::size_t>(width_ * height_), TileType::Empty);

  fillTiles(0, 13, width_ - 1, 14, TileType::Ground);
  for (const auto& gap : std::array<std::pair<int, int>, 4>{{{28, 31}, {63, 66}, {103, 107}, {150, 153}}}) {
    fillTiles(gap.first, 13, gap.second, 14, TileType::Empty);
  }

  fillTiles(12, 9, 15, 9, TileType::Brick);
  setTile(16, 9, TileType::QuestionCoin);
  setTile(17, 9, TileType::QuestionMushroom);
  setTile(18, 9, TileType::QuestionCoin);
  fillTiles(34, 10, 38, 10, TileType::Brick);
  setTile(39, 10, TileType::QuestionCoin);
  fillTiles(69, 9, 73, 9, TileType::Stone);
  fillTiles(92, 8, 95, 8, TileType::Brick);
  setTile(96, 8, TileType::QuestionCoin);
  fillTiles(130, 10, 132, 10, TileType::Brick);
  setTile(133, 10, TileType::QuestionMushroom);
  fillTiles(160, 12, 160, 12, TileType::Stone);
  fillTiles(161, 11, 161, 12, TileType::Stone);
  fillTiles(162, 10, 162, 12, TileType::Stone);
  fillTiles(163, 9, 163, 12, TileType::Stone);
  fillTiles(164, 8, 164, 12, TileType::Stone);
  fillTiles(165, 7, 165, 12, TileType::Stone);

  placePipe(42, 3);
  placePipe(78, 4);
  placePipe(116, 3);

  addCoinLine(12, 18, 7);
  addCoinLine(34, 39, 8);
  addCoinArc(90, 7, 3);
  addCoinLine(128, 134, 8);
  addCoinArc(159, 7, 4);

  addMovingPlatform({54.0f * kTileSize, 9.2f * kTileSize}, {72.0f, 18.0f}, {118.0f, 0.0f}, 0.9f, 0.0f);
  addMovingPlatform({140.0f * kTileSize, 8.0f * kTileSize}, {72.0f, 18.0f}, {0.0f, 94.0f}, 1.1f, 1.2f);

  spawnEnemy(EnemyType::Walker, 22, 12);
  spawnEnemy(EnemyType::Walker, 46, 12);
  spawnEnemy(EnemyType::Hopper, 70, 12);
  spawnEnemy(EnemyType::Spiky, 90, 12);
  spawnEnemy(EnemyType::Walker, 122, 12);
  spawnEnemy(EnemyType::Flyer, 141, 8, -1.0f);
  spawnEnemy(EnemyType::Hopper, 158, 12);
}

void PlatformerWorld::buildFoundryNight() {
  width_ = 194;
  height_ = 15;
  flagColumn_ = 186;
  playerSpawn_ = {2.0f * kTileSize, 13.0f * kTileSize - player_.size.y};
  tiles_.assign(static_cast<std::size_t>(width_ * height_), TileType::Empty);

  fillTiles(0, 13, width_ - 1, 14, TileType::Ground);
  fillTiles(18, 11, 24, 12, TileType::Stone);
  fillTiles(32, 9, 35, 12, TileType::Stone);
  fillTiles(49, 10, 52, 12, TileType::Stone);
  fillTiles(65, 8, 71, 8, TileType::Brick);
  setTile(72, 8, TileType::QuestionCoin);
  setTile(73, 8, TileType::QuestionMushroom);
  fillTiles(90, 10, 94, 12, TileType::Stone);
  fillTiles(110, 9, 114, 9, TileType::Brick);
  setTile(115, 9, TileType::QuestionCoin);
  fillTiles(136, 12, 136, 12, TileType::Stone);
  fillTiles(137, 11, 137, 12, TileType::Stone);
  fillTiles(138, 10, 138, 12, TileType::Stone);
  fillTiles(139, 9, 139, 12, TileType::Stone);
  fillTiles(140, 8, 140, 12, TileType::Stone);
  fillTiles(161, 8, 165, 8, TileType::Brick);
  setTile(166, 8, TileType::QuestionCoin);

  for (const auto& gap : std::array<std::pair<int, int>, 4>{{{37, 40}, {79, 82}, {121, 124}, {170, 173}}}) {
    fillTiles(gap.first, 13, gap.second, 14, TileType::Empty);
  }

  placePipe(57, 3);
  placePipe(100, 5);
  placePipe(149, 4);

  addCoinLine(18, 24, 9);
  addCoinArc(69, 6, 4);
  addCoinLine(110, 116, 7);
  addCoinArc(145, 6, 5);
  addCoinLine(178, 184, 8);

  addMovingPlatform({43.0f * kTileSize, 9.0f * kTileSize}, {72.0f, 18.0f}, {0.0f, 110.0f}, 1.0f, 0.4f);
  addMovingPlatform({84.0f * kTileSize, 8.2f * kTileSize}, {72.0f, 18.0f}, {104.0f, 0.0f}, 0.85f, 1.1f);
  addMovingPlatform({127.0f * kTileSize, 7.5f * kTileSize}, {72.0f, 18.0f}, {0.0f, 132.0f}, 1.15f, 2.4f);

  spawnEnemy(EnemyType::Walker, 16, 12);
  spawnEnemy(EnemyType::Spiky, 29, 12);
  spawnEnemy(EnemyType::Hopper, 54, 12);
  spawnEnemy(EnemyType::Walker, 76, 12);
  spawnEnemy(EnemyType::Flyer, 93, 8);
  spawnEnemy(EnemyType::Spiky, 118, 12);
  spawnEnemy(EnemyType::Hopper, 144, 12);
  spawnEnemy(EnemyType::Flyer, 176, 7);
}

void PlatformerWorld::buildSkyBridge() {
  width_ = 214;
  height_ = 15;
  flagColumn_ = 206;
  playerSpawn_ = {2.0f * kTileSize, 13.0f * kTileSize - player_.size.y};
  tiles_.assign(static_cast<std::size_t>(width_ * height_), TileType::Empty);

  fillTiles(0, 13, 22, 14, TileType::Ground);
  fillTiles(31, 13, 48, 14, TileType::Ground);
  fillTiles(57, 13, 80, 14, TileType::Ground);
  fillTiles(89, 13, 109, 14, TileType::Ground);
  fillTiles(118, 13, 142, 14, TileType::Ground);
  fillTiles(152, 13, 178, 14, TileType::Ground);
  fillTiles(188, 13, width_ - 1, 14, TileType::Ground);

  fillTiles(18, 10, 21, 10, TileType::Brick);
  setTile(22, 10, TileType::QuestionCoin);
  fillTiles(63, 8, 66, 8, TileType::Brick);
  setTile(67, 8, TileType::QuestionMushroom);
  fillTiles(122, 10, 125, 10, TileType::Brick);
  setTile(126, 10, TileType::QuestionCoin);
  fillTiles(168, 9, 172, 9, TileType::Stone);
  fillTiles(198, 12, 198, 12, TileType::Stone);
  fillTiles(199, 11, 199, 12, TileType::Stone);
  fillTiles(200, 10, 200, 12, TileType::Stone);
  fillTiles(201, 9, 201, 12, TileType::Stone);
  fillTiles(202, 8, 202, 12, TileType::Stone);

  addCoinLine(18, 22, 8);
  addCoinArc(52, 7, 4);
  addCoinArc(87, 6, 5);
  addCoinArc(117, 7, 4);
  addCoinLine(166, 173, 7);

  addMovingPlatform({24.0f * kTileSize, 9.0f * kTileSize}, {72.0f, 18.0f}, {132.0f, 0.0f}, 0.8f, 0.0f);
  addMovingPlatform({50.0f * kTileSize, 7.2f * kTileSize}, {72.0f, 18.0f}, {0.0f, 126.0f}, 1.0f, 1.1f);
  addMovingPlatform({82.0f * kTileSize, 8.0f * kTileSize}, {72.0f, 18.0f}, {118.0f, 0.0f}, 0.9f, 2.3f);
  addMovingPlatform({111.0f * kTileSize, 7.0f * kTileSize}, {72.0f, 18.0f}, {0.0f, 118.0f}, 1.2f, 0.8f);
  addMovingPlatform({145.0f * kTileSize, 8.8f * kTileSize}, {72.0f, 18.0f}, {126.0f, 0.0f}, 0.85f, 1.8f);
  addMovingPlatform({180.0f * kTileSize, 6.8f * kTileSize}, {72.0f, 18.0f}, {0.0f, 124.0f}, 1.15f, 2.6f);

  spawnEnemy(EnemyType::Walker, 13, 12);
  spawnEnemy(EnemyType::Flyer, 38, 7);
  spawnEnemy(EnemyType::Hopper, 66, 12);
  spawnEnemy(EnemyType::Spiky, 98, 12);
  spawnEnemy(EnemyType::Flyer, 127, 7, -1.0f);
  spawnEnemy(EnemyType::Hopper, 160, 12);
  spawnEnemy(EnemyType::Flyer, 191, 6, -1.0f);
}

void PlatformerWorld::fillTiles(int left, int top, int right, int bottom, TileType tile) {
  for (int tileY = top; tileY <= bottom; ++tileY) {
    for (int tileX = left; tileX <= right; ++tileX) {
      setTile(tileX, tileY, tile);
    }
  }
}

void PlatformerWorld::placePipe(int column, int heightTiles) {
  const int topRow = 13 - heightTiles;
  setTile(column, topRow, TileType::PipeCapLeft);
  setTile(column + 1, topRow, TileType::PipeCapRight);
  for (int tileY = topRow + 1; tileY <= 12; ++tileY) {
    setTile(column, tileY, TileType::PipeBodyLeft);
    setTile(column + 1, tileY, TileType::PipeBodyRight);
  }
}

void PlatformerWorld::addCoin(int tileX, int tileY) {
  coins_.push_back(Coin{{tileX * kTileSize + kTileSize * 0.5f, tileY * kTileSize + kTileSize * 0.5f}, false,
                        static_cast<float>(coins_.size()) * 0.31f});
}

void PlatformerWorld::addCoinLine(int left, int right, int tileY) {
  for (int tileX = left; tileX <= right; ++tileX) {
    addCoin(tileX, tileY);
  }
}

void PlatformerWorld::addCoinArc(int centerX, int baseTileY, int radiusTiles) {
  for (int offset = -radiusTiles; offset <= radiusTiles; ++offset) {
    const float normalized = static_cast<float>(offset) / std::max(1.0f, static_cast<float>(radiusTiles));
    const int tileY = baseTileY - static_cast<int>(std::round((1.0f - normalized * normalized) * static_cast<float>(radiusTiles)));
    addCoin(centerX + offset, tileY);
  }
}

void PlatformerWorld::spawnEnemy(EnemyType type, int tileX, int tileY, float direction) {
  Enemy enemy;
  enemy.type = type;
  enemy.position = {tileX * kTileSize + 6.0f, tileY * kTileSize + 2.0f};
  enemy.baseY = enemy.position.y;
  enemy.phase = static_cast<float>(enemies_.size()) * 0.9f;
  enemy.facingRight = direction > 0.0f;

  switch (type) {
    case EnemyType::Walker:
      enemy.size = {36.0f, 32.0f};
      enemy.velocity.x = direction * kEnemySpeed;
      enemy.position.y = tileY * kTileSize + (kTileSize - enemy.size.y);
      break;
    case EnemyType::Hopper:
      enemy.size = {36.0f, 34.0f};
      enemy.velocity.x = direction * 74.0f;
      enemy.position.y = tileY * kTileSize + (kTileSize - enemy.size.y);
      break;
    case EnemyType::Spiky:
      enemy.size = {38.0f, 34.0f};
      enemy.velocity.x = direction * 66.0f;
      enemy.position.y = tileY * kTileSize + (kTileSize - enemy.size.y);
      break;
    case EnemyType::Flyer:
      enemy.size = {40.0f, 30.0f};
      enemy.velocity.x = direction * 108.0f;
      enemy.baseY = tileY * kTileSize + 6.0f;
      enemy.position.y = enemy.baseY;
      break;
  }

  enemies_.push_back(enemy);
}

void PlatformerWorld::addMovingPlatform(const sf::Vector2f& position,
                                        const sf::Vector2f& size,
                                        const sf::Vector2f& travel,
                                        float speed,
                                        float phase) {
  MovingPlatform platform;
  platform.basePosition = position;
  platform.previousPosition = position;
  platform.position = position;
  platform.size = size;
  platform.travel = travel;
  platform.speed = speed;
  platform.phase = phase;
  platforms_.push_back(platform);
}

void PlatformerWorld::updateIntro(float deltaSeconds) {
  updatePlatforms(deltaSeconds);
  updateCamera(deltaSeconds);
  levelIntroTimer_ = std::max(0.0f, levelIntroTimer_ - deltaSeconds);
  if (levelIntroTimer_ <= 0.0f) {
    screenState_ = ScreenState::Playing;
  }
}

void PlatformerWorld::updatePlaying(float deltaSeconds) {
  timeRemaining_ = std::max(0.0f, timeRemaining_ - deltaSeconds);
  if (timeRemaining_ <= 0.0f) {
    player_.superForm = false;
    player_.invulnerable = false;
    player_.lives = std::min(player_.lives, 1);
    handlePlayerDamage();
    return;
  }

  if (player_.invulnerableTimer > 0.0f) {
    player_.invulnerableTimer = std::max(0.0f, player_.invulnerableTimer - deltaSeconds);
    player_.invulnerable = player_.invulnerableTimer > 0.0f;
  }

  updatePlatforms(deltaSeconds);
  updatePlayer(deltaSeconds);
  updateEnemies(deltaSeconds);
  updateMushrooms(deltaSeconds);
  resolvePlayerCollectibles();
  resolvePlayerEnemyInteractions();
  updateCamera(deltaSeconds);

  if (player_.position.y > height_ * kTileSize + 160.0f) {
    handlePlayerDamage();
    return;
  }

  const sf::FloatRect flagBounds(flagColumn_ * kTileSize + 10.0f, 4.0f * kTileSize, 20.0f, 9.0f * kTileSize);
  if (intersects(playerBounds(), flagBounds) || player_.position.x > flagColumn_ * kTileSize + 24.0f) {
    completeLevel();
  }
}

void PlatformerWorld::updatePlayer(float deltaSeconds) {
  player_.previousPosition = player_.position;
  if (standingPlatformIndex_ >= 0 && standingPlatformIndex_ < static_cast<int>(platforms_.size())) {
    const sf::Vector2f platformDelta = platforms_[static_cast<std::size_t>(standingPlatformIndex_)].position -
                                       platforms_[static_cast<std::size_t>(standingPlatformIndex_)].previousPosition;
    player_.position += platformDelta;
    player_.previousPosition += platformDelta;
  }
  standingPlatformIndex_ = -1;

  const float targetSpeed = runHeld_ ? kRunSpeed : kWalkSpeed;
  if (moveLeftHeld_ == moveRightHeld_) {
    const float drag = player_.onGround ? kGroundDrag : kAirDrag;
    if (player_.velocity.x > 0.0f) {
      player_.velocity.x = std::max(0.0f, player_.velocity.x - drag * deltaSeconds);
    } else if (player_.velocity.x < 0.0f) {
      player_.velocity.x = std::min(0.0f, player_.velocity.x + drag * deltaSeconds);
    }
  } else if (moveLeftHeld_) {
    player_.velocity.x = std::max(-targetSpeed, player_.velocity.x - kWalkAcceleration * deltaSeconds);
    player_.facingRight = false;
  } else if (moveRightHeld_) {
    player_.velocity.x = std::min(targetSpeed, player_.velocity.x + kWalkAcceleration * deltaSeconds);
    player_.facingRight = true;
  }

  if (jumpQueued_ && player_.onGround) {
    player_.velocity.y = kJumpVelocity;
    player_.onGround = false;
    pushAudio(WorldAudioEvent::Jump);
  }
  jumpQueued_ = false;

  player_.velocity.y = std::min(kMaxFallSpeed, player_.velocity.y + kGravity * deltaSeconds);

  player_.position.x += player_.velocity.x * deltaSeconds;
  resolveHorizontalCollision(player_.position, player_.velocity, player_.size, false);

  player_.position.y += player_.velocity.y * deltaSeconds;
  resolveVerticalCollision(player_.position, player_.velocity, player_.size, player_.onGround, true);
  resolvePlayerPlatformCollisions();

  player_.position.x = std::max(0.0f, player_.position.x);
}

void PlatformerWorld::updateEnemies(float deltaSeconds) {
  for (Enemy& enemy : enemies_) {
    if (!enemy.alive) {
      continue;
    }

    enemy.previousPosition = enemy.position;
    enemy.behaviorTimer += deltaSeconds;

    if (enemy.squashed) {
      enemy.squashTimer = std::max(0.0f, enemy.squashTimer - deltaSeconds);
      if (enemy.squashTimer <= 0.0f) {
        enemy.alive = false;
      }
      continue;
    }

    if (enemy.type == EnemyType::Flyer) {
      enemy.position.x += enemy.velocity.x * deltaSeconds;
      enemy.position.y = enemy.baseY + std::sin(animationTimer_ * 3.0f + enemy.phase) * 16.0f;
      resolveHorizontalCollision(enemy.position, enemy.velocity, enemy.size, true);
      enemy.facingRight = enemy.velocity.x > 0.0f;
      continue;
    }

    if (enemy.type == EnemyType::Hopper && enemy.onGround && enemy.behaviorTimer >= 1.3f) {
      enemy.velocity.y = -520.0f;
      enemy.behaviorTimer = 0.0f;
    }

    enemy.velocity.y = std::min(kMaxFallSpeed, enemy.velocity.y + kGravity * deltaSeconds);
    if (std::abs(enemy.velocity.x) < 10.0f) {
      enemy.velocity.x = (enemy.facingRight ? 1.0f : -1.0f) *
                         (enemy.type == EnemyType::Spiky ? 66.0f : enemy.type == EnemyType::Hopper ? 74.0f : kEnemySpeed);
    }

    enemy.position.x += enemy.velocity.x * deltaSeconds;
    resolveHorizontalCollision(enemy.position, enemy.velocity, enemy.size, true);
    enemy.position.y += enemy.velocity.y * deltaSeconds;
    resolveVerticalCollision(enemy.position, enemy.velocity, enemy.size, enemy.onGround, false);
    enemy.facingRight = enemy.velocity.x > 0.0f;

    if (enemy.position.y > height_ * kTileSize + 100.0f) {
      enemy.alive = false;
    }
  }
}

void PlatformerWorld::updatePlatforms(float deltaSeconds) {
  for (MovingPlatform& platform : platforms_) {
    platform.previousPosition = platform.position;
    platform.phase += deltaSeconds * platform.speed;
    platform.position.x = platform.basePosition.x + std::sin(platform.phase) * platform.travel.x;
    platform.position.y = platform.basePosition.y + std::sin(platform.phase) * platform.travel.y;
  }
}

void PlatformerWorld::updateMushrooms(float deltaSeconds) {
  for (Mushroom& mushroom : mushrooms_) {
    if (!mushroom.active) {
      continue;
    }

    if (mushroom.emerging) {
      const float rise = std::min(deltaSeconds * 60.0f, 34.0f - mushroom.emergeDistance);
      mushroom.position.y -= rise;
      mushroom.emergeDistance += rise;
      if (mushroom.emergeDistance >= 34.0f) {
        mushroom.emerging = false;
      }
      continue;
    }

    mushroom.velocity.y = std::min(kMaxFallSpeed, mushroom.velocity.y + kGravity * deltaSeconds);
    mushroom.position.x += mushroom.velocity.x * deltaSeconds;
    resolveHorizontalCollision(mushroom.position, mushroom.velocity, mushroom.size, true);

    bool onGround = false;
    mushroom.position.y += mushroom.velocity.y * deltaSeconds;
    resolveVerticalCollision(mushroom.position, mushroom.velocity, mushroom.size, onGround, false);

    if (mushroom.position.y > height_ * kTileSize + 80.0f) {
      mushroom.active = false;
    }
  }
}

void PlatformerWorld::updateCamera(float) {
  const float target = clampf(player_.position.x - kScreenWidth * 0.34f, 0.0f, std::max(0.0f, width_ * kTileSize - kScreenWidth));
  cameraX_ += (target - cameraX_) * 0.12f;
}

void PlatformerWorld::resolvePlayerEnemyInteractions() {
  for (Enemy& enemy : enemies_) {
    if (!enemy.alive || enemy.squashed) {
      continue;
    }

    if (!intersects(playerBounds(), enemyBounds(enemy))) {
      continue;
    }

    const float previousBottom = player_.previousPosition.y + player_.size.y;
    const bool stomp = enemy.type != EnemyType::Spiky && player_.velocity.y > 100.0f && previousBottom <= enemy.position.y + 12.0f;
    if (stomp) {
      enemy.squashed = true;
      enemy.velocity = {};
      enemy.squashTimer = 0.45f;
      player_.velocity.y = enemy.type == EnemyType::Flyer ? -360.0f : -320.0f;
      player_.score += enemy.type == EnemyType::Flyer ? 300 : 200;
      pushAudio(WorldAudioEvent::Stomp);
      return;
    }

    handlePlayerDamage();
    return;
  }
}

void PlatformerWorld::resolvePlayerCollectibles() {
  for (Coin& coin : coins_) {
    if (coin.collected) {
      continue;
    }

    const sf::FloatRect coinBounds(coin.center.x - 14.0f, coin.center.y - 14.0f, 28.0f, 28.0f);
    if (intersects(playerBounds(), coinBounds)) {
      coin.collected = true;
      collectCoin(1, 100);
    }
  }

  for (Mushroom& mushroom : mushrooms_) {
    if (!mushroom.active || mushroom.emerging) {
      continue;
    }

    if (intersects(playerBounds(), mushroomBounds(mushroom))) {
      mushroom.active = false;
      makePlayerSuper();
      return;
    }
  }
}

void PlatformerWorld::resolvePlayerPlatformCollisions() {
  sf::FloatRect playerRect = playerBounds();
  player_.onGround = player_.onGround && player_.velocity.y == 0.0f;

  for (std::size_t index = 0; index < platforms_.size(); ++index) {
    const MovingPlatform& platform = platforms_[index];
    const sf::FloatRect platformRect(platform.position.x, platform.position.y, platform.size.x, platform.size.y);
    if (!overlapsOnX(playerRect, platformRect)) {
      continue;
    }

    const float previousBottom = player_.previousPosition.y + player_.size.y;
    const float currentBottom = player_.position.y + player_.size.y;
    if (previousBottom <= platform.previousPosition.y + kPlatformCarryTolerance &&
        currentBottom >= platform.position.y && currentBottom <= platform.position.y + platform.size.y + 26.0f &&
        player_.velocity.y >= 0.0f) {
      player_.position.y = platform.position.y - player_.size.y;
      player_.velocity.y = 0.0f;
      player_.onGround = true;
      standingPlatformIndex_ = static_cast<int>(index);
      playerRect = playerBounds();
    }
  }
}

void PlatformerWorld::handlePlayerDamage() {
  if (screenState_ != ScreenState::Playing || player_.invulnerable) {
    return;
  }

  if (player_.superForm) {
    player_.superForm = false;
    player_.position.y += 18.0f;
    player_.size.y = 46.0f;
    player_.invulnerable = true;
    player_.invulnerableTimer = 1.8f;
    pushAudio(WorldAudioEvent::Hurt);
    return;
  }

  pushAudio(WorldAudioEvent::Hurt);
  --player_.lives;
  if (player_.lives <= 0) {
    screenState_ = ScreenState::GameOver;
    pushAudio(WorldAudioEvent::GameOver);
    return;
  }

  resetCurrentLevel(true);
}

void PlatformerWorld::makePlayerSuper() {
  if (player_.superForm) {
    player_.score += 400;
    pushAudio(WorldAudioEvent::PowerUp);
    return;
  }

  player_.superForm = true;
  player_.position.y -= 18.0f;
  player_.size.y = 64.0f;
  player_.score += 1000;
  pushAudio(WorldAudioEvent::PowerUp);
}

void PlatformerWorld::collectCoin(int amount, int scoreValue) {
  player_.coins += amount;
  player_.score += scoreValue;
  pushAudio(WorldAudioEvent::Coin);
  if (player_.coins >= 100) {
    player_.coins -= 100;
    ++player_.lives;
    pushAudio(WorldAudioEvent::ExtraLife);
  }
}

void PlatformerWorld::bumpBlock(int tileX, int tileY) {
  const TileType tile = tileAt(tileX, tileY);
  switch (tile) {
    case TileType::Brick:
      if (player_.superForm) {
        setTile(tileX, tileY, TileType::Empty);
        player_.score += 50;
      }
      pushAudio(WorldAudioEvent::Block);
      break;
    case TileType::QuestionCoin:
      setTile(tileX, tileY, TileType::UsedBlock);
      collectCoin(1, 200);
      break;
    case TileType::QuestionMushroom:
      setTile(tileX, tileY, TileType::UsedBlock);
      spawnMushroom(tileX, tileY);
      pushAudio(WorldAudioEvent::Block);
      break;
    case TileType::UsedBlock:
    case TileType::Empty:
    case TileType::Ground:
    case TileType::PipeBodyLeft:
    case TileType::PipeBodyRight:
    case TileType::PipeCapLeft:
    case TileType::PipeCapRight:
    case TileType::Stone:
      break;
  }
}

void PlatformerWorld::spawnMushroom(int tileX, int tileY) {
  Mushroom mushroom;
  mushroom.position = {tileX * kTileSize + 7.0f, tileY * kTileSize + 7.0f};
  mushrooms_.push_back(mushroom);
}

void PlatformerWorld::completeLevel() {
  if (screenState_ != ScreenState::Playing) {
    return;
  }

  player_.score += static_cast<int>(timeRemaining_) * 12;
  if (currentLevelIndex_ + 1 < kLevelCount) {
    unlockedLevelCount_ = std::max(unlockedLevelCount_, currentLevelIndex_ + 2);
  } else {
    campaignCleared_ = true;
  }

  screenState_ = ScreenState::LevelClear;
  pushAudio(WorldAudioEvent::LevelClear);
}

void PlatformerWorld::enterOverworld() {
  screenState_ = ScreenState::Overworld;
}

void PlatformerWorld::pushAudio(WorldAudioEvent event) {
  pendingAudio_.push_back(event);
}

bool PlatformerWorld::inBounds(int tileX, int tileY) const {
  return tileX >= 0 && tileX < width_ && tileY >= 0 && tileY < height_;
}

PlatformerWorld::TileType PlatformerWorld::tileAt(int tileX, int tileY) const {
  if (tileY < 0) {
    return TileType::Empty;
  }
  if (tileX < 0 || tileX >= width_ || tileY >= height_) {
    return TileType::Ground;
  }
  return tiles_[static_cast<std::size_t>(tileY * width_ + tileX)];
}

void PlatformerWorld::setTile(int tileX, int tileY, TileType tile) {
  if (!inBounds(tileX, tileY)) {
    return;
  }
  tiles_[static_cast<std::size_t>(tileY * width_ + tileX)] = tile;
}

bool PlatformerWorld::isSolid(TileType tile) const {
  return tile != TileType::Empty;
}

bool PlatformerWorld::isBreakable(TileType tile) const {
  return tile == TileType::Brick || tile == TileType::QuestionCoin || tile == TileType::QuestionMushroom;
}

sf::FloatRect PlatformerWorld::tileBounds(int tileX, int tileY) const {
  return {tileX * kTileSize, tileY * kTileSize, kTileSize, kTileSize};
}

sf::FloatRect PlatformerWorld::playerBounds() const {
  return {player_.position.x, player_.position.y, player_.size.x, player_.size.y};
}

sf::FloatRect PlatformerWorld::enemyBounds(const Enemy& enemy) const {
  return {enemy.position.x, enemy.position.y, enemy.size.x, enemy.size.y};
}

sf::FloatRect PlatformerWorld::mushroomBounds(const Mushroom& mushroom) const {
  return {mushroom.position.x, mushroom.position.y, mushroom.size.x, mushroom.size.y};
}

void PlatformerWorld::resolveHorizontalCollision(sf::Vector2f& position,
                                                 sf::Vector2f& velocity,
                                                 const sf::Vector2f& size,
                                                 bool reverseOnHit) {
  sf::FloatRect bounds(position.x, position.y, size.x, size.y);
  const int left = std::max(0, static_cast<int>(std::floor(bounds.left / kTileSize)) - 1);
  const int right = std::min(width_ - 1, static_cast<int>(std::floor((bounds.left + bounds.width - 1.0f) / kTileSize)) + 1);
  const int top = std::max(-1, static_cast<int>(std::floor(bounds.top / kTileSize)) - 1);
  const int bottom =
      std::min(height_ - 1, static_cast<int>(std::floor((bounds.top + bounds.height - 1.0f) / kTileSize)) + 1);

  for (int tileY = top; tileY <= bottom; ++tileY) {
    for (int tileX = left; tileX <= right; ++tileX) {
      if (!isSolid(tileAt(tileX, tileY))) {
        continue;
      }

      const sf::FloatRect solid = tileBounds(tileX, tileY);
      if (!intersects(bounds, solid)) {
        continue;
      }

      const float direction = velocity.x >= 0.0f ? 1.0f : -1.0f;
      if (direction > 0.0f) {
        position.x = solid.left - size.x;
      } else {
        position.x = solid.left + solid.width;
      }

      if (reverseOnHit) {
        velocity.x = -direction * std::max(60.0f, std::abs(velocity.x));
      } else {
        velocity.x = 0.0f;
      }
      bounds.left = position.x;
    }
  }
}

bool PlatformerWorld::resolveVerticalCollision(sf::Vector2f& position,
                                               sf::Vector2f& velocity,
                                               const sf::Vector2f& size,
                                               bool& onGround,
                                               bool triggerBlocks) {
  bool collided = false;
  onGround = false;
  sf::FloatRect bounds(position.x, position.y, size.x, size.y);
  const int left = std::max(0, static_cast<int>(std::floor(bounds.left / kTileSize)) - 1);
  const int right = std::min(width_ - 1, static_cast<int>(std::floor((bounds.left + bounds.width - 1.0f) / kTileSize)) + 1);
  const int top = std::max(-1, static_cast<int>(std::floor(bounds.top / kTileSize)) - 1);
  const int bottom =
      std::min(height_ - 1, static_cast<int>(std::floor((bounds.top + bounds.height - 1.0f) / kTileSize)) + 1);

  for (int tileY = top; tileY <= bottom; ++tileY) {
    for (int tileX = left; tileX <= right; ++tileX) {
      if (!isSolid(tileAt(tileX, tileY))) {
        continue;
      }

      const sf::FloatRect solid = tileBounds(tileX, tileY);
      if (!intersects(bounds, solid)) {
        continue;
      }

      collided = true;
      if (velocity.y > 0.0f) {
        position.y = solid.top - size.y;
        velocity.y = 0.0f;
        onGround = true;
      } else if (velocity.y < 0.0f) {
        position.y = solid.top + solid.height;
        velocity.y = 0.0f;
        if (triggerBlocks && isBreakable(tileAt(tileX, tileY))) {
          bumpBlock(tileX, tileY);
        }
      }
      bounds.top = position.y;
    }
  }

  return collided;
}

void PlatformerWorld::drawMenuBackground(sf::RenderWindow& window) const {
  sf::VertexArray sky(sf::Quads, 4);
  sky[0].position = {0.0f, 0.0f};
  sky[1].position = {kScreenWidth, 0.0f};
  sky[2].position = {kScreenWidth, kScreenHeight};
  sky[3].position = {0.0f, kScreenHeight};
  sky[0].color = rgb(24, 33, 68);
  sky[1].color = rgb(42, 52, 106);
  sky[2].color = rgb(86, 101, 158);
  sky[3].color = rgb(54, 64, 124);
  window.draw(sky);

  for (int index = 0; index < 16; ++index) {
    sf::CircleShape star(1.5f + static_cast<float>(index % 3), 8);
    star.setFillColor(rgb(255, 255, 245, 180));
    star.setPosition(60.0f + index * 74.0f + std::sin(titlePulse_ + index) * 4.0f, 45.0f + (index % 4) * 42.0f);
    window.draw(star);
  }

  for (int index = 0; index < 6; ++index) {
    sf::CircleShape hill(220.0f, 90);
    hill.setOrigin(220.0f, 220.0f);
    hill.setScale(1.0f, 0.58f);
    hill.setPosition(180.0f + index * 220.0f, 760.0f);
    hill.setFillColor(index % 2 == 0 ? rgb(48, 92, 82) : rgb(36, 70, 65));
    window.draw(hill);
  }
}

void PlatformerWorld::drawBackground(sf::RenderWindow& window) const {
  sf::VertexArray sky(sf::Quads, 4);
  sky[0].position = {cameraX_, 0.0f};
  sky[1].position = {cameraX_ + kScreenWidth, 0.0f};
  sky[2].position = {cameraX_ + kScreenWidth, kScreenHeight};
  sky[3].position = {cameraX_, kScreenHeight};
  sky[0].color = skyTop_;
  sky[1].color = skyTop_;
  sky[2].color = skyBottom_;
  sky[3].color = skyBottom_;
  window.draw(sky);

  for (int index = -1; index < 8; ++index) {
    const float parallax = cameraX_ * 0.28f;
    sf::CircleShape hill(220.0f, 90);
    hill.setOrigin(220.0f, 220.0f);
    hill.setScale(1.0f, 0.54f);
    hill.setPosition(index * 290.0f + 180.0f + parallax, 700.0f);
    hill.setFillColor(hillFar_);
    window.draw(hill);
  }

  for (int index = -1; index < 10; ++index) {
    const float parallax = cameraX_ * 0.44f;
    sf::CircleShape hill(170.0f, 90);
    hill.setOrigin(170.0f, 170.0f);
    hill.setScale(1.0f, 0.52f);
    hill.setPosition(index * 220.0f + 120.0f + parallax, 720.0f);
    hill.setFillColor(hillNear_);
    window.draw(hill);
  }

  for (int index = -1; index < 9; ++index) {
    const float cloudX = cameraX_ * 0.12f + index * 240.0f + 90.0f;
    const float cloudY = 80.0f + static_cast<float>(index % 3) * 62.0f;
    for (int puff = 0; puff < 3; ++puff) {
      sf::CircleShape cloud(22.0f + puff * 4.0f, 24);
      cloud.setFillColor(rgb(255, 255, 255, 190));
      cloud.setPosition(cloudX + puff * 24.0f, cloudY - (puff == 1 ? 10.0f : 0.0f));
      window.draw(cloud);
    }
  }

  drawSprite(window, spriteRect(SpriteId::Flag), {flagColumn_ * kTileSize - 6.0f, 4.0f * kTileSize}, {48.0f, 48.0f});
}

void PlatformerWorld::drawTiles(sf::RenderWindow& window) const {
  const int left = std::max(0, static_cast<int>(cameraX_ / kTileSize) - 2);
  const int right = std::min(width_ - 1, static_cast<int>((cameraX_ + kScreenWidth) / kTileSize) + 3);

  for (int tileY = 0; tileY < height_; ++tileY) {
    for (int tileX = left; tileX <= right; ++tileX) {
      const TileType tile = tileAt(tileX, tileY);
      if (tile == TileType::Empty) {
        continue;
      }

      const sf::Vector2f position(tileX * kTileSize, tileY * kTileSize);
      SpriteId id = SpriteId::Ground;
      switch (tile) {
        case TileType::Ground:
          id = SpriteId::Ground;
          break;
        case TileType::Brick:
          id = SpriteId::Brick;
          break;
        case TileType::QuestionCoin:
        case TileType::QuestionMushroom:
          id = SpriteId::Question;
          break;
        case TileType::UsedBlock:
          id = SpriteId::UsedBlock;
          break;
        case TileType::PipeCapLeft:
          id = SpriteId::PipeCapLeft;
          break;
        case TileType::PipeCapRight:
          id = SpriteId::PipeCapRight;
          break;
        case TileType::PipeBodyLeft:
          id = SpriteId::PipeBodyLeft;
          break;
        case TileType::PipeBodyRight:
          id = SpriteId::PipeBodyRight;
          break;
        case TileType::Stone:
          id = SpriteId::Stone;
          break;
        case TileType::Empty:
          break;
      }
      drawSprite(window, spriteRect(id), position, {kTileSize, kTileSize});
    }
  }
}

void PlatformerWorld::drawPlatforms(sf::RenderWindow& window) const {
  for (const MovingPlatform& platform : platforms_) {
    if (!isOnScreen(cameraX_, platform.position, platform.size)) {
      continue;
    }
    drawSprite(window, spriteRect(SpriteId::Platform), platform.position, platform.size);
  }
}

void PlatformerWorld::drawCoins(sf::RenderWindow& window) const {
  const SpriteId frame = static_cast<int>(animationTimer_ * 10.0f) % 2 == 0 ? SpriteId::CoinA : SpriteId::CoinB;
  for (const Coin& coin : coins_) {
    if (coin.collected) {
      continue;
    }
    const sf::Vector2f size(26.0f, 26.0f);
    const sf::Vector2f position(coin.center.x - size.x * 0.5f, coin.center.y - size.y * 0.5f + std::sin(animationTimer_ * 6.0f + coin.phase) * 3.5f);
    if (!isOnScreen(cameraX_, position, size)) {
      continue;
    }
    drawSprite(window, spriteRect(frame), position, size);
  }
}

void PlatformerWorld::drawMushrooms(sf::RenderWindow& window) const {
  for (const Mushroom& mushroom : mushrooms_) {
    if (!mushroom.active || !isOnScreen(cameraX_, mushroom.position, mushroom.size)) {
      continue;
    }
    drawSprite(window, spriteRect(SpriteId::Mushroom), mushroom.position, mushroom.size);
  }
}

void PlatformerWorld::drawEnemies(sf::RenderWindow& window) const {
  for (const Enemy& enemy : enemies_) {
    if (!enemy.alive || !isOnScreen(cameraX_, enemy.position, enemy.size)) {
      continue;
    }

    if (enemy.squashed) {
      drawSprite(window, spriteRect(enemySprite(enemy.type, animationTimer_)), enemy.position + sf::Vector2f(0.0f, enemy.size.y - 14.0f),
                 {enemy.size.x, 14.0f}, !enemy.facingRight, sf::Color(255, 255, 255, 220));
      continue;
    }

    drawSprite(window, spriteRect(enemySprite(enemy.type, animationTimer_)), enemy.position, enemy.size, !enemy.facingRight);
  }
}

void PlatformerWorld::drawPlayer(sf::RenderWindow& window) const {
  if (player_.invulnerable && std::fmod(player_.invulnerableTimer * 12.0f, 2.0f) < 1.0f) {
    return;
  }

  const SpriteId frame = playerSprite(player_.superForm, player_.velocity.x, player_.velocity.y, animationTimer_);
  drawSprite(window, spriteRect(frame), player_.position, player_.size, !player_.facingRight);
}

void PlatformerWorld::drawHud(sf::RenderWindow& window) const {
  sf::RectangleShape bar({kScreenWidth, 66.0f});
  bar.setFillColor(rgb(7, 12, 25, 214));
  window.draw(bar);

  drawText(window, "MUSHROOM RUN", 18U, {26.0f, 10.0f}, rgb(255, 246, 236));
  drawText(window, "SCORE " + std::to_string(player_.score), 17U, {26.0f, 36.0f}, rgb(255, 246, 236));
  drawText(window, "COINS " + std::to_string(player_.coins), 17U, {270.0f, 36.0f}, rgb(255, 214, 58));
  drawText(window, currentLevelCode_ + "  " + currentLevelName_, 17U, {510.0f, 36.0f}, rgb(255, 246, 236));
  drawText(window, "LIVES " + std::to_string(player_.lives), 17U, {836.0f, 36.0f}, rgb(255, 246, 236));
  drawText(window, "TIME " + std::to_string(static_cast<int>(std::ceil(timeRemaining_))), 17U, {1030.0f, 36.0f},
           rgb(255, 246, 236));
}

void PlatformerWorld::drawTitleScreen(sf::RenderWindow& window) const {
  drawText(window, "MUSHROOM RUN", 58U, {kScreenWidth * 0.5f, 126.0f + std::sin(titlePulse_ * 1.8f) * 5.0f},
           rgb(255, 246, 236), true);
  drawText(window, "A full side-scrolling platformer with campaign flow, multiple levels, enemy variety, and textured sprites.",
           22U, {kScreenWidth * 0.5f, 206.0f}, rgb(226, 233, 248), true);

  drawSprite(window, spriteRect(SpriteId::PlayerRunA), {320.0f, 292.0f}, {112.0f, 138.0f}, false);
  drawSprite(window, spriteRect(SpriteId::WalkerA), {518.0f, 334.0f}, {92.0f, 84.0f});
  drawSprite(window, spriteRect(SpriteId::FlyerA), {698.0f, 302.0f + std::sin(titlePulse_ * 2.8f) * 10.0f}, {98.0f, 76.0f});
  drawSprite(window, spriteRect(SpriteId::Platform), {498.0f, 426.0f}, {128.0f, 28.0f});
  drawSprite(window, spriteRect(SpriteId::CoinA), {864.0f, 310.0f + std::sin(titlePulse_ * 5.0f) * 6.0f}, {46.0f, 46.0f});
  drawSprite(window, spriteRect(SpriteId::Mushroom), {952.0f, 334.0f}, {74.0f, 74.0f});

  sf::RectangleShape panel({760.0f, 178.0f});
  panel.setOrigin(380.0f, 89.0f);
  panel.setPosition({kScreenWidth * 0.5f, 540.0f});
  panel.setFillColor(rgb(10, 18, 32, 230));
  panel.setOutlineThickness(3.0f);
  panel.setOutlineColor(rgb(250, 236, 208));
  window.draw(panel);

  drawText(window, "Campaign flow: Overworld  ->  Course  ->  Rewards  ->  Next mission", 22U, {kScreenWidth * 0.5f, 494.0f},
           rgb(255, 214, 58), true);
  drawText(window, "Enemy roster: walkers, hoppers, spikies, flyers, plus moving-platform traversal.", 20U,
           {kScreenWidth * 0.5f, 534.0f}, rgb(226, 233, 248), true);
  drawText(window, "ENTER / SPACE TO START CAMPAIGN", 24U, {kScreenWidth * 0.5f, 586.0f}, rgb(120, 218, 255), true);
  drawText(window, "Arrows or A,D move   Space jump   Shift run   R restart current stage", 18U,
           {kScreenWidth * 0.5f, 626.0f}, rgb(215, 222, 235), true);
}

void PlatformerWorld::drawOverworld(sf::RenderWindow& window) const {
  drawText(window, "OVERWORLD", 48U, {kScreenWidth * 0.5f, 90.0f}, rgb(255, 246, 236), true);
  drawText(window, "Choose the next route. Completed stages stay unlocked.", 22U, {kScreenWidth * 0.5f, 142.0f},
           rgb(226, 233, 248), true);

  const std::array<sf::Vector2f, kLevelCount> nodePositions{{{240.0f, 388.0f}, {600.0f, 286.0f}, {962.0f, 388.0f}}};
  for (int index = 0; index < kLevelCount - 1; ++index) {
    sf::Vertex line[] = {
        sf::Vertex(nodePositions[static_cast<std::size_t>(index)], rgb(210, 216, 240, 170)),
        sf::Vertex(nodePositions[static_cast<std::size_t>(index + 1)], rgb(210, 216, 240, 170)),
    };
    window.draw(line, 2, sf::Lines);
  }

  for (int index = 0; index < kLevelCount; ++index) {
    const bool unlocked = index < unlockedLevelCount_;
    const bool completed = index < unlockedLevelCount_ - 1;
    const bool selected = index == selectedLevelIndex_;
    const float pulse = selected ? 1.0f + std::sin(titlePulse_ * 4.0f) * 0.08f : 1.0f;

    sf::CircleShape node(34.0f, 32);
    node.setOrigin(34.0f, 34.0f);
    node.setPosition(nodePositions[static_cast<std::size_t>(index)]);
    node.setScale(pulse, pulse);
    node.setFillColor(!unlocked ? rgb(72, 78, 106) : completed ? rgb(86, 194, 102) : rgb(124, 168, 255));
    node.setOutlineThickness(4.0f);
    node.setOutlineColor(selected ? rgb(255, 240, 188) : rgb(240, 244, 252));
    window.draw(node);

    const SpriteId icon = index == 0 ? SpriteId::WalkerA : index == 1 ? SpriteId::HopperA : SpriteId::FlyerA;
    drawSprite(window, spriteRect(icon), nodePositions[static_cast<std::size_t>(index)] - sf::Vector2f(26.0f, 28.0f),
               {52.0f, 52.0f}, false, unlocked ? sf::Color::White : rgb(150, 156, 178));
  }

  const LevelDescriptor& descriptor = kLevels[static_cast<std::size_t>(selectedLevelIndex_)];
  sf::RectangleShape panel({760.0f, 210.0f});
  panel.setOrigin(380.0f, 105.0f);
  panel.setPosition({kScreenWidth * 0.5f, 570.0f});
  panel.setFillColor(rgb(10, 18, 32, 228));
  panel.setOutlineThickness(3.0f);
  panel.setOutlineColor(rgb(250, 236, 208));
  window.draw(panel);

  drawText(window, std::string(descriptor.code) + "  " + descriptor.name, 32U, {220.0f, 498.0f}, rgb(255, 246, 236));
  drawText(window, descriptor.subtitle, 19U, {220.0f, 546.0f}, rgb(220, 228, 242));
  drawText(window, "Lives " + std::to_string(player_.lives) + "   Coins " + std::to_string(player_.coins) + "   Score " +
                       std::to_string(player_.score),
           19U, {220.0f, 588.0f}, rgb(255, 214, 58));
  drawText(window, "LEFT / RIGHT SELECT   ENTER LAUNCH   ESC TITLE", 20U, {220.0f, 630.0f}, rgb(120, 218, 255));
}

void PlatformerWorld::drawOverlay(sf::RenderWindow& window) const {
  if (screenState_ == ScreenState::Playing) {
    drawText(window, currentLevelSubtitle_, 16U, {kScreenWidth * 0.5f, 685.0f}, rgb(24, 34, 52, 190), true);
    return;
  }

  sf::RectangleShape veil({kScreenWidth, kScreenHeight});
  veil.setFillColor(rgb(6, 8, 16, 126));
  window.draw(veil);

  sf::RectangleShape panel({640.0f, 236.0f});
  panel.setOrigin(320.0f, 118.0f);
  panel.setPosition({kScreenWidth * 0.5f, kScreenHeight * 0.5f});
  panel.setFillColor(rgb(10, 18, 32, 238));
  panel.setOutlineThickness(3.0f);
  panel.setOutlineColor(rgb(250, 236, 208));
  window.draw(panel);

  if (screenState_ == ScreenState::LevelIntro) {
    drawText(window, currentLevelCode_, 28U, {kScreenWidth * 0.5f, 274.0f}, rgb(120, 218, 255), true);
    drawText(window, currentLevelName_, 42U, {kScreenWidth * 0.5f, 330.0f}, rgb(255, 246, 236), true);
    drawText(window, currentLevelSubtitle_, 21U, {kScreenWidth * 0.5f, 388.0f}, rgb(226, 233, 248), true);
    drawText(window, "Get ready", 22U, {kScreenWidth * 0.5f, 450.0f}, rgb(255, 214, 58), true);
    return;
  }

  if (screenState_ == ScreenState::LevelClear) {
    drawText(window, campaignCleared_ ? "CAMPAIGN CLEAR" : "COURSE CLEAR", 42U, {kScreenWidth * 0.5f, 260.0f},
             campaignCleared_ ? rgb(121, 230, 142) : rgb(120, 218, 255), true);
    drawText(window, "Score " + std::to_string(player_.score) + "   Coins " + std::to_string(player_.coins), 24U,
             {kScreenWidth * 0.5f, 332.0f}, rgb(255, 246, 236), true);
    drawText(window, campaignCleared_ ? "All routes complete. Enter to return to title." : "Enter to return to the overworld.",
             22U, {kScreenWidth * 0.5f, 398.0f}, rgb(255, 214, 58), true);
    return;
  }

  drawText(window, "GAME OVER", 42U, {kScreenWidth * 0.5f, 270.0f}, rgb(255, 112, 112), true);
  drawText(window, "Final score " + std::to_string(player_.score), 24U, {kScreenWidth * 0.5f, 340.0f}, rgb(255, 246, 236),
           true);
  drawText(window, "Enter to start a new campaign", 22U, {kScreenWidth * 0.5f, 410.0f}, rgb(120, 218, 255), true);
}

void PlatformerWorld::drawSprite(sf::RenderTarget& target,
                                 const sf::IntRect& textureRect,
                                 const sf::Vector2f& position,
                                 const sf::Vector2f& size,
                                 bool flipX,
                                 const sf::Color& tint) const {
  if (!hasAtlas_) {
    return;
  }

  sf::Sprite sprite(atlasTexture_);
  sprite.setTextureRect(textureRect);
  sprite.setColor(tint);

  if (flipX) {
    sprite.setOrigin(static_cast<float>(textureRect.width), 0.0f);
    sprite.setPosition(position.x + size.x, position.y);
    sprite.setScale(-(size.x / static_cast<float>(textureRect.width)), size.y / static_cast<float>(textureRect.height));
  } else {
    sprite.setPosition(position);
    sprite.setScale(size.x / static_cast<float>(textureRect.width), size.y / static_cast<float>(textureRect.height));
  }

  target.draw(sprite);
}

void PlatformerWorld::drawText(sf::RenderTarget& target,
                               const std::string& text,
                               unsigned int size,
                               const sf::Vector2f& position,
                               const sf::Color& color,
                               bool centered) const {
  if (!hasFont_ || uiFont_ == nullptr) {
    return;
  }

  sf::Text label;
  label.setFont(*uiFont_);
  label.setString(text);
  label.setCharacterSize(size);
  label.setFillColor(color);

  if (centered) {
    const sf::FloatRect bounds = label.getLocalBounds();
    label.setPosition({position.x - (bounds.width * 0.5f + bounds.left), position.y - (bounds.height * 0.5f + bounds.top)});
  } else {
    label.setPosition(position);
  }

  target.draw(label);
}

}  // namespace Game
