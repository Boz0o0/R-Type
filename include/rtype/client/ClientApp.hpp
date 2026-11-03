#pragma once

#include <ampersand/audio/SfmlAudioEngine.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <ampersand/core/math/Vec2.hpp>
#include <ampersand/input/resources/InputManager.hpp>
#include <ampersand/render/Color.hpp>
#include <ampersand/render/SfmlRenderer.hpp>
#include <cstdint>
#include <optional>
#include <rtype/client/ClientConfig.hpp>
#include <rtype/client/ServerConnection.hpp>

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
  /** @brief Width of the world, in world units. */
  static constexpr float WORLD_WIDTH = 1920.0F;
  /** @brief Height of the world, in world units. */
  static constexpr float WORLD_HEIGHT = 1080.0F;
  /** @brief Size of the window, in pixels; the world is scaled to fit it. */
  static constexpr ampersand::core::math::Vec2u WINDOW_SIZE{1280, 720};
  /** @brief Background color, behind the starfield. */
  static constexpr ampersand::render::Color SPACE_COLOR{
      .r = 4,
      .g = 4,
      .b = 14,
  };

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
  /** @brief Window and drawing, through SFML. */
  ampersand::render::SfmlRenderer _backend;
  /** @brief Sounds, through SFML. */
  ampersand::audio::SfmlAudioEngine _audioEngine;
  /** @brief Every local entity: starfield, mirrored entities and texts. */
  ampersand::core::ecs::World _world;
};

}  // namespace rtype::client
