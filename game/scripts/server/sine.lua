--- An enemy on a sine wave: moves up and down
local Sine = {}

--- Highest vertical speed, in world units per second.
local SPEED = 150
--- Seconds of one up and down.
local PERIOD = 2

--- Starts at the middle of the wave, going up.
function Sine:start()
    self.time = 0
end

--- Sets the vertical speed for the time since it appeared.
function Sine:update(dt)
    self.time = self.time + dt
    getVelocity(self.entity).value.y =
        -SPEED * math.cos(2 * math.pi * self.time / PERIOD)
end

return Sine
