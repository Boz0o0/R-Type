#pragma once

#include <ampersand/audio/SfmlAudioEngine.hpp>
#include <ampersand/audio/systems/AudioSystem.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <ampersand/core/math/Vec2.hpp>
#include <ampersand/input/resources/InputManager.hpp>
#include <ampersand/input/systems/InputSystem.hpp>
#include <ampersand/physics/systems/MovementSystem.hpp>
#include <ampersand/render/Color.hpp>
#include <ampersand/render/SfmlRenderer.hpp>
#include <ampersand/render/systems/AnimationSystem.hpp>
#include <ampersand/render/systems/RenderSystem.hpp>
#include <cstdint>
#include <optional>
#include <rtype/client/ClientConfig.hpp>
#include <rtype/client/Hud.hpp>
#include <rtype/client/LobbyMenu.hpp>
#include <rtype/client/ServerConnection.hpp>
#include <rtype/client/WorldMirror.hpp>
#include <rtype/client/systems/ExplosionSystem.hpp>
#include <rtype/client/systems/ScrollingSystem.hpp>
#include <rtype/common/protocol/PacketCodec.hpp>

namespace rtype::client {

/**
 * @brief The client program: the window, the connection to the server and
 *        the frame loop.
 *
 * Each frame reads the keys, updates the ServerConnection, shows the lobby
 * menu or the HUD for its state, mirrors the game packets of the room in the
 * local world, sends the keys held, then runs the systems that move, animate,
 * play and draw the entities.
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
   * @brief Opens the window and runs the frame loop until it is closed, then
   *        tells the server the player quits. If a party code was given, it
   *        is joined as soon as the lobby is reached.
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

  /** @brief Longest frame time given to the systems, in seconds, so that a
   *         frozen window does not make the entities jump. */
  static constexpr float MAX_FRAME_TIME = 0.1F;

  /**
   * @brief Handles a key pressed this frame.
   *
   * Escape leaves the room, or quits from anywhere else. Once the connection
   * has ended, any key quits. In the lobby, the keys go to the menu and the
   * room it asks for is sent.
   *
   * @param key The key pressed.
   * @param menu The lobby menu.
   * @param windowOpen Set to false to quit.
   */
  void onKey(ampersand::input::KeyCode key, LobbyMenu& menu, bool& windowOpen);

  /**
   * @brief Shows the lobby menu while in the lobby, with the public rooms and
   *        what is going on (joining, leaving, why a room was refused).
   *
   * @param menu The lobby menu.
   */
  void showLobby(LobbyMenu& menu) const;

  /**
   * @brief Reads the events of the window for the InputSystem.
   *
   * Closing the window sets Renderer::isOpen to false; the other events the
   * engine does not use are skipped.
   *
   * @param world The world holding the Renderer resource.
   * @return The next keyboard, mouse or focus event, if any.
   */
  static std::optional<ampersand::input::Event> pollWindow(
      ampersand::core::ecs::World& world);

  /**
   * @brief Gives a game packet of the room to the HUD and to the mirror of
   *        the world.
   *
   * @param worldMirror The local copy of the entities of the server.
   * @param hud The HUD.
   * @param packet A game packet from ServerConnection::takePackets().
   */
  static void applyPacket(WorldMirror& worldMirror, Hud& hud,
                          const protocol::Payload& packet);

  /** @brief Options of the command line. */
  ClientConfig _config;
  /** @brief The link with the server. */
  ServerConnection _connection;
  /** @brief Set once the party code of the command line was sent. */
  bool _joinedPartyCode{false};
  /** @brief Window and drawing, through SFML. */
  ampersand::render::SfmlRenderer _backend;
  /** @brief Sounds, through SFML. */
  ampersand::audio::SfmlAudioEngine _audioEngine;
  /** @brief Every local entity: starfield, mirrored entities and texts. */
  ampersand::core::ecs::World _world;
  /** @brief Fills the InputManager from pollWindow(). */
  ampersand::input::InputSystem _inputSystem;
  /** @brief Moves the entities by their velocity. */
  ampersand::physics::MovementSystem _movementSystem;
  /** @brief Loops the starfield. */
  ScrollingSystem _scrollingSystem;
  /** @brief Plays the sprite animations. */
  ampersand::render::AnimationSystem _animationSystem;
  /** @brief Removes the explosions once played. */
  ExplosionSystem _explosionSystem;
  /** @brief Plays the sounds. */
  ampersand::audio::AudioSystem _audioSystem;
  /** @brief Draws the sprites and the texts. */
  ampersand::render::RenderSystem _renderSystem;
};

}  // namespace rtype::client
