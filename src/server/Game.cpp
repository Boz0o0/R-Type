#include <algorithm>
#include <ampersand/core/components/Transform.hpp>
#include <ampersand/core/ecs/Entity.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <ampersand/core/math/Vec2.hpp>
#include <ampersand/core/resources/Clock.hpp>
#include <ampersand/physics/components/BoxCollider.hpp>
#include <ampersand/physics/components/Velocity.hpp>
#include <ampersand/physics/resources/Collisions.hpp>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <rtype/common/Factories.hpp>
#include <rtype/common/components/Health.hpp>
#include <rtype/common/components/Kind.hpp>
#include <rtype/common/components/Owner.hpp>
#include <rtype/common/components/Weapon.hpp>
#include <rtype/common/protocol/Protocol.hpp>
#include <rtype/common/resources/Deaths.hpp>
#include <rtype/common/resources/Spawns.hpp>
#include <rtype/server/Game.hpp>
#include <rtype/server/GameEvents.hpp>
#include <vector>

namespace rtype::server {

using ampersand::core::Transform;
using ampersand::core::ecs::Entity;
using ampersand::core::ecs::World;
using ampersand::physics::BoxCollider;
using ampersand::physics::Velocity;
using common::DeathReason;
using common::EntityKind;
using common::Kind;
namespace InputFlag = protocol::InputFlag;

Game::Game(std::uint32_t seed) : _waves({SCREEN_WIDTH, SCREEN_HEIGHT}, seed) {
  this->_world.addResource<ampersand::core::Clock>();
  this->_world.addResource<ampersand::physics::Collisions>();
  this->_world.addResource<common::Deaths>();
  this->_world.addResource<common::Spawns>();
}

void Game::addPlayer(std::uint8_t playerId) {
  this->_players.at(playerId) = Player{};
}

void Game::removePlayer(std::uint8_t playerId) {
  auto& player = this->_players.at(playerId);
  if (!player) {
    return;
  }
  if (player->ship) {
    this->kill(*player->ship, DeathReason::Killed);
  }
  player.reset();
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
void Game::setInput(std::uint8_t playerId, std::uint8_t inputMask) {
  if (auto& player = this->_players.at(playerId)) {
    player->inputMask = inputMask;
  }
}

std::vector<GameEvent> Game::update(float deltaTime) {
  std::vector<GameEvent> events;
  auto& clock = this->_world.getResource<ampersand::core::Clock>();
  clock.dt = deltaTime;
  clock.elapsed += deltaTime;

  this->updatePhase(deltaTime, events);
  if (this->_phase == Phase::Playing) {
    this->_waves.update(this->_world, deltaTime);
    this->updatePlayers(deltaTime);
  }
  this->_shootingSystem.update(this->_world);
  this->_movementSystem.update(this->_world);
  this->keepShipsOnScreen();
  this->_collisionSystem.update(this->_world);
  this->_damageSystem.update(this->_world);
  this->_outOfScreenSystem.update(this->_world);
  if (this->_phase == Phase::Playing) {
    this->applyDeaths();
    this->checkEnd(events);
  }
  this->announce(events);
  this->reportPlayers(events);
  this->_cleanupSystem.update(this->_world);
  return events;
}

std::vector<EntityInfo> Game::entities() {
  std::vector<EntityInfo> entities;
  for (const Entity entity : this->_world.owners<NetworkId>()) {
    entities.push_back(this->infoOf(entity));
  }
  return entities;
}

std::vector<PlayerChanged> Game::playerStates() {
  std::vector<PlayerChanged> states;
  for (std::uint8_t playerId = 0; playerId < protocol::MAX_PLAYERS;
       ++playerId) {
    if (const auto& player = this->_players[playerId]) {
      states.push_back(this->stateOf(playerId, *player));
    }
  }
  return states;
}

Game::Phase Game::phase() const noexcept { return this->_phase; }

std::size_t Game::playerCount() const noexcept {
  return static_cast<std::size_t>(std::ranges::count_if(
      this->_players, [](const auto& player) { return player.has_value(); }));
}

std::optional<Entity> Game::shipOf(std::uint8_t playerId) const {
  const auto& player = this->_players.at(playerId);
  return player ? player->ship : std::nullopt;
}

World& Game::world() noexcept { return this->_world; }

void Game::updatePhase(float deltaTime, std::vector<GameEvent>& events) {
  switch (this->_phase) {
    case Phase::Waiting:
      if (this->hasPlayers()) {
        this->start(events);
      }
      break;
    case Phase::Playing:
      if (!this->hasPlayers()) {
        this->clear();
        this->_phase = Phase::Waiting;
      }
      break;
    case Phase::Over:
      this->_restartIn -= deltaTime;
      if (this->_restartIn > 0.0F) {
        break;
      }
      if (this->hasPlayers()) {
        this->start(events);
      } else {
        this->_phase = Phase::Waiting;
      }
      break;
  }
}

void Game::start(std::vector<GameEvent>& events) {
  for (auto& player : this->_players) {
    if (player) {
      player->lives = STARTING_LIVES;
      player->score = 0;
      player->respawnIn = 0.0F;
    }
  }
  this->_waves.reset();
  this->_phase = Phase::Playing;
  events.emplace_back(GameStarted{});
}

void Game::end(bool victory, std::vector<GameEvent>& events) {
  std::uint32_t score = 0;
  for (const auto& player : this->_players) {
    score += player ? player->score : 0;
  }
  events.emplace_back(GameEnded{.victory = victory, .score = score});
  this->clear();
  this->_phase = Phase::Over;
  this->_restartIn = RESTART_DELAY;
}

void Game::clear() {
  for (const Entity entity : this->_world.owners<Kind>()) {
    this->kill(entity, DeathReason::Killed);
  }
  for (auto& player : this->_players) {
    if (player) {
      player->ship.reset();
      player->respawnIn = 0.0F;
    }
  }
}

void Game::updatePlayers(float deltaTime) {
  for (std::uint8_t playerId = 0; playerId < protocol::MAX_PLAYERS;
       ++playerId) {
    auto& player = this->_players[playerId];
    if (!player) {
      continue;
    }
    if (!player->ship && player->lives > 0) {
      player->respawnIn -= deltaTime;
      if (player->respawnIn <= 0.0F) {
        this->spawnShip(*player, playerId);
      }
    }
    if (player->ship) {
      this->steer(*player->ship, player->inputMask);
    }
  }
}

void Game::steer(Entity ship, std::uint8_t inputMask) {
  const ampersand::core::math::Vec2f arrows = direction(inputMask);
  float speed = SHIP_SPEED;
  if (arrows.x != 0.0F && arrows.y != 0.0F) {
    speed *= DIAGONAL_FACTOR;
  }
  this->_world.get<Velocity>(ship).value = {arrows.x * speed, arrows.y * speed};
  this->_world.get<common::Weapon>(ship).firing =
      (inputMask & InputFlag::SHOOT) != 0;
}

void Game::spawnShip(Player& player, std::uint8_t playerId) {
  const float lane = SCREEN_HEIGHT / (protocol::MAX_PLAYERS + 1.0F);
  const auto ship = common::createShip(
      this->_world,
      {SHIP_START_X, (lane * static_cast<float>(playerId + 1)) - 16.0F});
  if (!ship) {
    return;  // the World is full: try again on the next tick
  }
  this->_world.add(*ship, common::Owner{.playerId = playerId});
  player.ship = ship;
}

void Game::keepShipsOnScreen() {
  for (const auto& player : this->_players) {
    if (!player || !player->ship) {
      continue;
    }
    const Entity ship = *player->ship;
    auto& position = this->_world.get<Transform>(ship).position;
    auto& velocity = this->_world.get<Velocity>(ship).value;
    const auto& bounds = this->_world.get<BoxCollider>(ship).bounds;
    const float maxX = SCREEN_WIDTH - bounds.width;
    const float maxY = SCREEN_HEIGHT - bounds.height;
    if (position.x < 0.0F || position.x > maxX) {
      position.x = std::clamp(position.x, 0.0F, maxX);
      velocity.x = 0.0F;
    }
    if (position.y < 0.0F || position.y > maxY) {
      position.y = std::clamp(position.y, 0.0F, maxY);
      velocity.y = 0.0F;
    }
  }
}

void Game::applyDeaths() {
  for (const common::Death& death :
       this->_world.getResource<common::Deaths>().entities) {
    if (death.reason == DeathReason::Killed && death.killer &&
        this->_world.has<common::Owner>(*death.killer) &&
        this->_world.has<Kind>(death.entity) &&
        this->_world.get<Kind>(death.entity).kind == EntityKind::Enemy) {
      auto& shooter = this->_players.at(
          this->_world.get<common::Owner>(*death.killer).playerId %
          protocol::MAX_PLAYERS);
      if (shooter) {
        shooter->score += ENEMY_SCORE;
      }
    }
    for (auto& player : this->_players) {
      if (player && player->ship == death.entity) {
        player->ship.reset();
        player->lives = player->lives > 0 ? player->lives - 1 : 0;
        player->respawnIn = RESPAWN_DELAY;
      }
    }
  }
}

void Game::checkEnd(std::vector<GameEvent>& events) {
  if (!this->hasPlayers()) {
    return;
  }
  const bool everyShipLost =
      std::ranges::all_of(this->_players, [](const auto& player) {
        return !player || (!player->ship && player->lives == 0);
      });
  if (everyShipLost) {
    this->end(false, events);
    return;
  }
  if (!this->_waves.finished()) {
    return;
  }
  for (const Entity entity : this->_world.owners<Kind>()) {
    if (this->_world.get<Kind>(entity).kind == EntityKind::Enemy &&
        !this->isDying(entity)) {
      return;
    }
  }
  this->end(true, events);
}

void Game::announce(std::vector<GameEvent>& events) {
  for (const Entity entity :
       this->_world.getResource<common::Spawns>().entities) {
    if (!this->_world.alive(entity) || this->isDying(entity) ||
        this->_world.has<NetworkId>(entity)) {
      continue;
    }
    this->_world.add(entity, NetworkId{.value = this->_nextEntityId++});
    events.emplace_back(EntitySpawned{.entity = this->infoOf(entity)});
  }
  for (const common::Death& death :
       this->_world.getResource<common::Deaths>().entities) {
    if (!this->_world.has<NetworkId>(death.entity)) {
      continue;
    }
    events.emplace_back(EntityDestroyed{
        .id = this->_world.get<NetworkId>(death.entity).value,
        .reason = death.reason,
    });
    this->_world.remove<NetworkId>(death.entity);
  }
}

void Game::reportPlayers(std::vector<GameEvent>& events) {
  for (std::uint8_t playerId = 0; playerId < protocol::MAX_PLAYERS;
       ++playerId) {
    auto& slot = this->_players[playerId];
    if (!slot) {
      continue;
    }
    Player& player = *slot;
    const PlayerChanged state = this->stateOf(playerId, player);
    if (player.reported != state) {
      player.reported = state;
      events.emplace_back(state);
    }
  }
}

void Game::kill(Entity entity, DeathReason reason) {
  if (!this->_world.alive(entity) || this->isDying(entity)) {
    return;
  }
  this->_world.getResource<common::Deaths>().entities.push_back(
      {.entity = entity, .reason = reason});
}

bool Game::isDying(Entity entity) {
  return std::ranges::any_of(
      this->_world.getResource<common::Deaths>().entities,
      [entity](const common::Death& death) { return death.entity == entity; });
}

ampersand::core::math::Vec2f Game::direction(std::uint8_t inputMask) {
  const auto held = [inputMask](std::uint8_t key) {
    return (inputMask & key) != 0 ? 1.0F : 0.0F;
  };
  return {held(InputFlag::MOVE_RIGHT) - held(InputFlag::MOVE_LEFT),
          held(InputFlag::MOVE_DOWN) - held(InputFlag::MOVE_UP)};
}

bool Game::hasPlayers() const noexcept {
  return std::ranges::any_of(
      this->_players, [](const auto& player) { return player.has_value(); });
}

PlayerChanged Game::stateOf(std::uint8_t playerId, const Player& player) {
  std::int32_t health = 0;
  if (player.ship && this->_world.has<common::Health>(*player.ship)) {
    health = this->_world.get<common::Health>(*player.ship).current;
  }
  return PlayerChanged{
      .playerId = playerId,
      .health =
          static_cast<std::uint8_t>(std::clamp<std::int32_t>(health, 0, 255)),
      .lives = player.lives,
      .score = player.score,
  };
}

EntityInfo Game::infoOf(Entity entity) {
  const auto& position = this->_world.get<Transform>(entity).position;
  EntityInfo info{
      .id = this->_world.get<NetworkId>(entity).value,
      .kind = this->_world.has<Kind>(entity)
                  ? this->_world.get<Kind>(entity).kind
                  : EntityKind::Obstacle,
      .position = {position.x, position.y},
  };
  if (this->_world.has<common::Owner>(entity)) {
    info.ownerId = this->_world.get<common::Owner>(entity).playerId;
  }
  if (this->_world.has<Velocity>(entity)) {
    info.velocity = this->_world.get<Velocity>(entity).value;
  }
  return info;
}

}  // namespace rtype::server
