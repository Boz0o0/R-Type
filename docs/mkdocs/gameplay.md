# Gameplay


## Concepts

| Name            | Kind      | What it does                                                                      |
| --------------- | --------- | --------------------------------------------------------------------------------- |
| `Health`        | component | hit points: `current` and `max`; the entity dies when `current` reaches 0         |
| `Damage`        | component | hit points removed from each entity with a `Health` it touches                    |
| `Deaths`        | resource  | entities that died during the tick, and why (`reason`)                            |
| `DamageSystem`  | system    | turns the `Collisions` of the tick into damage, adds the dead to `Deaths`         |
| `CleanupSystem` | system    | destroys the entities of `Deaths`, then empties it                                |

An entity is what its components make it:

| Entity  | Components                        | Behaviour                                 |
| ------- | --------------------------------- | ----------------------------------------- |
| Ship    | `Health`                          | loses hit points, dies at 0               |
| Ennemy   | `Health`, `Damage`                | also hurts what touches it                |
| Missile | `Damage` (no `Health`)            | hurts what it touches, dies on first hit  |

## Example

```cpp
namespace common = rtype::common;
namespace physics = ampersand::physics;

world.addResource<ampersand::core::Clock>();
world.addResource<physics::Collisions>();
world.addResource<common::Deaths>();

physics::MovementSystem movement;
physics::CollisionSystem collision;
common::DamageSystem damage;
common::CleanupSystem cleanup;

while (running) {
  movement.update(world);
  collision.update(world);
  damage.update(world);  // after the collisions
  for (const common::Death& death : world.getResource<common::Deaths>().entities) {
    // server: send ENTITY_DESTROY for death.entity
  }
  cleanup.update(world);  // always last
}
```

## Deaths

No system destroys an entity in the middle of the tick: it adds it to `Deaths`
instead. The entity stays alive until the `CleanupSystem`, so the systems after
it can still read it (the server sends why it died), and nobody destroys an
entity another system is looping on.

An entity is in `Deaths` at most once per tick, even when two missiles kill it
at the same time.

## Limits

| Rule    | Value                                                                              |
| ------- | ---------------------------------------------------------------------------------- |
| Teams   | none yet: every `Damage` hurts every `Health`, ennemies included                      |
| Damage  | only on contact (from the `Collisions`), once per pair and per tick                |
| Missile | dies on its first hit, even if it touched several entities in the same tick       |
