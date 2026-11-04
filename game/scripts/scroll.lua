-- One of two star-field copies placed side by side: once it is fully off the
-- left of the screen, it jumps behind the other copy, so the stars never end.
-- The MovementSystem moves it with the Velocity of its prefab.
local Scroll = {}

local WIDTH = 1920
local COPIES = 2

function Scroll:update(dt)
    local transform = getTransform(self.entity)
    if transform.position.x <= -WIDTH then
        transform.position.x = transform.position.x + COPIES * WIDTH
    end
end

return Scroll
