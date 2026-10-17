# Gameplay


## Concepts

| Name            | Kind      | What it does                                                                      |
| --------------- | --------- | --------------------------------------------------------------------------------- |
| `Health`        | component | hit points: `current` and `max`; the entity dies when `current` reaches 0         |
| `Damage`        | component | hit points removed from each entity with a `Health` it touches                    |
| `Team`          | component | `side` of the entity (`Players` or `Enemies`): no damage inside a side (see [Teams](#teams)) |
| `Kind`          | component | what the entity is (`Ship`, `Enemy`, `PlayerMissile`, `EnemyMissile`)             |
| `Weapon`        | component | fires a `missile` every `cooldown` seconds while `firing` (see [Shooting](#shooting)) |
| `Projectile`    | component | tag of a missile: dies on its first hit                                           |
| `Deaths`        | resource  | entities that died during the tick, and why (`reason`)                            |
| `DamageSystem`  | system    | turns the `Collisions` of the tick into damage, adds the dead to `Deaths`         |
| `ShootingSystem` | system   | creates the missiles of the `Weapon`s that are firing                             |
| `CleanupSystem` | system    | destroys the entities of `Deaths`, then empties it                                |

An entity is what its components make it:

| Entity  | Components                        | Behaviour                                 |
| ------- | --------------------------------- | ----------------------------------------- |
| Ship    | `Health`, `Team`, `Weapon`                | loses hit points, dies at 0               |
| Ennemy   | `Health`, `Damage`, `Team`, `Weapon`      | also hurts what touches it                |
| Missile | `Damage`, `Projectile`, `Team`, `Kind`    | hurts what it touches, dies on first hit  |
| Destroyable tile | `Health`, `Damage`, no `Team`    | hurts and is hurt by every side           |
| Wall    | `Damage`, no `Health`, no `Team`  | hurts every side, never dies              |

## Example

```cpp
namespace common = rtype::common;
namespace physics = ampersand::physics;

world.addResource<ampersand::core::Clock>();
world.addResource<physics::Collisions>();
world.addResource<common::Deaths>();

physics::MovementSystem movement;
physics::CollisionSystem collision;
common::ShootingSystem shooting;
common::DamageSystem damage;
common::CleanupSystem cleanup;

while (running) {
  shooting.update(world);  // before the movement
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

## Shooting

A `Weapon` says when to fire and which `missile`. While `firing` is true, the
`ShootingSystem` calls `createMissile` every `cooldown` seconds. The missile
starts at the position of the shooter, with the `Team` of the shooter: it goes
through the ships of its side. Whoever holds the trigger sets `firing`: always
`true` for an enemy, the shoot key for a player.

```cpp
world.add(ship, common::Weapon{.cooldown = 0.25F,
                               .firing = true,
                               .missile = common::EntityKind::PlayerMissile});
```

What a missile is (velocity, damage, hitbox) lives in `createMissile`, one case
per kind:

| Kind            | Velocity         | Damage | Hitbox |
| --------------- | ---------------- | ------ | ------ |
| `PlayerMissile` | 800 to the right | 1      | 16x4   |
| `EnemyMissile`  | 400 to the left  | 1      | 8x8    |

## Teams

A ship and its missiles get the same `Team`, and so do an enemy and its
missiles: a `Damage` never hurts a `Health` of its own side, so players do not
shoot each other and enemies do not hurt each other. A missile also goes through
the ships of its side without dying.

An entity without a `Team` has no side: it hurts and is hurt by everyone.

```cpp
world.add(ship, common::Team{.side = common::Side::Players});
world.add(missile, common::Team{.side = common::Side::Players});
```

## Limits

| Rule    | Value                                                                              |
| ------- | ---------------------------------------------------------------------------------- |
| Damage  | only on contact (from the `Collisions`), once per pair and per tick                |
| Missile | dies on its first hit, even if it touched several entities in the same tick       |
