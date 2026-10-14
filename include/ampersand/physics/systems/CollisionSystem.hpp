#pragma once

#include <ampersand/core/ecs/ISystem.hpp>

namespace ampersand::physics {

/**
 * @brief Finds every pair of entities whose BoxColliders overlap and writes
 * them in the Collisions resource.
 *
 * Only entities on the same z (Transform position) can collide.
 */
class CollisionSystem final : public core::ecs::ISystem {
public:
    /**
     * @brief Clears the Collisions resource, then adds each overlapping pair
     * of entities that have a BoxCollider and a Transform, on the same z.
     *
     * @param world World holding the entities and the Collisions resource.
     * @throws ResourceNotFound if the World has no Collisions.
     */
    void update(core::ecs::World& world) override;
};

}  // namespace ampersand::physics
