#include <gtest/gtest.h>

#include <ampersand/input/Event.hpp>
#include <ampersand/input/resources/InputManager.hpp>
#include <cstdint>
#include <initializer_list>
#include <rtype/client/Controls.hpp>
#include <rtype/common/protocol/Protocol.hpp>

using ampersand::input::InputManager;
using ampersand::input::JoystickAxis;
using ampersand::input::JoystickButton;
using ampersand::input::JoystickButtonPressed;
using ampersand::input::JoystickMoved;
using ampersand::input::KeyCode;
using ampersand::input::KeyPressed;
using rtype::client::Controls;
namespace InputFlag = rtype::protocol::InputFlag;

static InputManager boundInput() {
  InputManager manager;
  Controls::bind(manager);
  manager.beginFrame();
  return manager;
}

static std::uint8_t maskWhileHolding(std::initializer_list<KeyCode> keys) {
  InputManager manager = boundInput();
  for (const KeyCode key : keys) {
    manager.apply(KeyPressed{.key = key});
  }
  return Controls::inputMask(manager);
}

static std::uint8_t maskWithStick(JoystickAxis axis, float position) {
  InputManager manager = boundInput();
  manager.apply(
      JoystickMoved{.joystick = 0, .axis = axis, .position = position});
  return Controls::inputMask(manager);
}

TEST(Controls, NothingHeldGivesAnEmptyMask) {
  EXPECT_EQ(maskWhileHolding({}), 0);
}

TEST(Controls, EachArrowMovesTheShip) {
  EXPECT_EQ(maskWhileHolding({KeyCode::UpArrow}), InputFlag::MOVE_UP);
  EXPECT_EQ(maskWhileHolding({KeyCode::DownArrow}), InputFlag::MOVE_DOWN);
  EXPECT_EQ(maskWhileHolding({KeyCode::LeftArrow}), InputFlag::MOVE_LEFT);
  EXPECT_EQ(maskWhileHolding({KeyCode::RightArrow}), InputFlag::MOVE_RIGHT);
}

TEST(Controls, SpaceShoots) {
  EXPECT_EQ(maskWhileHolding({KeyCode::Space}), InputFlag::SHOOT);
}

TEST(Controls, CombinesKeysHeldTogether) {
  // Unsigned operands: a | of two std::uint8_t would give a signed int.
  constexpr auto expected =
      static_cast<std::uint8_t>(static_cast<unsigned>(InputFlag::MOVE_UP) |
                                static_cast<unsigned>(InputFlag::MOVE_RIGHT) |
                                static_cast<unsigned>(InputFlag::SHOOT));
  EXPECT_EQ(
      maskWhileHolding({KeyCode::UpArrow, KeyCode::RightArrow, KeyCode::Space}),
      expected);
}

TEST(Controls, OppositeArrowsCancelOut) {
  EXPECT_EQ(maskWhileHolding({KeyCode::UpArrow, KeyCode::DownArrow}), 0);
  EXPECT_EQ(maskWhileHolding({KeyCode::LeftArrow, KeyCode::RightArrow}), 0);
}

TEST(Controls, IgnoresOtherKeys) {
  EXPECT_EQ(maskWhileHolding({KeyCode::Escape, KeyCode::A}), 0);
}

TEST(Controls, ControllerStickMovesTheShip) {
  EXPECT_EQ(maskWithStick(JoystickAxis::X, 1.0F), InputFlag::MOVE_RIGHT);
  EXPECT_EQ(maskWithStick(JoystickAxis::X, -1.0F), InputFlag::MOVE_LEFT);
  EXPECT_EQ(maskWithStick(JoystickAxis::Y, -1.0F), InputFlag::MOVE_UP);
  EXPECT_EQ(maskWithStick(JoystickAxis::Y, 1.0F), InputFlag::MOVE_DOWN);
}

TEST(Controls, ControllerStickHasADeadZone) {
  EXPECT_EQ(maskWithStick(JoystickAxis::X, 0.1F), 0);
}

TEST(Controls, ControllerButtonShoots) {
  InputManager manager = boundInput();
  manager.apply(
      JoystickButtonPressed{.joystick = 0, .button = JoystickButton::Button0});

  EXPECT_EQ(Controls::inputMask(manager), InputFlag::SHOOT);
}
