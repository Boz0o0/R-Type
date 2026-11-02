#include <gtest/gtest.h>

#include <ampersand/input/Event.hpp>
#include <cstdint>
#include <rtype/client/Controls.hpp>
#include <rtype/common/protocol/Protocol.hpp>

using ampersand::input::KeyCode;
using rtype::client::Controls;
namespace InputFlag = rtype::protocol::InputFlag;

TEST(Controls, NoKeyGivesAnEmptyMask) { EXPECT_EQ(Controls::inputMask({}), 0); }

TEST(Controls, EachArrowMovesTheShip) {
  EXPECT_EQ(Controls::inputMask({KeyCode::UpArrow}), InputFlag::MOVE_UP);
  EXPECT_EQ(Controls::inputMask({KeyCode::DownArrow}), InputFlag::MOVE_DOWN);
  EXPECT_EQ(Controls::inputMask({KeyCode::LeftArrow}), InputFlag::MOVE_LEFT);
  EXPECT_EQ(Controls::inputMask({KeyCode::RightArrow}), InputFlag::MOVE_RIGHT);
}

TEST(Controls, SpaceShoots) {
  EXPECT_EQ(Controls::inputMask({KeyCode::Space}), InputFlag::SHOOT);
}

TEST(Controls, CombinesKeysHeldTogether) {
  // Unsigned operands: a | of two std::uint8_t would give a signed int.
  constexpr auto expected =
      static_cast<std::uint8_t>(static_cast<unsigned>(InputFlag::MOVE_UP) |
                                static_cast<unsigned>(InputFlag::MOVE_RIGHT) |
                                static_cast<unsigned>(InputFlag::SHOOT));
  EXPECT_EQ(Controls::inputMask(
                {KeyCode::UpArrow, KeyCode::RightArrow, KeyCode::Space}),
            expected);
}

TEST(Controls, AllBoundKeysGiveEveryFlag) {
  EXPECT_EQ(Controls::inputMask({KeyCode::UpArrow, KeyCode::DownArrow,
                                 KeyCode::LeftArrow, KeyCode::RightArrow,
                                 KeyCode::Space}),
            InputFlag::ALL);
}

TEST(Controls, IgnoresOtherKeys) {
  EXPECT_EQ(Controls::inputMask({KeyCode::Escape, KeyCode::A}), 0);
}
