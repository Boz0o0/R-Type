--- A plane that tilts up while it climbs and down while it dives: gives its
--- vertical speed to the "speedY" parameter of its AnimationStateMachine. The
--- speed comes from the moves of the entity, so it works for every plane, the
--- predicted one included.
local Tilt = {}

--- Part of the measured speed kept each frame, to smooth the corrections.
local SMOOTHING = 0.25

--- Starts at rest.
function Tilt:start()
    self.lastY = getTransform(self.entity).position.y
    self.speedY = 0
end

--- Measures the vertical speed and gives it to the animation.
function Tilt:update(dt)
    local y = getTransform(self.entity).position.y
    if dt > 0 then
        local measured = (y - self.lastY) / dt
        self.speedY = self.speedY + SMOOTHING * (measured - self.speedY)
    end
    self.lastY = y
    animation.setFloat(self.entity, "speedY", self.speedY)
end

return Tilt
