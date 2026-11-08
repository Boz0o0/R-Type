#include <gtest/gtest.h>

#include <ampersand/core/components/Transform.hpp>
#include <ampersand/core/ecs/Entity.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <ampersand/physics/components/Velocity.hpp>
#include <ampersand/render/components/Sprite.hpp>
#include <cstdint>
#include <optional>
#include <rtype/client/WorldMirror.hpp>
#include <rtype/client/components/Correction.hpp>
#include <rtype/client/components/Predicted.hpp>
#include <rtype/common/components/Kind.hpp>
#include <rtype/common/protocol/Packets.hpp>
#include <rtype/common/protocol/Protocol.hpp>
#include <rtype/common/resources/Deaths.hpp>
#include <stdexcept>
#include <vector>

#include "FakeEntityFactory.hpp"

using ampersand::core::Transform;
using ampersand::core::ecs::Entity;
using ampersand::core::ecs::World;
using ampersand::physics::Velocity;
using ampersand::render::Sprite;
using rtype::client::Correction;
using rtype::client::Predicted;
using rtype::client::WorldMirror;
using rtype::client::test::FakeEntityFactory;
using rtype::common::DeathReason;
using rtype::common::EntityKind;
using rtype::protocol::EntityDestroy;
using rtype::protocol::EntitySpawn;
using rtype::protocol::EntityState;
using rtype::protocol::WorldSnapshot;

static EntitySpawn spawnPacket(
    std::uint32_t entityId, EntityKind kind,
    std::uint8_t ownerId = rtype::protocol::NO_OWNER) {
  return EntitySpawn{
      .entityId = entityId,
      .entityType = kind,
      .ownerId = ownerId,
      .x = 100.0F,
      .y = 200.0F,
      .velocityX = -50.0F,
      .velocityY = 0.0F,
  };
}

static Entity entityOf(const WorldMirror& mirror, std::uint32_t entityId) {
  const auto entity = mirror.find(entityId);
  if (!entity) {
    throw std::runtime_error("entity is not mirrored");
  }
  return *entity;
}

// The death effects created, found by their fake texture.
static std::vector<Entity> deathsOf(World& world, EntityKind kind) {
  std::vector<Entity> found;
  for (const Entity entity : world.owners<Sprite>()) {
    if (world.get<Sprite>(entity).texture ==
        FakeEntityFactory::deathTextureOf(kind)) {
      found.push_back(entity);
    }
  }
  return found;
}

static float positionX(World& world, const WorldMirror& mirror,
                       std::uint32_t entityId) {
  return world.get<Transform>(entityOf(mirror, entityId)).position.x;
}

TEST(WorldMirror, SpawnsTheEntityWhereTheServerSays) {
  World world;
  FakeEntityFactory factory(world);
  WorldMirror mirror(world, factory);

  mirror.spawn(spawnPacket(7, EntityKind::Enemy));

  EXPECT_EQ(mirror.size(), 1U);
  const Entity entity = entityOf(mirror, 7);
  const auto& transform = world.get<Transform>(entity);
  EXPECT_FLOAT_EQ(transform.position.x, 100.0F);
  EXPECT_FLOAT_EQ(transform.position.y, 200.0F);
  EXPECT_FLOAT_EQ(transform.scale.x, 3.0F);
  EXPECT_FLOAT_EQ(transform.scale.y, 3.0F);
  EXPECT_FLOAT_EQ(world.get<Velocity>(entity).value.x, -50.0F);
  EXPECT_EQ(world.get<Sprite>(entity).texture,
            FakeEntityFactory::textureOf(EntityKind::Enemy, 0));
}

TEST(WorldMirror, PicksTheSpriteOfEachKind) {
  World world;
  FakeEntityFactory factory(world);
  WorldMirror mirror(world, factory);

  mirror.spawn(spawnPacket(1, EntityKind::PlayerMissile));
  mirror.spawn(spawnPacket(2, EntityKind::EnemyMissile));
  mirror.spawn(spawnPacket(3, EntityKind::PowerUp));
  mirror.spawn(spawnPacket(4, EntityKind::Obstacle));

  EXPECT_EQ(world.get<Sprite>(entityOf(mirror, 1)).texture,
            FakeEntityFactory::textureOf(EntityKind::PlayerMissile, 0));
  EXPECT_EQ(world.get<Sprite>(entityOf(mirror, 2)).texture,
            FakeEntityFactory::textureOf(EntityKind::EnemyMissile, 0));
  EXPECT_EQ(world.get<Sprite>(entityOf(mirror, 3)).texture,
            FakeEntityFactory::textureOf(EntityKind::PowerUp, 0));
  EXPECT_EQ(world.get<Sprite>(entityOf(mirror, 4)).texture,
            FakeEntityFactory::textureOf(EntityKind::Obstacle, 0));
}

TEST(WorldMirror, GivesEachShipTheSpriteOfItsOwner) {
  World world;
  FakeEntityFactory factory(world);
  WorldMirror mirror(world, factory);

  for (std::uint8_t owner = 0; owner < rtype::protocol::MAX_PLAYERS; ++owner) {
    mirror.spawn(spawnPacket(owner + 1U, EntityKind::Ship, owner));
  }

  for (std::uint8_t owner = 0; owner < rtype::protocol::MAX_PLAYERS; ++owner) {
    EXPECT_EQ(world.get<Sprite>(entityOf(mirror, owner + 1U)).texture, owner);
  }
}

TEST(WorldMirror, IgnoresASpawnItAlreadyHas) {
  World world;
  FakeEntityFactory factory(world);
  WorldMirror mirror(world, factory);
  mirror.spawn(spawnPacket(7, EntityKind::Enemy));

  EntitySpawn again = spawnPacket(7, EntityKind::Enemy);
  again.x = 500.0F;
  mirror.spawn(again);

  EXPECT_EQ(mirror.size(), 1U);
  EXPECT_FLOAT_EQ(positionX(world, mirror, 7), 100.0F);
}

TEST(WorldMirror, SnapshotMovesKnownEntities) {
  World world;
  FakeEntityFactory factory(world);
  WorldMirror mirror(world, factory);
  mirror.spawn(spawnPacket(7, EntityKind::Enemy));

  mirror.applySnapshot(WorldSnapshot{
      .serverTick = 10,
      .entities =
          {
              EntityState{
                  .entityId = 7,
                  .x = 300.0F,
                  .y = 400.0F,
                  .velocityX = 0.0F,
                  .velocityY = 25.0F,
              },
          },
  });

  const Entity entity = entityOf(mirror, 7);
  EXPECT_FLOAT_EQ(world.get<Transform>(entity).position.x, 300.0F);
  EXPECT_FLOAT_EQ(world.get<Transform>(entity).position.y, 400.0F);
  EXPECT_FLOAT_EQ(world.get<Velocity>(entity).value.x, 0.0F);
  EXPECT_FLOAT_EQ(world.get<Velocity>(entity).value.y, 25.0F);
}

TEST(WorldMirror, SnapshotSmoothsASmallError) {
  World world;
  FakeEntityFactory factory(world);
  WorldMirror mirror(world, factory);
  mirror.spawn(spawnPacket(7, EntityKind::Enemy));

  mirror.applySnapshot(WorldSnapshot{
      .serverTick = 10,
      .entities = {EntityState{.entityId = 7, .x = 110.0F, .y = 195.0F}},
  });

  const Entity entity = entityOf(mirror, 7);
  EXPECT_FLOAT_EQ(world.get<Transform>(entity).position.x, 100.0F);
  EXPECT_FLOAT_EQ(world.get<Correction>(entity).offset.x, 10.0F);
  EXPECT_FLOAT_EQ(world.get<Correction>(entity).offset.y, -5.0F);
}

TEST(WorldMirror, PredictsOnlyOurOwnShip) {
  World world;
  FakeEntityFactory factory(world);
  WorldMirror mirror(world, factory);
  mirror.setLocalPlayer(2);

  mirror.spawn(spawnPacket(1, EntityKind::Ship, 2));
  mirror.spawn(spawnPacket(2, EntityKind::Ship, 3));

  const Entity ours = entityOf(mirror, 1);
  const Entity theirs = entityOf(mirror, 2);
  EXPECT_TRUE(world.has<Predicted>(ours));
  EXPECT_FALSE(world.has<Velocity>(ours));
  EXPECT_FALSE(world.has<Predicted>(theirs));
  EXPECT_TRUE(world.has<Velocity>(theirs));
}

// Each room gives its own player id: in the next room, our old id is someone
// else's ship.
TEST(WorldMirror, PredictsTheShipOfOurIdInTheNewRoom) {
  World world;
  FakeEntityFactory factory(world);
  WorldMirror mirror(world, factory);
  mirror.setLocalPlayer(2);
  mirror.spawn(spawnPacket(1, EntityKind::Ship, 2));

  mirror.clear();
  mirror.setLocalPlayer(0);
  mirror.spawn(spawnPacket(1, EntityKind::Ship, 2));
  mirror.spawn(spawnPacket(2, EntityKind::Ship, 0));

  EXPECT_FALSE(world.has<Predicted>(entityOf(mirror, 1)));
  EXPECT_TRUE(world.has<Predicted>(entityOf(mirror, 2)));
}

TEST(WorldMirror, PredictsNoShipOutOfARoom) {
  World world;
  FakeEntityFactory factory(world);
  WorldMirror mirror(world, factory);
  mirror.setLocalPlayer(std::nullopt);

  mirror.spawn(spawnPacket(1, EntityKind::Ship, 0));

  EXPECT_FALSE(world.has<Predicted>(entityOf(mirror, 1)));
}

TEST(WorldMirror, SnapshotOnlyRemembersWhereTheServerHasOurShip) {
  World world;
  FakeEntityFactory factory(world);
  WorldMirror mirror(world, factory);
  mirror.setLocalPlayer(2);
  mirror.spawn(spawnPacket(1, EntityKind::Ship, 2));

  mirror.applySnapshot(WorldSnapshot{
      .serverTick = 10,
      .entities = {EntityState{
          .entityId = 1, .x = 900.0F, .y = 300.0F, .velocityX = 450.0F}},
  });

  const Entity ship = entityOf(mirror, 1);
  EXPECT_FLOAT_EQ(world.get<Transform>(ship).position.x, 100.0F);
  EXPECT_FLOAT_EQ(world.get<Predicted>(ship).serverPosition.x, 900.0F);
  EXPECT_FLOAT_EQ(world.get<Predicted>(ship).serverVelocity.x, 450.0F);
}

TEST(WorldMirror, SnapshotSkipsUnknownEntities) {
  World world;
  FakeEntityFactory factory(world);
  WorldMirror mirror(world, factory);
  mirror.spawn(spawnPacket(7, EntityKind::Enemy));

  mirror.applySnapshot(WorldSnapshot{
      .serverTick = 10,
      .entities =
          {
              EntityState{.entityId = 99, .x = 1.0F},
              EntityState{.entityId = 7, .x = 300.0F},
          },
  });

  EXPECT_EQ(mirror.size(), 1U);
  EXPECT_FALSE(mirror.find(99).has_value());
  EXPECT_FLOAT_EQ(positionX(world, mirror, 7), 300.0F);
}

TEST(WorldMirror, IgnoresASnapshotOlderThanTheLastOne) {
  World world;
  FakeEntityFactory factory(world);
  WorldMirror mirror(world, factory);
  mirror.spawn(spawnPacket(7, EntityKind::Enemy));

  mirror.applySnapshot(WorldSnapshot{
      .serverTick = 10,
      .entities = {EntityState{.entityId = 7, .x = 300.0F}},
  });
  mirror.applySnapshot(WorldSnapshot{
      .serverTick = 9,
      .entities = {EntityState{.entityId = 7, .x = 999.0F}},
  });

  EXPECT_FLOAT_EQ(positionX(world, mirror, 7), 300.0F);
}

// Big snapshots are split into several packets with the same tick.
TEST(WorldMirror, AcceptsSeveralSnapshotsWithTheSameTick) {
  World world;
  FakeEntityFactory factory(world);
  WorldMirror mirror(world, factory);
  mirror.spawn(spawnPacket(1, EntityKind::Enemy));
  mirror.spawn(spawnPacket(2, EntityKind::Enemy));

  mirror.applySnapshot(WorldSnapshot{
      .serverTick = 10,
      .entities = {EntityState{.entityId = 1, .x = 300.0F}},
  });
  mirror.applySnapshot(WorldSnapshot{
      .serverTick = 10,
      .entities = {EntityState{.entityId = 2, .x = 600.0F}},
  });

  EXPECT_FLOAT_EQ(positionX(world, mirror, 1), 300.0F);
  EXPECT_FLOAT_EQ(positionX(world, mirror, 2), 600.0F);
}

TEST(WorldMirror, DestroyRemovesTheEntity) {
  World world;
  FakeEntityFactory factory(world);
  WorldMirror mirror(world, factory);
  mirror.spawn(spawnPacket(7, EntityKind::Enemy));
  const Entity entity = entityOf(mirror, 7);

  mirror.destroy(EntityDestroy{.entityId = 7, .reason = DeathReason::Killed});

  EXPECT_EQ(mirror.size(), 0U);
  EXPECT_FALSE(mirror.find(7).has_value());
  EXPECT_FALSE(world.alive(entity));
}

TEST(WorldMirror, DestroyOfAnUnknownIdChangesNothing) {
  World world;
  FakeEntityFactory factory(world);
  WorldMirror mirror(world, factory);
  mirror.spawn(spawnPacket(7, EntityKind::Enemy));

  mirror.destroy(
      EntityDestroy{.entityId = 99, .reason = DeathReason::OutOfScreen});

  EXPECT_EQ(mirror.size(), 1U);
  EXPECT_TRUE(mirror.find(7).has_value());
}

// Reliable packets can arrive out of order: the destroy may come first.
TEST(WorldMirror, DropsASpawnThatArrivesAfterItsDestroy) {
  World world;
  FakeEntityFactory factory(world);
  WorldMirror mirror(world, factory);

  mirror.destroy(EntityDestroy{.entityId = 7, .reason = DeathReason::Killed});
  mirror.spawn(spawnPacket(7, EntityKind::Enemy));

  EXPECT_EQ(mirror.size(), 0U);
  EXPECT_FALSE(mirror.find(7).has_value());
}

TEST(WorldMirror, KilledEnemyDiesOnTheCenterOfItsHitbox) {
  World world;
  FakeEntityFactory factory(world);
  WorldMirror mirror(world, factory);
  mirror.spawn(spawnPacket(7, EntityKind::Enemy));

  mirror.destroy(EntityDestroy{.entityId = 7, .reason = DeathReason::Killed});

  const auto deaths = deathsOf(world, EntityKind::Enemy);
  ASSERT_EQ(deaths.size(), 1U);
  const auto& position = world.get<Transform>(deaths.front()).position;
  EXPECT_FLOAT_EQ(position.x, 130.0F);
  EXPECT_FLOAT_EQ(position.y, 230.0F);
}

TEST(WorldMirror, EntitiesLeavingTheScreenHaveNoEffect) {
  World world;
  FakeEntityFactory factory(world);
  WorldMirror mirror(world, factory);
  mirror.spawn(spawnPacket(1, EntityKind::PlayerMissile));
  mirror.spawn(spawnPacket(2, EntityKind::Enemy));

  mirror.destroy(
      EntityDestroy{.entityId = 1, .reason = DeathReason::OutOfScreen});
  mirror.destroy(
      EntityDestroy{.entityId = 2, .reason = DeathReason::OutOfScreen});

  EXPECT_TRUE(deathsOf(world, EntityKind::PlayerMissile).empty());
  EXPECT_TRUE(deathsOf(world, EntityKind::Enemy).empty());
  EXPECT_EQ(mirror.size(), 0U);
}

// A full World refuses new entities: the spawn is dropped, nothing crashes.
TEST(WorldMirror, DropsASpawnWhenTheWorldIsFull) {
  World world(1);
  FakeEntityFactory factory(world);
  WorldMirror mirror(world, factory);
  mirror.spawn(spawnPacket(1, EntityKind::Enemy));

  EXPECT_NO_THROW(mirror.spawn(spawnPacket(2, EntityKind::Enemy)));

  EXPECT_EQ(mirror.size(), 1U);
  EXPECT_FALSE(mirror.find(2).has_value());
}

TEST(WorldMirror, KilledEnemyInAFullWorldJustDisappears) {
  World world(1);
  FakeEntityFactory factory(world);
  WorldMirror mirror(world, factory);
  mirror.spawn(spawnPacket(7, EntityKind::Enemy));

  EXPECT_NO_THROW(mirror.destroy(
      EntityDestroy{.entityId = 7, .reason = DeathReason::Killed}));

  EXPECT_TRUE(deathsOf(world, EntityKind::Enemy).empty());
  EXPECT_EQ(mirror.size(), 0U);
}

// Each room numbers its entities and ticks from the start: after clear(), the
// ids and ticks of the old room do not get in the way.
TEST(WorldMirror, ClearForgetsTheOldRoom) {
  World world;
  FakeEntityFactory factory(world);
  WorldMirror mirror(world, factory);
  mirror.spawn(spawnPacket(1, EntityKind::Enemy));
  const Entity old = entityOf(mirror, 1);
  mirror.destroy(EntityDestroy{.entityId = 2, .reason = DeathReason::Killed});
  mirror.applySnapshot(WorldSnapshot{.serverTick = 500, .entities = {}});

  mirror.clear();

  EXPECT_EQ(mirror.size(), 0U);
  EXPECT_FALSE(world.alive(old));
  mirror.spawn(spawnPacket(2, EntityKind::Enemy));
  ASSERT_TRUE(mirror.find(2).has_value());
  mirror.applySnapshot(WorldSnapshot{
      .serverTick = 3,
      .entities = {EntityState{.entityId = 2, .x = 300.0F}},
  });
  EXPECT_FLOAT_EQ(positionX(world, mirror, 2), 300.0F);
}
