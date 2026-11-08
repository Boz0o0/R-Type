#pragma once

#include <ampersand/core/components/Transform.hpp>
#include <ampersand/core/ecs/ECSExceptions.hpp>
#include <ampersand/core/ecs/Entity.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <ampersand/physics/components/BoxCollider.hpp>
#include <ampersand/render/TextureId.hpp>
#include <ampersand/render/components/Sprite.hpp>
#include <cstdint>
#include <optional>
#include <rtype/client/IEntityFactory.hpp>
#include <rtype/common/components/Kind.hpp>

namespace rtype::client::test {

// Plain entities instead of prefabs, which need a window. The texture tells
// what was asked: the owner for a ship, 10 + the kind otherwise, and
// deathTextureOf(kind) for the effect of a death. Enemies are 20x20 at scale 3
// with a 60x60 hitbox; power-ups and obstacles have no death effect.
class FakeEntityFactory final : public IEntityFactory {
 public:
  explicit FakeEntityFactory(ampersand::core::ecs::World& world)
      : _world(world) {}

  [[nodiscard]] static ampersand::render::TextureId textureOf(
      common::EntityKind kind, std::uint8_t ownerId) {
    if (kind == common::EntityKind::Ship) {
      return ownerId;
    }
    return 10U + static_cast<ampersand::render::TextureId>(kind);
  }

  [[nodiscard]] static ampersand::render::TextureId deathTextureOf(
      common::EntityKind kind) {
    return 100U + static_cast<ampersand::render::TextureId>(kind);
  }

  [[nodiscard]] std::optional<ampersand::core::ecs::Entity> create(
      common::EntityKind kind, std::uint8_t /*variant*/,
      std::uint8_t ownerId) override {
    const float scale = kind == common::EntityKind::Enemy ? 3.0F : 1.0F;
    const auto entity = this->make(
        ampersand::render::Sprite{.texture = textureOf(kind, ownerId),
                                  .source = {0, 0, 20, 20}},
        scale);
    if (entity && kind == common::EntityKind::Enemy) {
      this->_world.add(*entity, ampersand::physics::BoxCollider{
                                    .bounds = {0.0F, 0.0F, 60.0F, 60.0F}});
    }
    return entity;
  }

  [[nodiscard]] std::optional<ampersand::core::ecs::Entity> createDeath(
      common::EntityKind kind, std::uint8_t /*variant*/,
      std::uint8_t /*ownerId*/) override {
    if (kind == common::EntityKind::PowerUp ||
        kind == common::EntityKind::Obstacle) {
      return std::nullopt;
    }
    return this->make(ampersand::render::Sprite{.texture = deathTextureOf(kind),
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
