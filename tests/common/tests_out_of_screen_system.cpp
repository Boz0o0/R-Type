#include <gtest/gtest.h>

#include <ampersand/core/components/Transform.hpp>
#include <ampersand/core/ecs/Entity.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <ampersand/core/math/Vec2.hpp>
#include <ampersand/physics/components/BoxCollider.hpp>
#include <ampersand/physics/components/Velocity.hpp>
#include <rtype/common/components/Kind.hpp>
#include <rtype/common/resources/Deaths.hpp>
#include <rtype/common/systems/OutOfScreenSystem.hpp>

#include "GameplayTestHelpers.hpp"

using ampersand::core::ecs::Entity;
using ampersand::core::ecs::World;
using namespace rtype::common;
using rtype::common::test::addGameplayResources;
using rtype::common::test::deathsOf;

namespace {

OutOfScreenSystem screenSystem() {
  return OutOfScreenSystem({0.0F, 0.0F, 1920.0F, 1080.0F});
}

/** @brief A 16x16 entity at a position, flying horizontally. */
Entity box(World& world, ampersand::core::math::Vec2f position, float velocityX,
           EntityKind kind = EntityKind::PlayerMissile) {
  const Entity entity = world.create();
  world.add(entity, ampersand::core::Transform{
                        .position = {position.x, position.y, 0.0F},
                    });
  world.add(entity, ampersand::physics::BoxCollider{
                        .bounds = {0.0F, 0.0F, 16.0F, 16.0F},
                    });
  world.add(entity, ampersand::physics::Velocity{.value = {velocityX, 0.0F}});
  world.add(entity, Kind{.kind = kind});
  return entity;
}

}  // namespace

TEST(OutOfScreenSystem, KillsWhatLeftTheScreen) {
  World world;
  addGameplayResources(world);
  const Entity right = box(world, {1930.0F, 500.0F}, 800.0F);
  const Entity left = box(world, {-20.0F, 500.0F}, -400.0F);

  screenSystem().update(world);

  EXPECT_EQ(deathsOf(world, right), 1);
  EXPECT_EQ(deathsOf(world, left), 1);
  EXPECT_EQ(world.getResource<Deaths>().entities.at(0).reason,
            DeathReason::OutOfScreen);
}

TEST(OutOfScreenSystem, KeepsWhatIsOnScreenOrComingIn) {
  World world;
  addGameplayResources(world);
  const Entity visible = box(world, {1910.0F, 500.0F}, 800.0F);  // half in
  const Entity coming =
      box(world, {2500.0F, 500.0F}, -200.0F, EntityKind::Enemy);

  screenSystem().update(world);

  EXPECT_EQ(deathsOf(world, visible), 0);
  EXPECT_EQ(deathsOf(world, coming), 0);
}

TEST(OutOfScreenSystem, NeverKillsShips) {
  World world;
  addGameplayResources(world);
  const Entity ship = box(world, {-500.0F, 500.0F}, 0.0F, EntityKind::Ship);

  screenSystem().update(world);

  EXPECT_EQ(deathsOf(world, ship), 0);
}

TEST(OutOfScreenSystem, DoesNotKillTwice) {
  World world;
  addGameplayResources(world);
  const Entity shot = box(world, {1930.0F, 500.0F}, 800.0F);
  world.getResource<Deaths>().entities.push_back(
      {.entity = shot, .reason = DeathReason::Killed});

  screenSystem().update(world);

  EXPECT_EQ(deathsOf(world, shot), 1);
}
