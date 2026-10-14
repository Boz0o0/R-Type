# Render

The render module draws the entities in a window. The game only adds
components: the `RenderSystem` draws them through the `IRenderer` stored in the
`Renderer` resource (`SfmlRenderer` here), so the game never calls SFML itself.
The server does not need this module.

## Concepts

| Name              | Kind      | What it does                                                                  |
| ----------------- | --------- | ----------------------------------------------------------------------------- |
| `Sprite`          | component | image to draw: `texture`, `source` (part of the texture), `color` (tint)      |
| `Text`            | component | text to draw: `font`, `value`, `size`, `color`                                |
| `Animation`       | component | list of frames played on the `Sprite`, `isFinished` (see [Animation](#animation)) |
| `Renderer`        | resource  | the `IRenderer` and its window: `size`, `title`, `isOpen` (see [Window](#window)) |
| `Clock`           | resource  | `dt` of the frame in seconds, filled by the game (from `core`)                |
| `Camera`          | resource  | part of the world shown in the window (see [Camera](#camera)), optional       |
| `ClearColor`      | resource  | background color of the window, optional (black without it)                  |
| `AnimationSystem` | system    | moves every `Animation` forward and writes its frame in `Sprite::source`      |
| `RenderSystem`    | system    | draws one whole frame: clear, camera, every `Sprite` and `Text`, display      |
| `IRenderer`       | interface | the window: open, events, load textures and fonts, draw (`SfmlRenderer`)     |

The systems only use the entities that also have a `Transform` (and a `Sprite`
for the `AnimationSystem`). The `RenderSystem` throws `ResourceNotFound`
without a `Renderer`, and the `AnimationSystem` without a `Clock`.

The `z` of the `Transform` is the draw order: the bigger `z` is drawn on top.

## Example

```cpp
namespace render = ampersand::render;

render::SfmlRenderer sfml;  // the only line that names the backend
auto& window = world.addResource<render::Renderer>(
    sfml, render::WindowConfig{.size = {1280, 720}, .title = "R-Type"});
const auto ships = window.renderer.loadTexture("assets/ships.png");
const auto font = window.renderer.loadFont("assets/font.ttf");

world.addResource<ampersand::core::Clock>();
world.addResource<render::ClearColor>().value = {.r = 8, .g = 8, .b = 24};

const ecs::Entity ship = world.create();
world.add(ship, ampersand::core::Transform{});
world.add(ship, render::Sprite{.texture = ships, .color = {.r = 255, .g = 80, .b = 80}});
world.add(ship, render::Animation{.frames = {{0, 0, 33, 17}, {33, 0, 33, 17}}});

const ecs::Entity score = world.create();
world.add(score, ampersand::core::Transform{});
world.add(score, render::Text{.font = font, .value = "0000"});

render::AnimationSystem animation;
render::RenderSystem draw;  // finds the renderer in the Renderer resource

while (window.isOpen) {
    while (const auto event = window.renderer.pollEvent()) {
        if (std::holds_alternative<render::Closed>(*event)) window.isOpen = false;
    }
    world.getResource<ampersand::core::Clock>().dt = frameTime();
    animation.update(world);
    draw.update(world);  // always last: it displays the frame
}
```

## Window

The `Renderer` resource is the one place where the backend is given: adding it
opens the window with the `WindowConfig`, and every system then finds the
renderer and the window in the World, without holding a reference itself.

```cpp
const auto& window = world.getResource<render::Renderer>();
window.renderer;  // IRenderer: load textures and fonts, poll events, draw by hand
window.size;      // size of the window in pixels
window.title;     // title given when it was opened
window.isOpen;    // false once the window is closed
```

At the start of each update, the `RenderSystem` keeps the resource up to date:

- `isOpen = false` (set by the game) closes the window, and nothing is drawn.
- A window closed another way (`window.renderer.close()`) sets `isOpen` back to
  `false`.
- `size` is copied from the window, so it follows the user resizing it.

Only `isOpen` acts on the window: writing `size` or `title` changes nothing
(the next update overwrites `size`). If the window cannot be opened, `isOpen`
is `false` from the start.

The backend (`sfml` in the example) is not copied: it must stay alive as long
as the World, so declare it before the World.

## Sprites

A texture is loaded once and gets an id (`TextureId`, `FontId` for fonts).
Components only keep this id, so many entities share the same image. A failed
load returns `0`, and an entity with an unknown id is not drawn.

`source` is the part of the texture to draw, in pixels. It lets one image (a
spritesheet) hold many sprites. An empty `source` draws the whole texture.

```
ships.png
x: 0     33    66
   +-----+-----+-----+  y: 0
   |  ▶  | ▶▶  | ▶▶▶ |   blue ship
   +-----+-----+-----+  y: 17
   |  ▶  | ▶▶  | ▶▶▶ |   red ship
   +-----+-----+-----+

Sprite{.texture = ships, .source = {33, 17, 33, 17}}  → red ship, 2nd frame
```

`color` is multiplied with the image: white (the default) keeps the image as is.
With a white or grey image, one texture gives a ship of each color, one per
player.

## Animation

`frames` is the list of the `source` rects to show, one after the other, each
during `frameDuration` seconds. They can be anywhere on the texture, in any
order.

- `loop = true` (default): starts again after the last frame.
- `loop = false`: stays on the last frame (an explosion that the game destroys
  after).
- Set `elapsed` to `0` to start again from the first frame.

The engine only plays an `Animation`: what comes next is decided by the game.
`isFinished(animation)` is true once a non-looping animation was fully played,
so a system of the game can switch back to another one:

```cpp
// when the ship is hit
world.get<render::Animation>(ship) = hitAnimation();  // loop = false
world.add(ship, Hit{});                               // a component of the game

// a system of the game, every frame
std::vector<ecs::Entity> done;
for (const ecs::Entity entity : world.owners<Hit>()) {
    if (render::isFinished(world.get<render::Animation>(entity))) done.push_back(entity);
}
for (const ecs::Entity entity : done) {  // not inside the loop: owners() is the real list
    world.get<render::Animation>(entity) = idleAnimation();
    world.remove<Hit>(entity);
}
```

## Camera

The `Camera` is the rectangle of the world shown in the window: `center` is the
world point in the middle of the window, `size` the width and height shown.
Without it, one world unit is one pixel of the window.

For R-Type, set it once to a fixed size. Every client then shows the same part
of the world, whatever the size of its window:

```cpp
world.addResource<render::Camera>() = {.center = {960.0F, 540.0F},
                                       .size = {1920.0F, 1080.0F}};
```

To follow a player, change `center` every frame. To zoom, change `size`. If the
window does not have the same ratio as `size`, the image is stretched.

## Drawing by hand

`IRenderer` (`window.renderer`) can also draw without the ECS: `beginFrame`,
then `draw` (with a `Sprite`, a `Text` or a `Rectangle`, `Circle` or `Line`),
then `endFrame`. The
`RenderSystem` already does `beginFrame` and `endFrame`, so use one or the
other, not both in the same frame.
