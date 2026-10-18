#pragma once

#include <ampersand/core/ecs/ISystem.hpp>
#include <ampersand/core/ecs/World.hpp>

namespace rtype::client {

class ScrollingSystem final : public ampersand::core::ecs::ISystem {
 public:
  void update(ampersand::core::ecs::World& world) override;

 private:
  static constexpr float COPIES = 2.0F;
};

}  // namespace rtype::client
