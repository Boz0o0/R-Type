--- A blimp that plays its attack each time its gun fires. The server fires it
--- FIRST_SHOT seconds after it appears, then every COOLDOWN seconds (the
--- Weapon of world/prefabs/common/enemy.json).
local Blimp = {}

--- Seconds before the first shot, then between two shots.
local FIRST_SHOT = 1.5
local COOLDOWN = 1.5
--- Seconds of the attack played before the bullet leaves.
local WIND_UP = 0.25

--- Waits for the first shot.
function Blimp:start()
    self.nextAttack = FIRST_SHOT - WIND_UP
end

--- Plays the attack once per shot.
function Blimp:update(dt)
    self.nextAttack = self.nextAttack - dt
    if self.nextAttack <= 0 then
        animation.playOnce(self.entity, "attack")
        self.nextAttack = self.nextAttack + COOLDOWN
    end
end

return Blimp
