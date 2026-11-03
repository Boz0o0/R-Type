#include <ampersand/core/components/Transform.hpp>
#include <ampersand/core/ecs/Entity.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <ampersand/core/math/Vec2.hpp>
#include <ampersand/core/resources/Clock.hpp>
#include <ampersand/input/resources/InputManager.hpp>
#include <rtype/client/Controls.hpp>
#include <rtype/client/components/Correction.hpp>
#include <rtype/client/components/Predicted.hpp>
#include <rtype/client/systems/PredictionSystem.hpp>
#include <rtype/common/ShipMovement.hpp>

namespace rtype::client {

using ampersand::core::math::Vec2f;

void PredictionSystem::update(ampersand::core::ecs::World& world) {
  const float deltaTime = world.getResource<ampersand::core::Clock>().dt;
  const Vec2f velocity = common::ShipMovement::velocity(
      Controls::inputMask(world.getResource<ampersand::input::InputManager>()));
  for (const ampersand::core::ecs::Entity entity : world.owners<Predicted>()) {
    auto& position = world.get<ampersand::core::Transform>(entity).position;
    const Vec2f moved = common::ShipMovement::keepOnScreen(
        {position.x + (velocity.x * deltaTime),
         position.y + (velocity.y * deltaTime)});
    position.x = moved.x;
    position.y = moved.y;
    // While moving, the server is always a bit behind us, so its position is
    // only the truth once both have stopped.
    const Predicted& predicted = world.get<Predicted>(entity);
    if (isStill(velocity) && isStill(predicted.serverVelocity)) {
      world.get<Correction>(entity).offset = {
          predicted.serverPosition.x - position.x,
          predicted.serverPosition.y - position.y};
    }
  }
}

bool PredictionSystem::isStill(Vec2f velocity) {
  return velocity.x == 0.0F && velocity.y == 0.0F;
}

}  // namespace rtype::client
