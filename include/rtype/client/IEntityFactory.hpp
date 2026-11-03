#pragma once

#include <ampersand/core/ecs/Entity.hpp>
#include <cstdint>
#include <optional>
#include <rtype/common/components/Kind.hpp>

namespace rtype::client {

// Creates the entities WorldMirror shows, with their looks and sounds but at
// the origin. In the game they come from prefabs, which need a window, so the
// tests give plain entities instead. std::nullopt when it cannot be created.
class IEntityFactory {
 public:
  virtual ~IEntityFactory() = default;
  IEntityFactory(const IEntityFactory&) = delete;
  IEntityFactory& operator=(const IEntityFactory&) = delete;
  IEntityFactory(IEntityFactory&&) = delete;
  IEntityFactory& operator=(IEntityFactory&&) = delete;

  [[nodiscard]] virtual std::optional<ampersand::core::ecs::Entity> create(
      common::EntityKind kind, std::uint8_t ownerId) = 0;
  [[nodiscard]] virtual std::optional<ampersand::core::ecs::Entity>
  createExplosion() = 0;

 protected:
  IEntityFactory() = default;
};

}  // namespace rtype::client
