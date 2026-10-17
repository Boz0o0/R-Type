#include <gtest/gtest.h>

#include <ampersand/core/ecs/Entity.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <rtype/common/resources/Deaths.hpp>
#include <rtype/common/resources/Spawns.hpp>
#include <rtype/common/systems/CleanupSystem.hpp>

#include "GameplayTestHelpers.hpp"

using ampersand::core::ecs::Entity;
using ampersand::core::ecs::World;
using namespace rtype::common;
using rtype::common::test::addGameplayResources;

TEST(CleanupSystem, DestroysTheDeadAndEmptiesTheLists) {
  World world;
  addGameplayResources(world);
  const Entity dead = world.create();
  const Entity alive = world.create();
  world.getResource<Deaths>().entities.push_back(
      {.entity = dead, .reason = DeathReason::Killed});
  world.getResource<Spawns>().entities.push_back(alive);

  CleanupSystem{}.update(world);

  EXPECT_FALSE(world.alive(dead));
  EXPECT_TRUE(world.alive(alive));
  EXPECT_TRUE(world.getResource<Deaths>().entities.empty());
  EXPECT_TRUE(world.getResource<Spawns>().entities.empty());
}

TEST(CleanupSystem, SkipsAnEntityAlreadyDestroyed) {
  World world;
  addGameplayResources(world);
  const Entity entity = world.create();
  world.getResource<Deaths>().entities.push_back(
      {.entity = entity, .reason = DeathReason::Killed});
  world.destroy(entity);

  EXPECT_NO_THROW(CleanupSystem{}.update(world));
  EXPECT_TRUE(world.getResource<Deaths>().entities.empty());
}
