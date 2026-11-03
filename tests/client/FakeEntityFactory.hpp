#pragma once

#include <ampersand/core/components/Transform.hpp>
#include <ampersand/core/ecs/ECSExceptions.hpp>
#include <ampersand/core/ecs/Entity.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <ampersand/render/TextureId.hpp>
#include <ampersand/render/components/Sprite.hpp>
#include <cstdint>
#include <optional>
#include <rtype/client/IEntityFactory.hpp>
#include <rtype/common/components/Kind.hpp>

namespace rtype::client::test {

// Plain entities instead of prefabs, which need a window. The texture tells
// what was asked: the owner for a ship, 10 + the kind otherwise, 99 for an
// explosion. Enemies are 20x20 at scale 3, explosions 10x10 at scale 1.
class FakeEntityFactory final : public IEntityFactory {
 public:
  static constexpr ampersand::render::TextureId EXPLOSION_TEXTURE = 99;

  explicit FakeEntityFactory(ampersand::core::ecs::World& world)
      : _world(world) {}

  [[nodiscard]] static ampersand::render::TextureId textureOf(
      common::EntityKind kind, std::uint8_t ownerId) {
    if (kind == common::EntityKind::Ship) {
      return ownerId;
    }
    return 10U + static_cast<ampersand::render::TextureId>(kind);
  }

  [[nodiscard]] std::optional<ampersand::core::ecs::Entity> create(
      common::EntityKind kind, std::uint8_t ownerId) override {
    const float scale = kind == common::EntityKind::Enemy ? 3.0F : 1.0F;
    return this->make(
        ampersand::render::Sprite{.texture = textureOf(kind, ownerId),
                                  .source = {0, 0, 20, 20}},
        scale);
  }

  [[nodiscard]] std::optional<ampersand::core::ecs::Entity> createExplosion()
      override {
    return this->make(ampersand::render::Sprite{.texture = EXPLOSION_TEXTURE,
                                                .source = {0, 0, 10, 10}},
                      1.0F);
  }

 private:
  std::optional<ampersand::core::ecs::Entity> make(
      const ampersand::render::Sprite& sprite, float scale) {
    try {
      const ampersand::core::ecs::Entity entity = this->_world.create();
      this->_world.add(entity, ampersand::core::Transform{
                                   .scale = {scale, scale, 1.0F},
                               });
      this->_world.add(entity, sprite);
      return entity;
    } catch (const ampersand::core::ecs::EntityLimitReached&) {
      return std::nullopt;
    }
  }

  ampersand::core::ecs::World& _world;
};

}  // namespace rtype::client::test
