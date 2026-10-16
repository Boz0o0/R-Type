# Input

The input module gives the keyboard and mouse of the frame in one resource:
the events that happened, what is held, and where the mouse is. It does not
know the window, so it runs with any source of events (the window of the
render module, a replay, a test).

## Concepts

| Name           | Kind     | What it does                                                                  |
| -------------- | -------- | ----------------------------------------------------------------------------- |
| `InputManager` | resource | `events` of the frame, `keysDown`, `buttonsDown`, `mouse` (window pixels)     |
| `Event`        | type     | one of `KeyPressed`, `KeyReleased`, `MousePressed`, `MouseReleased`, `MouseMoved`, `FocusLost` |
| `Key`          | enum     | a key, by its **physical position** on the keyboard (see [Limits](#limits))  |
| `Button`       | enum     | a mouse button: `Left`, `Right`, `Middle`                                     |
| `InputSystem`  | system   | fills the `InputManager` with the events given by a poll function            |

The `InputSystem` throws `ResourceNotFound` if the World has no `InputManager`.

## Example

```cpp
namespace input = ampersand::input;

world.addResource<input::InputManager>();
input::InputSystem inputSystem(pollWindow);
PlayerSystem player;                         // a system of the game
render::RenderSystem draw;

while (world.getResource<render::Renderer>().isOpen) {
    inputSystem.update(world);  // first, so the other systems see this frame
    player.update(world);
    draw.update(world);         // last: it displays the frame
}
```

## Reading the input

`InputManager` is plain data: the `InputSystem` fills it, and the systems of
the game only read it.

```cpp
const auto& manager = world.getResource<input::InputManager>();
```

**What is true right now?** Look in `keysDown` and `buttonsDown`, for anything
that lasts while a key is held:

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

## Filling the input

The `InputSystem` is given a **poll function** when it is created: it receives
the World, and each call returns the next input event, or `std::nullopt` when
there is none left.

Each frame, `update` empties `events`, then calls the poll function until it
returns `std::nullopt`, adds each event and keeps `keysDown`, `buttonsDown` and
`mouse` up to date:

- a press of a key or a button that is already held is ignored (the keyboard
  repeats a held key);
- `FocusLost` releases every held key and button, and adds their `KeyReleased`
  and `MouseReleased`.

With the render module, the poll function reads the window of the `Renderer`
resource (see [Render](render.md#window)). It also receives the other events of
the window, so it handles them there: `Closed` sets `isOpen` to `false`, and the
`RenderSystem` closes the window.

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
        } else if (std::holds_alternative<input::FocusLost>(*event)) {
            return input::FocusLost{};
        }
    }
    return std::nullopt;
}
```

This function is the only one that polls the window: it replaces the
`pollEvent()` loop of the game. An event polled anywhere else is lost for the
others.

## Events of the other modules

The engine has no event bus. An event is data of the frame in a resource of the
World: the module that produces it fills the resource, and the systems that run
after it read it. `InputManager::events` follows this rule, and so does the
`Collisions` resource of [Physics](physics.md):

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

## Limits

| Rule    | Value                                                                                          |
| ------- | ---------------------------------------------------------------------------------------------- |
| Keys    | `A`-`Z`, `Num0`-`Num9`, arrows, `Space`, `Enter`, `Escape`, `Tab`, `Backspace`, left and right `Shift`, `Control`, `Alt`. Any other key is `Unknown` |
| Letters | a key is a physical position, named after QWERTY: on AZERTY, the key labelled `Z` is `Key::W`, so `WASD` is the same place on every keyboard |
| Mouse   | `Left`, `Right` and `Middle` buttons (others are `Unknown`), position in window pixels: without a `Camera`, the same as world units |
| Wheel   | not reported                                                                                   |
| Gamepad | not supported yet                                                                              |
| Focus   | losing the focus releases every held key and button, so a key still held when the focus comes back must be pressed again |
