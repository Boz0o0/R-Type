# Gameplay

## Concepts

| Name             | Type      | Role                                                         |
| ---------------- | --------- | ------------------------------------------------------------ |
| `Health`         | component | hit points (`current`, `max`), dies at 0                     |
| `Damage`         | component | hit points removed from what it touches                      |
| `Team`           | component | `Players` or `Enemies`, no damage inside a team              |
| `Kind`           | component | what it is: `Ship`, `Enemy`, `PlayerMissile`, `EnemyMissile`, `PowerUp`, `Obstacle` (sent to the clients, values never change), and its `variant` (0 by default, 1 for the sine enemy) |
| `Weapon`         | component | fires a `missile` every `cooldown` seconds while `firing`    |
| `Projectile`     | component | tag, dies on its first hit                                   |
| `Owner`          | component | player a ship belongs to; its missiles get it too            |
| `Deaths`         | resource  | entities dead this tick, why, and what dealt the last hit    |
| `ShootingSystem` | system    | creates the missiles                                         |
| `DamageSystem`   | system    | applies the damage of the `Collisions`, fills `Deaths`       |
| `OutOfScreenSystem` | system | kills (`OutOfScreen`) what left the screen for good          |
| `CleanupSystem`  | system    | destroys `Deaths`, empties `Deaths`                          |

## Entities

The entities are the prefabs of `game/world/prefabs/common/`, with the
gameplay components only. The client prefabs inherit them and add the look,
the server prefabs of `server/` add the scripts. `registerComponents` lets a
`SceneManager` read these components, and `createShip` and `createMissile`
instantiate the prefabs.

| Prefab                | Health | Damage | Team      | Speed     | Size  | Weapon                        |
| --------------------- | ------ | ------ | --------- | --------- | ----- | ----------------------------- |
| `ship.json`           | 3      | none   | `Players` | 0         | 64x32 | `player-missile.json` / 0.25 s, off |
| `enemy.json`          | 2      | 1      | `Enemies` | 200 left  | 64x64 | `enemy-missile.json` / 1.5 s, on    |
| `enemy-sine.json`     | 2      | 1      | `Enemies` | 200 left, up and down | 64x64 | `enemy-missile.json` / 1.5 s, on |
| `player-missile.json` | none   | 1      | shooter's | 800 right | 16x4  | none (and the shooter's `Owner`) |
| `enemy-missile.json`  | none   | 1      | shooter's | 400 left  | 8x8   | none                          |

```cpp
common::registerComponents(scenes);
scenes.loadConfig(world, "game/world/server.json");
if (const auto ship = common::createShip(world, scenes, {100, 500})) {
  world.get<common::Weapon>(*ship).firing = true;  // shoot
}
```

## Tick

```cpp
world.addResource<ampersand::core::Clock>();
world.addResource<physics::Collisions>();
world.addResource<common::Deaths>();

while (running) {
  shooting.update(world);   // 1. before the movement
  movement.update(world);   // 2. engine
  collision.update(world);  // 3. engine
  damage.update(world);     // 4. after the collisions
  outOfScreen.update(world);// 5. after the movement
  // 6. server: send Spawns (ENTITY_SPAWN) and Deaths (ENTITY_DESTROY)
  cleanup.update(world);    // 7. always last
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
- **Kills**: when `Damage` takes the last hit points, `Death::killer` is the
  entity that hit. The server gives the score to its `Owner`.
- **Screen**: an entity dies `OutOfScreen` once its collider is fully out of
  the screen and its speed does not bring it back: an enemy waiting on the
  right and flying left stays alive. Ships are never killed this way.
- **Collisions**: only between entities with the same z. All prefabs use z = 0.

## Limits

- Damage only on contact, once per pair and per tick.
- A factory returns nothing (`std::nullopt`) when the World is full or the
  prefab cannot be loaded.

## Server rules

The server runs the game in `rtype::server::Game` (see
[Protocol code](protocol/implementation.md#server-loop)):

| Rule | Value |
| ---- | ----- |
| Game starts | as soon as one player is in the room (each room has its own game) |
| Ship speed | 450 units/s, keys from `PLAYER_INPUT`, kept on the screen |
| Lives | 3 per player; a lost ship comes back after 2 s |
| Waves | 10 waves, one every 5 s (first after 2 s); wave n has 3 + n Bydos, on a sine every other wave (`game/scripts/server/waves.lua`) |
| Score | 100 per Bydos, to the player whose missile killed it |
| Defeat | every player has lost all its lives |
| Victory | every wave was sent and no Bydos is left |
| Restart | 5 s after the end, with the players still in the room |
