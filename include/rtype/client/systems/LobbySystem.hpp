#pragma once

#include <ampersand/core/ecs/ISystem.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <rtype/client/LobbyMenu.hpp>
#include <rtype/client/ServerConnection.hpp>
#include <string>

namespace rtype::client {

// In the lobby: joins the party code of the command line once, gives the keys
// to the LobbyMenu and sends the room it asks for. Shows the menu while out
// of a room, with what is going on.
class LobbySystem final : public ampersand::core::ecs::ISystem {
 public:
  LobbySystem(ServerConnection& connection, LobbyMenu& menu,
              std::string partyCode);
  void update(ampersand::core::ecs::World& world) override;

 private:
  void show();

  ServerConnection& _connection;
  LobbyMenu& _menu;
  // Emptied once sent.
  std::string _partyCode;
};

}  // namespace rtype::client
