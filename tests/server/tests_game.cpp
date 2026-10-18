#include <gtest/gtest.h>

#include <ampersand/core/components/Transform.hpp>
#include <ampersand/core/ecs/Entity.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <ampersand/physics/components/Velocity.hpp>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <rtype/common/Factories.hpp>
#include <rtype/common/components/Health.hpp>
#include <rtype/common/components/Kind.hpp>
#include <rtype/common/protocol/Protocol.hpp>
#include <rtype/common/resources/Deaths.hpp>
#include <rtype/server/Game.hpp>
#include <rtype/server/GameEvents.hpp>
#include <rtype/server/systems/WaveSystem.hpp>
#include <set>
#include <stdexcept>
#include <variant>
#include <vector>

using ampersand::core::Transform;
using ampersand::core::ecs::Entity;
using ampersand::physics::Velocity;
using rtype::common::DeathReason;
using rtype::common::EntityKind;
using namespace rtype::server;
namespace InputFlag = rtype::protocol::InputFlag;

namespace {

constexpr float TICK = 1.0F / 60.0F;
constexpr std::uint32_t SEED = 42;

/** @brief The events of type T in a list. */
template <typename T>
std::vector<T> eventsOf(const std::vector<GameEvent>& events) {
  std::vector<T> found;
  for (const GameEvent& event : events) {
    if (const auto* typed = std::get_if<T>(&event)) {
      found.push_back(*typed);
    }
  }
  return found;
}

/** @brief Runs ticks for some seconds and returns all their events. */
std::vector<GameEvent> runFor(Game& game, float seconds) {
  std::vector<GameEvent> events;
  const auto ticks = static_cast<int>(std::ceil(seconds / TICK));
  for (int tick = 0; tick < ticks; ++tick) {
    for (const GameEvent& event : game.update(TICK)) {
      events.push_back(event);
    }
  }
  return events;
}

/** @brief Spawned entities of a kind in a list of events. */
std::size_t spawnedCount(const std::vector<GameEvent>& events,
                         EntityKind kind) {
  std::size_t count = 0;
  for (const EntitySpawned& spawned : eventsOf<EntitySpawned>(events)) {
    count += spawned.entity.kind == kind ? 1 : 0;
  }
  return count;
}

/** @brief The ship of a player, which must have one. */
Entity shipOf(const Game& game, std::uint8_t playerId) {
  const auto ship = game.shipOf(playerId);
  if (!ship) {
    throw std::logic_error("the player has no ship");
  }
  return *ship;
}

/** @brief Destroys a ship as if it had been shot down. */
void shootDown(Game& game, Entity ship) {
  game.world().getResource<rtype::common::Deaths>().entities.push_back(
      {.entity = ship, .reason = DeathReason::Killed});
}

/**
 * @brief Shoots down every ship of player 0, waiting for each to come back.
 *
 * @return The events of the tick the last ship was lost.
 */
std::vector<GameEvent> loseEveryLife(Game& game) {
  for (std::uint8_t life = 1; life < Game::STARTING_LIVES; ++life) {
    shootDown(game, shipOf(game, 0));
    (void)game.update(TICK);
    (void)runFor(game, Game::RESPAWN_DELAY + TICK);  // it comes back
  }
  shootDown(game, shipOf(game, 0));  // the last one
  return game.update(TICK);
}

}  // namespace

TEST(Game, WaitsForAPlayerThenStarts) {
  Game game(SEED);
  EXPECT_TRUE(runFor(game, 1.0F).empty());
  EXPECT_EQ(game.phase(), Game::Phase::Waiting);

  game.addPlayer(0);
  const auto events = game.update(TICK);

  EXPECT_EQ(game.phase(), Game::Phase::Playing);
  EXPECT_EQ(eventsOf<GameStarted>(events).size(), 1U);
  const auto spawned = eventsOf<EntitySpawned>(events);
  ASSERT_EQ(spawned.size(), 1U);
  EXPECT_EQ(spawned[0].entity.kind, EntityKind::Ship);
  EXPECT_EQ(spawned[0].entity.ownerId, 0);
  const auto states = eventsOf<PlayerChanged>(events);
  ASSERT_EQ(states.size(), 1U);
  EXPECT_EQ(states[0].lives, Game::STARTING_LIVES);
  EXPECT_GT(states[0].health, 0);
  EXPECT_EQ(states[0].score, 0U);
}

TEST(Game, MovesTheShipWithTheKeys) {
  Game game(SEED);
  game.addPlayer(1);
  (void)game.update(TICK);
  const Entity ship = shipOf(game, 1);
  const float startX = game.world().get<Transform>(ship).position.x;

  game.setInput(1, InputFlag::MOVE_RIGHT);
  (void)game.update(TICK);
  EXPECT_NEAR(game.world().get<Transform>(ship).position.x - startX,
              Game::SHIP_SPEED * TICK, 0.01F);

  game.setInput(1, InputFlag::MOVE_RIGHT | InputFlag::MOVE_UP);
  (void)game.update(TICK);
  const auto velocity = game.world().get<Velocity>(ship).value;
  EXPECT_GT(velocity.x, 0.0F);
  EXPECT_LT(velocity.y, 0.0F);
  EXPECT_NEAR(std::hypot(velocity.x, velocity.y), Game::SHIP_SPEED, 0.1F);
}

TEST(Game, KeepsShipsOnTheScreen) {
  Game game(SEED);
  game.addPlayer(0);
  game.setInput(0, InputFlag::MOVE_LEFT | InputFlag::MOVE_UP);

  (void)runFor(game, 3.0F);

  const Entity ship = shipOf(game, 0);
  const auto& position = game.world().get<Transform>(ship).position;
  EXPECT_FLOAT_EQ(position.x, 0.0F);
  EXPECT_FLOAT_EQ(position.y, 0.0F);
  EXPECT_FLOAT_EQ(game.world().get<Velocity>(ship).value.x, 0.0F);
}

TEST(Game, ShipFiresWhileShootIsHeld) {
  Game game(SEED);
  game.addPlayer(2);
  game.setInput(2, InputFlag::SHOOT);

  const auto events = runFor(game, 1.0F);

  const auto spawned = eventsOf<EntitySpawned>(events);
  std::size_t missiles = 0;
  for (const EntitySpawned& entity : spawned) {
    if (entity.entity.kind == EntityKind::PlayerMissile) {
      ++missiles;
      EXPECT_EQ(entity.entity.ownerId, 2);
      EXPECT_GT(entity.entity.velocity.x, 0.0F);
    }
  }
  EXPECT_GE(missiles, 4U);  // one every 0.25 s
}

TEST(Game, NeverReusesAnEntityId) {
  Game game(SEED);
  game.addPlayer(0);
  game.setInput(0, InputFlag::SHOOT);

  // Long enough for missiles to leave the screen and their rows to be reused.
  const auto events = runFor(game, 6.0F);

  std::set<std::uint32_t> ids;
  for (const EntitySpawned& spawned : eventsOf<EntitySpawned>(events)) {
    EXPECT_TRUE(ids.insert(spawned.entity.id).second);
  }
  EXPECT_FALSE(eventsOf<EntityDestroyed>(events).empty());
}

TEST(Game, SendsWavesOfEnemies) {
  Game game(SEED);
  game.addPlayer(0);

  const auto events = runFor(game, WaveSystem::FIRST_WAVE_DELAY + TICK);

  EXPECT_EQ(spawnedCount(events, EntityKind::Enemy),
            WaveSystem::FIRST_WAVE_SIZE);
}

TEST(Game, DestroysTheShipOfAPlayerWhoLeaves) {
  Game game(SEED);
  game.addPlayer(0);
  game.addPlayer(1);
  const auto started = game.update(TICK);
  ASSERT_EQ(spawnedCount(started, EntityKind::Ship), 2U);

  game.removePlayer(1);
  const auto events = game.update(TICK);

  EXPECT_EQ(game.playerCount(), 1U);
  EXPECT_FALSE(game.shipOf(1).has_value());
  const auto destroyed = eventsOf<EntityDestroyed>(events);
  ASSERT_EQ(destroyed.size(), 1U);
  EXPECT_EQ(destroyed[0].id, eventsOf<EntitySpawned>(started)[1].entity.id);
  EXPECT_EQ(game.phase(), Game::Phase::Playing);
}

TEST(Game, GoesBackToWaitingWhenEveryoneLeaves) {
  Game game(SEED);
  game.addPlayer(0);
  (void)game.update(TICK);

  game.removePlayer(0);
  (void)game.update(TICK);

  EXPECT_EQ(game.phase(), Game::Phase::Waiting);
  EXPECT_TRUE(game.entities().empty());
}

TEST(Game, CreditsTheShooterOfAnEnemy) {
  Game game(SEED);
  game.addPlayer(3);
  (void)game.update(TICK);
  auto& world = game.world();
  const auto enemy = rtype::common::createEnemy(world, {1000.0F, 500.0F});
  const auto shot = rtype::common::createMissile(
      world, EntityKind::PlayerMissile, shipOf(game, 3));
  ASSERT_TRUE(enemy && shot);
  world.get<rtype::common::Health>(*enemy).current = 1;
  world.get<Transform>(*shot).position = {1010.0F, 520.0F, 0.0F};

  const auto events = game.update(TICK);

  const auto states = eventsOf<PlayerChanged>(events);
  ASSERT_EQ(states.size(), 1U);
  EXPECT_EQ(states[0].playerId, 3);
  EXPECT_EQ(states[0].score, Game::ENEMY_SCORE);
}

TEST(Game, RespawnsShipsUntilTheLivesRunOut) {
  Game game(SEED);
  game.addPlayer(0);
  (void)game.update(TICK);

  const auto events = loseEveryLife(game);

  EXPECT_EQ(game.phase(), Game::Phase::Over);
  const auto ended = eventsOf<GameEnded>(events);
  ASSERT_EQ(ended.size(), 1U);
  EXPECT_FALSE(ended[0].victory);
  const auto states = eventsOf<PlayerChanged>(events);
  ASSERT_FALSE(states.empty());
  EXPECT_EQ(states.back().lives, 0);
}

TEST(Game, RestartsAMomentAfterTheEnd) {
  Game game(SEED);
  game.addPlayer(0);
  (void)game.update(TICK);
  (void)loseEveryLife(game);

  const auto restarted = runFor(game, Game::RESTART_DELAY + TICK);

  EXPECT_EQ(eventsOf<GameStarted>(restarted).size(), 1U);
  EXPECT_EQ(game.phase(), Game::Phase::Playing);
  EXPECT_TRUE(game.shipOf(0).has_value());
}

TEST(Game, WinsOnceEveryWaveIsBeaten) {
  Game game(SEED);
  game.addPlayer(0);
  std::vector<GameEnded> ended;
  const float levelLength =
      WaveSystem::FIRST_WAVE_DELAY +
      (WaveSystem::WAVE_INTERVAL * WaveSystem::WAVE_COUNT);

  const auto ticks = static_cast<int>(levelLength / TICK);
  for (int tick = 0; tick < ticks && ended.empty(); ++tick) {
    // Every enemy is shot down as soon as it appears.
    auto& world = game.world();
    for (const Entity entity : world.owners<rtype::common::Kind>()) {
      if (world.get<rtype::common::Kind>(entity).kind == EntityKind::Enemy) {
        world.getResource<rtype::common::Deaths>().entities.push_back(
            {.entity = entity, .reason = DeathReason::Killed});
      }
    }
    ended = eventsOf<GameEnded>(game.update(TICK));
  }

  ASSERT_EQ(ended.size(), 1U);
  EXPECT_TRUE(ended[0].victory);
  EXPECT_EQ(game.phase(), Game::Phase::Over);
}
