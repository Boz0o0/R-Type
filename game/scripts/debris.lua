--- A piece thrown by an explosion: its prefab gives its first speed, it falls
--- with GRAVITY and fades out during its LIFE, then is destroyed.
local Debris = {}

--- Fall acceleration, in world units per second squared.
local GRAVITY = 900
--- Seconds the piece is seen.
local LIFE = 1.2

--- Starts fully seen.
function Debris:start()
    self.age = 0
end

--- Makes the piece fall and fade, then destroys it.
function Debris:update(dt)
    self.age = self.age + dt
    if self.age >= LIFE then
        destroy(self.entity)
        return
    end
    local velocity = getVelocity(self.entity).value
    velocity.y = velocity.y + GRAVITY * dt
    getSprite(self.entity).color.a = math.floor(255 * (1 - self.age / LIFE))
end

return Debris
