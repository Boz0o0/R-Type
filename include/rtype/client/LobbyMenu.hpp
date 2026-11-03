#pragma once

#include <ampersand/core/ecs/Entity.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <ampersand/core/math/Vec2.hpp>
#include <ampersand/input/Event.hpp>
#include <ampersand/render/Color.hpp>
#include <ampersand/render/FontId.hpp>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <rtype/client/ServerConnection.hpp>
#include <rtype/common/protocol/Packets.hpp>
#include <string>
#include <vector>

namespace rtype::client {

/**
 * @brief The lobby screen: a menu chosen with the arrows and Enter.
 *
 * The first lines are always there: quick match, create a public room,
 * create a private room, and join with a code (typed with the digits while
 * that line is chosen). Then come the public rooms of the server,
 * VISIBLE_ROOMS at a time. press() turns a key into the request to send, if
 * any.
 *
 * The engine gives keys by their QWERTY position, not the characters typed:
 * on AZERTY, the key marked Z is KeyCode::W and M is no key at all. So a code
 * is typed with the digits only, which have the same keys on every layout;
 * the codes the server makes are digits. A code with letters can still be
 * given on the command line.
 */
class LobbyMenu {
 public:
  /** @brief Rooms shown at once; the list scrolls with the choice. */
  static constexpr std::size_t VISIBLE_ROOMS = 7;

  /**
   * @brief Creates the text entities of the menu, hidden.
   *
   * @param world The world to create the texts in.
   * @param font Font of every text.
   */
  LobbyMenu(ampersand::core::ecs::World& world, ampersand::render::FontId font);

  /**
   * @brief Shows or hides the menu. Hidden, every text is empty.
   *
   * @param visible Whether the menu is on screen.
   */
  void show(bool visible);

  /**
   * @brief Replaces the list of the public rooms.
   *
   * Keeps the same room chosen if it is still in the list; if it is gone,
   * goes back to quick match. Does nothing if the list did not change.
   *
   * @param rooms The public rooms, from ServerConnection::rooms().
   */
  void setRooms(const std::vector<protocol::RoomInfo>& rooms);

  /**
   * @brief Sets the line under the menu ("Joining...", why a room was
   *        refused...).
   *
   * @param message The text, or empty to show the hints of the menu instead.
   */
  void setMessage(std::string message);

  /**
   * @brief Handles a key pressed.
   *
   * Up and Down choose a line, Enter confirms it. On the code line, the
   * digits type the code and Backspace erases.
   *
   * @param key The key pressed.
   * @return The request to send to the server when a line is confirmed, or
   *         nothing (also when the code line is confirmed with no code).
   */
  [[nodiscard]] std::optional<RoomRequest> press(ampersand::input::KeyCode key);

  /** @brief Index of the chosen line: the fixed lines first, then the rooms.
   */
  [[nodiscard]] std::size_t selected() const noexcept;
  /** @brief The party code typed so far. */
  [[nodiscard]] const std::string& code() const noexcept;
  /** @brief The texts on screen, top to bottom. */
  [[nodiscard]] std::vector<std::string> lines() const;

 private:
  /** @brief The fixed lines of the menu, in their order on screen. */
  enum class Choice : std::uint8_t {
    /** @brief Joins the first public room with a free place. */
    QuickMatch,
    /** @brief Opens a public room. */
    CreatePublic,
    /** @brief Opens a private room, with a code made by the server. */
    CreatePrivate,
    /** @brief Joins the private room of the code typed. */
    JoinCode,
  };
  /** @brief Number of fixed lines, before the rooms. */
  static constexpr std::size_t CHOICES = 4;

  /** @brief Character size of the title. */
  static constexpr unsigned int TITLE_SIZE = 80;
  /** @brief Character size of the choices, the rooms and the message. */
  static constexpr unsigned int LINE_SIZE = 40;
  /** @brief Character size of the key help. */
  static constexpr unsigned int HELP_SIZE = 28;
  /** @brief X of every text. */
  static constexpr float LEFT = 560.0F;
  /** @brief Y of the title. */
  static constexpr float TITLE_Y = 80.0F;
  /** @brief Y of the first fixed line. */
  static constexpr float CHOICES_Y = 240.0F;
  /** @brief Y of the header of the room list. */
  static constexpr float HEADER_Y = 510.0F;
  /** @brief Y of the first room shown. */
  static constexpr float ROOMS_Y = 570.0F;
  /** @brief Space between two lines of a list. */
  static constexpr float LINE_STEP = 50.0F;
  /** @brief Y of the message line. */
  static constexpr float MESSAGE_Y = 950.0F;
  /** @brief Y of the key help. */
  static constexpr float HELP_Y = 1010.0F;
  /** @brief Color of the lines not chosen. */
  static constexpr ampersand::render::Color TEXT_COLOR{
      .r = 255, .g = 255, .b = 255};
  /** @brief Color of the chosen line. */
  static constexpr ampersand::render::Color CHOSEN_COLOR{
      .r = 90, .g = 220, .b = 255};
  /** @brief Color of the header and of the key help. */
  static constexpr ampersand::render::Color DIM_COLOR{
      .r = 150, .g = 150, .b = 170};
  /** @brief Color of the title and of the message. */
  static constexpr ampersand::render::Color MESSAGE_COLOR{
      .r = 255, .g = 220, .b = 80};

  /**
   * @brief The digit of a key.
   *
   * @param key A key of the keyboard.
   * @return '0' to '9' for the digit keys of the top row, nothing otherwise.
   */
  [[nodiscard]] static std::optional<char> digitOf(
      ampersand::input::KeyCode key);

  /**
   * @brief Confirms the chosen line.
   *
   * @return The request of that line, or nothing if the code line has no
   *         code yet (a hint is then shown).
   */
  [[nodiscard]] std::optional<RoomRequest> confirm();

  /** @brief Number of lines that can be chosen: the fixed ones and the
   *         rooms. */
  [[nodiscard]] std::size_t lineCount() const noexcept;

  /**
   * @brief Text of a fixed line.
   *
   * @param choice The line.
   * @return Its label; the code line shows the code typed, and a cursor
   *         while it is chosen.
   */
  [[nodiscard]] std::string choiceText(Choice choice) const;

  /**
   * @brief Text of a room line.
   *
   * @param room The room.
   * @return Its number and its players, and whether it is full.
   */
  [[nodiscard]] static std::string roomText(const protocol::RoomInfo& room);

  /** @brief First room shown, so that the chosen one is on screen. */
  [[nodiscard]] std::size_t firstVisibleRoom() const noexcept;

  /**
   * @brief Writes a line of the menu, empty while hidden.
   *
   * @param entity The text entity of the line.
   * @param value Its text.
   * @param color Its color.
   */
  void setLine(ampersand::core::ecs::Entity entity, std::string value,
               ampersand::render::Color color);

  /** @brief Writes every text from the state of the menu. */
  void refresh();

  /** @brief The world holding the texts. */
  ampersand::core::ecs::World& _world;
  /** @brief Text of the title. */
  ampersand::core::ecs::Entity _title;
  /** @brief Texts of the fixed lines, in the order of Choice. */
  std::array<ampersand::core::ecs::Entity, CHOICES> _choices{};
  /** @brief Text above the room list. */
  ampersand::core::ecs::Entity _header;
  /** @brief Texts of the rooms on screen. */
  std::array<ampersand::core::ecs::Entity, VISIBLE_ROOMS> _roomLines{};
  /** @brief Text of the message line. */
  ampersand::core::ecs::Entity _message;
  /** @brief Text of the key help. */
  ampersand::core::ecs::Entity _help;
  /** @brief Whether the menu is on screen. */
  bool _visible{false};
  /** @brief Index of the chosen line, see selected(). */
  std::size_t _selected{0};
  /** @brief The party code typed so far. */
  std::string _code;
  /** @brief Given by setMessage(); shown instead of the hint. */
  std::string _messageText;
  /** @brief Set by the menu itself, until the next key. */
  std::string _hint;
  /** @brief The public rooms of the server. */
  std::vector<protocol::RoomInfo> _rooms;
};

}  // namespace rtype::client
