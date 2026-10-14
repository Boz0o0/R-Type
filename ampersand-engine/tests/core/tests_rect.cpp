#include <gtest/gtest.h>

#include <ampersand/core/math/Rect.hpp>

using ampersand::core::math::Rect;

TEST(Rect, IntersectsWhenOverlapping) {
    const Rect<float> first{0.0F, 0.0F, 12.0F, 12.0F};
    const Rect<float> second{11.0F, 11.0F, 12.0F, 12.0F};

    EXPECT_TRUE(first.intersects(second));
    EXPECT_TRUE(second.intersects(first));
}

TEST(Rect, DoesNotIntersectWhenOnlyTouchingByAnEdge) {
    const Rect<float> first{0.0F, 0.0F, 12.0F, 12.0F};
    const Rect<float> right{12.0F, 0.0F, 12.0F, 12.0F};
    const Rect<float> below{0.0F, 12.0F, 12.0F, 12.0F};

    EXPECT_FALSE(first.intersects(right));
    EXPECT_FALSE(first.intersects(below));
}

TEST(Rect, DoesNotIntersectWhenApart) {
    const Rect<float> first{0.0F, 0.0F, 12.0F, 12.0F};
    const Rect<float> second{13.0F, 0.0F, 12.0F, 12.0F};

    EXPECT_FALSE(first.intersects(second));
}

TEST(Rect, DoesNotIntersectWhenOverlappingOnOneAxisOnly) {
    const Rect<float> first{0.0F, 0.0F, 12.0F, 12.0F};
    const Rect<float> second{5.0F, 20.0F, 12.0F, 12.0F};

    EXPECT_FALSE(first.intersects(second));
}

TEST(Rect, IntersectsWhenOneContainsTheOther) {
    const Rect<float> outer{0.0F, 0.0F, 100.0F, 100.0F};
    const Rect<float> inner{40.0F, 40.0F, 10.0F, 10.0F};

    EXPECT_TRUE(outer.intersects(inner));
    EXPECT_TRUE(inner.intersects(outer));
}
