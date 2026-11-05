--- The texts over the game: our player and room, our stats, the last event
--- and a banner. They are the children of the entity (see
--- world/prefabs/hud.json).
local Hud = {}

local State = server.State
local Reason = server.RejectReason

--- Color of each player, by player id.
local PLAYER_COLORS = {
    { r = 90, g = 220, b = 255 },
    { r = 255, g = 110, b = 255 },
    { r = 110, g = 255, b = 110 },
    { r = 255, g = 110, b = 110 },
}
--- Character size of the banner.
local BANNER_SIZE = 80
--- X the banner is centered on.
local SCREEN_CENTER_X = 960
--- Width of a letter, as a part of the character size, to center the banner.
local LETTER_WIDTH = 0.55
--- Shown once the connection has ended.
local QUIT_HINT = "Press any key to quit"

--- Why the server refused us. The account reasons are shown by the login
--- screen, which comes back.
local REFUSED = {
    [Reason.ServerFull] = "Server full",
    [Reason.GameInProgress] = "Game already started",
    [Reason.VersionMismatch] = "Wrong game version",
    [Reason.PartyFull] = "Room full",
    [Reason.InvalidPartyCode] = "Invalid room code",
    [Reason.RoomNotFound] = "Room closed",
}

--- "P1" to "P4", from a player id.
local function playerLabel(playerId)
    return "P" .. (playerId + 1)
end

--- Label of a room: its code if it is private, its number otherwise.
local function roomLabel(room)
    if room.code ~= "" then
        return "Code " .. room.code
    end
    return "Room " .. room.roomId
end

--- End of the sentence telling a player left.
local function leftText(reason)
    if reason == server.DisconnectReason.Timeout then
        return " lost connection"
    end
    if reason == server.DisconnectReason.Kicked then
        return " was kicked"
    end
    return " left"
end

--- Whether the lobby screen is shown: in the lobby, and while joining or
--- leaving a room.
local function isLobby(state)
    return state == State.InLobby or state == State.Joining
        or state == State.Leaving
end

--- Finds the four texts; nothing shown yet.
function Hud:start()
    local texts = getChildren(self.entity)
    self.status = texts[1]
    self.stats = texts[2]
    self.event = texts[3]
    self.banner = texts[4]
    self.playerId = nil
    self.gameOver = false
    self.shownState = nil
end

--- Writes a text of the HUD.
function Hud:setText(entity, value)
    getText(entity).value = value
end

--- Writes the banner, centered. Once the game is over it keeps the result,
--- until the next game starts.
function Hud:setBanner(value)
    if self.gameOver then
        return
    end
    getTransform(self.banner).position.x = SCREEN_CENTER_X
        - (#value * BANNER_SIZE * LETTER_WIDTH / 2)
    self:setText(self.banner, value)
end

--- Empties every text, for the lobby.
function Hud:clear()
    self.gameOver = false
    self.playerId = nil
    for _, entity in ipairs({ self.status, self.stats, self.event,
        self.banner }) do
        self:setText(entity, "")
    end
end

--- Shows the state of the connection: nothing while logged out (the login
--- screen shows it), emptied once on the way to the lobby (the packets of
--- the next room may already come while joining), our player and room in a
--- room, and why the connection ended.
function Hud:showConnection(state, room, reason)
    local enteringLobby = isLobby(state)
        and (self.shownState == nil or not isLobby(self.shownState))
    self.shownState = state
    if state == State.LoggedOut then
        self:setText(self.status, "")
    elseif state == State.Connecting then
        self:setText(self.status, "Connecting...")
    elseif isLobby(state) then
        if enteringLobby then
            self:clear()
        end
    elseif state == State.InRoom then
        local player = room and room.playerId or 0
        self.playerId = player
        local status = "You: " .. playerLabel(player)
        if room then
            status = status .. "   " .. roomLabel(room)
        end
        self:setText(self.status, status)
        local color = PLAYER_COLORS[player % server.MAX_PLAYERS + 1]
        local text = getText(self.status)
        text.color.r = color.r
        text.color.g = color.g
        text.color.b = color.b
    elseif state == State.Rejected then
        self:setBanner(REFUSED[reason] or "Refused by the server")
        self:setText(self.event, QUIT_HINT)
    elseif state == State.Unreachable then
        self:setBanner("Server unreachable")
        self:setText(self.event, QUIT_HINT)
    elseif state == State.Disconnected then
        self:setBanner("Disconnected")
        self:setText(self.event, QUIT_HINT)
    end
end

--- Updates the texts from a game packet: players joining or leaving, our
--- stats, the start and the end of a game. Other packets are ignored.
function Hud:apply(packet)
    if packet.type == "PlayerJoined" then
        self:setText(self.event, playerLabel(packet.playerId) .. " "
            .. packet.playerName .. " joined")
    elseif packet.type == "PlayerLeft" then
        self:setText(self.event,
            playerLabel(packet.playerId) .. leftText(packet.reason))
    elseif packet.type == "PlayerState" then
        if packet.playerId == self.playerId then
            self:setText(self.stats, "HP " .. packet.health .. "   Lives "
                .. packet.lives .. "   Score " .. packet.score)
        end
    elseif packet.type == "GameOver" then
        local result = packet.result == server.GameResult.Victory
            and "VICTORY" or "GAME OVER"
        self:setBanner(result .. "   Score " .. packet.finalScore)
        self.gameOver = true
    elseif packet.type == "GameStart" then
        self.gameOver = false
        self:setBanner("")
    end
end

--- Shows the state of the connection, then the game packets of the frame.
function Hud:update(dt)
    self:showConnection(server.state(), server.room(), server.rejectReason())
    for _, packet in ipairs(server.packets()) do
        self:apply(packet)
    end
end

return Hud
