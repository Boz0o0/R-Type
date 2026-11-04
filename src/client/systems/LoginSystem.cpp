#include <ampersand/core/ecs/World.hpp>
#include <ampersand/input/Event.hpp>
#include <ampersand/input/resources/InputManager.hpp>
#include <rtype/client/Hud.hpp>
#include <rtype/client/LoginMenu.hpp>
#include <rtype/client/ServerConnection.hpp>
#include <rtype/client/systems/LoginSystem.hpp>
#include <utility>

namespace rtype::client {

using State = ServerConnection::State;

LoginSystem::LoginSystem(ServerConnection& connection, LoginMenu& menu)
    : _connection(connection), _menu(menu) {}

void LoginSystem::update(ampersand::core::ecs::World& world) {
  if (this->_connection.state() == State::LoggedOut) {
    const auto& manager = world.getResource<ampersand::input::InputManager>();
    const bool shift =
        manager.keysDown().contains(ampersand::input::KeyCode::LeftShift) ||
        manager.keysDown().contains(ampersand::input::KeyCode::RightShift);
    for (const ampersand::input::KeyCode key : manager.keysPressed()) {
      if (auto request = this->_menu.press(key, shift)) {
        this->_connection.logIn(std::move(request->username),
                                std::move(request->password),
                                request->createAccount);
        break;  // the next keys are not for the menu any more
      }
    }
  }
  this->show();
}

void LoginSystem::show() {
  const State state = this->_connection.state();
  this->_menu.show(state == State::LoggedOut || state == State::Connecting);
  if (state == State::Connecting) {
    this->_menu.setMessage("Connecting...");
  } else if (const auto reason = this->_connection.rejectReason();
             state == State::LoggedOut && reason) {
    this->_menu.setMessage(Hud::rejectText(reason));
  } else {
    this->_menu.setMessage({});
  }
}

}  // namespace rtype::client
