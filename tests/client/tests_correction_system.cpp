#include <gtest/gtest.h>

#include <ampersand/core/components/Transform.hpp>
#include <ampersand/core/ecs/Entity.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <ampersand/core/resources/Clock.hpp>
#include <rtype/client/components/Correction.hpp>
#include <rtype/client/systems/CorrectionSystem.hpp>

using ampersand::core::Clock;
using ampersand::core::Transform;
using ampersand::core::ecs::Entity;
using ampersand::core::ecs::World;
using rtype::client::Correction;
using rtype::client::CorrectionSystem;

static constexpr float FRAME = 1.0F / 60.0F;

// An entity at x = 100, offsetX away from where the server says it is.
static Entity entityWithError(World& world, float offsetX) {
  const Entity entity = world.create();
  world.add(entity, Transform{.position = {100.0F, 0.0F, 0.0F}});
  world.add(entity, Correction{.offset = {offsetX, 0.0F}});
  return entity;
}

static void runFrames(World& world, int frames) {
  world.getResource<Clock>().dt = FRAME;
  CorrectionSystem system;
  for (int frame = 0; frame < frames; ++frame) {
    system.update(world);
  }
}

TEST(CorrectionSystem, AbsorbsOnlyPartOfTheErrorEachFrame) {
  World world;
  world.addResource<Clock>();
  const Entity entity = entityWithError(world, 60.0F);

  runFrames(world, 1);

  const float positionX = world.get<Transform>(entity).position.x;
  EXPECT_GT(positionX, 100.0F);
  EXPECT_LT(positionX, 130.0F);
  EXPECT_FLOAT_EQ(positionX + world.get<Correction>(entity).offset.x, 160.0F);
}

TEST(CorrectionSystem, ReachesTheServerPositionWithoutOvershooting) {
  World world;
  world.addResource<Clock>();
  const Entity entity = entityWithError(world, 60.0F);

  runFrames(world, 60);

  EXPECT_NEAR(world.get<Transform>(entity).position.x, 160.0F, 0.01F);
  EXPECT_LE(world.get<Transform>(entity).position.x, 160.0F);
}

TEST(CorrectionSystem, DoesNothingWithoutAnError) {
  World world;
  world.addResource<Clock>();
  const Entity entity = entityWithError(world, 0.0F);

  runFrames(world, 10);

  EXPECT_FLOAT_EQ(world.get<Transform>(entity).position.x, 100.0F);
}
