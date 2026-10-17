#include <algorithm>
#include <ampersand/core/components/Transform.hpp>
#include <ampersand/core/ecs/Entity.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <ampersand/core/resources/Clock.hpp>
#include <rtype/common/Factories.hpp>
#include <rtype/common/components/Weapon.hpp>
#include <rtype/common/systems/ShootingSystem.hpp>

namespace rtype::common {

void ShootingSystem::update(ampersand::core::ecs::World& world) {
  const float deltaTime = world.getResource<ampersand::core::Clock>().dt;
  for (const ampersand::core::ecs::Entity entity : world.owners<Weapon>()) {
    auto& weapon = world.get<Weapon>(entity);
    weapon.remaining -= deltaTime;
    if (!weapon.firing || !world.has<ampersand::core::Transform>(entity)) {
      weapon.remaining = std::max(weapon.remaining, 0.0F);
    } else if (weapon.remaining <= 0.0F) {
      weapon.remaining = std::max(weapon.remaining + weapon.cooldown, 0.0F);
      createMissile(world, weapon.missile, entity);
    }
  }
}

}  // namespace rtype::common
