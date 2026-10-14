#pragma once

#include <ampersand/core/components/Transform.hpp>
#include <ampersand/core/math/Vec2.hpp>
#include <ampersand/render/Color.hpp>
#include <ampersand/render/Event.hpp>
#include <ampersand/render/Font.hpp>
#include <ampersand/render/Primitive.hpp>
#include <ampersand/render/Texture.hpp>
#include <ampersand/render/WindowConfig.hpp>
#include <ampersand/render/components/Sprite.hpp>
#include <ampersand/render/components/Text.hpp>
#include <ampersand/render/resources/Camera.hpp>
#include <cstdint>
#include <filesystem>
#include <optional>

namespace ampersand::render {

/**
 * @brief Abstract interface for all renderer backends.
 *
 * This interface defines the functionality required to create a windowed
 * rendering context, process events, load assets, and draw 2D content.
 * Concrete implementations provide platform-specific behavior while keeping
 * the high-level API uniform.
 */
class IRenderer {
public:
    /**
     * @brief Destroys the renderer instance.
     */
    virtual ~IRenderer() = default;

    IRenderer(const IRenderer&) = delete;
    IRenderer& operator=(const IRenderer&) = delete;

    /**
     * @brief Opens the renderer using the provided window configuration.
     *
     * @param config Settings used to initialize the rendering window.
     * @return true if the renderer was successfully opened, false otherwise.
     */
    virtual bool open(const WindowConfig& config) = 0;

    /**
     * @brief Closes the renderer and releases any active window resources.
     */
    virtual void close() = 0;

    /**
     * @brief Checks whether the renderer is currently open.
     *
     * @return true if the rendering context is active, false otherwise.
     */
    [[nodiscard]] virtual bool isOpen() const = 0;

    /**
     * @brief Retrieves the current size of the rendering window.
     *
     * @return Dimensions of the window in pixels.
     */
    [[nodiscard]] virtual core::math::Vec2u size() const = 0;

    /**
     * @brief Polls the next queued event from the window.
     *
     * @return The next event if one is available, otherwise std::nullopt.
     */
    virtual std::optional<Event> pollEvent() = 0;

    /**
     * @brief Sets the world-space camera view used for subsequent draw
     * commands.
     *
     * @throws std::invalid_argument if the camera is not finite or its size
     * is not positive.
     */
    virtual void setView(const Camera& camera) = 0;

    /**
     * @brief Restores the default view covering the window in pixel
     * coordinates.
     */
    virtual void resetView() = 0;

    /**
     * @brief Loads a texture from the provided file path.
     *
     * @param path Location of the image file to load.
     * @return Identifier of the loaded texture, or an invalid value if loading
     * failed.
     */
    [[nodiscard]] virtual TextureId loadTexture(
        const std::filesystem::path& path) = 0;

    /**
     * @brief Loads a font and returns its resource identifier, or 0 on failure.
     */
    [[nodiscard]] virtual FontId loadFont(
        const std::filesystem::path& path) = 0;

    /**
     * @brief Begins a new frame and clears the screen with the specified color.
     *
     * @param clear Color used to clear the framebuffer before drawing.
     */
    virtual void beginFrame(Color clear) = 0;

    /**
     * @brief Draws a sprite; the transform's Z position determines draw order.
     */
    virtual void draw(const Sprite& sprite,
                      const core::Transform& transform) = 0;

    /**
     * @brief Draws a filled or outlined rectangle.
     */
    virtual void draw(const Rectangle& rectangle,
                      const core::Transform& transform) = 0;

    /**
     * @brief Draws a filled or outlined circle.
     */
    virtual void draw(const Circle& circle,
                      const core::Transform& transform) = 0;

    /**
     * @brief Draws a line segment.
     */
    virtual void draw(const Line& line, const core::Transform& transform) = 0;

    /**
     * @brief Draws text using a previously loaded font.
     */
    virtual void draw(const Text& text, const core::Transform& transform) = 0;

    /**
     * @brief Presents the current frame to the display.
     */
    virtual void endFrame() = 0;

protected:
    /**
     * @brief Creates a default renderer instance.
     */
    IRenderer() = default;
};

}  // namespace ampersand::render
