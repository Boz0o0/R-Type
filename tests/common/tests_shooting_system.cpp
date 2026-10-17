#include <gtest/gtest.h>

#include <ampersand/core/components/Transform.hpp>
#include <ampersand/core/ecs/Entity.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <cstddef>
#include <rtype/common/components/Kind.hpp>
#include <rtype/common/components/Team.hpp>
#include <rtype/common/components/Weapon.hpp>
#include <rtype/common/resources/Spawns.hpp>
#include <rtype/common/systems/ShootingSystem.hpp>

#include "GameplayTestHelpers.hpp"

using ampersand::core::ecs::Entity;
using ampersand::core::ecs::World;
using namespace rtype::common;
using rtype::common::test::addGameplayResources;

namespace {

/** @brief 1/8 s: exact in a float, so the shots land on exact ticks. */
constexpr float TICK = 0.125F;

Entity gunner(World& world, bool firing) {
  const Entity entity = world.create();
  world.add(entity,
            ampersand::core::Transform{.position = {100.0F, 200.0F, 0.0F}});
  world.add(entity, Team{.side = Side::Players});
  world.add(entity, Weapon{.cooldown = 0.25F,
                           .firing = firing,
                           .missile = EntityKind::PlayerMissile});
  return entity;
}

/** @brief Runs the ShootingSystem and returns how many missiles it fired. */
std::size_t shoot(World& world, int ticks) {
  auto& spawns = world.getResource<Spawns>().entities;
  spawns.clear();
  for (int tick = 0; tick < ticks; ++tick) {
    ShootingSystem{}.update(world);
  }
  return spawns.size();
}

}  // namespace

TEST(ShootingSystem, DoesNotFireWithoutTheTrigger) {
  World world;
  addGameplayResources(world, TICK);
  gunner(world, false);

  EXPECT_EQ(shoot(world, 16), 0U);
}

TEST(ShootingSystem, FiresAtOnceThenEveryCooldown) {
  World world;
  addGameplayResources(world, TICK);
  gunner(world, true);

  EXPECT_EQ(shoot(world, 1), 1U);  // tick 1: at once
  EXPECT_EQ(shoot(world, 1), 0U);  // tick 2: 0.125 s later, too early
  EXPECT_EQ(shoot(world, 1), 1U);  // tick 3: 0.25 s later
  EXPECT_EQ(shoot(world, 8), 4U);  // 1 s: 4 shots
}

TEST(ShootingSystem, FiresAtOnceWhenTheTriggerIsPressed) {
  World world;
  addGameplayResources(world, TICK);
  const Entity ship = gunner(world, false);
  shoot(world, 16);

  world.get<Weapon>(ship).firing = true;

  EXPECT_EQ(shoot(world, 1), 1U);
}

TEST(ShootingSystem, MissileStartsOnItsShooterWithItsTeam) {
  World world;
  addGameplayResources(world, TICK);
  const Entity ship = gunner(world, true);
  shoot(world, 1);

  const Entity shot = world.getResource<Spawns>().entities.at(0);
  EXPECT_EQ(world.get<Kind>(shot).kind, EntityKind::PlayerMissile);
  EXPECT_EQ(world.get<Team>(shot).side, Side::Players);
  const auto& position = world.get<ampersand::core::Transform>(shot).position;
  const auto& origin = world.get<ampersand::core::Transform>(ship).position;
  EXPECT_FLOAT_EQ(position.x, origin.x);
  EXPECT_FLOAT_EQ(position.y, origin.y);
  EXPECT_FLOAT_EQ(position.z, origin.z);
}
