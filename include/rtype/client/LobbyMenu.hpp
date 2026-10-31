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

// The lobby screen: a menu chosen with the arrows and Enter.
//
// The first lines are always there: quick match, create a public room,
// create a private room, and join with a code (typed with the letters and
// digits while that line is chosen). Then come the public rooms of the
// server, VISIBLE_ROOMS at a time. press() turns a key into the request to
// send, if any.
class LobbyMenu {
 public:
  // Rooms shown at once; the list scrolls with the choice.
  static constexpr std::size_t VISIBLE_ROOMS = 7;

  LobbyMenu(ampersand::core::ecs::World& world, ampersand::render::FontId font);

  // Hidden, every text is empty.
  void show(bool visible);
  // Keeps the same room chosen if it is still in the list.
  void setRooms(const std::vector<protocol::RoomInfo>& rooms);
  // A line under the menu ("Joining...", why a room was refused...); empty
  // to show the hints of the menu instead.
  void setMessage(std::string message);

  // Up and Down choose a line, Enter confirms it. On the code line, letters
  // and digits type the code and Backspace erases.
  [[nodiscard]] std::optional<RoomRequest> press(ampersand::input::Key key);

  // Index of the chosen line: the fixed lines first, then the rooms.
  [[nodiscard]] std::size_t selected() const noexcept;
  [[nodiscard]] const std::string& code() const noexcept;
  // The texts on screen, top to bottom.
  [[nodiscard]] std::vector<std::string> lines() const;

 private:
  enum class Choice : std::uint8_t {
    QuickMatch,
    CreatePublic,
    CreatePrivate,
    JoinCode,
  };
  static constexpr std::size_t CHOICES = 4;

  static constexpr unsigned int TITLE_SIZE = 80;
  static constexpr unsigned int LINE_SIZE = 40;
  static constexpr unsigned int HELP_SIZE = 28;
  static constexpr float LAYER = 10.0F;
  static constexpr float LEFT = 560.0F;
  static constexpr float TITLE_Y = 80.0F;
  static constexpr float CHOICES_Y = 240.0F;
  static constexpr float HEADER_Y = 510.0F;
  static constexpr float ROOMS_Y = 570.0F;
  static constexpr float LINE_STEP = 50.0F;
  static constexpr float MESSAGE_Y = 950.0F;
  static constexpr float HELP_Y = 1010.0F;
  static constexpr ampersand::render::Color TEXT_COLOR{
      .r = 255, .g = 255, .b = 255};
  static constexpr ampersand::render::Color CHOSEN_COLOR{
      .r = 90, .g = 220, .b = 255};
  static constexpr ampersand::render::Color DIM_COLOR{
      .r = 150, .g = 150, .b = 170};
  static constexpr ampersand::render::Color MESSAGE_COLOR{
      .r = 255, .g = 220, .b = 80};

  // The letter or digit of a key, in upper case.
  [[nodiscard]] static std::optional<char> characterOf(
      ampersand::input::Key key);
  [[nodiscard]] std::optional<RoomRequest> confirm();
  [[nodiscard]] std::size_t lineCount() const noexcept;
  [[nodiscard]] std::string choiceText(Choice choice) const;
  [[nodiscard]] static std::string roomText(const protocol::RoomInfo& room);
  // First room shown, so that the chosen one is on screen.
  [[nodiscard]] std::size_t firstVisibleRoom() const noexcept;

  ampersand::core::ecs::Entity addText(ampersand::core::math::Vec2f position,
                                       unsigned int size,
                                       ampersand::render::Color color);
  void setLine(ampersand::core::ecs::Entity entity, std::string value,
               ampersand::render::Color color);
  // Writes every text from the state of the menu.
  void refresh();

  ampersand::core::ecs::World& _world;
  ampersand::render::FontId _font;
  ampersand::core::ecs::Entity _title;
  std::array<ampersand::core::ecs::Entity, CHOICES> _choices{};
  ampersand::core::ecs::Entity _header;
  std::array<ampersand::core::ecs::Entity, VISIBLE_ROOMS> _roomLines{};
  ampersand::core::ecs::Entity _message;
  ampersand::core::ecs::Entity _help;
  bool _visible{false};
  std::size_t _selected{0};
  std::string _code;
  // Given by setMessage(); shown instead of the hint.
  std::string _messageText;
  // Set by the menu itself, until the next key.
  std::string _hint;
  std::vector<protocol::RoomInfo> _rooms;
};

}  // namespace rtype::client
