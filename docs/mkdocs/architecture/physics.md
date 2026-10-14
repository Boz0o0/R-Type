# Physics

The physics module moves entities, makes them fall, and finds the ones that
touch. It only needs `core`, so it runs on the server, which has no render.

## Concepts

| Name              | Kind      | What it does                                                                 |
| ----------------- | --------- | ---------------------------------------------------------------------------- |
| `Velocity`        | component | speed of the entity, in units per second (`x` and `y`, the `z` never moves)  |
| `Gravity`         | component | makes the entity fall: `scale` 1 falls, -1 goes up, 0 does nothing           |
| `BoxCollider`     | component | hitbox of the entity (see [Hitboxes](#hitboxes))                             |
| `Clock`           | resource  | `dt` of the frame in seconds, filled by the game (from `core`)               |
| `GravityForce`    | resource  | speed gained each second by a `Gravity` of scale 1 (positive goes down)      |
| `Collisions`      | resource  | pairs of entities that touch this frame                                      |
| `MovementSystem`  | system    | velocity += gravity × dt, then position += velocity × dt                     |
| `CollisionSystem` | system    | fills `Collisions` with every pair of overlapping hitboxes **on the same z** |

The systems only use the entities that also have a `Transform`. They throw
`ResourceNotFound` if a resource they need is missing (`GravityForce` is only
needed when an entity has a `Gravity`).

## Example

```cpp
namespace physics = ampersand::physics;

world.addResource<ampersand::core::Clock>();
world.addResource<physics::GravityForce>().value = 980.0F;
world.addResource<physics::Collisions>();

const ecs::Entity bomb = world.create();
world.add(bomb, ampersand::core::Transform{});
world.add(bomb, physics::Velocity{.value = {100.0F, 0.0F}});
world.add(bomb, physics::Gravity{});
world.add(bomb, physics::BoxCollider{{0.0F, 0.0F, 16.0F, 16.0F}});

physics::MovementSystem movement;
physics::CollisionSystem collision;
DamageSystem damage;  // a system of the game

while (running) {
    world.getResource<ampersand::core::Clock>().dt = frameTime();
    movement.update(world);
    collision.update(world);
    damage.update(world);  // reads the Collisions
}
```

The game decides what a collision does. The order inside a pair is not defined,
so check both sides:

```cpp
for (const auto& [first, second] : world.getResource<physics::Collisions>().pairs) {
    if (world.has<Bullet>(first) && world.has<Health>(second)) { ... }
    if (world.has<Bullet>(second) && world.has<Health>(first)) { ... }
}
```

Destroying entities in this loop is safe (it is not a pool), but an entity can be
in several pairs: check `world.alive(entity)` before using it.

## Hitboxes

The `bounds` of a `BoxCollider` are in the **local coordinates** of the entity:
always measured from the local (0, 0), not from the origin. The `Transform`
places them exactly like the render places a sprite:

```
world point = position + (local point - origin) * scale
```

With a sprite, the local coordinates are the pixels of its image, like
`Sprite::source`: read the hitbox in the image editor. For a 32x32 ship image
whose ship only fills the middle, the hitbox is `{8, 10, 16, 12}`:

```
      0       8               24      32
   0  ┌───────────────────────────────┐
      │                               │
  10  │       ┌───────────────┐       │
      │       │     ship      │       │
  22  │       └───────────────┘       │
      │                               │
  32  └───────────────────────────────┘
```

The `origin` is the point of the entity that is put on `position` (it is also the
center of the scale and the rotation). It moves the whole entity, sprite and
hitbox together, so the hitbox stays on the drawing:

=== "origin (0, 0)"

    `position` is the top-left corner.

    ```
    position (300, 200)
       ●───────────────┐
       │               │    image: 300 → 332
       │               │    hitbox: 308 → 324
       └───────────────┘
    ```

=== "origin (16, 16)"

    `position` is the center.

    ```
       ┌───────────────┐
       │       ●       │    image: 284 → 316
       │               │    hitbox: 292 → 308
       └───────────────┘
    ```

Without a sprite (on the server, or for an invisible zone), with the default
origin, `bounds` is just an offset and a size from `position`.

`worldBounds(transform, collider)` gives the hitbox in the world. The rotation is
ignored: hitboxes are rectangles that never turn. Two hitboxes that only touch by
an edge do not collide.
