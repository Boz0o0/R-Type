#pragma once

#include <ampersand/core/math/Vec2.hpp>
#include <ampersand/render/IRenderer.hpp>
#include <ampersand/render/WindowConfig.hpp>
#include <string>

namespace ampersand::render {

/**
 * @brief The renderer used by the World and the state of its window.
 *
 * Required by RenderSystem: add it once with the backend (an SfmlRenderer,
 * for example), which opens the window. The RenderSystem then keeps `size`
 * and `isOpen` up to date at the start of each update.
 */
struct Renderer {
    /**
     * @brief Opens the window of the backend and fills the fields.
     *
     * @param backend Renderer to draw with. Must outlive the World.
     * @param config Size, title and vsync of the window.
     */
    Renderer(IRenderer& backend, const WindowConfig& config)
        : renderer(backend), title(config.title) {
        isOpen = backend.open(config);
        size = backend.size();
    }

    /** @brief Renderer to load assets, poll events or draw by hand. */
    IRenderer& renderer;
    /** @brief Size of the window in pixels, kept up to date (read only). */
    core::math::Vec2u size;
    /** @brief Title given when the window was opened (read only). */
    std::string title;
    /**
     * @brief Whether the window is open, kept up to date (closed with the
     * cross, for example). Set it to false to close the window.
     */
    bool isOpen{};
};

}  // namespace ampersand::render
