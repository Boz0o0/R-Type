#include <gtest/gtest.h>

#include <ampersand/core/components/Transform.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <ampersand/render/IRenderer.hpp>
#include <ampersand/render/components/Sprite.hpp>
#include <ampersand/render/components/Text.hpp>
#include <ampersand/render/resources/Camera.hpp>
#include <ampersand/render/resources/ClearColor.hpp>
#include <ampersand/render/resources/Renderer.hpp>
#include <ampersand/render/systems/RenderSystem.hpp>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace {

using ampersand::core::Transform;
using ampersand::core::ecs::Entity;
using ampersand::core::ecs::World;
using ampersand::render::Camera;
using ampersand::render::Circle;
using ampersand::render::ClearColor;
using ampersand::render::Color;
using ampersand::render::Event;
using ampersand::render::FontId;
using ampersand::render::IRenderer;
using ampersand::render::Line;
using ampersand::render::Rectangle;
using ampersand::render::Renderer;
using ampersand::render::RenderSystem;
using ampersand::render::Sprite;
using ampersand::render::Text;
using ampersand::render::TextureId;
using ampersand::render::WindowConfig;

/**
 * @brief Renderer that opens no window and records what it is asked, so the
 * RenderSystem can be tested without a screen.
 */
class FakeRenderer final : public IRenderer {
public:
    FakeRenderer() = default;

    bool open(const WindowConfig& /*config*/) override { return true; }
    void close() override {}
    [[nodiscard]] bool isOpen() const override { return true; }
    [[nodiscard]] ampersand::core::math::Vec2u size() const override {
        return {};
    }
    std::optional<Event> pollEvent() override { return std::nullopt; }

    void setView(const Camera& camera) override {
        calls.emplace_back("setView");
        lastCamera = camera;
    }

    void resetView() override {
        calls.emplace_back("resetView");
        lastCamera.reset();
    }

    [[nodiscard]] TextureId loadTexture(
        const std::filesystem::path& /*path*/) override {
        return 1;
    }

    [[nodiscard]] FontId loadFont(
        const std::filesystem::path& /*path*/) override {
        return 1;
    }

    void beginFrame(Color clear) override {
        calls.emplace_back("beginFrame");
        clearColor = clear;
    }

    void draw(const Sprite& sprite, const Transform& transform) override {
        calls.emplace_back("drawSprite");
        sprites.push_back(sprite);
        transforms.push_back(transform);
    }

    void draw(const Rectangle& /*rectangle*/,
              const Transform& /*transform*/) override {}
    void draw(const Circle& /*circle*/,
              const Transform& /*transform*/) override {}
    void draw(const Line& /*line*/, const Transform& /*transform*/) override {}

    void draw(const Text& text, const Transform& transform) override {
        calls.emplace_back("drawText");
        texts.push_back(text);
        transforms.push_back(transform);
    }

    void endFrame() override { calls.emplace_back("endFrame"); }

    std::vector<std::string> calls;
    std::vector<Sprite> sprites;
    std::vector<Text> texts;
    std::vector<Transform> transforms;
    Color clearColor{.r = 1, .g = 2, .b = 3};
    std::optional<Camera> lastCamera;
};

Entity createAt(World& world, float left, float top) {
    const Entity entity = world.create();
    Transform transform;
    transform.position = {left, top, 0.0F};
    world.add(entity, transform);
    return entity;
}

}  // namespace

TEST(RenderSystem, EmptyWorldStillDrawsAFrame) {
    FakeRenderer renderer;
    World world;
    world.addResource<Renderer>(renderer, WindowConfig{});

    RenderSystem{}.update(world);

    const std::vector<std::string> expected{
        "beginFrame",
        "resetView",
        "endFrame",
    };
    EXPECT_EQ(renderer.calls, expected);
}

TEST(RenderSystem, ClearsInBlackWithoutClearColor) {
    FakeRenderer renderer;
    World world;
    world.addResource<Renderer>(renderer, WindowConfig{});

    RenderSystem{}.update(world);

    EXPECT_EQ(renderer.clearColor.r, 0);
    EXPECT_EQ(renderer.clearColor.g, 0);
    EXPECT_EQ(renderer.clearColor.b, 0);
    EXPECT_EQ(renderer.clearColor.a, 255);
}

TEST(RenderSystem, ClearsWithTheClearColor) {
    FakeRenderer renderer;
    World world;
    world.addResource<Renderer>(renderer, WindowConfig{});
    world.addResource<ClearColor>().value = {.r = 10, .g = 20, .b = 30};

    RenderSystem{}.update(world);

    EXPECT_EQ(renderer.clearColor.r, 10);
    EXPECT_EQ(renderer.clearColor.g, 20);
    EXPECT_EQ(renderer.clearColor.b, 30);
}

TEST(RenderSystem, UsesTheCamera) {
    FakeRenderer renderer;
    World world;
    world.addResource<Renderer>(renderer, WindowConfig{});
    world.addResource<Camera>() = {
        .center = {960.0F, 540.0F},
        .size = {1920.0F, 1080.0F},
    };

    RenderSystem{}.update(world);

    ASSERT_TRUE(renderer.lastCamera.has_value());
    const Camera camera = renderer.lastCamera.value_or(Camera{});
    EXPECT_FLOAT_EQ(camera.center.x, 960.0F);
    EXPECT_FLOAT_EQ(camera.size.y, 1080.0F);
}

TEST(RenderSystem, RemovingTheCameraGoesBackToTheDefaultView) {
    FakeRenderer renderer;
    World world;
    world.addResource<Renderer>(renderer, WindowConfig{});
    world.addResource<Camera>() = {
        .center = {1.0F, 1.0F},
        .size = {2.0F, 2.0F},
    };
    RenderSystem render;
    render.update(world);

    world.removeResource<Camera>();
    render.update(world);

    EXPECT_FALSE(renderer.lastCamera.has_value());
}

TEST(RenderSystem, DrawsEverySpriteAtItsTransform) {
    FakeRenderer renderer;
    World world;
    world.addResource<Renderer>(renderer, WindowConfig{});
    const Entity ship = createAt(world, 10.0F, 20.0F);
    world.add(ship, Sprite{.texture = 1, .source = {0, 0, 33, 17}});
    const Entity enemy = createAt(world, 500.0F, 40.0F);
    world.add(enemy, Sprite{.texture = 2});

    RenderSystem{}.update(world);

    ASSERT_EQ(renderer.sprites.size(), 2U);
    EXPECT_EQ(renderer.sprites[0].texture, 1U);
    EXPECT_EQ(renderer.sprites[0].source.width, 33);
    EXPECT_FLOAT_EQ(renderer.transforms[0].position.x, 10.0F);
    EXPECT_EQ(renderer.sprites[1].texture, 2U);
    EXPECT_FLOAT_EQ(renderer.transforms[1].position.x, 500.0F);
}

TEST(RenderSystem, DrawsText) {
    FakeRenderer renderer;
    World world;
    world.addResource<Renderer>(renderer, WindowConfig{});
    const Entity score = createAt(world, 5.0F, 5.0F);
    world.add(score, Text{.font = 1, .value = "0042"});

    RenderSystem{}.update(world);

    ASSERT_EQ(renderer.texts.size(), 1U);
    EXPECT_EQ(renderer.texts[0].value, "0042");
    EXPECT_FLOAT_EQ(renderer.transforms[0].position.y, 5.0F);
}

TEST(RenderSystem, SkipsEntityWithoutTransform) {
    FakeRenderer renderer;
    World world;
    world.addResource<Renderer>(renderer, WindowConfig{});
    const Entity ghost = world.create();
    world.add(ghost, Sprite{.texture = 1});
    world.add(ghost, Text{.font = 1});

    RenderSystem{}.update(world);

    EXPECT_TRUE(renderer.sprites.empty());
    EXPECT_TRUE(renderer.texts.empty());
}

TEST(RenderSystem, DrawsBetweenBeginAndEndFrame) {
    FakeRenderer renderer;
    World world;
    world.addResource<Renderer>(renderer, WindowConfig{});
    world.addResource<Camera>() = {
        .center = {1.0F, 1.0F},
        .size = {2.0F, 2.0F},
    };
    world.add(createAt(world, 0.0F, 0.0F), Sprite{.texture = 1});
    world.add(createAt(world, 0.0F, 0.0F), Text{.font = 1});

    RenderSystem{}.update(world);

    const std::vector<std::string> expected{
        "beginFrame", "setView", "drawSprite", "drawText", "endFrame",
    };
    EXPECT_EQ(renderer.calls, expected);
}
