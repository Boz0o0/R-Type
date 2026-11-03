#pragma once

#include <ampersand/core/ecs/Entity.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <ampersand/core/math/Vec2.hpp>
#include <ampersand/render/Color.hpp>
#include <ampersand/render/FontId.hpp>
#include <array>
#include <cstdint>
#include <optional>
#include <rtype/client/ServerConnection.hpp>
#include <rtype/common/protocol/PacketCodec.hpp>
#include <rtype/common/protocol/Packets.hpp>
#include <rtype/common/protocol/Protocol.hpp>
#include <string>

namespace rtype::client {

/**
 * @brief The texts drawn over the game: our player and room, our stats, the
 *        last event of the room, and a banner for the end of a game or of
 *        the connection.
 */
class Hud {
 public:
  /** @brief Color of each player, by player id. */
  static constexpr std::array<ampersand::render::Color, protocol::MAX_PLAYERS>
      PLAYER_COLORS{{
          {.r = 90, .g = 220, .b = 255},
          {.r = 255, .g = 110, .b = 255},
          {.r = 110, .g = 255, .b = 110},
          {.r = 255, .g = 110, .b = 110},
      }};

  /**
   * @brief Creates the text entities of the HUD, empty.
   *
   * @param world The world to create the texts in.
   * @param font Font of every text.
   */
  Hud(ampersand::core::ecs::World& world, ampersand::render::FontId font);

  /**
   * @brief Shows the state of the connection.
   *
   * In the lobby the HUD is empty, the LobbyMenu is shown instead. In a room
   * it shows our player and the room (its code if it is private). Once the
   * connection has ended, the banner tells why.
   *
   * @param state State of the ServerConnection.
   * @param room The room we are in, if any.
   * @param rejectReason Why the server refused the connection, if it did.
   */
  void showConnection(ServerConnection::State state,
                      const std::optional<protocol::RoomJoined>& room,
                      std::optional<protocol::RejectReason> rejectReason);

  /**
   * @brief Updates the texts from a game packet: players joining or leaving,
   *        our stats, the start and the end of a game. Other packets are
   *        ignored.
   *
   * @param packet A game packet of the room.
   */
  void apply(const protocol::Payload& packet);

  /**
   * @brief Text telling why the server refused us.
   *
   * @param reason The reason given, if any.
   * @return A short sentence for the player.
   */
  [[nodiscard]] static std::string rejectText(
      std::optional<protocol::RejectReason> reason);

  /** @brief Text with our player and room. */
  [[nodiscard]] const std::string& status() const;
  /** @brief Text with our health, lives and score. */
  [[nodiscard]] const std::string& stats() const;
  /** @brief Text with the last event of the room. */
  [[nodiscard]] const std::string& event() const;
  /** @brief Text in the middle of the screen. */
  [[nodiscard]] const std::string& banner() const;

 private:
  /** @brief Character size of the status, stats and event. */
  static constexpr unsigned int TEXT_SIZE = 36;
  /** @brief Character size of the banner. */
  static constexpr unsigned int BANNER_SIZE = 80;
  /** @brief Space between the texts and the edges of the screen. */
  static constexpr float MARGIN = 30.0F;
  /** @brief X of the stats. */
  static constexpr float STATS_X = 1300.0F;
  /** @brief Y of the event. */
  static constexpr float EVENT_Y = 1020.0F;
  /** @brief Y of the banner. */
  static constexpr float BANNER_Y = 480.0F;
  /** @brief X the banner is centered on. */
  static constexpr float SCREEN_CENTER_X = 960.0F;
  /** @brief Color of the banner. */
  static constexpr ampersand::render::Color BANNER_COLOR{
      .r = 255, .g = 220, .b = 80};
  /** @brief Width of a letter, as a part of the character size, to center
   *         the banner. */
  static constexpr float LETTER_WIDTH = 0.55F;
  /** @brief Shown once the connection has ended. */
  static constexpr const char* QUIT_HINT = "Press any key to quit";

  /**
   * @brief Label of a player.
   *
   * @param playerId Id of the player, from 0.
   * @return "P1" to "P4".
   */
  [[nodiscard]] static std::string playerLabel(std::uint8_t playerId);

  /**
   * @brief Label of a room.
   *
   * @param room The room joined.
   * @return Its code if it is private, its number otherwise.
   */
  [[nodiscard]] static std::string roomLabel(const protocol::RoomJoined& room);

  /**
   * @brief End of the sentence telling a player left.
   *
   * @param reason Why the player left.
   * @return " lost connection", " was kicked" or " left".
   */
  [[nodiscard]] static std::string leftText(protocol::DisconnectReason reason);

  /**
   * @brief Whether the lobby menu takes the screen.
   *
   * @param state State of the ServerConnection.
   * @return True in the lobby, and while joining or leaving a room.
   */
  [[nodiscard]] static bool isLobby(ServerConnection::State state);

  /** @brief Empties every text, for the lobby. */
  void clear();

  /**
   * @brief Writes a text of the HUD.
   *
   * @param entity The text entity.
   * @param value Its text.
   */
  void setText(ampersand::core::ecs::Entity entity, std::string value);

  /**
   * @brief Writes the banner, centered. Kept as it is once the game is over,
   *        until the next game starts.
   *
   * @param value Its text.
   */
  void setBanner(std::string value);

  /** @brief The world holding the texts. */
  ampersand::core::ecs::World& _world;
  /** @brief Text with our player and room. */
  ampersand::core::ecs::Entity _status;
  /** @brief Text with our health, lives and score. */
  ampersand::core::ecs::Entity _stats;
  /** @brief Text with the last event of the room. */
  ampersand::core::ecs::Entity _event;
  /** @brief Text in the middle of the screen. */
  ampersand::core::ecs::Entity _banner;
  /** @brief Our player id in the room, once known. */
  std::optional<std::uint8_t> _playerId;
  /** @brief Set by GAME_OVER: the banner keeps the result. */
  bool _gameOver{false};
  /** @brief State of the last showConnection(), to clear the HUD once on the
   *         way to the lobby: the packets of the next room may already come
   *         while Joining. */
  std::optional<ServerConnection::State> _shownState;
};

}  // namespace rtype::client
