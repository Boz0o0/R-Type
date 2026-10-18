#include <ampersand/core/components/Transform.hpp>
#include <ampersand/core/ecs/ECSExceptions.hpp>
#include <ampersand/core/ecs/Entity.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <ampersand/core/math/Vec2.hpp>
#include <ampersand/core/math/Vec3.hpp>
#include <ampersand/physics/components/BoxCollider.hpp>
#include <ampersand/physics/components/Velocity.hpp>
#include <optional>
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

namespace rtype::common {

using ampersand::core::ecs::Entity;
using ampersand::core::ecs::World;
using ampersand::physics::BoxCollider;
using ampersand::physics::Velocity;

std::optional<Entity> createShip(World& world,
                                 const ampersand::core::math::Vec2f& position) {
  Entity ship;
  try {
    ship = world.create();
  } catch (const ampersand::core::ecs::EntityLimitReached&) {
    return std::nullopt;
  }
  world.add(ship, ampersand::core::Transform{
                      .position = {position.x, position.y, 0.0F},
                  });
  world.add(ship, Kind{.kind = EntityKind::Ship});
  world.add(ship, Velocity{});
  world.add(ship, BoxCollider{.bounds = {0.0F, 0.0F, 64.0F, 32.0F}});
  world.add(ship, Health{.current = 3, .max = 3});
  world.add(ship, Team{.side = Side::Players});
  world.add(ship,
            Weapon{.cooldown = 0.25F, .missile = EntityKind::PlayerMissile});
  world.getResource<Spawns>().entities.push_back(ship);
  return ship;
}

std::optional<Entity> createEnemy(
    World& world, const ampersand::core::math::Vec2f& position) {
  Entity enemy;
  try {
    enemy = world.create();
  } catch (const ampersand::core::ecs::EntityLimitReached&) {
    return std::nullopt;
  }
  world.add(enemy, ampersand::core::Transform{
                       .position = {position.x, position.y, 0.0F},
                   });
  world.add(enemy, Kind{.kind = EntityKind::Enemy});
  world.add(enemy, Velocity{.value = {-200.0F, 0.0F}});
  world.add(enemy, BoxCollider{.bounds = {0.0F, 0.0F, 64.0F, 64.0F}});
  world.add(enemy, Health{.current = 2, .max = 2});
  world.add(enemy, Damage{.amount = 1});
  world.add(enemy, Team{.side = Side::Enemies});
  world.add(enemy, Weapon{
                       .cooldown = 1.5F,
                       .remaining = 1.5F,
                       .firing = true,
                       .missile = EntityKind::EnemyMissile,
                   });
  world.getResource<Spawns>().entities.push_back(enemy);
  return enemy;
}

std::optional<Entity> createMissile(World& world, EntityKind kind,
                                    Entity shooter) {
  if (kind != EntityKind::PlayerMissile && kind != EntityKind::EnemyMissile) {
    throw std::invalid_argument("createMissile: not a missile kind");
  }
  const ampersand::core::math::Vec3f position =
      world.get<ampersand::core::Transform>(shooter).position;
  Entity missile;
  try {
    missile = world.create();
  } catch (const ampersand::core::ecs::EntityLimitReached&) {
    return std::nullopt;
  }
  world.add(missile, ampersand::core::Transform{.position = position});
  world.add(missile, Kind{.kind = kind});
  world.add(missile, Projectile{});
  if (world.has<Team>(shooter)) {
    world.add(missile, world.get<Team>(shooter));
  }
  if (world.has<Owner>(shooter)) {
    world.add(missile, world.get<Owner>(shooter));
  }
  if (kind == EntityKind::PlayerMissile) {
    world.add(missile, Velocity{.value = {800.0F, 0.0F}});
    world.add(missile, Damage{.amount = 1});
    world.add(missile, BoxCollider{.bounds = {0.0F, 0.0F, 16.0F, 4.0F}});
  } else {
    world.add(missile, Velocity{.value = {-400.0F, 0.0F}});
    world.add(missile, Damage{.amount = 1});
    world.add(missile, BoxCollider{.bounds = {0.0F, 0.0F, 8.0F, 8.0F}});
  }
  world.getResource<Spawns>().entities.push_back(missile);
  return missile;
}

}  // namespace rtype::common
