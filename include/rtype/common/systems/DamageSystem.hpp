#pragma once

#include <ampersand/core/ecs/Entity.hpp>
#include <ampersand/core/ecs/ISystem.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <rtype/common/resources/Deaths.hpp>

namespace rtype::common {

/**
 * @brief Turns the Collisions of the tick into damage and deaths.
 *
 * For each pair that touches, an entity with a Damage removes its amount from
 * the Health of the other one. An entity whose Health reaches 0 dies, and an
 * entity with a Damage but no Health (a missile) dies on its first hit.
 * Deaths are added to the Deaths resource; nothing is destroyed here.
 */
class DamageSystem final : public ampersand::core::ecs::ISystem {
 public:
  /**
   * @brief Applies the damage of every pair in the Collisions resource.
   *
   * Run it AFTER the CollisionSystem.
   *
   * @param world World holding the entities and the resources.
   * @throws ResourceNotFound if the World has no Collisions or no Deaths.
   */
  void update(ampersand::core::ecs::World& world) override;

 private:
  /**
   * @brief Applies the Damage of one entity to another one, if it has any.
   *
   * @param world World holding both entities.
   * @param deaths Deaths of the tick.
   * @param attacker Entity that may deal damage.
   * @param target Entity that may take it.
   */
  static void hit(ampersand::core::ecs::World& world, Deaths& deaths,
                  ampersand::core::ecs::Entity attacker,
                  ampersand::core::ecs::Entity target);
};

}  // namespace rtype::common
