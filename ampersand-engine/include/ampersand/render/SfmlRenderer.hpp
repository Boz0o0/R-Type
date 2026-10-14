#pragma once

#include <SFML/Graphics.hpp>
#include <ampersand/render/IRenderer.hpp>
#include <cstdint>
#include <functional>
#include <unordered_map>
#include <vector>

namespace ampersand::render {

/**
 * @brief SFML-backed implementation of the renderer interface.
 *
 * This implementation translates the engine abstraction to SFML window and
 * drawing primitives, including input events and texture loading.
 */
class SfmlRenderer final : public IRenderer {
public:
    /**
     * @brief Opens a window using the provided configuration.
     *
     * @param config Initial window settings.
     * @return true if the underlying SFML window was created successfully.
     */
    bool open(const WindowConfig& config) override;

    /**
     * @brief Closes the current SFML window.
     */
    void close() override;

    /**
     * @brief Checks whether the window remains open.
     *
     * @return true if the SFML window is currently open.
     */
    [[nodiscard]] bool isOpen() const override { return window_.isOpen(); }

    /**
     * @brief Retrieves the size of the SFML window.
     *
     * @return Current pixel dimensions of the window.
     */
    [[nodiscard]] core::math::Vec2u size() const override;

    /**
     * @brief Polls the next queued event from the SFML event loop.
     *
     * @return The next event if one exists, otherwise std::nullopt.
     */
    std::optional<Event> pollEvent() override;

    void setView(const Camera& camera) override;
    void resetView() override;

    /**
     * @brief Loads a texture from disk under a generated texture identifier.
     *
     * @param path File system path to the texture image.
     * @return Identifier assigned to the loaded texture, or 0 on failure.
     */
    [[nodiscard]] TextureId loadTexture(
        const std::filesystem::path& path) override;

    [[nodiscard]] FontId loadFont(const std::filesystem::path& path) override;

    /**
     * @brief Begins a new frame and clears the render target.
     *
     * @param clear Color to use when clearing the framebuffer.
     */
    void beginFrame(Color clear) override;

    void draw(const Sprite& sprite, const core::Transform& transform) override;
    void draw(const Rectangle& rectangle,
              const core::Transform& transform) override;
    void draw(const Circle& circle, const core::Transform& transform) override;
    void draw(const Line& line, const core::Transform& transform) override;
    void draw(const Text& text, const core::Transform& transform) override;

    /**
     * @brief Displays the rendered content on the active window.
     */
    void endFrame() override;

private:
    struct DrawCommand {
        float layer{};
        std::function<void()> draw;
    };

    [[nodiscard]] static float layerFor(const core::Transform& transform);
    static void applyTransform(sf::Transformable& drawable,
                               const core::Transform& transform);
    void updateDefaultView();

    sf::RenderWindow window_;
    std::unordered_map<TextureId, sf::Texture> textures_;
    std::unordered_map<FontId, sf::Font> fonts_;
    std::vector<DrawCommand> drawCommands_;
    sf::View defaultView_;
    TextureId nextId_{1};
    FontId nextFontId_{1};
    bool customView_{};
};

}  // namespace ampersand::render
