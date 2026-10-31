#pragma once

#include <ampersand/core/ecs/ISystem.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <ampersand/input/resources/InputManager.hpp>
#include <rtype/client/ServerConnection.hpp>

namespace rtype::client {

class QuitSystem final : public ampersand::core::ecs::ISystem {
 public:
  explicit QuitSystem(const ServerConnection& connection);
  void update(ampersand::core::ecs::World& world) override;

  [[nodiscard]] static bool shouldQuit(
      const ampersand::input::InputManager& manager, bool ended);

 private:
  const ServerConnection& _connection;
};

}  // namespace rtype::client
