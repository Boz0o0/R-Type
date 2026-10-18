#include <ampersand/core/ecs/Entity.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <ampersand/physics/resources/Collisions.hpp>
#include <rtype/common/components/Damage.hpp>
#include <rtype/common/components/Health.hpp>
#include <rtype/common/components/Projectile.hpp>
#include <rtype/common/components/Team.hpp>
#include <rtype/common/resources/Deaths.hpp>
#include <rtype/common/systems/DamageSystem.hpp>

namespace rtype::common {

using ampersand::core::ecs::Entity;
using ampersand::core::ecs::World;

void DamageSystem::update(World& world) {
  auto& deaths = world.getResource<Deaths>();
  const auto& collisions = world.getResource<ampersand::physics::Collisions>();
  for (const auto& [first, second] : collisions.pairs) {
    hit(world, deaths, first, second);
    hit(world, deaths, second, first);
  }
}

void DamageSystem::hit(World& world, Deaths& deaths, Entity attacker,
                       Entity target) {
  if (!world.has<Damage>(attacker) || !world.has<Health>(target) ||
      isSameSide(world, attacker, target)) {
    return;
  }
  auto& health = world.get<Health>(target);
  if (health.current > 0) {
    health.current -= world.get<Damage>(attacker).amount;
    if (health.current <= 0) {
      deaths.entities.push_back({
          .entity = target,
          .reason = DeathReason::Killed,
          .killer = attacker,
      });
    }
  }
  if (world.has<Projectile>(attacker)) {
    world.remove<Damage>(attacker);
    deaths.entities.push_back(
        {.entity = attacker, .reason = DeathReason::Killed});
  }
}

bool DamageSystem::isSameSide(World& world, Entity first, Entity second) {
  return world.has<Team>(first) && world.has<Team>(second) &&
         world.get<Team>(first).side == world.get<Team>(second).side;
}

}  // namespace rtype::common
