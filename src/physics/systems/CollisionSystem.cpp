#include <ampersand/core/components/Transform.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <ampersand/core/math/Rect.hpp>
#include <ampersand/physics/components/BoxCollider.hpp>
#include <ampersand/physics/resources/Collisions.hpp>
#include <ampersand/physics/systems/CollisionSystem.hpp>
#include <cstddef>
#include <vector>

namespace ampersand::physics {

namespace {

struct Box {
    core::ecs::Entity entity;
    core::math::Rect<float> bounds;
    float z;
};

}  // namespace

void CollisionSystem::update(core::ecs::World& world) {
    auto& collisions = world.getResource<Collisions>();
    collisions.pairs.clear();
    std::vector<Box> boxes;
    for (const core::ecs::Entity entity : world.owners<BoxCollider>()) {
        if (!world.has<core::Transform>(entity)) {
            continue;
        }
        const auto& transform = world.get<core::Transform>(entity);
        boxes.push_back(
            {.entity = entity,
             .bounds = worldBounds(transform, world.get<BoxCollider>(entity)),
             .z = transform.position.z});
    }
    for (std::size_t first = 0; first < boxes.size(); first++) {
        for (std::size_t second = first + 1; second < boxes.size(); second++) {
            if (boxes[first].z == boxes[second].z &&
                boxes[first].bounds.intersects(boxes[second].bounds)) {
                collisions.pairs.emplace_back(boxes[first].entity,
                                              boxes[second].entity);
            }
        }
    }
}

}  // namespace ampersand::physics
