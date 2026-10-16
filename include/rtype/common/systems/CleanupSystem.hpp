#pragma once

#include <ampersand/core/ecs/ISystem.hpp>
#include <ampersand/core/ecs/World.hpp>

namespace rtype::common {

/**
 * @brief Destroys the entities that died during the tick.
 *
 * Run it last, once everything that needs the Deaths (the server sending
 * them) has read them.
 */
class CleanupSystem final : public ampersand::core::ecs::ISystem {
 public:
  /**
   * @brief Destroys every entity of the Deaths resource, then empties it.
   *
   * @param world World holding the entities and the Deaths resource.
   * @throws ResourceNotFound if the World has no Deaths.
   */
  void update(ampersand::core::ecs::World& world) override;
};

}  // namespace rtype::common
