#include <gtest/gtest.h>

#include <ampersand/core/components/Transform.hpp>
#include <ampersand/core/ecs/Entity.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <ampersand/core/resources/Clock.hpp>
#include <ampersand/physics/components/Velocity.hpp>
#include <ampersand/physics/systems/MovementSystem.hpp>
#include <rtype/client/components/Scrolling.hpp>
#include <rtype/client/systems/ScrollingSystem.hpp>

using ampersand::core::Clock;
using ampersand::core::Transform;
using ampersand::core::ecs::Entity;
using ampersand::core::ecs::World;
using ampersand::physics::MovementSystem;
using ampersand::physics::Velocity;
using rtype::client::Scrolling;
using rtype::client::ScrollingSystem;

static constexpr float WIDTH = 1920.0F;
static constexpr float SPEED = 90.0F;

static Entity copyAt(World& world, float positionX) {
  const Entity entity = world.create();
  world.add(entity, Transform{.position = {positionX, 0.0F, 0.0F}});
  world.add(entity, Velocity{.value = {-SPEED, 0.0F}});
  world.add(entity, Scrolling{.width = WIDTH});
  return entity;
}

// Runs the systems like the client loop does, with frames of the same length.
static float positionAfter(int frames, Clock clock) {
  World world;
  world.addResource<Clock>(clock);
  const Entity entity = copyAt(world, 0.0F);
  MovementSystem movementSystem;
  ScrollingSystem scrollingSystem;
  for (int frame = 0; frame < frames; ++frame) {
    movementSystem.update(world);
    scrollingSystem.update(world);
  }
  return world.get<Transform>(entity).position.x;
}

TEST(ScrollingSystem, MovesACopyBehindTheOtherOnceOffScreen) {
  World world;
  const Entity entity = copyAt(world, -WIDTH);

  ScrollingSystem{}.update(world);

  EXPECT_FLOAT_EQ(world.get<Transform>(entity).position.x, WIDTH);
}

TEST(ScrollingSystem, KeepsACopyThatIsStillOnScreen) {
  World world;
  const Entity entity = copyAt(world, 1.0F - WIDTH);

  ScrollingSystem{}.update(world);

  EXPECT_FLOAT_EQ(world.get<Transform>(entity).position.x, 1.0F - WIDTH);
}

TEST(ScrollingSystem, ScrollsTheSameDistanceAtAnyFrameRate) {
  EXPECT_NEAR(positionAfter(30, Clock{.dt = 1.0F / 30.0F}), -SPEED, 0.01F);
  EXPECT_NEAR(positionAfter(60, Clock{.dt = 1.0F / 60.0F}), -SPEED, 0.01F);
  EXPECT_NEAR(positionAfter(144, Clock{.dt = 1.0F / 144.0F}), -SPEED, 0.01F);
}

TEST(ScrollingSystem, KeepsLoopingOverTime) {
  // 30 s at 90 px/s is 2700 px: one jump of 2 widths, so 3840 - 2700.
  EXPECT_NEAR(positionAfter(1800, Clock{.dt = 1.0F / 60.0F}), 1140.0F, 0.5F);
}
