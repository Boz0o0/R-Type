#pragma once

#include <ampersand/core/ecs/World.hpp>
#include <ampersand/input/resources/InputManager.hpp>
#include <cstdint>
#include <optional>
#include <rtype/client/ClientConfig.hpp>
#include <rtype/client/ServerConnection.hpp>
#include <string_view>

namespace rtype::client {

/**
 * @brief The client program: the window, the connection to the server and
 *        the systems run each frame by the engine Scheduler.
 *
 * Each frame reads the inputs, handles quitting, talks to the server (lobby,
 * room, game packets, our input), then moves, animates, plays and draws the
 * entities.
 */
class ClientApp {
 public:
  /** @brief Exit code of the program when it fails. */
  static constexpr int32_t EXIT_ERROR = 84;

  /**
   * @brief Prepares the connection to the server of the configuration.
   *        Nothing is sent and no window is opened before run().
   *
   * @param config Options of the command line.
   * @throws ampersand::net::NetError If the host cannot be resolved.
   */
  explicit ClientApp(ClientConfig config);

  /**
   * @brief Opens the window and runs the Scheduler until the window is
   *        closed, then tells the server the player quits. If a party code
   *        was given, it is joined as soon as the lobby is reached.
   *
   * @throws std::runtime_error If the window cannot be opened, or if the
   *         connection to the server ended.
   */
  void run();

 private:
  /** @brief The window, inputs, sounds and star field: see game/world/. */
  static constexpr std::string_view CONFIG = "game/world/config.json";
  /** @brief Font of the HUD and the lobby, in the assets of the game. */
  static constexpr std::string_view FONT = "assets/fonts/tuffy.ttf";

  /**
   * @brief Reads the events of the window for the InputSystem.
   *
   * Closing the window sets Renderer::isOpen to false; the other events go
   * to the InputManager if it knows them (keys, mouse, controllers).
   *
   * @param world The world holding the Renderer resource.
   * @return The next input event, if any.
   */
  static std::optional<ampersand::input::Event> pollWindow(
      ampersand::core::ecs::World& world);

  /** @brief Options of the command line. */
  ClientConfig _config;
  /** @brief The link with the server. */
  ServerConnection _connection;
  /** @brief Every local entity: starfield, mirrored entities and texts. */
  ampersand::core::ecs::World _world;
};

}  // namespace rtype::client
