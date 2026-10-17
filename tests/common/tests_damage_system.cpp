#include <gtest/gtest.h>

#include <algorithm>
#include <ampersand/core/ecs/Entity.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <cstdint>
#include <rtype/common/components/Damage.hpp>
#include <rtype/common/components/Health.hpp>
#include <rtype/common/components/Projectile.hpp>
#include <rtype/common/components/Team.hpp>
#include <rtype/common/resources/Deaths.hpp>
#include <rtype/common/systems/DamageSystem.hpp>

#include "GameplayTestHelpers.hpp"

using ampersand::core::ecs::Entity;
using ampersand::core::ecs::World;
using namespace rtype::common;
using rtype::common::test::addGameplayResources;
using rtype::common::test::deathsOf;
using rtype::common::test::touch;

namespace {

Entity target(World& world, std::int32_t health) {
  const Entity entity = world.create();
  world.add(entity, Health{.current = health, .max = health});
  return entity;
}

Entity missile(World& world, std::int32_t damage) {
  const Entity entity = world.create();
  world.add(entity, Damage{.amount = damage});
  world.add(entity, Projectile{});
  return entity;
}

}  // namespace

TEST(DamageSystem, MissileRemovesItsDamageFromTheHealth) {
  World world;
  addGameplayResources(world);
  const Entity ship = target(world, 3);
  const Entity shot = missile(world, 1);
  touch(world, ship, shot);  // the order of a pair does not matter

  DamageSystem{}.update(world);

  EXPECT_EQ(world.get<Health>(ship).current, 2);
  EXPECT_EQ(deathsOf(world, ship), 0);
  EXPECT_TRUE(world.alive(ship));
}

TEST(DamageSystem, EntityDiesWhenItsHealthReachesZero) {
  World world;
  addGameplayResources(world);
  const Entity enemy = target(world, 1);
  touch(world, missile(world, 5), enemy);

  DamageSystem{}.update(world);

  ASSERT_EQ(deathsOf(world, enemy), 1);
  EXPECT_TRUE(world.alive(enemy));  // destroyed later, by the CleanupSystem
}

TEST(DamageSystem, MissileDiesOnItsFirstHitOnly) {
  World world;
  addGameplayResources(world);
  const Entity shot = missile(world, 1);
  const Entity first = target(world, 3);
  const Entity second = target(world, 3);
  touch(world, shot, first);
  touch(world, shot, second);

  DamageSystem{}.update(world);

  EXPECT_EQ(world.get<Health>(first).current, 2);
  EXPECT_EQ(world.get<Health>(second).current, 3);
  EXPECT_EQ(deathsOf(world, shot), 1);
  for (const Death& death : world.getResource<Deaths>().entities) {
    EXPECT_EQ(death.reason, DeathReason::Killed);
  }
}

TEST(DamageSystem, TwoMissilesKillTheTargetOnce) {
  World world;
  addGameplayResources(world);
  const Entity enemy = target(world, 1);
  const Entity firstShot = missile(world, 1);
  const Entity secondShot = missile(world, 1);
  touch(world, firstShot, enemy);
  touch(world, secondShot, enemy);

  DamageSystem{}.update(world);

  EXPECT_EQ(deathsOf(world, enemy), 1);
  EXPECT_EQ(deathsOf(world, firstShot), 1);
  EXPECT_EQ(deathsOf(world, secondShot), 1);
}

TEST(DamageSystem, SameSideDoesNotHurt) {
  World world;
  addGameplayResources(world);
  const Entity ship = target(world, 3);
  const Entity shot = missile(world, 1);
  world.add(ship, Team{.side = Side::Players});
  world.add(shot, Team{.side = Side::Players});
  touch(world, shot, ship);

  DamageSystem{}.update(world);

  EXPECT_EQ(world.get<Health>(ship).current, 3);
  EXPECT_EQ(deathsOf(world, shot), 0);  // goes through its own ships
}

TEST(DamageSystem, OtherSideAndNoTeamHurt) {
  World world;
  addGameplayResources(world);
  const Entity ship = target(world, 3);
  world.add(ship, Team{.side = Side::Players});
  const Entity enemyShot = missile(world, 1);
  world.add(enemyShot, Team{.side = Side::Enemies});
  touch(world, enemyShot, ship);
  touch(world, missile(world, 1), ship);  // no Team: hurts every side

  DamageSystem{}.update(world);

  EXPECT_EQ(world.get<Health>(ship).current, 1);
}

TEST(DamageSystem, ContactDamageWithoutProjectileDoesNotDie) {
  World world;
  addGameplayResources(world);
  const Entity ship = target(world, 3);
  const Entity enemy = target(world, 2);
  world.add(enemy, Damage{.amount = 1});
  touch(world, enemy, ship);

  DamageSystem{}.update(world);

  EXPECT_EQ(world.get<Health>(ship).current, 2);
  EXPECT_EQ(world.get<Health>(enemy).current, 2);  // the ship has no Damage
  EXPECT_EQ(deathsOf(world, enemy), 0);
  EXPECT_TRUE(world.has<Damage>(enemy));
}

TEST(DamageSystem, RemembersWhatDealtTheLastHit) {
  World world;
  addGameplayResources(world);
  const Entity enemy = target(world, 2);
  const Entity firstShot = missile(world, 1);
  const Entity lastShot = missile(world, 1);
  touch(world, firstShot, enemy);
  touch(world, enemy, lastShot);

  DamageSystem{}.update(world);

  const auto& deaths = world.getResource<Deaths>().entities;
  const auto death = std::ranges::find_if(
      deaths, [enemy](const Death& entry) { return entry.entity == enemy; });
  ASSERT_NE(death, deaths.end());
  ASSERT_TRUE(death->killer.has_value());
  EXPECT_EQ(*death->killer, lastShot);
}
