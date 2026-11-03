#pragma once

#include <ampersand/core/ecs/ISystem.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <ampersand/input/resources/InputManager.hpp>
#include <cstdint>
#include <rtype/client/ServerConnection.hpp>

namespace rtype::client {

// The Quit action leaves the room, or closes the window from anywhere else.
// Once the connection has ended, any new key or button closes it.
class QuitSystem final : public ampersand::core::ecs::ISystem {
 public:
  enum class Action : std::uint8_t {
    None,
    LeaveRoom,
    Quit,
  };

  explicit QuitSystem(ServerConnection& connection);
  void update(ampersand::core::ecs::World& world) override;

  [[nodiscard]] static Action actionFor(
      const ampersand::input::InputManager& manager,
      ServerConnection::State state, bool ended);

 private:
  ServerConnection& _connection;
};

}  // namespace rtype::client
