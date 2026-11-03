#include <ampersand/core/ecs/World.hpp>
#include <ampersand/input/Event.hpp>
#include <ampersand/input/resources/InputManager.hpp>
#include <rtype/client/Hud.hpp>
#include <rtype/client/LobbyMenu.hpp>
#include <rtype/client/ServerConnection.hpp>
#include <rtype/client/systems/LobbySystem.hpp>
#include <rtype/common/protocol/Packets.hpp>
#include <string>
#include <utility>

namespace rtype::client {

using State = ServerConnection::State;

LobbySystem::LobbySystem(ServerConnection& connection, LobbyMenu& menu,
                         std::string partyCode)
    : _connection(connection), _menu(menu), _partyCode(std::move(partyCode)) {}

void LobbySystem::update(ampersand::core::ecs::World& world) {
  if (this->_connection.state() == State::InLobby) {
    if (!this->_partyCode.empty()) {
      this->_connection.enterRoom(protocol::RoomJoin{
          .roomId = 0,
          .code = std::exchange(this->_partyCode, {}),
      });
    }
    for (const ampersand::input::KeyCode key :
         world.getResource<ampersand::input::InputManager>().keysPressed()) {
      if (const auto request = this->_menu.press(key)) {
        this->_connection.enterRoom(*request);
      }
    }
  }
  this->show();
}

void LobbySystem::show() {
  const State state = this->_connection.state();
  this->_menu.show(state == State::InLobby || state == State::Joining ||
                   state == State::Leaving);
  this->_menu.setRooms(this->_connection.rooms());
  if (state == State::Joining) {
    this->_menu.setMessage("Joining...");
  } else if (state == State::Leaving) {
    this->_menu.setMessage("Leaving the room...");
  } else if (const auto reason = this->_connection.roomRejectReason()) {
    this->_menu.setMessage(Hud::rejectText(reason));
  } else {
    this->_menu.setMessage({});
  }
}

}  // namespace rtype::client
