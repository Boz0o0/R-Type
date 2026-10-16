# Input

The input module is one resource, `InputManager`: the keyboard and mouse
events of the frame, what is held, and where the mouse is. The `InputSystem`
fills it each frame.

```cpp
struct InputManager {
    std::vector<Event> events;     // events of the current frame, in order
    std::set<Key> keysDown;        // keys currently held
    std::set<Button> buttonsDown;  // mouse buttons currently held
    core::math::Vec2i mouse;       // position of the mouse in the window, in pixels
};
```

It is plain shared data, with no logic: the `InputSystem` fills it, and the
systems of the game read it. An `Event` is one of:

| Event           | Holds                 |
| --------------- | --------------------- |
| `KeyPressed`    | `key`                 |
| `KeyReleased`   | `key`                 |
| `MousePressed`  | `button`, `position`  |
| `MouseReleased` | `button`, `position`  |
| `MouseMoved`    | `position`            |

## Reading the input

```cpp
namespace input = ampersand::input;

const auto& manager = world.getResource<input::InputManager>();
```

**What is true right now?** Look in `keysDown`, for anything that lasts while
a key is held:

```cpp
if (manager.keysDown.contains(input::Key::Left)) {
    position.x -= speed * dt;
}
if (manager.buttonsDown.contains(input::Button::Left)) {
    aimAt(manager.mouse);
}
```

**What happened during this frame?** Loop on `events`, for anything that
happens once:

```cpp
for (const input::Event& event : manager.events) {
    if (std::holds_alternative<input::KeyPressed>(event) &&
        std::get<input::KeyPressed>(event).key == input::Key::Space) {
        shoot();
    }
}
```

## Events of the other modules

The engine has no event bus. An event is data of the frame in a resource of
the `World`: the module that produces it fills the resource, and the systems
that run after it read it. `InputManager::events` follows this rule, and so
does the `Collisions` resource of [Physics](physics.md):

```cpp
void PlayerSystem::update(ecs::World& world) {
    for (const input::Event& event :
         world.getResource<input::InputManager>().events) { ... }
    for (const auto& [first, second] :
         world.getResource<physics::Collisions>().pairs) { ... }
}
```

The game adds its own events the same way, with a resource of its own
(`struct EnemyDeaths { std::vector<ecs::Entity> entities; };`) that it empties
at the start of each frame.

## Filling the input

The input module does not know the window, so the `InputSystem` is given a
**poll function** when it is created: it receives the World, and each call
returns the next input event, or `std::nullopt` when there is none left.

Each frame, `update` empties `events`, then calls the poll function until it
returns `std::nullopt`, adds each event and keeps `keysDown`, `buttonsDown` and
`mouse` up to date. A press of a key or a button that is already held is
ignored (the keyboard repeats a held key). It throws `ResourceNotFound` if the
World has no `InputManager`.

With the render module, the poll function reads the window of the `Renderer`
resource (see [Render](rendering.md#window)). It also receives the other events
of the window, so it handles them there: `Closed` sets `isOpen` to `false`, and
the `RenderSystem` closes the window.

```cpp
std::optional<input::Event> pollWindow(ecs::World& world) {
    auto& window = world.getResource<render::Renderer>();
    while (const auto event = window.renderer.pollEvent()) {
        if (std::holds_alternative<render::Closed>(*event)) {
            window.isOpen = false;
        } else if (std::holds_alternative<input::KeyPressed>(*event)) {
            return std::get<input::KeyPressed>(*event);
        } else if (std::holds_alternative<input::KeyReleased>(*event)) {
            return std::get<input::KeyReleased>(*event);
        } else if (std::holds_alternative<input::MousePressed>(*event)) {
            return std::get<input::MousePressed>(*event);
        } else if (std::holds_alternative<input::MouseReleased>(*event)) {
            return std::get<input::MouseReleased>(*event);
        } else if (std::holds_alternative<input::MouseMoved>(*event)) {
            return std::get<input::MouseMoved>(*event);
        }
    }
    return std::nullopt;
}

world.addResource<input::InputManager>();
input::InputSystem inputSystem(pollWindow);
render::RenderSystem draw;

while (world.getResource<render::Renderer>().isOpen) {
    inputSystem.update(world);  // first, so the other systems see this frame
    player.update(world);
    draw.update(world);         // last: it displays the frame
}
```

This function is the only one that polls the window: it replaces the
`pollEvent()` loop of the game. An event polled anywhere else is lost for the
others.

## Limits

| Rule        | Value                                                                                          |
| ----------- | ---------------------------------------------------------------------------------------------- |
| Keys        | `A`-`Z`, `Num0`-`Num9`, arrows, `Space`, `Enter`, `Escape`, `Tab`, `Backspace`, left and right `Shift`, `Control`, `Alt`. Any other key is `Unknown` |
| Letters     | follow the layout of the keyboard: on AZERTY, the key at the place of the QWERTY `W` is `Z`  |
| Mouse       | `Left`, `Right` and `Middle` buttons (others are `Unknown`), position in window pixels: without a `Camera`, the same as world units |
| Wheel       | not reported                                                                                   |
| Focus       | a key held while the window loses the focus stays in `keysDown` until its next release        |
