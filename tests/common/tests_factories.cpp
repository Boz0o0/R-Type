#include <gtest/gtest.h>

#include <ampersand/core/components/Transform.hpp>
#include <ampersand/core/ecs/Entity.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <ampersand/physics/components/BoxCollider.hpp>
#include <ampersand/physics/components/Velocity.hpp>
#include <ampersand/scene/resources/SceneManager.hpp>
#include <rtype/common/Factories.hpp>
#include <rtype/common/ShipMovement.hpp>
#include <rtype/common/components/Damage.hpp>
#include <rtype/common/components/Health.hpp>
#include <rtype/common/components/Kind.hpp>
#include <rtype/common/components/Owner.hpp>
#include <rtype/common/components/Projectile.hpp>
#include <rtype/common/components/Team.hpp>
#include <rtype/common/components/Weapon.hpp>
#include <string>

#include "GameplayTestHelpers.hpp"

using ampersand::core::ecs::Entity;
using ampersand::core::ecs::World;
using ampersand::physics::BoxCollider;
using ampersand::physics::Velocity;
using ampersand::scene::SceneManager;
using namespace rtype::common;
using rtype::common::test::addGameplayResources;
using rtype::common::test::loadPrefabs;

namespace {

const std::string ENEMY = "world/prefabs/common/enemy.json";

class Factories : public testing::Test {
 protected:
  void SetUp() override {
    addGameplayResources(this->world);
    loadPrefabs(this->scenes, this->world);
  }

  Entity enemy(float x, float y) {
    const Entity entity = this->scenes.instantiate(this->world, ENEMY);
    this->world.get<ampersand::core::Transform>(entity).position = {x, y, 0.0F};
    return entity;
  }

  SceneManager scenes;
  World world;
};

}  // namespace

TEST_F(Factories, ShipIsAPlayerAtRestNotFiring) {
  const auto ship = createShip(world, scenes, {100.0F, 500.0F});

  ASSERT_TRUE(ship.has_value());
  EXPECT_EQ(world.get<Kind>(*ship).kind, EntityKind::Ship);
  EXPECT_EQ(world.get<Team>(*ship).side, Side::Players);
  EXPECT_GT(world.get<Health>(*ship).current, 0);
  EXPECT_FALSE(world.has<Damage>(*ship));
  EXPECT_FALSE(world.get<Weapon>(*ship).firing);
  EXPECT_EQ(world.get<Weapon>(*ship).missile,
            "world/prefabs/common/player-missile.json");
  EXPECT_FLOAT_EQ(world.get<ampersand::core::Transform>(*ship).position.x,
                  100.0F);
}

TEST_F(Factories, ShipHitboxIsTheOneTheClientPredicts) {
  const auto ship = createShip(world, scenes, {0.0F, 0.0F});

  ASSERT_TRUE(ship.has_value());
  const auto& bounds = world.get<BoxCollider>(*ship).bounds;
  EXPECT_FLOAT_EQ(bounds.width, ShipMovement::SIZE.x);
  EXPECT_FLOAT_EQ(bounds.height, ShipMovement::SIZE.y);
}

TEST_F(Factories, EnemyFliesLeftAndFires) {
  const Entity entity = this->enemy(1920.0F, 300.0F);

  EXPECT_EQ(world.get<Kind>(entity).kind, EntityKind::Enemy);
  EXPECT_EQ(world.get<Kind>(entity).variant, 0);
  EXPECT_EQ(world.get<Team>(entity).side, Side::Enemies);
  EXPECT_TRUE(world.has<Damage>(entity));
  EXPECT_LT(world.get<Velocity>(entity).value.x, 0.0F);
  EXPECT_TRUE(world.get<Weapon>(entity).firing);
}

TEST_F(Factories, SineEnemyIsAnEnemyOfVariantOne) {
  const Entity entity =
      scenes.instantiate(world, "world/prefabs/common/enemy-sine.json");

  EXPECT_EQ(world.get<Kind>(entity).kind, EntityKind::Enemy);
  EXPECT_EQ(world.get<Kind>(entity).variant, 1);
  EXPECT_EQ(world.get<Team>(entity).side, Side::Enemies);
}

TEST_F(Factories, MissilesGoTowardTheOtherSide) {
  const auto ship = createShip(world, scenes, {100.0F, 500.0F});
  const Entity foe = this->enemy(1500.0F, 500.0F);
  ASSERT_TRUE(ship.has_value());

  const auto playerShot =
      createMissile(world, scenes, world.get<Weapon>(*ship).missile, *ship);
  const auto enemyShot =
      createMissile(world, scenes, world.get<Weapon>(foe).missile, foe);

  ASSERT_TRUE(playerShot && enemyShot);
  EXPECT_EQ(world.get<Kind>(*playerShot).kind, EntityKind::PlayerMissile);
  EXPECT_TRUE(world.has<Projectile>(*playerShot));
  EXPECT_GT(world.get<Velocity>(*playerShot).value.x, 0.0F);
  EXPECT_EQ(world.get<Team>(*playerShot).side, Side::Players);
  EXPECT_EQ(world.get<Kind>(*enemyShot).kind, EntityKind::EnemyMissile);
  EXPECT_LT(world.get<Velocity>(*enemyShot).value.x, 0.0F);
  EXPECT_EQ(world.get<Team>(*enemyShot).side, Side::Enemies);
}

TEST_F(Factories, MissileStartsOnItsShooter) {
  const auto ship = createShip(world, scenes, {100.0F, 500.0F});
  ASSERT_TRUE(ship.has_value());

  const auto shot =
      createMissile(world, scenes, world.get<Weapon>(*ship).missile, *ship);

  ASSERT_TRUE(shot.has_value());
  const auto& position = world.get<ampersand::core::Transform>(*shot).position;
  EXPECT_FLOAT_EQ(position.x, 100.0F);
  EXPECT_FLOAT_EQ(position.y, 500.0F);
}

TEST_F(Factories, ReturnNothingForAPrefabThatCannotBeLoaded) {
  const auto ship = createShip(world, scenes, {0.0F, 0.0F});
  ASSERT_TRUE(ship.has_value());

  EXPECT_FALSE(createMissile(world, scenes, "world/prefabs/none.json", *ship));
}

TEST(FactoriesFull, ReturnNothingWhenTheWorldIsFull) {
  World world(2);
  SceneManager scenes;
  addGameplayResources(world);
  loadPrefabs(scenes, world);
  const auto ship = createShip(world, scenes, {0.0F, 0.0F});
  ASSERT_TRUE(ship.has_value());

  EXPECT_FALSE(createShip(world, scenes, {0.0F, 0.0F}).has_value());
  EXPECT_FALSE(
      createMissile(world, scenes, world.get<Weapon>(*ship).missile, *ship)
          .has_value());
}

TEST_F(Factories, MissileBelongsToThePlayerWhoFired) {
  const auto ship = createShip(world, scenes, {100.0F, 500.0F});
  const Entity foe = this->enemy(1500.0F, 500.0F);
  ASSERT_TRUE(ship.has_value());
  world.add(*ship, Owner{.playerId = 2});

  const auto playerShot =
      createMissile(world, scenes, world.get<Weapon>(*ship).missile, *ship);
  const auto enemyShot =
      createMissile(world, scenes, world.get<Weapon>(foe).missile, foe);

  ASSERT_TRUE(playerShot && enemyShot);
  ASSERT_TRUE(world.has<Owner>(*playerShot));
  EXPECT_EQ(world.get<Owner>(*playerShot).playerId, 2);
  EXPECT_FALSE(world.has<Owner>(*enemyShot));
}
