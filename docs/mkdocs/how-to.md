# How-to

Quick changes to the game, without touching the C++. Every file is in `game/`:
the client reads it from its binary folder (`cmake --build build --target
client_game` copies it there), the server from where it runs. To create a new
enemy, see the [Tutorial](tutorial.md).

## Change the waves

The constants at the top of `game/scripts/server/waves.lua`:

| Constant | Sets |
| --- | --- |
| `WAVE_COUNT` | waves in a level (10) |
| `FIRST_WAVE_SIZE` | enemies in the first wave (3), each next wave has one more |
| `FIRST_WAVE_DELAY` | seconds before the first wave (2) |
| `WAVE_INTERVAL` | seconds between two waves (5) |
| `ENEMY_SPACING` | horizontal gap between two enemies of a wave |

Which enemy each wave sends is chosen in `Waves:sendWave()`.

## Change an enemy

The gameplay of an enemy is its prefab in `game/world/prefabs/common/`. In
`enemy.json`:

```json
"Velocity": { "value": { "x": -200, "y": 0 } },
"Health": { "current": 2, "max": 2 },
"Damage": { "amount": 1 },
"Weapon": { "cooldown": 1.5, ... }
```

`enemy-sine.json` inherits it, so a change here changes both enemies. To
change only one, set the component in its own prefab: only the fields given
replace the inherited ones. The same goes for `ship.json` (the planes) and the
missiles.

## Change a sprite or a sound

The look is in the client prefabs, in `game/world/prefabs/`: the `texture` of
the `Sprite`, and the `sound` and `volume` of the `AudioSource`.

```json
"Sprite": { "texture": "assets/sprites/blimp-green.png", ... }
"AudioSource": { "sound": "assets/sounds/shot.wav", "volume": 30, ... }
```

The files go in `game/assets/`. A new sprite sheet also needs new frames in
the `AnimationStateMachine` of the prefab.

## Change the keys and the volume

In `game/world/config.json`, each action of the `InputManager` lists its keys
and buttons, and the `AudioListener` takes a global `volume` (100 by default):

```json
"AudioListener": { "backend": "sfml", "volume": 60 },
"InputManager": {
  "buttons": {
    "Fire": ["Space", "Button0", "LeftControl"]
  }
}
```

## Give an enemy a new movement

A server script moves an enemy by changing its speed, like
`game/scripts/server/sine.lua`:

```lua
function Sine:update(dt)
    self.time = self.time + dt
    getVelocity(self.entity).value.y =
        -SPEED * math.cos(2 * math.pi * self.time / PERIOD)
end
```

It is given to the enemy by its prefab in `game/world/prefabs/server/`, in
`Scripts`. The Lua functions are listed in the
[engine docs](https://boz0o0.github.io/Ampersand-Engine/architecture/scripting/#engine-api).

