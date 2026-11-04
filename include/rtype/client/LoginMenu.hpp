#pragma once

#include <ampersand/core/ecs/Entity.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <ampersand/input/Event.hpp>
#include <ampersand/render/Color.hpp>
#include <ampersand/render/FontId.hpp>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace rtype::client {

/** @brief What the player asked for in the login menu. */
struct LoginRequest {
  std::string username;
  std::string password;
  /** @brief REGISTER instead of LOGIN. */
  bool createAccount{false};
};

/**
 * @brief The menu shown before the lobby: a username, a password (shown as
 *        stars), and the choice to log in or to create an account.
 *
 * Up, Down and Tab move between the lines. Letters (Shift for upper case)
 * and digits are typed in the selected field, Backspace erases. Enter goes
 * from the username to the password, and from the password (or "Log in")
 * logs in; on "Create an account" it registers. The engine only gives keys,
 * not text, so only ASCII letters and digits can be typed.
 */
class LoginMenu {
 public:
  /**
   * @brief Creates the text entities of the menu, empty until shown.
   *
   * @param world World the texts are drawn from.
   * @param font Font of the texts.
   * @param username Username filled in at start, from the command line.
   */
  LoginMenu(ampersand::core::ecs::World& world, ampersand::render::FontId font,
            std::string username = {});

  /** @brief Shows or hides the menu. */
  void show(bool visible);

  /** @brief Line shown under the menu: what is going on, or an error. */
  void setMessage(std::string message);

  /**
   * @brief Handles a key pressed by the player.
   *
   * @param key The key.
   * @param shift Whether Shift is held, for upper case letters.
   * @return The login asked for, if the key confirmed one.
   */
  [[nodiscard]] std::optional<LoginRequest> press(ampersand::input::KeyCode key,
                                                  bool shift);

  /** @brief Index of the selected line (see Line). */
  [[nodiscard]] std::size_t selected() const noexcept;
  [[nodiscard]] const std::string& username() const noexcept;
  [[nodiscard]] const std::string& password() const noexcept;
  /** @brief Every text of the menu, top to bottom, for the tests. */
  [[nodiscard]] std::vector<std::string> lines() const;

 private:
  /** @brief The lines the player can select. */
  enum class Line : std::uint8_t {
    Username,
    Password,
    LogIn,
    CreateAccount,
  };
  static constexpr std::size_t LINES = 4;

  static constexpr unsigned int TITLE_SIZE = 80;
  static constexpr unsigned int LINE_SIZE = 40;
  static constexpr unsigned int HELP_SIZE = 28;
  static constexpr float LEFT = 560.0F;
  static constexpr float TITLE_Y = 80.0F;
  static constexpr float LINES_Y = 300.0F;
  static constexpr float LINE_STEP = 70.0F;
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

  /** @brief The character a key types, if it types one. */
  [[nodiscard]] static std::optional<char> characterOf(
      ampersand::input::KeyCode key, bool shift);

  /** @brief Enter on the selected line. */
  [[nodiscard]] std::optional<LoginRequest> confirm(bool createAccount);

  /** @brief The field typed in on the selected line, if it is one. */
  [[nodiscard]] std::string* field() noexcept;

  [[nodiscard]] std::string lineText(Line line) const;

  void setLine(ampersand::core::ecs::Entity entity, std::string value,
               ampersand::render::Color color);

  void refresh();

  ampersand::core::ecs::World& _world;
  ampersand::core::ecs::Entity _title;
  std::array<ampersand::core::ecs::Entity, LINES> _lines{};
  ampersand::core::ecs::Entity _message;
  ampersand::core::ecs::Entity _help;
  bool _visible{false};
  std::size_t _selected{0};
  std::string _username;
  std::string _password;
  std::string _messageText;
  /** @brief Shown instead of the message after a key that did nothing. */
  std::string _hint;
};

}  // namespace rtype::client
