#pragma once

#include <ampersand/core/ecs/ISystem.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <ampersand/core/math/Vec2.hpp>

namespace rtype::client {

class PredictionSystem final : public ampersand::core::ecs::ISystem {
 public:
  void update(ampersand::core::ecs::World& world) override;

 private:
  [[nodiscard]] static bool isStill(ampersand::core::math::Vec2f velocity);
};

}  // namespace rtype::client
