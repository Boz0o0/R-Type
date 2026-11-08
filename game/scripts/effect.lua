--- An effect played once (an explosion, a spark, a flash): destroys its entity
--- once its animation and its sound are over.
local Effect = {}

--- Destroys the entity when nothing plays anymore.
function Effect:update(dt)
    local sounding = hasAudioSource(self.entity)
        and getAudioSource(self.entity).playing
    if animation.isFinished(self.entity) and not sounding then
        destroy(self.entity)
    end
end

return Effect
