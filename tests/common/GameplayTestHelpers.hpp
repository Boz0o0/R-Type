#pragma once

#include <ampersand/core/ecs/Entity.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <ampersand/core/resources/Clock.hpp>
#include <ampersand/physics/resources/Collisions.hpp>
#include <rtype/common/resources/Deaths.hpp>
#include <rtype/common/resources/Spawns.hpp>

namespace rtype::common::test {

/** @brief Adds every resource the gameplay systems need. */
inline void addGameplayResources(ampersand::core::ecs::World& world,
                                 float deltaTime = 1.0F / 60.0F) {
  world.addResource<ampersand::core::Clock>(
      ampersand::core::Clock{.dt = deltaTime});
  world.addResource<ampersand::physics::Collisions>();
  world.addResource<Deaths>();
  world.addResource<Spawns>();
}

/** @brief Makes two entities touch this tick, as the CollisionSystem would. */
inline void touch(ampersand::core::ecs::World& world,
                  ampersand::core::ecs::Entity left,
                  ampersand::core::ecs::Entity right) {
  world.getResource<ampersand::physics::Collisions>().pairs.emplace_back(left,
                                                                         right);
}

/** @brief Number of times an entity is in the Deaths of this tick. */
inline int deathsOf(ampersand::core::ecs::World& world,
                    ampersand::core::ecs::Entity entity) {
  int count = 0;
  for (const Death& death : world.getResource<Deaths>().entities) {
    count += death.entity == entity ? 1 : 0;
  }
  return count;
}

}  // namespace rtype::common::test
