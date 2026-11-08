--- The waves of enemies of a level, one every WAVE_INTERVAL seconds.
local Waves = {}

--- Waves in a level.
local WAVE_COUNT = 10
--- Enemies in the first wave; each next wave has one more.
local FIRST_WAVE_SIZE = 3
--- Seconds before the first wave
local FIRST_WAVE_DELAY = 2
--- Seconds between two waves.
local WAVE_INTERVAL = 5
--- Horizontal gap between two enemies of a wave.
local ENEMY_SPACING = 170
--- Free space above and below the enemies.
local VERTICAL_MARGIN = 64

--- Size of the screen, in world units (common::ShipMovement::SCREEN).
local SCREEN_WIDTH, SCREEN_HEIGHT = 1920, 1080
--- Prefabs of the enemies, and their height (the hitbox of the prefabs).
local ENEMY = "world/prefabs/common/enemy.json"
local SINE_ENEMY = "world/prefabs/server/enemy-sine.json"
local ENEMY_HEIGHT = 60

--- Creates an enemy from its prefab, its top-left corner at x and y.
local function spawn(prefab, x, y)
    local position = getTransform(instantiate(prefab)).position
    position.x = x
    position.y = y
end

--- Starts the level: the first wave comes after FIRST_WAVE_DELAY.
function Waves:start()
    self.wave = 0
    self.nextWaveIn = FIRST_WAVE_DELAY
end

--- Sends the next wave when it is time to.
function Waves:update(dt)
    if self.wave >= WAVE_COUNT then
        return
    end
    self.nextWaveIn = self.nextWaveIn - dt
    if self.nextWaveIn > 0 then
        return
    end
    self:sendWave()
    self.nextWaveIn = self.nextWaveIn + WAVE_INTERVAL
    if self.wave == WAVE_COUNT then
        finishWaves()
    end
end

--- Lines up the enemies of the next wave, each one at a random height.
function Waves:sendWave()
    local prefab = self.wave % 2 == 1 and SINE_ENEMY or ENEMY
    local heights = SCREEN_HEIGHT - 2 * VERTICAL_MARGIN - ENEMY_HEIGHT
    for index = 0, FIRST_WAVE_SIZE + self.wave - 1 do
        spawn(prefab, SCREEN_WIDTH + index * ENEMY_SPACING,
            VERTICAL_MARGIN + math.random() * heights)
    end
    self.wave = self.wave + 1
end

return Waves
