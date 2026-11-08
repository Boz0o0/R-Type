--- The ghost of a lost plane: its prefab flies it up, and it fades out during
--- the second half of its LIFE, then is destroyed.
local Ghost = {}

--- Seconds the ghost is seen.
local LIFE = 3

--- Starts fully seen.
function Ghost:start()
    self.age = 0
end

--- Fades the ghost out, then destroys it.
function Ghost:update(dt)
    self.age = self.age + dt
    local left = 1 - self.age / LIFE
    if left <= 0 then
        destroy(self.entity)
        return
    end
    getSprite(self.entity).color.a = math.floor(255 * math.min(1, left * 2))
end

return Ghost
