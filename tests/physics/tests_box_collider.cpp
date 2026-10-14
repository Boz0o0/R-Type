#include <gtest/gtest.h>

#include <ampersand/core/components/Transform.hpp>
#include <ampersand/core/math/Rect.hpp>
#include <ampersand/physics/components/BoxCollider.hpp>

namespace {

using ampersand::core::Transform;
using ampersand::core::math::Rect;
using ampersand::physics::BoxCollider;
using ampersand::physics::worldBounds;

void expectRect(const Rect<float>& rect, float left, float top, float width,
                float height) {
    EXPECT_FLOAT_EQ(rect.x, left);
    EXPECT_FLOAT_EQ(rect.y, top);
    EXPECT_FLOAT_EQ(rect.width, width);
    EXPECT_FLOAT_EQ(rect.height, height);
}

}  // namespace

TEST(WorldBounds, DefaultTransformKeepsTheBox) {
    const Transform transform;
    const BoxCollider collider{{0.0F, 0.0F, 32.0F, 32.0F}};

    expectRect(worldBounds(transform, collider), 0.0F, 0.0F, 32.0F, 32.0F);
}

TEST(WorldBounds, AddsThePosition) {
    Transform transform;
    transform.position = {100.0F, 50.0F, 0.0F};
    const BoxCollider collider{{8.0F, 4.0F, 16.0F, 16.0F}};

    expectRect(worldBounds(transform, collider), 108.0F, 54.0F, 16.0F, 16.0F);
}

TEST(WorldBounds, CentersOnPositionWithOriginAndScale) {
    Transform transform;
    transform.position = {100.0F, 100.0F, 0.0F};
    transform.origin = {16.0F, 16.0F, 0.0F};
    transform.scale = {2.0F, 2.0F, 1.0F};
    const BoxCollider collider{{0.0F, 0.0F, 32.0F, 32.0F}};

    expectRect(worldBounds(transform, collider), 68.0F, 68.0F, 64.0F, 64.0F);
}

TEST(WorldBounds, NegativeScaleFlipsTheBox) {
    Transform transform;
    transform.position = {100.0F, 0.0F, 0.0F};
    transform.scale = {-1.0F, 1.0F, 1.0F};
    const BoxCollider collider{{0.0F, 0.0F, 32.0F, 16.0F}};

    expectRect(worldBounds(transform, collider), 68.0F, 0.0F, 32.0F, 16.0F);
}

TEST(WorldBounds, MovedEntitiesCollide) {
    Transform ship;
    Transform bullet;
    bullet.position = {20.0F, 10.0F, 0.0F};
    const BoxCollider box{{0.0F, 0.0F, 32.0F, 32.0F}};
    const BoxCollider small{{0.0F, 0.0F, 4.0F, 4.0F}};

    EXPECT_TRUE(worldBounds(ship, box).intersects(worldBounds(bullet, small)));

    bullet.position.x = 32.0F;
    EXPECT_FALSE(worldBounds(ship, box).intersects(worldBounds(bullet, small)));
}
