#include <ampersand/core/ecs/World.hpp>
#include <ampersand/input/resources/InputManager.hpp>
#include <ampersand/render/resources/Renderer.hpp>
#include <rtype/client/Controls.hpp>
#include <rtype/client/ServerConnection.hpp>
#include <rtype/client/systems/QuitSystem.hpp>
#include <string>

namespace rtype::client {

QuitSystem::QuitSystem(const ServerConnection& connection)
    : _connection(connection) {}

// Runs before NetworkSystem, so hasEnded() is still last frame's value: the
// key pressed in the frame the game ends does not close the end screen.
void QuitSystem::update(ampersand::core::ecs::World& world) {
  if (shouldQuit(world.getResource<ampersand::input::InputManager>(),
                 this->_connection.hasEnded())) {
    world.getResource<ampersand::render::Renderer>().isOpen = false;
  }
}

bool QuitSystem::shouldQuit(const ampersand::input::InputManager& manager,
                            bool ended) {
  if (manager.getButton(std::string(Controls::QUIT))) {
    return true;
  }
  return ended && (!manager.keysPressed().empty() ||
                   !manager.joystickButtonsPressed().empty());
}

}  // namespace rtype::client
