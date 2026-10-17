# Gameplay

## Concepts

| Name             | Type      | Role                                                         |
| ---------------- | --------- | ------------------------------------------------------------ |
| `Health`         | component | hit points (`current`, `max`), dies at 0                     |
| `Damage`         | component | hit points removed from what it touches                      |
| `Team`           | component | `Players` or `Enemies`, no damage inside a team              |
| `Kind`           | component | what it is: `Ship`, `Enemy`, `PlayerMissile`, `EnemyMissile` |
| `Weapon`         | component | fires a `missile` every `cooldown` seconds while `firing`    |
| `Projectile`     | component | tag, dies on its first hit                                   |
| `Deaths`         | resource  | entities dead this tick, and why                             |
| `Spawns`         | resource  | entities created this tick                                   |
| `ShootingSystem` | system    | creates the missiles                                         |
| `DamageSystem`   | system    | applies the damage of the `Collisions`, fills `Deaths`       |
| `CleanupSystem`  | system    | destroys `Deaths`, empties `Deaths` and `Spawns`             |

## Entities

Always create an entity with a factory: it adds all the components and fills
`Spawns`.

| Factory                | Health | Damage | Team      | Speed     | Size  | Weapon                        |
| ---------------------- | ------ | ------ | --------- | --------- | ----- | ----------------------------- |
| `createShip`           | 3      | none   | `Players` | 0         | 64x32 | `PlayerMissile` / 0.25 s, off |
| `createEnemy`          | 2      | 1      | `Enemies` | 200 left  | 64x64 | `EnemyMissile` / 1.5 s, on    |
| `createMissile` player | none   | 1      | shooter's | 800 right | 16x4  | none                          |
| `createMissile` enemy  | none   | 1      | shooter's | 400 left  | 8x8   | none                          |

```cpp
const Entity ship = common::createShip(world, {100, 500});
world.get<common::Weapon>(ship).firing = true;  // shoot
```

## Tick

```cpp
world.addResource<ampersand::core::Clock>();
world.addResource<physics::Collisions>();
world.addResource<common::Deaths>();
world.addResource<common::Spawns>();

while (running) {
  shooting.update(world);   // 1. before the movement
  movement.update(world);   // 2. engine
  collision.update(world);  // 3. engine
  damage.update(world);     // 4. after the collisions
  // 5. server: send Spawns (ENTITY_SPAWN) and Deaths (ENTITY_DESTROY)
  cleanup.update(world);    // 6. always last
}
```

## Rules

- **Deaths**: no system destroys an entity during the tick. The entity goes in
  `Deaths`, and the `CleanupSystem` destroys it at the end. An entity is in
  `Deaths` only once per tick.
- **Teams**: a missile gets the `Team` of its shooter, so it does not hurt its
  own team. No `Team` = hurts and is hurt by everyone.
- **Missiles**: a `Projectile` dies on its first hit, even if it touches several
  entities in the same tick.
- **Walls**: `Damage` without `Health` and without `Projectile` never dies.
- **Collisions**: only between entities with the same z. All factories use z = 0.

## Limits

- Damage only on contact, once per pair and per tick.
- Nothing removes what leaves the screen yet: a missile that hits nothing never
  dies.
- `Spawns` only has the entities created by a factory.
