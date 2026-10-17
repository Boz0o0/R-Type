#include <ampersand/core/ecs/World.hpp>
#include <rtype/common/resources/Deaths.hpp>
#include <rtype/common/resources/Spawns.hpp>
#include <rtype/common/systems/CleanupSystem.hpp>

namespace rtype::common {

void CleanupSystem::update(ampersand::core::ecs::World& world) {
  auto& deaths = world.getResource<Deaths>();
  for (const Death& death : deaths.entities) {
    if (world.alive(death.entity)) {
      world.destroy(death.entity);
    }
  }
  deaths.entities.clear();
  world.getResource<Spawns>().entities.clear();
}

}  // namespace rtype::common
