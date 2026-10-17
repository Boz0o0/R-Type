#include <ampersand/core/components/Transform.hpp>
#include <ampersand/core/ecs/Entity.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <ampersand/core/math/Vec3.hpp>
#include <ampersand/physics/components/BoxCollider.hpp>
#include <ampersand/physics/components/Velocity.hpp>
#include <rtype/common/Factories.hpp>
#include <rtype/common/components/Damage.hpp>
#include <rtype/common/components/Kind.hpp>
#include <rtype/common/components/Projectile.hpp>
#include <rtype/common/components/Team.hpp>
#include <stdexcept>

namespace rtype::common {

using ampersand::core::ecs::Entity;
using ampersand::core::ecs::World;
using ampersand::physics::BoxCollider;
using ampersand::physics::Velocity;

Entity createMissile(World& world, EntityKind kind, Entity shooter) {
  if (kind != EntityKind::PlayerMissile && kind != EntityKind::EnemyMissile) {
    throw std::invalid_argument("createMissile: not a missile kind");
  }
  const ampersand::core::math::Vec3f position =
      world.get<ampersand::core::Transform>(shooter).position;
  const Entity missile = world.create();
  world.add(missile, ampersand::core::Transform{.position = position});
  world.add(missile, Kind{.kind = kind});
  world.add(missile, Projectile{});
  if (world.has<Team>(shooter)) {
    world.add(missile, world.get<Team>(shooter));
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
  return missile;
}

}  // namespace rtype::common
