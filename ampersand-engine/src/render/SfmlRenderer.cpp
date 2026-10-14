#include <algorithm>
#include <ampersand/render/SfmlConversions.hpp>
#include <ampersand/render/SfmlRenderer.hpp>
#include <array>
#include <cmath>
#include <memory>
#include <stdexcept>
#include <utility>

namespace ampersand::render {

bool SfmlRenderer::open(const WindowConfig& config) {
    this->window_.create(sf::VideoMode({config.size.x, config.size.y}),
                         config.title);
    this->window_.setVerticalSyncEnabled(config.vsync);
    this->customView_ = false;
    this->updateDefaultView();
    this->window_.setView(this->defaultView_);
    return this->window_.isOpen();
}

void SfmlRenderer::close() {
    this->drawCommands_.clear();
    this->window_.close();
}

core::math::Vec2u SfmlRenderer::size() const {
    const auto windowSize = this->window_.getSize();
    return {windowSize.x, windowSize.y};
}

std::optional<Event> SfmlRenderer::pollEvent() {
    while (const auto event = this->window_.pollEvent()) {
        if (event->is<sf::Event::Closed>()) return Closed{};
        if (const auto* const resized = event->getIf<sf::Event::Resized>()) {
            this->updateDefaultView();
            if (!this->customView_) this->window_.setView(this->defaultView_);
            return Resized{{resized->size.x, resized->size.y}};
        }
        if (const auto* const keyPressed =
                event->getIf<sf::Event::KeyPressed>()) {
            return ampersand::input::KeyPressed{toKey(keyPressed->code)};
        }
        if (const auto* const keyReleased =
                event->getIf<sf::Event::KeyReleased>()) {
            return ampersand::input::KeyReleased{toKey(keyReleased->code)};
        }
    }
    return std::nullopt;
}

void SfmlRenderer::setView(const Camera& camera) {
    if (!std::isfinite(camera.center.x) || !std::isfinite(camera.center.y) ||
        !std::isfinite(camera.size.x) || !std::isfinite(camera.size.y) ||
        camera.size.x <= 0.0F || camera.size.y <= 0.0F) {
        throw std::invalid_argument(
            "Camera center and size must be finite, and "
            "camera size must be positive");
    }
    this->window_.setView(sf::View({camera.center.x, camera.center.y},
                                   {camera.size.x, camera.size.y}));
    this->customView_ = true;
}

void SfmlRenderer::resetView() {
    this->customView_ = false;
    this->updateDefaultView();
    this->window_.setView(this->defaultView_);
}

TextureId SfmlRenderer::loadTexture(const std::filesystem::path& path) {
    sf::Texture texture;
    if (!texture.loadFromFile(path)) return 0;

    const TextureId textureId = this->nextId_++;
    this->textures_.emplace(textureId, std::move(texture));
    return textureId;
}

FontId SfmlRenderer::loadFont(const std::filesystem::path& path) {
    sf::Font font;
    if (!font.openFromFile(path)) return 0;

    const FontId fontId = this->nextFontId_++;
    this->fonts_.emplace(fontId, std::move(font));
    return fontId;
}

void SfmlRenderer::beginFrame(Color clear) {
    this->drawCommands_.clear();
    this->window_.clear(toSfColor(clear));
}

void SfmlRenderer::draw(const Sprite& sprite,
                        const core::Transform& transform) {
    this->drawCommands_.push_back({
        layerFor(transform),
        [this, sprite, transform] {
            const auto texture = this->textures_.find(sprite.texture);
            if (texture == this->textures_.end()) return;

            sf::Sprite renderedSprite(texture->second);
            if (sprite.source.width > 0 && sprite.source.height > 0) {
                renderedSprite.setTextureRect(
                    sf::IntRect({sprite.source.x, sprite.source.y},
                                {sprite.source.width, sprite.source.height}));
            }
            renderedSprite.setColor(toSfColor(sprite.color));
            applyTransform(renderedSprite, transform);
            this->window_.draw(renderedSprite);
        },
    });
}

void SfmlRenderer::draw(const Rectangle& rectangle,
                        const core::Transform& transform) {
    this->drawCommands_.push_back({
        layerFor(transform),
        [this, rectangle, transform] {
            sf::RectangleShape shape({rectangle.size.x, rectangle.size.y});
            shape.setFillColor(toSfColor(rectangle.fill));
            shape.setOutlineColor(toSfColor(rectangle.outline));
            shape.setOutlineThickness(rectangle.outlineThickness);
            applyTransform(shape, transform);
            this->window_.draw(shape);
        },
    });
}

void SfmlRenderer::draw(const Circle& circle,
                        const core::Transform& transform) {
    this->drawCommands_.push_back({
        layerFor(transform),
        [this, circle, transform] {
            sf::CircleShape shape(circle.radius);
            shape.setFillColor(toSfColor(circle.fill));
            shape.setOutlineColor(toSfColor(circle.outline));
            shape.setOutlineThickness(circle.outlineThickness);
            applyTransform(shape, transform);
            this->window_.draw(shape);
        },
    });
}

void SfmlRenderer::draw(const Line& line, const core::Transform& transform) {
    this->drawCommands_.push_back({
        layerFor(transform),
        [this, line, transform] {
            sf::Transformable transformable;
            applyTransform(transformable, transform);
            const sf::Color color = toSfColor(line.color);
            const std::array<sf::Vertex, 2> vertices{
                {
                    {.position = {line.start.x, line.start.y}, .color = color},
                    {.position = {line.end.x, line.end.y}, .color = color},
                },
            };
            this->window_.draw(vertices.data(), vertices.size(),
                               sf::PrimitiveType::Lines,
                               sf::RenderStates(transformable.getTransform()));
        },
    });
}

void SfmlRenderer::draw(const Text& text, const core::Transform& transform) {
    const auto ownedText = std::make_shared<const Text>(text);
    this->drawCommands_.push_back({
        layerFor(transform),
        [this, ownedText, transform] {
            const auto font = this->fonts_.find(ownedText->font);
            if (font == this->fonts_.end()) return;

            sf::Text drawable(font->second, ownedText->value, ownedText->size);
            drawable.setFillColor(toSfColor(ownedText->color));
            applyTransform(drawable, transform);
            this->window_.draw(drawable);
        },
    });
}

void SfmlRenderer::endFrame() {
    std::ranges::stable_sort(this->drawCommands_, {}, &DrawCommand::layer);
    for (const DrawCommand& command : this->drawCommands_) command.draw();
    this->drawCommands_.clear();
    this->window_.display();
}

float SfmlRenderer::layerFor(const core::Transform& transform) {
    if (!std::isfinite(transform.position.z)) {
        throw std::invalid_argument(
            "Transform Z position must be finite for render ordering");
    }
    return transform.position.z;
}

void SfmlRenderer::applyTransform(sf::Transformable& drawable,
                                  const core::Transform& transform) {
    drawable.setPosition({transform.position.x, transform.position.y});
    drawable.setScale({transform.scale.x, transform.scale.y});
    drawable.setRotation(sf::degrees(transform.rotationDegrees.z));
    drawable.setOrigin({transform.origin.x, transform.origin.y});
}

void SfmlRenderer::updateDefaultView() {
    const auto windowSize = this->window_.getSize();
    if (windowSize.x == 0 || windowSize.y == 0) return;
    this->defaultView_ = sf::View(
        {static_cast<float>(windowSize.x) / 2.0F,
         static_cast<float>(windowSize.y) / 2.0F},
        {static_cast<float>(windowSize.x), static_cast<float>(windowSize.y)});
}

}  // namespace ampersand::render
