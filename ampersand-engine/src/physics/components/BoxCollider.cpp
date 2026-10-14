#include <algorithm>
#include <ampersand/physics/components/BoxCollider.hpp>
#include <cmath>

namespace ampersand::physics {

core::math::Rect<float> worldBounds(const core::Transform& transform,
                                    const BoxCollider& collider) {
    const auto& bounds = collider.bounds;
    const float left = transform.position.x +
                       ((bounds.x - transform.origin.x) * transform.scale.x);
    const float top = transform.position.y +
                      ((bounds.y - transform.origin.y) * transform.scale.y);
    const float width = bounds.width * transform.scale.x;
    const float height = bounds.height * transform.scale.y;
    return {std::min(left, left + width), std::min(top, top + height),
            std::abs(width), std::abs(height)};
}

}  // namespace ampersand::physics
