#include <ampersand/core/components/Transform.hpp>
#include <ampersand/core/ecs/Entity.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <ampersand/core/resources/Clock.hpp>
#include <cmath>
#include <rtype/client/components/Correction.hpp>
#include <rtype/client/systems/CorrectionSystem.hpp>

namespace rtype::client {

void CorrectionSystem::update(ampersand::core::ecs::World& world) {
  const float deltaTime = world.getResource<ampersand::core::Clock>().dt;
  // The same share whatever the frame rate: 1 - e^(-t / T).
  const float share = 1.0F - std::exp(-deltaTime / SMOOTHING_TIME);
  for (const ampersand::core::ecs::Entity entity : world.owners<Correction>()) {
    if (!world.has<ampersand::core::Transform>(entity)) {
      continue;
    }
    auto& offset = world.get<Correction>(entity).offset;
    auto& position = world.get<ampersand::core::Transform>(entity).position;
    const float stepX = offset.x * share;
    const float stepY = offset.y * share;
    position.x += stepX;
    position.y += stepY;
    offset.x -= stepX;
    offset.y -= stepY;
  }
}

}  // namespace rtype::client
