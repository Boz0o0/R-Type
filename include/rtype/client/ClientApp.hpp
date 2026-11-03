#pragma once

#include <ampersand/core/ecs/World.hpp>
#include <ampersand/input/resources/InputManager.hpp>
#include <cstdint>
#include <optional>
#include <rtype/client/ClientConfig.hpp>
#include <rtype/client/ServerConnection.hpp>
#include <string_view>

namespace rtype::client {

class ClientApp {
 public:
  static constexpr int32_t EXIT_ERROR = 84;

  explicit ClientApp(ClientConfig config);
  void run();

 private:
  // The window, inputs, sounds and star field: see game/world/.
  static constexpr std::string_view CONFIG = "game/world/config.json";
  static constexpr std::string_view FONT = "assets/fonts/tuffy.ttf";

  static std::optional<ampersand::input::Event> pollWindow(
      ampersand::core::ecs::World& world);

  ClientConfig _config;
  ServerConnection _connection;
  ampersand::core::ecs::World _world;
};

}  // namespace rtype::client
