#include <algorithm>
#include <ampersand/core/components/Transform.hpp>
#include <ampersand/core/ecs/Entity.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <ampersand/core/math/Rect.hpp>
#include <ampersand/core/math/Vec2.hpp>
#include <ampersand/physics/components/BoxCollider.hpp>
#include <ampersand/physics/components/Velocity.hpp>
#include <rtype/common/components/Kind.hpp>
#include <rtype/common/resources/Deaths.hpp>
#include <rtype/common/systems/OutOfScreenSystem.hpp>

namespace rtype::common {

using ampersand::core::ecs::Entity;
using ampersand::core::math::Rect;
using ampersand::core::math::Vec2f;

OutOfScreenSystem::OutOfScreenSystem(Rect<float> screen) : _screen(screen) {}

void OutOfScreenSystem::update(ampersand::core::ecs::World& world) {
  auto& deaths = world.getResource<Deaths>();
  for (const Entity entity : world.owners<ampersand::physics::BoxCollider>()) {
    if (!world.has<ampersand::core::Transform>(entity) ||
        (world.has<Kind>(entity) &&
         world.get<Kind>(entity).kind == EntityKind::Ship)) {
      continue;
    }
    const Rect<float> box = ampersand::physics::worldBounds(
        world.get<ampersand::core::Transform>(entity),
        world.get<ampersand::physics::BoxCollider>(entity));
    const Vec2f velocity =
        world.has<ampersand::physics::Velocity>(entity)
            ? world.get<ampersand::physics::Velocity>(entity).value
            : Vec2f{};
    const bool gone =
        (box.x + box.width < this->_screen.x && velocity.x <= 0.0F) ||
        (box.x > this->_screen.x + this->_screen.width && velocity.x >= 0.0F) ||
        (box.y + box.height < this->_screen.y && velocity.y <= 0.0F) ||
        (box.y > this->_screen.y + this->_screen.height && velocity.y >= 0.0F);
    const bool alreadyDead =
        std::ranges::any_of(deaths.entities, [entity](const Death& death) {
          return death.entity == entity;
        });
    if (gone && !alreadyDead) {
      deaths.entities.push_back(
          {.entity = entity, .reason = DeathReason::OutOfScreen});
    }
  }
}

}  // namespace rtype::common
