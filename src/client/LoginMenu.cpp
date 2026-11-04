#include <algorithm>
#include <ampersand/core/ecs/Entity.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <ampersand/input/Event.hpp>
#include <ampersand/render/Color.hpp>
#include <ampersand/render/FontId.hpp>
#include <ampersand/render/components/Text.hpp>
#include <cstddef>
#include <optional>
#include <rtype/client/LoginMenu.hpp>
#include <rtype/client/TextEntity.hpp>
#include <rtype/common/protocol/Protocol.hpp>
#include <string>
#include <utility>
#include <vector>

namespace rtype::client {

namespace core = ampersand::core;
namespace input = ampersand::input;
namespace render = ampersand::render;

LoginMenu::LoginMenu(core::ecs::World& world, render::FontId font,
                     std::string username)
    : _world(world),
      _title(
          createText(world, font, {LEFT, TITLE_Y}, TITLE_SIZE, MESSAGE_COLOR)),
      _message(
          createText(world, font, {LEFT, MESSAGE_Y}, LINE_SIZE, MESSAGE_COLOR)),
      _help(createText(world, font, {LEFT, HELP_Y}, HELP_SIZE, DIM_COLOR)),
      _username(std::move(username)) {
  if (this->_username.size() > protocol::MAX_PLAYER_NAME_LENGTH) {
    this->_username.clear();
  }
  for (std::size_t index = 0; index < LINES; ++index) {
    this->_lines.at(index) = createText(
        world, font, {LEFT, LINES_Y + (static_cast<float>(index) * LINE_STEP)},
        LINE_SIZE, TEXT_COLOR);
  }
  // Straight to the password when the username came from the command line.
  if (!this->_username.empty()) {
    this->_selected = static_cast<std::size_t>(Line::Password);
  }
}

void LoginMenu::show(bool visible) {
  if (this->_visible != visible) {
    this->_visible = visible;
    this->refresh();
  }
}

void LoginMenu::setMessage(std::string message) {
  if (this->_messageText != message) {
    this->_messageText = std::move(message);
    this->refresh();
  }
}

std::optional<LoginRequest> LoginMenu::press(input::KeyCode key, bool shift) {
  std::optional<LoginRequest> request;
  this->_hint.clear();
  std::string* const typed = this->field();
  if (key == input::KeyCode::UpArrow) {
    this->_selected = this->_selected == 0 ? 0 : this->_selected - 1;
  } else if (key == input::KeyCode::DownArrow) {
    this->_selected = std::min(this->_selected + 1, LINES - 1);
  } else if (key == input::KeyCode::Tab) {
    this->_selected = (this->_selected + 1) % LINES;
  } else if (key == input::KeyCode::Return) {
    if (this->_selected == static_cast<std::size_t>(Line::Username)) {
      this->_selected = static_cast<std::size_t>(Line::Password);
    } else {
      request = this->confirm(this->_selected ==
                              static_cast<std::size_t>(Line::CreateAccount));
    }
  } else if (typed != nullptr && key == input::KeyCode::Backspace) {
    if (!typed->empty()) {
      typed->pop_back();
    }
  } else if (const auto character = characterOf(key, shift);
             typed != nullptr && character) {
    const std::size_t limit = typed == &this->_username
                                  ? protocol::MAX_PLAYER_NAME_LENGTH
                                  : protocol::MAX_PASSWORD_LENGTH;
    if (typed->size() < limit) {
      typed->push_back(*character);
    }
  }
  this->refresh();
  return request;
}

std::size_t LoginMenu::selected() const noexcept { return this->_selected; }

const std::string& LoginMenu::username() const noexcept {
  return this->_username;
}

const std::string& LoginMenu::password() const noexcept {
  return this->_password;
}

std::vector<std::string> LoginMenu::lines() const {
  std::vector<std::string> texts;
  texts.push_back(this->_world.get<render::Text>(this->_title).value);
  for (const core::ecs::Entity entity : this->_lines) {
    texts.push_back(this->_world.get<render::Text>(entity).value);
  }
  texts.push_back(this->_world.get<render::Text>(this->_message).value);
  texts.push_back(this->_world.get<render::Text>(this->_help).value);
  return texts;
}

std::optional<char> LoginMenu::characterOf(input::KeyCode key, bool shift) {
  const auto value = static_cast<int>(key);
  const auto letterA = static_cast<int>(input::KeyCode::A);
  const auto digit0 = static_cast<int>(input::KeyCode::Alpha0);
  if (value >= letterA && value <= static_cast<int>(input::KeyCode::Z)) {
    return static_cast<char>((shift ? 'A' : 'a') + (value - letterA));
  }
  if (value >= digit0 && value <= static_cast<int>(input::KeyCode::Alpha9)) {
    return static_cast<char>('0' + (value - digit0));
  }
  return std::nullopt;
}

std::optional<LoginRequest> LoginMenu::confirm(bool createAccount) {
  if (this->_username.empty()) {
    this->_hint = "Type your username first";
    this->_selected = static_cast<std::size_t>(Line::Username);
    return std::nullopt;
  }
  if (this->_password.empty()) {
    this->_hint = "Type your password first";
    this->_selected = static_cast<std::size_t>(Line::Password);
    return std::nullopt;
  }
  // The password is not kept here once sent.
  return LoginRequest{
      .username = this->_username,
      .password = std::exchange(this->_password, {}),
      .createAccount = createAccount,
  };
}

std::string* LoginMenu::field() noexcept {
  switch (static_cast<Line>(this->_selected)) {
    case Line::Username:
      return &this->_username;
    case Line::Password:
      return &this->_password;
    case Line::LogIn:
    case Line::CreateAccount:
      break;
  }
  return nullptr;
}

std::string LoginMenu::lineText(Line line) const {
  const bool typing = this->_selected == static_cast<std::size_t>(line);
  const std::string cursor = typing ? "_" : "";
  switch (line) {
    case Line::Username:
      return "Username: " + this->_username + cursor;
    case Line::Password:
      return "Password: " + std::string(this->_password.size(), '*') + cursor;
    case Line::LogIn:
      return "Log in";
    case Line::CreateAccount:
      return "Create an account";
  }
  return {};
}

void LoginMenu::setLine(core::ecs::Entity entity, std::string value,
                        render::Color color) {
  auto& text = this->_world.get<render::Text>(entity);
  text.value = this->_visible ? std::move(value) : std::string{};
  text.color = color;
}

void LoginMenu::refresh() {
  this->setLine(this->_title, "R-TYPE", MESSAGE_COLOR);
  for (std::size_t index = 0; index < LINES; ++index) {
    const bool chosen = this->_selected == index;
    this->setLine(
        this->_lines.at(index),
        (chosen ? "> " : "  ") + this->lineText(static_cast<Line>(index)),
        chosen ? CHOSEN_COLOR : TEXT_COLOR);
  }
  this->setLine(this->_message,
                this->_hint.empty() ? this->_messageText : this->_hint,
                MESSAGE_COLOR);
  this->setLine(this->_help,
                "Up/Down/Tab: choose   Shift: upper case   Enter: confirm"
                "   Esc: quit",
                DIM_COLOR);
}

}  // namespace rtype::client
