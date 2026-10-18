#include <ampersand/core/components/Transform.hpp>
#include <ampersand/core/ecs/Entity.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <rtype/client/components/Scrolling.hpp>
#include <rtype/client/systems/ScrollingSystem.hpp>

namespace rtype::client {

void ScrollingSystem::update(ampersand::core::ecs::World& world) {
  for (const ampersand::core::ecs::Entity entity : world.owners<Scrolling>()) {
    if (!world.has<ampersand::core::Transform>(entity)) {
      continue;
    }
    const float width = world.get<Scrolling>(entity).width;
    auto& position = world.get<ampersand::core::Transform>(entity).position;
    if (position.x <= -width) {
      position.x += COPIES * width;
    }
  }
}

}  // namespace rtype::client
