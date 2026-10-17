#include <gtest/gtest.h>

#include <ampersand/core/components/Transform.hpp>
#include <ampersand/core/ecs/Entity.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <ampersand/physics/components/Velocity.hpp>
#include <ampersand/render/components/Sprite.hpp>
#include <cstdint>
#include <rtype/client/WorldMirror.hpp>
#include <rtype/common/components/Kind.hpp>
#include <rtype/common/protocol/Packets.hpp>
#include <rtype/common/protocol/Protocol.hpp>
#include <rtype/common/resources/Deaths.hpp>
#include <stdexcept>

using ampersand::core::Transform;
using ampersand::core::ecs::Entity;
using ampersand::core::ecs::World;
using ampersand::physics::Velocity;
using ampersand::render::Sprite;
using rtype::client::EntitySprites;
using rtype::client::WorldMirror;
using rtype::common::DeathReason;
using rtype::common::EntityKind;
using rtype::protocol::EntityDestroy;
using rtype::protocol::EntitySpawn;
using rtype::protocol::EntityState;
using rtype::protocol::WorldSnapshot;

// Each sprite gets its own texture id so the tests can tell them apart.
static EntitySprites testSprites() {
  EntitySprites sprites;
  for (std::uint32_t player = 0; player < rtype::protocol::MAX_PLAYERS;
       ++player) {
    sprites.ships[player].texture = player;
  }
  sprites.enemy.texture = 10;
  sprites.playerMissile.texture = 11;
  sprites.enemyMissile.texture = 12;
  sprites.powerUp.texture = 13;
  sprites.obstacle.texture = 14;
  sprites.scale = 3.0F;
  return sprites;
}

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

static float positionX(World& world, const WorldMirror& mirror,
                       std::uint32_t entityId) {
  return world.get<Transform>(entityOf(mirror, entityId)).position.x;
}

TEST(WorldMirror, SpawnsTheEntityWhereTheServerSays) {
  World world;
  WorldMirror mirror(world, testSprites());

  mirror.spawn(spawnPacket(7, EntityKind::Enemy));

  EXPECT_EQ(mirror.size(), 1U);
  const Entity entity = entityOf(mirror, 7);
  const auto& transform = world.get<Transform>(entity);
  EXPECT_FLOAT_EQ(transform.position.x, 100.0F);
  EXPECT_FLOAT_EQ(transform.position.y, 200.0F);
  EXPECT_FLOAT_EQ(transform.scale.x, 3.0F);
  EXPECT_FLOAT_EQ(transform.scale.y, 3.0F);
  EXPECT_FLOAT_EQ(world.get<Velocity>(entity).value.x, -50.0F);
  EXPECT_EQ(world.get<Sprite>(entity).texture, 10U);
}

TEST(WorldMirror, PicksTheSpriteOfEachKind) {
  World world;
  WorldMirror mirror(world, testSprites());

  mirror.spawn(spawnPacket(1, EntityKind::PlayerMissile));
  mirror.spawn(spawnPacket(2, EntityKind::EnemyMissile));
  mirror.spawn(spawnPacket(3, EntityKind::PowerUp));
  mirror.spawn(spawnPacket(4, EntityKind::Obstacle));

  EXPECT_EQ(world.get<Sprite>(entityOf(mirror, 1)).texture, 11U);
  EXPECT_EQ(world.get<Sprite>(entityOf(mirror, 2)).texture, 12U);
  EXPECT_EQ(world.get<Sprite>(entityOf(mirror, 3)).texture, 13U);
  EXPECT_EQ(world.get<Sprite>(entityOf(mirror, 4)).texture, 14U);
}

TEST(WorldMirror, GivesEachShipTheSpriteOfItsOwner) {
  World world;
  WorldMirror mirror(world, testSprites());

  for (std::uint8_t owner = 0; owner < rtype::protocol::MAX_PLAYERS; ++owner) {
    mirror.spawn(spawnPacket(owner + 1U, EntityKind::Ship, owner));
  }

  for (std::uint8_t owner = 0; owner < rtype::protocol::MAX_PLAYERS; ++owner) {
    EXPECT_EQ(world.get<Sprite>(entityOf(mirror, owner + 1U)).texture, owner);
  }
}

TEST(WorldMirror, IgnoresASpawnItAlreadyHas) {
  World world;
  WorldMirror mirror(world, testSprites());
  mirror.spawn(spawnPacket(7, EntityKind::Enemy));

  EntitySpawn again = spawnPacket(7, EntityKind::Enemy);
  again.x = 500.0F;
  mirror.spawn(again);

  EXPECT_EQ(mirror.size(), 1U);
  EXPECT_FLOAT_EQ(positionX(world, mirror, 7), 100.0F);
}

TEST(WorldMirror, SnapshotMovesKnownEntities) {
  World world;
  WorldMirror mirror(world, testSprites());
  mirror.spawn(spawnPacket(7, EntityKind::Enemy));

  mirror.applySnapshot(WorldSnapshot{
      .serverTick = 10,
      .entities = {EntityState{
          .entityId = 7,
          .x = 300.0F,
          .y = 400.0F,
          .velocityX = 0.0F,
          .velocityY = 25.0F,
      }},
  });

  const Entity entity = entityOf(mirror, 7);
  EXPECT_FLOAT_EQ(world.get<Transform>(entity).position.x, 300.0F);
  EXPECT_FLOAT_EQ(world.get<Transform>(entity).position.y, 400.0F);
  EXPECT_FLOAT_EQ(world.get<Velocity>(entity).value.x, 0.0F);
  EXPECT_FLOAT_EQ(world.get<Velocity>(entity).value.y, 25.0F);
}

TEST(WorldMirror, SnapshotSkipsUnknownEntities) {
  World world;
  WorldMirror mirror(world, testSprites());
  mirror.spawn(spawnPacket(7, EntityKind::Enemy));

  mirror.applySnapshot(WorldSnapshot{
      .serverTick = 10,
      .entities = {EntityState{.entityId = 99, .x = 1.0F},
                   EntityState{.entityId = 7, .x = 300.0F}},
  });

  EXPECT_EQ(mirror.size(), 1U);
  EXPECT_FALSE(mirror.find(99).has_value());
  EXPECT_FLOAT_EQ(positionX(world, mirror, 7), 300.0F);
}

TEST(WorldMirror, IgnoresASnapshotOlderThanTheLastOne) {
  World world;
  WorldMirror mirror(world, testSprites());
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
  WorldMirror mirror(world, testSprites());
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
  WorldMirror mirror(world, testSprites());
  mirror.spawn(spawnPacket(7, EntityKind::Enemy));
  const Entity entity = entityOf(mirror, 7);

  mirror.destroy(EntityDestroy{.entityId = 7, .reason = DeathReason::Killed});

  EXPECT_EQ(mirror.size(), 0U);
  EXPECT_FALSE(mirror.find(7).has_value());
  EXPECT_FALSE(world.alive(entity));
}

TEST(WorldMirror, DestroyOfAnUnknownIdChangesNothing) {
  World world;
  WorldMirror mirror(world, testSprites());
  mirror.spawn(spawnPacket(7, EntityKind::Enemy));

  mirror.destroy(
      EntityDestroy{.entityId = 99, .reason = DeathReason::OutOfScreen});

  EXPECT_EQ(mirror.size(), 1U);
  EXPECT_TRUE(mirror.find(7).has_value());
}

// Reliable packets can arrive out of order: the destroy may come first.
TEST(WorldMirror, DropsASpawnThatArrivesAfterItsDestroy) {
  World world;
  WorldMirror mirror(world, testSprites());

  mirror.destroy(EntityDestroy{.entityId = 7, .reason = DeathReason::Killed});
  mirror.spawn(spawnPacket(7, EntityKind::Enemy));

  EXPECT_EQ(mirror.size(), 0U);
  EXPECT_FALSE(mirror.find(7).has_value());
}

// A full World refuses new entities: the spawn is dropped, nothing crashes.
TEST(WorldMirror, DropsASpawnWhenTheWorldIsFull) {
  World world(1);
  WorldMirror mirror(world, testSprites());
  mirror.spawn(spawnPacket(1, EntityKind::Enemy));

  EXPECT_NO_THROW(mirror.spawn(spawnPacket(2, EntityKind::Enemy)));

  EXPECT_EQ(mirror.size(), 1U);
  EXPECT_FALSE(mirror.find(2).has_value());
}
