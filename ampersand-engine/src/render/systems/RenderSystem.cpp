#include <ampersand/core/components/Transform.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <ampersand/render/components/Sprite.hpp>
#include <ampersand/render/components/Text.hpp>
#include <ampersand/render/resources/Camera.hpp>
#include <ampersand/render/resources/ClearColor.hpp>
#include <ampersand/render/resources/Renderer.hpp>
#include <ampersand/render/systems/RenderSystem.hpp>

namespace ampersand::render {

void RenderSystem::update(core::ecs::World& world) {
    auto& window = world.getResource<Renderer>();
    if (!syncWindow(window)) {
        return;
    }
    IRenderer& renderer = window.renderer;
    renderer.beginFrame(world.hasResource<ClearColor>()
                            ? world.getResource<ClearColor>().value
                            : Color{});
    if (world.hasResource<Camera>()) {
        renderer.setView(world.getResource<Camera>());
    } else {
        renderer.resetView();
    }
    for (const core::ecs::Entity entity : world.owners<Sprite>()) {
        if (!world.has<core::Transform>(entity)) {
            continue;
        }
        renderer.draw(world.get<Sprite>(entity),
                      world.get<core::Transform>(entity));
    }
    for (const core::ecs::Entity entity : world.owners<Text>()) {
        if (!world.has<core::Transform>(entity)) {
            continue;
        }
        renderer.draw(world.get<Text>(entity),
                      world.get<core::Transform>(entity));
    }
    renderer.endFrame();
}

bool RenderSystem::syncWindow(Renderer& window) {
    if (!window.isOpen) {
        window.renderer.close();
    }
    window.isOpen = window.renderer.isOpen();
    window.size = window.renderer.size();
    return window.isOpen;
}

}  // namespace ampersand::render
