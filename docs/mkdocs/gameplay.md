# Gameplay

## The game

A horizontal shoot'em up for 1 to 4 players, in the same room. Each player
flies a plane on the left of the screen, the enemies come from the right.

| | |
| --- | --- |
| Goal | survive the 10 waves together, and kill as many enemies as possible |
| Controls | move in every direction (450 units/s, never out of the screen), hold Shoot to fire 4 missiles per second |
| Health | 3 hit points per plane, shown in the HUD; an enemy bullet takes 1, touching an enemy kills at once |
| Lives | 3 per player; a lost plane comes back after 2 s, while lives are left |
| Enemies | blimps with 2 hit points (2 missiles), firing every 1.5 s: the green one flies straight, the purple one on a sine |
| Waves | the first after 2 s, then one every 5 s; wave n has 3 + n enemies at random heights, the purple ones every other wave |
| Score | 100 per enemy, to the player who dealt the last hit; each player has their own |
| Friendly fire | none: the missiles only hurt the other team |
| Victory | every wave was sent and no enemy is left |
| Defeat | every player lost all their lives |
| Restart | 5 s after the end, with the players still in the room |

A player who joins during a game gets a plane at once. The waves are in
`game/scripts/server/waves.lua`, the numbers of each entity in its prefab of
`game/world/prefabs/common/`.

## In the code

The rules are in `src/common/` (shared by the server and the tests) and
`src/server/Game.cpp` (lives, score, waves, victory). The server runs them, the
clients only show the result.

| Name | Type | Role |
| --- | --- | --- |
| `Health` | component | hit points (`current`, `max`), dies at 0 |
| `Damage` | component | hit points removed from what it touches |
| `Team` | component | `Players` or `Enemies`, no damage inside a team |
| `Kind` | component | what it is (`Ship`, `Enemy`, `PlayerMissile`, `EnemyMissile`...) and its `variant`, sent to the clients |
| `Weapon` | component | fires its `missile` prefab every `cooldown` seconds while `firing` |
| `Projectile` | component | dies on its first hit |
| `Owner` | component | the player of a ship, copied to its missiles |
| `Deaths` | resource | the entities dead this tick, why, and what hit them last |
| `ShootingSystem` | system | creates the missiles, in front of their shooter |
| `DamageSystem` | system | applies the damage of the collisions, fills `Deaths` |
| `OutOfScreenSystem` | system | kills what left the screen for good (ships never leave it) |
| `CleanupSystem` | system | destroys the `Deaths`, last of the tick |

Each tick, in this order:

```cpp
shooting.update(world);    // the missiles, before they move
movement.update(world);    // engine
collision.update(world);   // engine
damage.update(world);      // after the collisions
outOfScreen.update(world); // after the movement
// server: lives, score, and the new entities and deaths sent to the clients
cleanup.update(world);     // always last
```

No system destroys an entity during the tick: it goes in `Deaths`, once, and
the `CleanupSystem` destroys it at the end.
