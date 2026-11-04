#pragma once

#include <ampersand/core/ecs/ISystem.hpp>
#include <ampersand/core/ecs/World.hpp>

namespace rtype::client {

// A broken script is turned off without stopping the game: this says why.
class ScriptErrorSystem final : public ampersand::core::ecs::ISystem {
 public:
  void update(ampersand::core::ecs::World& world) override;
};

}  // namespace rtype::client
