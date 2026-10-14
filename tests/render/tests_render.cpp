#include <gtest/gtest.h>

#include <ampersand/render/SfmlRenderer.hpp>
#include <ampersand/render/resources/ClearColor.hpp>

TEST(RenderModule, MissingAssetsReturnZeroIds) {
    ampersand::render::SfmlRenderer renderer;

    EXPECT_EQ(renderer.loadTexture("/definitely/not/a/real/texture.png"), 0);
    EXPECT_EQ(renderer.loadFont("/definitely/not/a/real/font.ttf"), 0);
}

TEST(RenderComponents, SpriteAndTextDefaultToWhite) {
    const ampersand::render::Sprite sprite;
    const ampersand::render::Text text;

    EXPECT_EQ(sprite.color.r, 255);
    EXPECT_EQ(sprite.color.g, 255);
    EXPECT_EQ(sprite.color.b, 255);
    EXPECT_EQ(sprite.color.a, 255);
    EXPECT_EQ(text.color.r, 255);
    EXPECT_EQ(text.color.a, 255);
    EXPECT_EQ(text.size, 24U);
}

TEST(RenderResources, ClearColorDefaultsToOpaqueBlack) {
    const ampersand::render::ClearColor clear;

    EXPECT_EQ(clear.value.r, 0);
    EXPECT_EQ(clear.value.g, 0);
    EXPECT_EQ(clear.value.b, 0);
    EXPECT_EQ(clear.value.a, 255);
}
