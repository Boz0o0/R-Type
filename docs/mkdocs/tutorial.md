# Tutorial: a new enemy

This page adds a zigzag enemy and puts it in the waves. It only uses JSON and
Lua, plus one line of C++ for its look. The formats come from the engine: see
its [prefabs](https://boz0o0.github.io/Ampersand-Engine/architecture/scene/#prefabs)
and its [Lua functions](https://boz0o0.github.io/Ampersand-Engine/architecture/scripting/#engine-api).

An enemy is three prefabs: its **gameplay** in `common/` (read by both sides),
its **behavior** in `server/` (a script, run by the server only) and its
**look** (read by the client only). The server tells the client which look to
use with the `variant` of the `Kind`.

## 1. The gameplay

`game/world/prefabs/common/enemy-zigzag.json`: the normal enemy, as variant 2.
Any component of `enemy.json` can be changed here, like its health.

```json
{
  "version": 2,
  "name": "Enemy on a zigzag",
  "prefab": "world/prefabs/common/enemy.json",
  "components": {
    "Kind": { "variant": 2 },
    "Health": { "current": 3, "max": 3 }
  }
}
```

## 2. The behavior

`game/scripts/server/zigzag.lua`: every second, the enemy changes its vertical
direction.

```lua
--- An enemy on a zigzag: goes up and down, changing every TURN seconds.
local Zigzag = {}

--- Vertical speed, in world units per second.
local SPEED = 250
--- Seconds between two turns.
local TURN = 1

function Zigzag:start()
    self.left = TURN
    getVelocity(self.entity).value.y = -SPEED
end

function Zigzag:update(dt)
    self.left = self.left - dt
    if self.left <= 0 then
        self.left = self.left + TURN
        local velocity = getVelocity(self.entity).value
        velocity.y = -velocity.y
    end
end

return Zigzag
```

`game/world/prefabs/server/enemy-zigzag.json` gives it to the enemy:

```json
{
  "version": 2,
  "name": "Enemy on a zigzag",
  "prefab": "world/prefabs/common/enemy-zigzag.json",
  "components": {
    "Scripts": { "scripts": ["scripts/server/zigzag.lua"] }
  }
}
```

## 3. The look

`game/world/prefabs/enemy-zigzag.json`: the green blimp, in red.

```json
{
  "version": 2,
  "name": "Enemy on a zigzag",
  "prefab": "world/prefabs/enemy.json",
  "components": {
    "Sprite": { "color": { "r": 255, "g": 120, "b": 120, "a": 255 } }
  }
}
```

Then add it at index 2 (its variant) of `ENEMIES` in
`include/rtype/client/PrefabFactory.hpp`, and rebuild the client:

```cpp
static constexpr std::array<std::string_view, 3> ENEMIES{
    "world/prefabs/enemy.json",
    "world/prefabs/enemy-sine.json",
    "world/prefabs/enemy-zigzag.json",
};
```

## 4. The waves

`game/scripts/server/waves.lua` chooses the prefab of each wave in
`Waves:sendWave()`. For example, every third wave on a zigzag:

```lua
local ZIGZAG_ENEMY = "world/prefabs/server/enemy-zigzag.json"

local prefab = ENEMY
if self.wave % 3 == 2 then
    prefab = ZIGZAG_ENEMY
elseif self.wave % 2 == 1 then
    prefab = SINE_ENEMY
end
```

The other constants at the top of the file set the waves themselves: how many
(`WAVE_COUNT`), their size (`FIRST_WAVE_SIZE`) and their rhythm
(`FIRST_WAVE_DELAY`, `WAVE_INTERVAL`).

## 5. Try it
