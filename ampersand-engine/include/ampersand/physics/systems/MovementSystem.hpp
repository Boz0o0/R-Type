#pragma once

#include <ampersand/core/ecs/ISystem.hpp>

namespace ampersand::physics {

/**
 * @brief Moves every entity that has a Velocity and a Transform, using the
 * dt of the Clock resource, and makes the ones with a Gravity fall.
 */
class MovementSystem final : public core::ecs::ISystem {
public:
    /**
     * @brief Adds gravity * dt to the velocity of the entities with a
     * Gravity, then adds velocity * dt to the x and y of each Transform
     * position.
     *
     * @param world World holding the entities and the Clock resource (and the
     * GravityForce resource if an entity has a Gravity).
     * @throws ResourceNotFound if the World has no Clock, or no GravityForce
     * while an entity has a Gravity.
     */
    void update(core::ecs::World& world) override;
};

}  // namespace ampersand::physics
