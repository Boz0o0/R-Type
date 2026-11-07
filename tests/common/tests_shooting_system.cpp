#include <gtest/gtest.h>

#include <ampersand/core/components/Transform.hpp>
#include <ampersand/core/ecs/Entity.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <ampersand/scene/resources/SceneManager.hpp>
#include <cstddef>
#include <rtype/common/components/Kind.hpp>
#include <rtype/common/components/Projectile.hpp>
#include <rtype/common/components/Team.hpp>
#include <rtype/common/components/Weapon.hpp>
#include <rtype/common/systems/ShootingSystem.hpp>

#include "GameplayTestHelpers.hpp"

using ampersand::core::ecs::Entity;
using ampersand::core::ecs::World;
using namespace rtype::common;
using ampersand::scene::SceneManager;
using rtype::common::test::addGameplayResources;
using rtype::common::test::loadPrefabs;

namespace {

/** @brief 1/8 s: exact in a float, so the shots land on exact ticks. */
constexpr float TICK = 0.125F;

Entity gunner(World& world, bool firing) {
  const Entity entity = world.create();
  world.add(entity,
            ampersand::core::Transform{.position = {100.0F, 200.0F, 0.0F}});
  world.add(entity, Team{.side = Side::Players});
  world.add(entity, Weapon{
                        .cooldown = 0.25F,
                        .firing = firing,
                        .missile = "world/prefabs/common/player-missile.json",
                    });
  return entity;
}

class ShootingSystemTest : public testing::Test {
 protected:
  void SetUp() override {
    addGameplayResources(this->world, TICK);
    loadPrefabs(this->scenes, this->world);
  }

  /** @brief Runs the ShootingSystem, returns how many missiles it fired. */
  std::size_t shoot(int ticks) {
    const std::size_t before = this->world.owners<Projectile>().size();
    for (int tick = 0; tick < ticks; ++tick) {
      this->system.update(this->world);
    }
    return this->world.owners<Projectile>().size() - before;
  }

  SceneManager scenes;
  World world;
  ShootingSystem system{scenes};
};

}  // namespace

TEST_F(ShootingSystemTest, DoesNotFireWithoutTheTrigger) {
  gunner(world, false);

  EXPECT_EQ(shoot(16), 0U);
}

TEST_F(ShootingSystemTest, FiresAtOnceThenEveryCooldown) {
  gunner(world, true);

  EXPECT_EQ(shoot(1), 1U);  // tick 1: at once
  EXPECT_EQ(shoot(1), 0U);  // tick 2: 0.125 s later, too early
  EXPECT_EQ(shoot(1), 1U);  // tick 3: 0.25 s later
  EXPECT_EQ(shoot(8), 4U);  // 1 s: 4 shots
}

TEST_F(ShootingSystemTest, FiresAtOnceWhenTheTriggerIsPressed) {
  const Entity ship = gunner(world, false);
  shoot(16);

  world.get<Weapon>(ship).firing = true;

  EXPECT_EQ(shoot(1), 1U);
}

TEST_F(ShootingSystemTest, MissileStartsOnItsShooterWithItsTeam) {
  const Entity ship = gunner(world, true);
  shoot(1);

  const Entity shot = world.owners<Projectile>().at(0);
  EXPECT_EQ(world.get<Kind>(shot).kind, EntityKind::PlayerMissile);
  EXPECT_EQ(world.get<Team>(shot).side, Side::Players);
  const auto& position = world.get<ampersand::core::Transform>(shot).position;
  const auto& origin = world.get<ampersand::core::Transform>(ship).position;
  EXPECT_FLOAT_EQ(position.x, origin.x);
  EXPECT_FLOAT_EQ(position.y, origin.y);
  EXPECT_FLOAT_EQ(position.z, origin.z);
}
