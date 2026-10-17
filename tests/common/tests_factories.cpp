#include <gtest/gtest.h>

#include <ampersand/core/components/Transform.hpp>
#include <ampersand/core/ecs/Entity.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <ampersand/physics/components/Velocity.hpp>
#include <rtype/common/Factories.hpp>
#include <rtype/common/components/Damage.hpp>
#include <rtype/common/components/Health.hpp>
#include <rtype/common/components/Kind.hpp>
#include <rtype/common/components/Owner.hpp>
#include <rtype/common/components/Projectile.hpp>
#include <rtype/common/components/Team.hpp>
#include <rtype/common/components/Weapon.hpp>
#include <rtype/common/resources/Spawns.hpp>
#include <stdexcept>

#include "GameplayTestHelpers.hpp"

using ampersand::core::ecs::Entity;
using ampersand::core::ecs::World;
using ampersand::physics::Velocity;
using namespace rtype::common;
using rtype::common::test::addGameplayResources;

TEST(Factories, ShipIsAPlayerAtRestNotFiring) {
  World world;
  addGameplayResources(world);

  const auto ship = createShip(world, {100.0F, 500.0F});

  ASSERT_TRUE(ship.has_value());
  EXPECT_EQ(world.get<Kind>(*ship).kind, EntityKind::Ship);
  EXPECT_EQ(world.get<Team>(*ship).side, Side::Players);
  EXPECT_GT(world.get<Health>(*ship).current, 0);
  EXPECT_FALSE(world.has<Damage>(*ship));
  EXPECT_FALSE(world.get<Weapon>(*ship).firing);
  EXPECT_EQ(world.get<Weapon>(*ship).missile, EntityKind::PlayerMissile);
  EXPECT_FLOAT_EQ(world.get<ampersand::core::Transform>(*ship).position.x,
                  100.0F);
  EXPECT_EQ(world.getResource<Spawns>().entities.at(0), *ship);
}

TEST(Factories, EnemyFliesLeftAndFires) {
  World world;
  addGameplayResources(world);

  const auto enemy = createEnemy(world, {1920.0F, 300.0F});

  ASSERT_TRUE(enemy.has_value());
  EXPECT_EQ(world.get<Kind>(*enemy).kind, EntityKind::Enemy);
  EXPECT_EQ(world.get<Team>(*enemy).side, Side::Enemies);
  EXPECT_TRUE(world.has<Damage>(*enemy));
  EXPECT_LT(world.get<Velocity>(*enemy).value.x, 0.0F);
  EXPECT_TRUE(world.get<Weapon>(*enemy).firing);
  EXPECT_EQ(world.get<Weapon>(*enemy).missile, EntityKind::EnemyMissile);
  EXPECT_EQ(world.getResource<Spawns>().entities.at(0), *enemy);
}

TEST(Factories, MissilesGoTowardTheOtherSide) {
  World world;
  addGameplayResources(world);
  const auto ship = createShip(world, {100.0F, 500.0F});
  const auto enemy = createEnemy(world, {1500.0F, 500.0F});
  ASSERT_TRUE(ship && enemy);

  const auto playerShot =
      createMissile(world, EntityKind::PlayerMissile, *ship);
  const auto enemyShot = createMissile(world, EntityKind::EnemyMissile, *enemy);

  ASSERT_TRUE(playerShot && enemyShot);
  EXPECT_TRUE(world.has<Projectile>(*playerShot));
  EXPECT_GT(world.get<Velocity>(*playerShot).value.x, 0.0F);
  EXPECT_EQ(world.get<Team>(*playerShot).side, Side::Players);
  EXPECT_LT(world.get<Velocity>(*enemyShot).value.x, 0.0F);
  EXPECT_EQ(world.get<Team>(*enemyShot).side, Side::Enemies);
  EXPECT_EQ(world.getResource<Spawns>().entities.size(), 4U);
}

TEST(Factories, RefusesAMissileThatIsNotAMissile) {
  World world;
  addGameplayResources(world);
  const auto ship = createShip(world, {0.0F, 0.0F});
  ASSERT_TRUE(ship.has_value());

  EXPECT_THROW(createMissile(world, EntityKind::Ship, *ship),
               std::invalid_argument);
}

TEST(Factories, ReturnNothingWhenTheWorldIsFull) {
  World world(1);
  addGameplayResources(world);
  const auto ship = createShip(world, {0.0F, 0.0F});
  ASSERT_TRUE(ship.has_value());

  EXPECT_FALSE(createEnemy(world, {0.0F, 0.0F}).has_value());
  EXPECT_FALSE(
      createMissile(world, EntityKind::PlayerMissile, *ship).has_value());
  EXPECT_EQ(world.getResource<Spawns>().entities.size(), 1U);
}

TEST(Factories, MissileBelongsToThePlayerWhoFired) {
  World world;
  addGameplayResources(world);
  const auto ship = createShip(world, {100.0F, 500.0F});
  const auto enemy = createEnemy(world, {1500.0F, 500.0F});
  ASSERT_TRUE(ship && enemy);
  world.add(*ship, Owner{.playerId = 2});

  const auto playerShot =
      createMissile(world, EntityKind::PlayerMissile, *ship);
  const auto enemyShot = createMissile(world, EntityKind::EnemyMissile, *enemy);

  ASSERT_TRUE(playerShot && enemyShot);
  ASSERT_TRUE(world.has<Owner>(*playerShot));
  EXPECT_EQ(world.get<Owner>(*playerShot).playerId, 2);
  EXPECT_FALSE(world.has<Owner>(*enemyShot));
}
