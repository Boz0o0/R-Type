#include <gtest/gtest.h>

#include <ampersand/core/ecs/ECSExceptions.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <ampersand/input/Event.hpp>
#include <ampersand/input/resources/InputManager.hpp>
#include <ampersand/input/systems/InputSystem.hpp>
#include <deque>
#include <optional>
#include <utility>
#include <variant>

using ampersand::core::ecs::ResourceNotFound;
using ampersand::core::ecs::World;
using ampersand::input::Button;
using ampersand::input::Event;
using ampersand::input::InputManager;
using ampersand::input::InputSystem;
using ampersand::input::Key;
using ampersand::input::KeyPressed;
using ampersand::input::KeyReleased;
using ampersand::input::MouseMoved;
using ampersand::input::MousePressed;
using ampersand::input::MouseReleased;

/** @brief Events waiting to be polled, in place of a window. */
struct PendingEvents {
    std::deque<Event> events;
};

/** @brief Poll function of the tests: gives the PendingEvents one by one. */
std::optional<Event> pollPending(World& world) {
    auto& pending = world.getResource<PendingEvents>().events;
    if (pending.empty()) {
        return std::nullopt;
    }
    const Event event = pending.front();
    pending.pop_front();
    return event;
}

/** @brief Runs one frame of the InputSystem on these events. */
void runFrame(World& world, std::deque<Event> events) {
    world.getResource<PendingEvents>().events = std::move(events);
    InputSystem(pollPending).update(world);
}

bool isKeyPressed(const Event& event, Key key) {
    return std::holds_alternative<KeyPressed>(event) &&
           std::get<KeyPressed>(event).key == key;
}

bool isKeyReleased(const Event& event, Key key) {
    return std::holds_alternative<KeyReleased>(event) &&
           std::get<KeyReleased>(event).key == key;
}

TEST(InputSystem, ThrowsWithoutInputManager) {
    World world;
    world.addResource<PendingEvents>();

    EXPECT_THROW(runFrame(world, {}), ResourceNotFound);
}

TEST(InputSystem, HoldsAPressedKey) {
    World world;
    const auto& manager = world.addResource<InputManager>();
    world.addResource<PendingEvents>();

    runFrame(world, {KeyPressed{Key::Space}});

    ASSERT_EQ(manager.events.size(), 1U);
    EXPECT_TRUE(isKeyPressed(manager.events[0], Key::Space));
    EXPECT_TRUE(manager.keysDown.contains(Key::Space));
}

TEST(InputSystem, IgnoresTheRepeatOfAHeldKey) {
    World world;
    const auto& manager = world.addResource<InputManager>();
    world.addResource<PendingEvents>();

    runFrame(world, {KeyPressed{Key::A}, KeyPressed{Key::A}});
    EXPECT_EQ(manager.events.size(), 1U);

    runFrame(world, {KeyPressed{Key::A}});
    EXPECT_TRUE(manager.events.empty());
    EXPECT_TRUE(manager.keysDown.contains(Key::A));
}

TEST(InputSystem, ReleasesAKey) {
    World world;
    const auto& manager = world.addResource<InputManager>();
    world.addResource<PendingEvents>();
    runFrame(world, {KeyPressed{Key::Left}});

    runFrame(world, {KeyReleased{Key::Left}});

    ASSERT_EQ(manager.events.size(), 1U);
    EXPECT_TRUE(isKeyReleased(manager.events[0], Key::Left));
    EXPECT_TRUE(manager.keysDown.empty());
}

TEST(InputSystem, KeepsTheEventsInOrder) {
    World world;
    const auto& manager = world.addResource<InputManager>();
    const auto& pending = world.addResource<PendingEvents>().events;

    runFrame(world,
             {KeyPressed{Key::Z}, KeyPressed{Key::Q}, KeyReleased{Key::Z}});

    EXPECT_TRUE(pending.empty());
    ASSERT_EQ(manager.events.size(), 3U);
    EXPECT_TRUE(isKeyPressed(manager.events[0], Key::Z));
    EXPECT_TRUE(isKeyPressed(manager.events[1], Key::Q));
    EXPECT_TRUE(isKeyReleased(manager.events[2], Key::Z));
    EXPECT_EQ(manager.keysDown.size(), 1U);
    EXPECT_TRUE(manager.keysDown.contains(Key::Q));
}

TEST(InputSystem, EmptiesTheEventsEachFrame) {
    World world;
    const auto& manager = world.addResource<InputManager>();
    world.addResource<PendingEvents>();
    runFrame(world, {KeyPressed{Key::Up}, MouseMoved{{4, 2}}});

    runFrame(world, {});

    EXPECT_TRUE(manager.events.empty());
    EXPECT_TRUE(manager.keysDown.contains(Key::Up));
    EXPECT_EQ(manager.mouse.x, 4);
    EXPECT_EQ(manager.mouse.y, 2);
}

TEST(InputSystem, HoldsAPressedButtonAndMovesTheMouse) {
    World world;
    const auto& manager = world.addResource<InputManager>();
    world.addResource<PendingEvents>();

    runFrame(world,
             {MousePressed{.button = Button::Left, .position = {10, 20}}});

    ASSERT_EQ(manager.events.size(), 1U);
    ASSERT_TRUE(std::holds_alternative<MousePressed>(manager.events[0]));
    EXPECT_EQ(std::get<MousePressed>(manager.events[0]).button, Button::Left);
    EXPECT_TRUE(manager.buttonsDown.contains(Button::Left));
    EXPECT_EQ(manager.mouse.x, 10);
    EXPECT_EQ(manager.mouse.y, 20);
}

TEST(InputSystem, IgnoresThePressOfAHeldButton) {
    World world;
    const auto& manager = world.addResource<InputManager>();
    world.addResource<PendingEvents>();

    runFrame(world,
             {
                 MousePressed{.button = Button::Right, .position = {1, 1}},
                 MousePressed{.button = Button::Right, .position = {3, 3}},
             });

    EXPECT_EQ(manager.events.size(), 1U);
    EXPECT_EQ(manager.buttonsDown.size(), 1U);
    EXPECT_EQ(manager.mouse.x, 3);
}

TEST(InputSystem, ReleasesAButtonAndMovesTheMouse) {
    World world;
    const auto& manager = world.addResource<InputManager>();
    world.addResource<PendingEvents>();
    runFrame(world,
             {MousePressed{.button = Button::Middle, .position = {1, 2}}});

    runFrame(world,
             {MouseReleased{.button = Button::Middle, .position = {30, 40}}});

    ASSERT_EQ(manager.events.size(), 1U);
    ASSERT_TRUE(std::holds_alternative<MouseReleased>(manager.events[0]));
    EXPECT_EQ(std::get<MouseReleased>(manager.events[0]).button,
              Button::Middle);
    EXPECT_TRUE(manager.buttonsDown.empty());
    EXPECT_EQ(manager.mouse.x, 30);
    EXPECT_EQ(manager.mouse.y, 40);
}

TEST(InputSystem, FollowsTheMouse) {
    World world;
    const auto& manager = world.addResource<InputManager>();
    world.addResource<PendingEvents>();

    runFrame(world, {MouseMoved{{1, 1}}, MouseMoved{{5, 6}}});

    ASSERT_EQ(manager.events.size(), 2U);
    EXPECT_TRUE(std::holds_alternative<MouseMoved>(manager.events[1]));
    EXPECT_EQ(manager.mouse.x, 5);
    EXPECT_EQ(manager.mouse.y, 6);
}
