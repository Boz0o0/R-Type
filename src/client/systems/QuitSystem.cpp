#include <ampersand/core/ecs/World.hpp>
#include <ampersand/input/resources/InputManager.hpp>
#include <ampersand/render/resources/Renderer.hpp>
#include <rtype/client/Controls.hpp>
#include <rtype/client/ServerConnection.hpp>
#include <rtype/client/systems/QuitSystem.hpp>
#include <string>

namespace rtype::client {

QuitSystem::QuitSystem(ServerConnection& connection)
    : _connection(connection) {}

// Runs before NetworkSystem, so hasEnded() is still last frame's value: the
// key pressed in the frame the game ends does not close the end screen.
void QuitSystem::update(ampersand::core::ecs::World& world) {
  switch (actionFor(world.getResource<ampersand::input::InputManager>(),
                    this->_connection.state(), this->_connection.hasEnded())) {
    case Action::LeaveRoom:
      this->_connection.leaveRoom();
      break;
    case Action::Quit:
      world.getResource<ampersand::render::Renderer>().isOpen = false;
      break;
    case Action::None:
      break;
  }
}

// Pressed this frame, not held: the press that leaves the room must not also
// quit from the lobby on the next frame.
QuitSystem::Action QuitSystem::actionFor(
    const ampersand::input::InputManager& manager,
    ServerConnection::State state, bool ended) {
  if (ended) {
    const bool pressed = !manager.keysPressed().empty() ||
                         !manager.joystickButtonsPressed().empty();
    return pressed ? Action::Quit : Action::None;
  }
  if (!manager.getButtonDown(std::string(Controls::QUIT))) {
    return Action::None;
  }
  return state == ServerConnection::State::InRoom ? Action::LeaveRoom
                                                  : Action::Quit;
}

}  // namespace rtype::client
