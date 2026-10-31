#include <algorithm>
#include <ampersand/core/components/Transform.hpp>
#include <ampersand/core/ecs/Entity.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <ampersand/core/math/Vec2.hpp>
#include <ampersand/input/Event.hpp>
#include <ampersand/render/Color.hpp>
#include <ampersand/render/FontId.hpp>
#include <ampersand/render/components/Text.hpp>
#include <cstddef>
#include <optional>
#include <rtype/client/LobbyMenu.hpp>
#include <rtype/client/ServerConnection.hpp>
#include <rtype/common/protocol/Packets.hpp>
#include <rtype/common/protocol/Protocol.hpp>
#include <string>
#include <utility>
#include <vector>

namespace rtype::client {

namespace core = ampersand::core;
namespace input = ampersand::input;
namespace render = ampersand::render;

LobbyMenu::LobbyMenu(core::ecs::World& world, render::FontId font)
    : _world(world), _font(font) {
  this->_title = this->addText({LEFT, TITLE_Y}, TITLE_SIZE, MESSAGE_COLOR);
  for (std::size_t index = 0; index < CHOICES; ++index) {
    this->_choices.at(index) = this->addText(
        {LEFT, CHOICES_Y + (static_cast<float>(index) * LINE_STEP)}, LINE_SIZE,
        TEXT_COLOR);
  }
  this->_header = this->addText({LEFT, HEADER_Y}, LINE_SIZE, DIM_COLOR);
  for (std::size_t index = 0; index < VISIBLE_ROOMS; ++index) {
    this->_roomLines.at(index) =
        this->addText({LEFT, ROOMS_Y + (static_cast<float>(index) * LINE_STEP)},
                      LINE_SIZE, TEXT_COLOR);
  }
  this->_message = this->addText({LEFT, MESSAGE_Y}, LINE_SIZE, MESSAGE_COLOR);
  this->_help = this->addText({LEFT, HELP_Y}, HELP_SIZE, DIM_COLOR);
}

void LobbyMenu::show(bool visible) {
  if (this->_visible != visible) {
    this->_visible = visible;
    this->refresh();
  }
}

void LobbyMenu::setRooms(const std::vector<protocol::RoomInfo>& rooms) {
  std::optional<std::uint32_t> chosenRoom;
  if (this->_selected >= CHOICES) {
    chosenRoom = this->_rooms[this->_selected - CHOICES].roomId;
  }
  this->_rooms = rooms;
  if (chosenRoom) {
    const auto found = std::ranges::find(this->_rooms, *chosenRoom,
                                         &protocol::RoomInfo::roomId);
    if (found != this->_rooms.end()) {
      this->_selected =
          CHOICES + static_cast<std::size_t>(found - this->_rooms.begin());
    }
  }
  this->_selected = std::min(this->_selected, this->lineCount() - 1);
  this->refresh();
}

void LobbyMenu::setMessage(std::string message) {
  if (this->_messageText != message) {
    this->_messageText = std::move(message);
    this->refresh();
  }
}

std::optional<RoomRequest> LobbyMenu::press(input::Key key) {
  std::optional<RoomRequest> request;
  this->_hint.clear();
  const bool typing =
      this->_selected == static_cast<std::size_t>(Choice::JoinCode);
  if (key == input::Key::Up) {
    this->_selected = this->_selected == 0 ? 0 : this->_selected - 1;
  } else if (key == input::Key::Down) {
    this->_selected = std::min(this->_selected + 1, this->lineCount() - 1);
  } else if (key == input::Key::Enter) {
    request = this->confirm();
  } else if (typing && key == input::Key::Backspace) {
    if (!this->_code.empty()) {
      this->_code.pop_back();
    }
  } else if (const auto character = characterOf(key);
             typing && character &&
             this->_code.size() < protocol::MAX_PARTY_CODE_LENGTH) {
    this->_code.push_back(*character);
  }
  this->refresh();
  return request;
}

std::size_t LobbyMenu::selected() const noexcept { return this->_selected; }

const std::string& LobbyMenu::code() const noexcept { return this->_code; }

std::vector<std::string> LobbyMenu::lines() const {
  std::vector<std::string> texts;
  texts.push_back(this->_world.get<render::Text>(this->_title).value);
  for (const core::ecs::Entity entity : this->_choices) {
    texts.push_back(this->_world.get<render::Text>(entity).value);
  }
  texts.push_back(this->_world.get<render::Text>(this->_header).value);
  for (const core::ecs::Entity entity : this->_roomLines) {
    texts.push_back(this->_world.get<render::Text>(entity).value);
  }
  texts.push_back(this->_world.get<render::Text>(this->_message).value);
  texts.push_back(this->_world.get<render::Text>(this->_help).value);
  return texts;
}

std::optional<char> LobbyMenu::characterOf(input::Key key) {
  const auto value = static_cast<int>(key);
  const auto first = static_cast<int>(input::Key::A);
  const auto firstDigit = static_cast<int>(input::Key::Num0);
  if (value >= first && value <= static_cast<int>(input::Key::Z)) {
    return static_cast<char>('A' + (value - first));
  }
  if (value >= firstDigit && value <= static_cast<int>(input::Key::Num9)) {
    return static_cast<char>('0' + (value - firstDigit));
  }
  return std::nullopt;
}

std::optional<RoomRequest> LobbyMenu::confirm() {
  if (this->_selected >= CHOICES) {
    return protocol::RoomJoin{
        .roomId = this->_rooms[this->_selected - CHOICES].roomId,
        .code = {},
    };
  }
  switch (static_cast<Choice>(this->_selected)) {
    case Choice::QuickMatch:
      return protocol::RoomJoin{};
    case Choice::CreatePublic:
      return protocol::RoomCreate{.visibility =
                                      protocol::RoomVisibility::Public};
    case Choice::CreatePrivate:
      return protocol::RoomCreate{
          .visibility = protocol::RoomVisibility::Private,
      };
    case Choice::JoinCode:
      if (this->_code.empty()) {
        this->_hint = "Type the code of the room first";
        return std::nullopt;
      }
      return protocol::RoomJoin{.roomId = 0, .code = this->_code};
  }
  return std::nullopt;
}

std::size_t LobbyMenu::lineCount() const noexcept {
  return CHOICES + this->_rooms.size();
}

std::string LobbyMenu::choiceText(Choice choice) const {
  switch (choice) {
    case Choice::QuickMatch:
      return "Quick match";
    case Choice::CreatePublic:
      return "Create a public room";
    case Choice::CreatePrivate:
      return "Create a private room";
    case Choice::JoinCode: {
      const bool typing =
          this->_selected == static_cast<std::size_t>(Choice::JoinCode);
      return "Join with a code: " + this->_code + (typing ? "_" : "");
    }
  }
  return {};
}

std::string LobbyMenu::roomText(const protocol::RoomInfo& room) {
  std::string text = "Room " + std::to_string(room.roomId) + "   " +
                     std::to_string(room.players) + "/" +
                     std::to_string(protocol::MAX_PLAYERS) + " players";
  if (room.players >= protocol::MAX_PLAYERS) {
    text += "   full";
  }
  return text;
}

std::size_t LobbyMenu::firstVisibleRoom() const noexcept {
  if (this->_selected < CHOICES || this->_selected - CHOICES < VISIBLE_ROOMS) {
    return 0;
  }
  return this->_selected - CHOICES - VISIBLE_ROOMS + 1;
}

core::ecs::Entity LobbyMenu::addText(core::math::Vec2f position,
                                     unsigned int size, render::Color color) {
  const core::ecs::Entity entity = this->_world.create();
  this->_world.add(entity, core::Transform{
                               .position = {position.x, position.y, LAYER},
                           });
  this->_world.add(entity, render::Text{
                               .font = this->_font,
                               .value = {},
                               .size = size,
                               .color = color,
                           });
  return entity;
}

void LobbyMenu::setLine(core::ecs::Entity entity, std::string value,
                        render::Color color) {
  auto& text = this->_world.get<render::Text>(entity);
  text.value = this->_visible ? std::move(value) : std::string{};
  text.color = color;
}

void LobbyMenu::refresh() {
  const auto marked = [this](std::size_t line, std::string text) {
    return (this->_selected == line ? "> " : "  ") + std::move(text);
  };
  const auto colorOf = [this](std::size_t line) {
    return this->_selected == line ? CHOSEN_COLOR : TEXT_COLOR;
  };

  this->setLine(this->_title, "LOBBY", MESSAGE_COLOR);
  for (std::size_t index = 0; index < CHOICES; ++index) {
    this->setLine(this->_choices.at(index),
                  marked(index, this->choiceText(static_cast<Choice>(index))),
                  colorOf(index));
  }
  std::string header = "Public rooms";
  if (this->_rooms.size() > VISIBLE_ROOMS) {
    header += " (" + std::to_string(this->_rooms.size()) + ")";
  }
  this->setLine(this->_header,
                this->_rooms.empty() ? "No public room yet" : header,
                DIM_COLOR);
  const std::size_t first = this->firstVisibleRoom();
  for (std::size_t index = 0; index < VISIBLE_ROOMS; ++index) {
    const std::size_t room = first + index;
    if (room >= this->_rooms.size()) {
      this->setLine(this->_roomLines.at(index), {}, TEXT_COLOR);
      continue;
    }
    const std::size_t line = CHOICES + room;
    this->setLine(this->_roomLines.at(index),
                  marked(line, roomText(this->_rooms[room])), colorOf(line));
  }
  this->setLine(this->_message,
                this->_messageText.empty() ? this->_hint : this->_messageText,
                MESSAGE_COLOR);
  this->setLine(this->_help,
                "Up/Down: choose   Enter: confirm   Esc: quit"
                "   (in a room, Esc comes back here)",
                DIM_COLOR);
}

}  // namespace rtype::client
