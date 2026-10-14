#pragma once

#include <ampersand/core/components/Transform.hpp>
#include <ampersand/core/math/Rect.hpp>

namespace ampersand::physics {

/**
 * @brief Hitbox of an entity, in its local coordinates: placed by the origin,
 * scale and position of its Transform (see worldBounds).
 *
 * Always measured from the local (0, 0), whatever the origin of the Transform.
 */
struct BoxCollider {
    core::math::Rect<float> bounds;
};

/**
 * @brief Computes the box of a collider in the world, applying the origin,
 * scale and position of the Transform (rotation is ignored).
 *
 * @param transform Transform of the entity.
 * @param collider Collider of the entity.
 * @return The collider box in world coordinates.
 */
[[nodiscard]] core::math::Rect<float> worldBounds(
    const core::Transform& transform, const BoxCollider& collider);

}  // namespace ampersand::physics
