#include <gtest/gtest.h>

#include <algorithm>
#include <ampersand/core/ecs/World.hpp>
#include <ampersand/input/Event.hpp>
#include <cstddef>
#include <optional>
#include <rtype/client/LoginMenu.hpp>
#include <rtype/common/protocol/Protocol.hpp>
#include <string>

using ampersand::core::ecs::World;
using ampersand::input::KeyCode;
using rtype::client::LoginMenu;
using rtype::client::LoginRequest;

namespace {

// The lines of the menu.
constexpr std::size_t USERNAME = 0;
constexpr std::size_t PASSWORD = 1;
constexpr std::size_t CREATE_ACCOUNT = 3;

bool shows(const LoginMenu& menu, const std::string& text) {
  const auto lines = menu.lines();
  return std::ranges::find(lines, text) != lines.end();
}

void type(LoginMenu& menu, std::initializer_list<KeyCode> keys,
          bool shift = false) {
  for (const KeyCode key : keys) {
    EXPECT_FALSE(menu.press(key, shift).has_value());
  }
}

}  // namespace

TEST(LoginMenu, ShowsNothingUntilShown) {
  World world;
  LoginMenu menu(world, 0);

  for (const std::string& line : menu.lines()) {
    EXPECT_TRUE(line.empty()) << line;
  }
  menu.show(true);
  EXPECT_EQ(menu.lines().front(), "R-TYPE");
}

TEST(LoginMenu, TypesLettersAndDigitsInTheUsername) {
  World world;
  LoginMenu menu(world, 0);
  menu.show(true);

  type(menu, {KeyCode::B}, true);
  type(menu, {KeyCode::O, KeyCode::B, KeyCode::Alpha4, KeyCode::Space});

  EXPECT_EQ(menu.username(), "Bob4");
  EXPECT_TRUE(shows(menu, "> Username: Bob4_"));
}

TEST(LoginMenu, HidesThePassword) {
  World world;
  LoginMenu menu(world, 0);
  menu.show(true);
  type(menu, {KeyCode::Tab, KeyCode::S, KeyCode::E, KeyCode::C});

  EXPECT_EQ(menu.password(), "sec");
  EXPECT_TRUE(shows(menu, "> Password: ***_"));
}

TEST(LoginMenu, ErasesWithBackspace) {
  World world;
  LoginMenu menu(world, 0);
  type(menu, {KeyCode::A, KeyCode::B, KeyCode::Backspace});

  EXPECT_EQ(menu.username(), "a");
}

TEST(LoginMenu, StopsAtTheLongestUsername) {
  World world;
  LoginMenu menu(world, 0);
  for (std::size_t index = 0;
       index < rtype::protocol::MAX_PLAYER_NAME_LENGTH + 5; ++index) {
    (void)menu.press(KeyCode::X, false);
  }

  EXPECT_EQ(menu.username().size(), rtype::protocol::MAX_PLAYER_NAME_LENGTH);
}

TEST(LoginMenu, StartsOnThePasswordWithAUsername) {
  World world;
  LoginMenu menu(world, 0, "Alice");

  EXPECT_EQ(menu.username(), "Alice");
  EXPECT_EQ(menu.selected(), PASSWORD);
}

TEST(LoginMenu, LogsInFromThePassword) {
  World world;
  LoginMenu menu(world, 0, "Alice");
  type(menu, {KeyCode::P, KeyCode::W});

  const std::optional<LoginRequest> request =
      menu.press(KeyCode::Return, false);

  ASSERT_TRUE(request.has_value());
  EXPECT_EQ(request->username, "Alice");
  EXPECT_EQ(request->password, "pw");
  EXPECT_FALSE(request->createAccount);
  EXPECT_TRUE(menu.password().empty());  // not kept once sent
}

TEST(LoginMenu, CreatesAnAccount) {
  World world;
  LoginMenu menu(world, 0, "Alice");
  type(menu, {KeyCode::P, KeyCode::W, KeyCode::DownArrow, KeyCode::DownArrow});
  ASSERT_EQ(menu.selected(), CREATE_ACCOUNT);

  const auto request = menu.press(KeyCode::Return, false);

  ASSERT_TRUE(request.has_value());
  EXPECT_TRUE(request->createAccount);
}

TEST(LoginMenu, GoesFromTheUsernameToThePassword) {
  World world;
  LoginMenu menu(world, 0);
  type(menu, {KeyCode::A});

  EXPECT_FALSE(menu.press(KeyCode::Return, false).has_value());
  EXPECT_EQ(menu.selected(), PASSWORD);
}

TEST(LoginMenu, AsksForWhatIsMissing) {
  World world;
  LoginMenu menu(world, 0);
  menu.show(true);
  type(menu, {KeyCode::DownArrow, KeyCode::DownArrow});

  EXPECT_FALSE(menu.press(KeyCode::Return, false).has_value());
  EXPECT_TRUE(shows(menu, "Type your username first"));
  EXPECT_EQ(menu.selected(), USERNAME);

  type(menu, {KeyCode::A, KeyCode::DownArrow, KeyCode::DownArrow});
  EXPECT_FALSE(menu.press(KeyCode::Return, false).has_value());
  EXPECT_TRUE(shows(menu, "Type your password first"));
  EXPECT_EQ(menu.selected(), PASSWORD);
}

TEST(LoginMenu, ShowsTheMessage) {
  World world;
  LoginMenu menu(world, 0);
  menu.show(true);

  menu.setMessage("Wrong username or password");

  EXPECT_TRUE(shows(menu, "Wrong username or password"));
}
