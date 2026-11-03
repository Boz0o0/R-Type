#pragma once

#include <ampersand/core/ecs/ISystem.hpp>
#include <ampersand/core/ecs/World.hpp>

namespace rtype::client {

class CorrectionSystem final : public ampersand::core::ecs::ISystem {
 public:
  void update(ampersand::core::ecs::World& world) override;

 private:
  // In this time, about two thirds of an error is absorbed.
  static constexpr float SMOOTHING_TIME = 0.1F;
};

}  // namespace rtype::client
