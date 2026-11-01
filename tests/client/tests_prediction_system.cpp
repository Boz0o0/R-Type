#include <gtest/gtest.h>

#include <ampersand/core/components/Transform.hpp>
#include <ampersand/core/ecs/Entity.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <ampersand/core/math/Vec2.hpp>
#include <ampersand/core/resources/Clock.hpp>
#include <ampersand/input/Event.hpp>
#include <ampersand/input/resources/InputManager.hpp>
#include <optional>
#include <rtype/client/Controls.hpp>
#include <rtype/client/components/Correction.hpp>
#include <rtype/client/components/Predicted.hpp>
#include <rtype/client/systems/PredictionSystem.hpp>
#include <rtype/common/ShipMovement.hpp>

using ampersand::core::Clock;
using ampersand::core::Transform;
using ampersand::core::ecs::Entity;
using ampersand::core::ecs::World;
using ampersand::core::math::Vec2f;
using ampersand::input::InputManager;
using ampersand::input::KeyCode;
using ampersand::input::KeyPressed;
using rtype::client::Controls;
using rtype::client::Correction;
using rtype::client::Predicted;
using rtype::client::PredictionSystem;
using rtype::common::ShipMovement;

static constexpr float DELTA_TIME = 0.1F;

// A world with our ship at (300, 400), the server agreeing and still.
static Entity setUp(World& world, std::optional<KeyCode> held) {
  world.addResource<Clock>().dt = DELTA_TIME;
  auto& manager = world.addResource<InputManager>();
  Controls::bind(manager);
  manager.beginFrame();
  if (held) {
    manager.apply(KeyPressed{.key = *held});
  }
  const Entity ship = world.create();
  world.add(ship, Transform{.position = {300.0F, 400.0F, 0.0F}});
  world.add(ship, Correction{});
  world.add(ship, Predicted{.serverPosition = {300.0F, 400.0F}});
  return ship;
}

static Vec2f positionOf(World& world, Entity entity) {
  const auto& position = world.get<Transform>(entity).position;
  return {position.x, position.y};
}

TEST(PredictionSystem, MovesTheShipRightAwayFromTheInput) {
  World world;
  const Entity ship = setUp(world, KeyCode::UpArrow);

  PredictionSystem{}.update(world);

  EXPECT_FLOAT_EQ(positionOf(world, ship).x, 300.0F);
  EXPECT_FLOAT_EQ(positionOf(world, ship).y,
                  400.0F - (ShipMovement::SPEED * DELTA_TIME));
}

TEST(PredictionSystem, KeepsTheShipOnScreen) {
  World world;
  const Entity ship = setUp(world, KeyCode::LeftArrow);
  world.get<Transform>(ship).position.x = 10.0F;

  PredictionSystem{}.update(world);

  EXPECT_FLOAT_EQ(positionOf(world, ship).x, 0.0F);
}

TEST(PredictionSystem, IgnoresTheServerWhileWeMove) {
  World world;
  const Entity ship = setUp(world, KeyCode::RightArrow);
  world.get<Predicted>(ship).serverPosition = {200.0F, 400.0F};

  PredictionSystem{}.update(world);

  EXPECT_FLOAT_EQ(world.get<Correction>(ship).offset.x, 0.0F);
}

TEST(PredictionSystem, IgnoresTheServerWhileItStillMoves) {
  World world;
  const Entity ship = setUp(world, std::nullopt);
  world.get<Predicted>(ship) =
      Predicted{.serverPosition = {280.0F, 400.0F},
                .serverVelocity = {ShipMovement::SPEED, 0.0F}};

  PredictionSystem{}.update(world);

  EXPECT_FLOAT_EQ(world.get<Correction>(ship).offset.x, 0.0F);
}

TEST(PredictionSystem, SlidesToTheServerOnceBothStopped) {
  World world;
  const Entity ship = setUp(world, std::nullopt);
  world.get<Predicted>(ship).serverPosition = {310.0F, 395.0F};

  PredictionSystem{}.update(world);

  EXPECT_FLOAT_EQ(positionOf(world, ship).x, 300.0F);
  EXPECT_FLOAT_EQ(world.get<Correction>(ship).offset.x, 10.0F);
  EXPECT_FLOAT_EQ(world.get<Correction>(ship).offset.y, -5.0F);
}
