#include <gtest/gtest.h>

#include <ampersand/core/components/Transform.hpp>

TEST(Transform, DefaultsToIdentityTransform) {
    const ampersand::core::Transform transform;

    EXPECT_FLOAT_EQ(transform.scale.x, 1.0F);
    EXPECT_FLOAT_EQ(transform.scale.y, 1.0F);
    EXPECT_FLOAT_EQ(transform.scale.z, 1.0F);
    EXPECT_FLOAT_EQ(transform.rotationDegrees.x, 0.0F);
    EXPECT_FLOAT_EQ(transform.rotationDegrees.y, 0.0F);
    EXPECT_FLOAT_EQ(transform.rotationDegrees.z, 0.0F);
}
