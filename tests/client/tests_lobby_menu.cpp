#include <gtest/gtest.h>

#include <algorithm>
#include <ampersand/core/ecs/World.hpp>
#include <ampersand/input/Event.hpp>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <rtype/client/LobbyMenu.hpp>
#include <rtype/client/ServerConnection.hpp>
#include <rtype/common/protocol/Packets.hpp>
#include <rtype/common/protocol/Protocol.hpp>
#include <string>
#include <variant>
#include <vector>

using ampersand::core::ecs::World;
using ampersand::input::Key;
using rtype::client::LobbyMenu;
using rtype::client::RoomRequest;
using rtype::protocol::RoomCreate;
using rtype::protocol::RoomInfo;
using rtype::protocol::RoomJoin;
using rtype::protocol::RoomVisibility;

namespace {

// Lines before the rooms: quick match, public, private, code.
constexpr std::size_t FIRST_ROOM = 4;

bool shows(const LobbyMenu& menu, const std::string& text) {
  const auto lines = menu.lines();
  return std::ranges::find(lines, text) != lines.end();
}

RoomJoin joinOf(const std::optional<RoomRequest>& request) {
  EXPECT_TRUE(request.has_value());
  EXPECT_TRUE(request && std::holds_alternative<RoomJoin>(*request));
  if (!request || !std::holds_alternative<RoomJoin>(*request)) {
    return {};
  }
  return std::get<RoomJoin>(*request);
}

void pressTimes(LobbyMenu& menu, Key key, std::size_t times) {
  for (std::size_t press = 0; press < times; ++press) {
    (void)menu.press(key);
  }
}

}  // namespace

TEST(LobbyMenu, ShowsNothingUntilShown) {
  World world;
  LobbyMenu menu(world, 0);

  for (const std::string& line : menu.lines()) {
    EXPECT_TRUE(line.empty()) << line;
  }

  menu.show(true);
  EXPECT_TRUE(shows(menu, "LOBBY"));
  EXPECT_TRUE(shows(menu, "> Quick match"));
  EXPECT_TRUE(shows(menu, "No public room yet"));

  menu.show(false);
  EXPECT_FALSE(shows(menu, "LOBBY"));
}

TEST(LobbyMenu, StartsOnQuickMatch) {
  World world;
  LobbyMenu menu(world, 0);

  const RoomJoin join = joinOf(menu.press(Key::Enter));

  EXPECT_EQ(join.roomId, 0U);
  EXPECT_TRUE(join.code.empty());
}

TEST(LobbyMenu, ChoosesWithTheArrows) {
  World world;
  LobbyMenu menu(world, 0);
  menu.show(true);

  (void)menu.press(Key::Up);  // already at the top
  EXPECT_EQ(menu.selected(), 0U);
  (void)menu.press(Key::Down);
  EXPECT_TRUE(shows(menu, "> Create a public room"));
  const auto publicRoom = menu.press(Key::Enter);
  (void)menu.press(Key::Down);
  const auto privateRoom = menu.press(Key::Enter);
  pressTimes(menu, Key::Down, 5);  // past the last line: stays on it

  ASSERT_TRUE(publicRoom && std::holds_alternative<RoomCreate>(*publicRoom));
  EXPECT_EQ(std::get<RoomCreate>(*publicRoom).visibility,
            RoomVisibility::Public);
  ASSERT_TRUE(privateRoom && std::holds_alternative<RoomCreate>(*privateRoom));
  EXPECT_EQ(std::get<RoomCreate>(*privateRoom).visibility,
            RoomVisibility::Private);
  EXPECT_EQ(menu.selected(), FIRST_ROOM - 1);
}

TEST(LobbyMenu, TypesTheCodeOnItsLine) {
  World world;
  LobbyMenu menu(world, 0);
  menu.show(true);
  (void)menu.press(Key::A);  // not on the code line: ignored
  pressTimes(menu, Key::Down, 3);

  for (const Key key : {Key::A, Key::B, Key::Num1, Key::Num2, Key::Space}) {
    (void)menu.press(key);
  }
  (void)menu.press(Key::Backspace);
  EXPECT_TRUE(shows(menu, "> Join with a code: AB1_"));
  const RoomJoin join = joinOf(menu.press(Key::Enter));

  EXPECT_EQ(join.roomId, 0U);
  EXPECT_EQ(join.code, "AB1");
}

TEST(LobbyMenu, StopsTheCodeAtItsLongestLength) {
  World world;
  LobbyMenu menu(world, 0);
  pressTimes(menu, Key::Down, 3);

  pressTimes(menu, Key::Num7, rtype::protocol::MAX_PARTY_CODE_LENGTH + 3);

  EXPECT_EQ(menu.code(),
            std::string(rtype::protocol::MAX_PARTY_CODE_LENGTH, '7'));
}

TEST(LobbyMenu, AsksForACodeBeforeJoiningWithIt) {
  World world;
  LobbyMenu menu(world, 0);
  menu.show(true);
  pressTimes(menu, Key::Down, 3);

  EXPECT_FALSE(menu.press(Key::Enter).has_value());
  EXPECT_TRUE(shows(menu, "Type the code of the room first"));

  (void)menu.press(Key::Num1);  // the hint goes away with the next key
  EXPECT_FALSE(shows(menu, "Type the code of the room first"));
}

TEST(LobbyMenu, ListsTheRoomsAndJoinsTheChosenOne) {
  World world;
  LobbyMenu menu(world, 0);
  menu.show(true);

  menu.setRooms({{.roomId = 1, .players = 2}, {.roomId = 4, .players = 4}});
  pressTimes(menu, Key::Down, FIRST_ROOM + 1);
  const RoomJoin join = joinOf(menu.press(Key::Enter));

  EXPECT_TRUE(shows(menu, "Public rooms"));
  EXPECT_TRUE(shows(menu, "  Room 1   2/4 players"));
  EXPECT_TRUE(shows(menu, "> Room 4   4/4 players   full"));
  EXPECT_EQ(join.roomId, 4U);
  EXPECT_TRUE(join.code.empty());
}

TEST(LobbyMenu, KeepsTheChosenRoomWhenTheListChanges) {
  World world;
  LobbyMenu menu(world, 0);
  menu.setRooms({{.roomId = 1, .players = 2}, {.roomId = 4, .players = 1}});
  pressTimes(menu, Key::Down, FIRST_ROOM + 1);

  menu.setRooms({{.roomId = 4, .players = 2}});  // room 1 was closed
  EXPECT_EQ(menu.selected(), FIRST_ROOM);
  EXPECT_EQ(joinOf(menu.press(Key::Enter)).roomId, 4U);

  menu.setRooms({});  // room 4 was closed too
  EXPECT_EQ(menu.selected(), FIRST_ROOM - 1);
}

TEST(LobbyMenu, ScrollsToTheChosenRoom) {
  World world;
  LobbyMenu menu(world, 0);
  menu.show(true);
  std::vector<RoomInfo> rooms;
  for (std::uint32_t id = 1; id <= 10; ++id) {
    rooms.push_back({.roomId = id, .players = 1});
  }
  menu.setRooms(rooms);
  EXPECT_TRUE(shows(menu, "  Room 1   1/4 players"));
  EXPECT_FALSE(shows(menu, "  Room 10   1/4 players"));
  EXPECT_TRUE(shows(menu, "Public rooms (10)"));

  pressTimes(menu, Key::Down, FIRST_ROOM + 9);

  EXPECT_TRUE(shows(menu, "> Room 10   1/4 players"));
  EXPECT_FALSE(shows(menu, "  Room 1   1/4 players"));
}

TEST(LobbyMenu, ShowsTheMessageOfTheApp) {
  World world;
  LobbyMenu menu(world, 0);
  menu.show(true);

  menu.setMessage("Room full");
  EXPECT_TRUE(shows(menu, "Room full"));

  menu.setMessage({});
  EXPECT_FALSE(shows(menu, "Room full"));
}
