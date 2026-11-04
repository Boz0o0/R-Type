#pragma once

#include <ampersand/core/ecs/ISystem.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <rtype/client/LoginMenu.hpp>
#include <rtype/client/ServerConnection.hpp>

namespace rtype::client {

// Before the lobby: gives the keys to the LoginMenu and logs in with what it
// asks for. Shows the menu while logged out or connecting, with what is going
// on, or why the server refused the account.
class LoginSystem final : public ampersand::core::ecs::ISystem {
 public:
  LoginSystem(ServerConnection& connection, LoginMenu& menu);
  void update(ampersand::core::ecs::World& world) override;

 private:
  void show();

  ServerConnection& _connection;
  LoginMenu& _menu;
};

}  // namespace rtype::client
