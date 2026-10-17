#pragma once

#include <ampersand/core/ecs/ISystem.hpp>
#include <ampersand/core/ecs/World.hpp>

namespace rtype::common {

/**
 * @brief Fires the missiles of every Weapon whose trigger is held.
 *
 * Each shot creates the missile of the Weapon with createMissile.
 */
class ShootingSystem final : public ampersand::core::ecs::ISystem {
 public:
  /**
   * @brief Counts down every Weapon and fires those that are ready.
   *
   * Run it BEFORE the MovementSystem, so a missile moves on the tick it is
   * fired.
   *
   * @param world World holding the entities and the Clock resource.
   * @throws ResourceNotFound if the World has no Clock.
   */
  void update(ampersand::core::ecs::World& world) override;
};

}  // namespace rtype::common
