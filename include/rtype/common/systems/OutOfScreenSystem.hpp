#pragma once

#include <ampersand/core/ecs/ISystem.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <ampersand/core/math/Rect.hpp>

namespace rtype::common {

/**
 * @brief Kills what left the screen for good: missiles that hit nothing,
 *        enemies that flew past the players...
 *
 * An entity dies (DeathReason::OutOfScreen) once its BoxCollider is fully
 * outside the screen and its Velocity does not bring it back. An enemy
 * waiting on the right of the screen and flying left stays alive.
 * Ships are never killed: the server keeps them on the screen.
 */
class OutOfScreenSystem final : public ampersand::core::ecs::ISystem {
 public:
  /**
   * @brief Creates the system.
   *
   * @param screen Visible area, in world units.
   */
  explicit OutOfScreenSystem(ampersand::core::math::Rect<float> screen);

  /**
   * @brief Adds to the Deaths every entity that left the screen.
   *
   * Run it AFTER the MovementSystem.
   *
   * @param world World holding the entities and the Deaths resource.
   * @throws ResourceNotFound if the World has no Deaths.
   */
  void update(ampersand::core::ecs::World& world) override;

 private:
  ampersand::core::math::Rect<float> _screen;
};

}  // namespace rtype::common
