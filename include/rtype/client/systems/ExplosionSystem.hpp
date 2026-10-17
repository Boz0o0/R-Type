#pragma once

#include <ampersand/core/ecs/ISystem.hpp>
#include <ampersand/core/ecs/World.hpp>

namespace rtype::client {

class ExplosionSystem final : public ampersand::core::ecs::ISystem {
 public:
  void update(ampersand::core::ecs::World& world) override;
};

}  // namespace rtype::client
