#include "Platformer/World.hpp"

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/VertexArray.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <sstream>

namespace Game {

namespace {

constexpr float kTileSize = 48.0f;
constexpr float kScreenWidth = 1280.0f;
constexpr float kScreenHeight = 720.0f;
constexpr float kGravity = 1850.0f;
constexpr float kJumpVelocity = -660.0f;
constexpr float kMaxFallSpeed = 920.0f;
constexpr float kWalkAcceleration = 1900.0f;
constexpr float kGroundDrag = 2100.0f;
constexpr float kAirDrag = 800.0f;
constexpr float kWalkSpeed = 255.0f;
constexpr float kRunSpeed = 340.0f;
constexpr float kEnemySpeed = 92.0f;
constexpr float kMushroomSpeed = 92.0f;

sf::Color rgb(int r, int g, int b, int a = 255) {
  return sf::Color(static_cast<sf::Uint8>(r), static_cast<sf::Uint8>(g), static_cast<sf::Uint8>(b),
                   static_cast<sf::Uint8>(a));
}

sf::Color shade(sf::Color color, int delta) {
  auto clamp = [](int value) { return static_cast<sf::Uint8>(std::clamp(value, 0, 255)); };
  color.r = clamp(static_cast<int>(color.r) + delta);
  color.g = clamp(static_cast<int>(color.g) + delta);
  color.b = clamp(static_cast<int>(color.b) + delta);
  return color;
}

float clampf(float value, float low, float high) {
  return std::clamp(value, low, high);
}

bool intersects(const sf::FloatRect& lhs, const sf::FloatRect& rhs) {
  return lhs.intersects(rhs);
}

}  // namespace

PlatformerWorld::PlatformerWorld() {
  worldView_.setSize(kScreenWidth, kScreenHeight);
  worldView_.setCenter(kScreenWidth * 0.5f, kScreenHeight * 0.5f);
  resetRun();
  screenState_ = ScreenState::Title;
}

void PlatformerWorld::setFont(const sf::Font* font, bool hasFont) {
  uiFont_ = font;
  hasFont_ = hasFont;
}

void PlatformerWorld::handleKeyPressed(sf::Keyboard::Key key) {
  if (screenState_ == ScreenState::Title || screenState_ == ScreenState::LevelClear || screenState_ == ScreenState::GameOver) {
    if (key == sf::Keyboard::Enter || key == sf::Keyboard::Space) {
      startGame();
    }
    return;
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
      resetLife(true);
      break;
    case sf::Keyboard::Escape:
      screenState_ = ScreenState::Title;
      break;
    default:
      break;
  }
}

void PlatformerWorld::handleKeyReleased(sf::Keyboard::Key key) {
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

  if (screenState_ != ScreenState::Playing) {
    return;
  }

  updatePlaying(deltaSeconds);
}

void PlatformerWorld::render(sf::RenderWindow& window) const {
  sf::View previousView = window.getView();
  sf::View worldView = worldView_;
  worldView.setCenter(cameraX_ + kScreenWidth * 0.5f, kScreenHeight * 0.5f);
  window.setView(worldView);

  drawBackground(window);
  drawTiles(window);
  drawCoins(window);
  drawMushrooms(window);
  drawEnemies(window);
  drawPlayer(window);

  window.setView(previousView);
  drawHud(window);
  drawOverlay(window);
}

std::vector<WorldAudioEvent> PlatformerWorld::consumeAudioEvents() {
  std::vector<WorldAudioEvent> events = pendingAudio_;
  pendingAudio_.clear();
  return events;
}

void PlatformerWorld::buildLevel() {
  width_ = 220;
  height_ = 15;
  tiles_.assign(static_cast<std::size_t>(width_ * height_), TileType::Empty);
  coins_.clear();
  enemies_.clear();
  mushrooms_.clear();
  flagColumn_ = 210;
  timeRemaining_ = 400.0f;
  cameraX_ = 0.0f;
  jumpQueued_ = false;
  moveLeftHeld_ = false;
  moveRightHeld_ = false;
  runHeld_ = false;
  jumpHeld_ = false;

  auto fill = [&](int left, int top, int right, int bottom, TileType tile) {
    for (int y = top; y <= bottom; ++y) {
      for (int x = left; x <= right; ++x) {
        setTile(x, y, tile);
      }
    }
  };

  auto addCoin = [&](int tileX, int tileY) {
    coins_.push_back(Coin{{tileX * kTileSize + kTileSize * 0.5f, tileY * kTileSize + kTileSize * 0.5f}, false});
  };

  auto addEnemy = [&](int tileX, int tileY) {
    Enemy enemy;
    enemy.position = {tileX * kTileSize + 6.0f, tileY * kTileSize + (kTileSize - enemy.size.y)};
    enemy.velocity.x = -kEnemySpeed;
    enemies_.push_back(enemy);
  };

  auto placePipe = [&](int column, int heightTiles) {
    const int topRow = 13 - heightTiles;
    setTile(column, topRow, TileType::PipeCapLeft);
    setTile(column + 1, topRow, TileType::PipeCapRight);
    for (int y = topRow + 1; y <= 12; ++y) {
      setTile(column, y, TileType::PipeLeft);
      setTile(column + 1, y, TileType::PipeRight);
    }
  };

  fill(0, 13, width_ - 1, 14, TileType::Ground);
  for (const auto& gap : std::array<std::pair<int, int>, 5>{{{28, 31}, {59, 62}, {88, 91}, {141, 145}, {176, 179}}}) {
    fill(gap.first, 13, gap.second, 14, TileType::Empty);
  }

  fill(12, 9, 15, 9, TileType::Brick);
  setTile(16, 9, TileType::QuestionCoin);
  setTile(17, 9, TileType::QuestionMushroom);
  setTile(18, 9, TileType::QuestionCoin);
  fill(33, 10, 35, 10, TileType::Brick);
  setTile(36, 10, TileType::QuestionCoin);
  fill(49, 8, 52, 8, TileType::Brick);
  setTile(53, 8, TileType::QuestionCoin);
  fill(74, 9, 78, 9, TileType::Stone);
  fill(98, 10, 101, 10, TileType::Brick);
  setTile(102, 10, TileType::QuestionCoin);
  setTile(103, 10, TileType::QuestionCoin);
  fill(121, 8, 125, 8, TileType::Brick);
  setTile(126, 8, TileType::QuestionMushroom);
  fill(150, 9, 154, 9, TileType::Stone);
  fill(167, 7, 170, 7, TileType::Brick);
  setTile(171, 7, TileType::QuestionCoin);
  fill(191, 12, 191, 12, TileType::Stone);
  fill(192, 11, 192, 12, TileType::Stone);
  fill(193, 10, 193, 12, TileType::Stone);
  fill(194, 9, 194, 12, TileType::Stone);
  fill(195, 8, 195, 12, TileType::Stone);
  fill(196, 7, 196, 12, TileType::Stone);
  fill(197, 6, 197, 12, TileType::Stone);
  fill(205, 10, 208, 12, TileType::Stone);

  placePipe(44, 3);
  placePipe(66, 4);
  placePipe(111, 3);
  placePipe(156, 5);

  for (int x = 13; x <= 18; ++x) {
    addCoin(x, 7);
  }
  for (int x = 49; x <= 53; ++x) {
    addCoin(x, 6);
  }
  for (int x = 98; x <= 103; ++x) {
    addCoin(x, 8);
  }
  for (int x = 167; x <= 171; ++x) {
    addCoin(x, 5);
  }
  for (int x = 182; x <= 188; ++x) {
    addCoin(x, 9 - std::abs(x - 185));
  }

  for (int x : {22, 34, 47, 70, 72, 96, 118, 134, 161, 186}) {
    addEnemy(x, 12);
  }

  player_.position = {2.0f * kTileSize, 13.0f * kTileSize - player_.size.y};
  player_.previousPosition = player_.position;
  player_.velocity = {};
  player_.onGround = false;
  player_.facingRight = true;
}

void PlatformerWorld::resetRun() {
  player_.coins = 0;
  player_.lives = 3;
  player_.score = 0;
  player_.superForm = false;
  player_.size = {34.0f, 46.0f};
  player_.invulnerable = false;
  player_.invulnerableTimer = 0.0f;
  buildLevel();
}

void PlatformerWorld::resetLife(bool keepProgress) {
  const int coins = keepProgress ? player_.coins : 0;
  const int lives = keepProgress ? player_.lives : 3;
  const int score = keepProgress ? player_.score : 0;

  player_.superForm = false;
  player_.size = {34.0f, 46.0f};
  player_.invulnerable = false;
  player_.invulnerableTimer = 0.0f;

  buildLevel();

  player_.coins = coins;
  player_.lives = lives;
  player_.score = score;
}

void PlatformerWorld::startGame() {
  resetRun();
  screenState_ = ScreenState::Playing;
  pushAudio(WorldAudioEvent::Start);
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

  const sf::FloatRect flagBounds(flagColumn_ * kTileSize + 14.0f, 4.0f * kTileSize, 18.0f, 9.0f * kTileSize);
  if (intersects(playerBounds(), flagBounds) || player_.position.x > flagColumn_ * kTileSize + 18.0f) {
    completeLevel();
  }
}

void PlatformerWorld::updatePlayer(float deltaSeconds) {
  player_.previousPosition = player_.position;

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

  player_.position.x = std::max(0.0f, player_.position.x);
}

void PlatformerWorld::updateEnemies(float deltaSeconds) {
  for (Enemy& enemy : enemies_) {
    if (!enemy.alive) {
      continue;
    }

    if (enemy.squashed) {
      enemy.squashTimer = std::max(0.0f, enemy.squashTimer - deltaSeconds);
      if (enemy.squashTimer <= 0.0f) {
        enemy.alive = false;
      }
      continue;
    }

    enemy.velocity.y = std::min(kMaxFallSpeed, enemy.velocity.y + kGravity * deltaSeconds);
    if (enemy.velocity.x == 0.0f) {
      enemy.velocity.x = -kEnemySpeed;
    }

    enemy.position.x += enemy.velocity.x * deltaSeconds;
    resolveHorizontalCollision(enemy.position, enemy.velocity, enemy.size, true);
    enemy.position.y += enemy.velocity.y * deltaSeconds;
    resolveVerticalCollision(enemy.position, enemy.velocity, enemy.size, enemy.onGround, false);

    if (enemy.position.y > height_ * kTileSize + 100.0f) {
      enemy.alive = false;
    }
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
  const float target = clampf(player_.position.x - kScreenWidth * 0.35f, 0.0f, std::max(0.0f, width_ * kTileSize - kScreenWidth));
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
    const bool stomp = player_.velocity.y > 100.0f && previousBottom <= enemy.position.y + 10.0f;
    if (stomp) {
      enemy.squashed = true;
      enemy.velocity = {};
      enemy.squashTimer = 0.45f;
      player_.velocity.y = -320.0f;
      player_.score += 200;
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

void PlatformerWorld::handlePlayerDamage() {
  if (screenState_ != ScreenState::Playing || player_.invulnerable) {
    return;
  }

  if (player_.superForm) {
    player_.superForm = false;
    player_.position.y += 22.0f;
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

  resetLife(true);
}

void PlatformerWorld::makePlayerSuper() {
  if (player_.superForm) {
    player_.score += 400;
    pushAudio(WorldAudioEvent::PowerUp);
    return;
  }

  player_.superForm = true;
  player_.position.y -= 22.0f;
  player_.size.y = 68.0f;
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
    case TileType::PipeLeft:
    case TileType::PipeRight:
    case TileType::PipeCapLeft:
    case TileType::PipeCapRight:
    case TileType::Stone:
      break;
  }
}

void PlatformerWorld::spawnMushroom(int tileX, int tileY) {
  Mushroom mushroom;
  mushroom.position = {tileX * kTileSize + 7.0f, tileY * kTileSize + 7.0f};
  mushroom.velocity.x = kMushroomSpeed;
  mushrooms_.push_back(mushroom);
}

void PlatformerWorld::completeLevel() {
  if (screenState_ != ScreenState::Playing) {
    return;
  }
  player_.score += static_cast<int>(timeRemaining_) * 10;
  screenState_ = ScreenState::LevelClear;
  pushAudio(WorldAudioEvent::LevelClear);
}

void PlatformerWorld::pushAudio(WorldAudioEvent event) {
  pendingAudio_.push_back(event);
}

bool PlatformerWorld::inBounds(int tileX, int tileY) const {
  return tileX >= 0 && tileX < width_ && tileY >= 0 && tileY < height_;
}

PlatformerWorld::TileType PlatformerWorld::tileAt(int tileX, int tileY) const {
  if (!inBounds(tileX, tileY)) {
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
  const int top = std::max(0, static_cast<int>(std::floor(bounds.top / kTileSize)) - 1);
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
  const int top = std::max(0, static_cast<int>(std::floor(bounds.top / kTileSize)) - 1);
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

void PlatformerWorld::drawBackground(sf::RenderWindow& window) const {
  sf::VertexArray sky(sf::Quads, 4);
  sky[0].position = {cameraX_, 0.0f};
  sky[1].position = {cameraX_ + kScreenWidth, 0.0f};
  sky[2].position = {cameraX_ + kScreenWidth, kScreenHeight};
  sky[3].position = {cameraX_, kScreenHeight};
  sky[0].color = rgb(121, 203, 255);
  sky[1].color = rgb(121, 203, 255);
  sky[2].color = rgb(189, 240, 255);
  sky[3].color = rgb(189, 240, 255);
  window.draw(sky);

  for (int index = 0; index < 8; ++index) {
    const float hillX = index * 340.0f + 120.0f;
    sf::CircleShape hill(180.0f, 90);
    hill.setOrigin(180.0f, 180.0f);
    hill.setPosition(hillX, 650.0f);
    hill.setScale(1.0f, 0.55f);
    hill.setFillColor(index % 2 == 0 ? rgb(100, 194, 115) : rgb(88, 176, 103));
    window.draw(hill);
  }

  for (int index = 0; index < 14; ++index) {
    const float cloudX = 120.0f + index * 290.0f + std::sin(titlePulse_ * 0.6f + index) * 8.0f;
    const float cloudY = 90.0f + static_cast<float>(index % 3) * 70.0f;
    for (int puff = 0; puff < 3; ++puff) {
      sf::CircleShape shape(28.0f + puff * 5.0f, 32);
      shape.setFillColor(rgb(255, 255, 255, 220));
      shape.setPosition(cloudX + puff * 26.0f, cloudY - (puff == 1 ? 10.0f : 0.0f));
      window.draw(shape);
    }
  }

  const float poleX = flagColumn_ * kTileSize + 24.0f;
  sf::RectangleShape pole({8.0f, 9.0f * kTileSize});
  pole.setPosition({poleX, 4.0f * kTileSize});
  pole.setFillColor(rgb(232, 243, 232));
  window.draw(pole);

  sf::RectangleShape flag({34.0f, 24.0f});
  flag.setPosition({poleX + 8.0f, 4.0f * kTileSize + 12.0f});
  flag.setFillColor(rgb(80, 200, 108));
  window.draw(flag);
}

void PlatformerWorld::drawTiles(sf::RenderWindow& window) const {
  for (int tileY = 0; tileY < height_; ++tileY) {
    for (int tileX = 0; tileX < width_; ++tileX) {
      const TileType tile = tileAt(tileX, tileY);
      if (tile == TileType::Empty) {
        continue;
      }

      const sf::Vector2f position(tileX * kTileSize, tileY * kTileSize);
      sf::RectangleShape rect({kTileSize, kTileSize});
      rect.setPosition(position);

      switch (tile) {
        case TileType::Ground: {
          rect.setFillColor(rgb(160, 103, 44));
          window.draw(rect);
          sf::RectangleShape grass({kTileSize, 10.0f});
          grass.setPosition(position);
          grass.setFillColor(rgb(82, 188, 76));
          window.draw(grass);
          break;
        }
        case TileType::Brick:
          rect.setFillColor(rgb(191, 110, 56));
          rect.setOutlineThickness(-2.0f);
          rect.setOutlineColor(rgb(140, 78, 38));
          window.draw(rect);
          break;
        case TileType::QuestionCoin:
        case TileType::QuestionMushroom: {
          rect.setFillColor(rgb(246, 189, 56));
          rect.setOutlineThickness(-2.0f);
          rect.setOutlineColor(rgb(184, 126, 34));
          window.draw(rect);
          drawText(window, "?", 26U, position + sf::Vector2f(18.0f, 8.0f), rgb(140, 74, 18));
          break;
        }
        case TileType::UsedBlock:
          rect.setFillColor(rgb(156, 154, 150));
          rect.setOutlineThickness(-2.0f);
          rect.setOutlineColor(rgb(112, 110, 104));
          window.draw(rect);
          break;
        case TileType::PipeCapLeft:
        case TileType::PipeCapRight:
        case TileType::PipeLeft:
        case TileType::PipeRight: {
          rect.setFillColor(rgb(47, 178, 76));
          rect.setOutlineThickness(-2.0f);
          rect.setOutlineColor(rgb(28, 122, 54));
          window.draw(rect);
          sf::RectangleShape shine({12.0f, kTileSize});
          shine.setPosition(position.x + 8.0f, position.y);
          shine.setFillColor(rgb(91, 224, 120, 140));
          window.draw(shine);
          break;
        }
        case TileType::Stone:
          rect.setFillColor(rgb(115, 120, 128));
          rect.setOutlineThickness(-2.0f);
          rect.setOutlineColor(rgb(72, 76, 82));
          window.draw(rect);
          break;
        case TileType::Empty:
          break;
      }
    }
  }
}

void PlatformerWorld::drawCoins(sf::RenderWindow& window) const {
  for (const Coin& coin : coins_) {
    if (coin.collected) {
      continue;
    }

    sf::CircleShape circle(12.0f, 24);
    circle.setOrigin(12.0f, 12.0f);
    circle.setScale(1.0f + std::sin(titlePulse_ * 10.0f + coin.center.x * 0.01f) * 0.18f, 1.0f);
    circle.setPosition(coin.center);
    circle.setFillColor(rgb(255, 214, 58));
    circle.setOutlineThickness(2.0f);
    circle.setOutlineColor(rgb(193, 148, 22));
    window.draw(circle);
  }
}

void PlatformerWorld::drawMushrooms(sf::RenderWindow& window) const {
  for (const Mushroom& mushroom : mushrooms_) {
    if (!mushroom.active) {
      continue;
    }

    sf::RectangleShape stem({16.0f, 18.0f});
    stem.setPosition({mushroom.position.x + 9.0f, mushroom.position.y + 16.0f});
    stem.setFillColor(rgb(255, 234, 210));
    window.draw(stem);

    sf::CircleShape cap(18.0f, 30);
    cap.setPosition(mushroom.position);
    cap.setFillColor(rgb(228, 54, 54));
    window.draw(cap);

    for (const sf::Vector2f& dot : std::array<sf::Vector2f, 3>{{{8.0f, 8.0f}, {18.0f, 4.0f}, {24.0f, 12.0f}}}) {
      sf::CircleShape spot(4.5f, 16);
      spot.setPosition(mushroom.position + dot);
      spot.setFillColor(rgb(255, 246, 236));
      window.draw(spot);
    }
  }
}

void PlatformerWorld::drawEnemies(sf::RenderWindow& window) const {
  for (const Enemy& enemy : enemies_) {
    if (!enemy.alive) {
      continue;
    }

    const sf::Vector2f position = enemy.position;
    if (enemy.squashed) {
      sf::RectangleShape squash({enemy.size.x, 14.0f});
      squash.setPosition({position.x, position.y + enemy.size.y - 14.0f});
      squash.setFillColor(rgb(124, 74, 32));
      window.draw(squash);
      continue;
    }

    sf::CircleShape body(18.0f, 28);
    body.setScale(1.0f, 0.92f);
    body.setPosition(position);
    body.setFillColor(rgb(155, 95, 45));
    window.draw(body);

    sf::RectangleShape feet({enemy.size.x, 8.0f});
    feet.setPosition({position.x, position.y + enemy.size.y - 8.0f});
    feet.setFillColor(rgb(95, 54, 23));
    window.draw(feet);

    for (const sf::Vector2f& eyeOffset : std::array<sf::Vector2f, 2>{{{11.0f, 11.0f}, {22.0f, 11.0f}}}) {
      sf::RectangleShape eye({4.0f, 9.0f});
      eye.setPosition(position + eyeOffset);
      eye.setFillColor(rgb(255, 246, 236));
      window.draw(eye);
    }
  }
}

void PlatformerWorld::drawPlayer(sf::RenderWindow& window) const {
  if (player_.invulnerable && std::fmod(player_.invulnerableTimer * 12.0f, 2.0f) < 1.0f) {
    return;
  }

  const sf::Vector2f position = player_.position;
  const float height = player_.size.y;

  sf::RectangleShape overalls({26.0f, height * 0.46f});
  overalls.setPosition({position.x + 4.0f, position.y + height * 0.46f});
  overalls.setFillColor(rgb(46, 112, 214));
  window.draw(overalls);

  sf::RectangleShape shirt({26.0f, height * 0.26f});
  shirt.setPosition({position.x + 4.0f, position.y + height * 0.32f});
  shirt.setFillColor(rgb(214, 46, 46));
  window.draw(shirt);

  sf::RectangleShape legs({26.0f, height * 0.20f});
  legs.setPosition({position.x + 4.0f, position.y + height * 0.78f});
  legs.setFillColor(rgb(92, 52, 28));
  window.draw(legs);

  sf::CircleShape head(11.0f, 24);
  head.setPosition({position.x + 6.0f, position.y + 8.0f});
  head.setFillColor(rgb(255, 220, 183));
  window.draw(head);

  sf::RectangleShape cap({30.0f, 10.0f});
  cap.setPosition({position.x + 2.0f, position.y});
  cap.setFillColor(rgb(219, 46, 46));
  window.draw(cap);

  sf::RectangleShape visor({18.0f, 4.0f});
  visor.setPosition({position.x + (player_.facingRight ? 14.0f : 2.0f), position.y + 10.0f});
  visor.setFillColor(rgb(160, 30, 30));
  window.draw(visor);
}

void PlatformerWorld::drawHud(sf::RenderWindow& window) const {
  sf::RectangleShape bar({kScreenWidth, 66.0f});
  bar.setFillColor(rgb(7, 12, 25, 210));
  window.draw(bar);

  drawText(window, "MUSHROOM RUN", 18U, {26.0f, 10.0f}, rgb(255, 246, 236));
  drawText(window, "SCORE " + std::to_string(player_.score), 17U, {26.0f, 36.0f}, rgb(255, 246, 236));
  drawText(window, "COINS " + std::to_string(player_.coins), 17U, {278.0f, 36.0f}, rgb(255, 214, 58));
  drawText(window, "WORLD 1-1", 17U, {512.0f, 36.0f}, rgb(255, 246, 236));
  drawText(window, "LIVES " + std::to_string(player_.lives), 17U, {738.0f, 36.0f}, rgb(255, 246, 236));
  drawText(window, "TIME " + std::to_string(static_cast<int>(std::ceil(timeRemaining_))), 17U, {958.0f, 36.0f},
           rgb(255, 246, 236));
}

void PlatformerWorld::drawOverlay(sf::RenderWindow& window) const {
  if (screenState_ == ScreenState::Playing) {
    drawText(window, "Arrows/A,D move   Space jump   Shift run   R restart", 14U, {kScreenWidth * 0.5f, 685.0f},
             rgb(20, 34, 54, 180), true);
    return;
  }

  sf::RectangleShape veil({kScreenWidth, kScreenHeight});
  veil.setFillColor(rgb(6, 8, 16, 166));
  window.draw(veil);

  sf::RectangleShape panel({560.0f, 300.0f});
  panel.setOrigin(280.0f, 150.0f);
  panel.setPosition({kScreenWidth * 0.5f, kScreenHeight * 0.5f});
  panel.setFillColor(rgb(10, 18, 32, 238));
  panel.setOutlineThickness(3.0f);
  panel.setOutlineColor(rgb(250, 236, 208));
  window.draw(panel);

  if (screenState_ == ScreenState::Title) {
    drawText(window, "MUSHROOM RUN", 48U, {kScreenWidth * 0.5f, 214.0f + std::sin(titlePulse_ * 2.0f) * 6.0f},
             rgb(255, 246, 236), true);
    drawText(window, "A fast side-scrolling platformer inspired by the arcade feel you asked for.", 20U,
             {kScreenWidth * 0.5f, 294.0f}, rgb(222, 231, 244), true);
    drawText(window, "Run, jump, stomp, break bricks, collect coins, grab mushrooms, reach the flag.", 18U,
             {kScreenWidth * 0.5f, 332.0f}, rgb(255, 214, 58), true);
    drawText(window, "ENTER / SPACE TO START", 24U, {kScreenWidth * 0.5f, 408.0f}, rgb(120, 218, 255), true);
    drawText(window, "Arrows or A,D move   Space jump   Shift run", 17U, {kScreenWidth * 0.5f, 452.0f},
             rgb(215, 222, 235), true);
    return;
  }

  if (screenState_ == ScreenState::LevelClear) {
    drawText(window, "COURSE CLEAR", 42U, {kScreenWidth * 0.5f, 240.0f}, rgb(121, 230, 142), true);
    drawText(window, "Final score " + std::to_string(player_.score), 24U, {kScreenWidth * 0.5f, 320.0f},
             rgb(255, 246, 236), true);
    drawText(window, "Coins " + std::to_string(player_.coins) + "   Lives " + std::to_string(player_.lives), 20U,
             {kScreenWidth * 0.5f, 360.0f}, rgb(255, 214, 58), true);
    drawText(window, "ENTER TO PLAY AGAIN", 24U, {kScreenWidth * 0.5f, 428.0f}, rgb(120, 218, 255), true);
    return;
  }

  drawText(window, "GAME OVER", 42U, {kScreenWidth * 0.5f, 250.0f}, rgb(255, 112, 112), true);
  drawText(window, "Final score " + std::to_string(player_.score), 24U, {kScreenWidth * 0.5f, 324.0f},
           rgb(255, 246, 236), true);
  drawText(window, "ENTER TO RESTART", 24U, {kScreenWidth * 0.5f, 416.0f}, rgb(120, 218, 255), true);
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
