--- Holds the effects of a death (explosion, pieces, ghost), which destroy
--- themselves: destroys the holder once all of them are gone.
local Burst = {}

--- Destroys the holder when it has no effect left.
function Burst:update(dt)
    if #getChildren(self.entity) == 0 then
        destroy(self.entity)
    end
end

return Burst
