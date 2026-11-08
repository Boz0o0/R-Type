--- The texts over the game: our player and room, our stats, the last event
--- and a banner, and the voice of the announcer at the start and the end of a
--- game. They are the children of the entity (see world/prefabs/hud.json).
local Hud = {}

local State = server.State
local Reason = server.RejectReason

--- Color of each player, by player id: the color of its plane.
local PLAYER_COLORS = {
    { r = 255, g = 90, b = 90 },
    { r = 90, g = 160, b = 255 },
    { r = 255, g = 215, b = 60 },
    { r = 220, g = 220, b = 220 },
}
--- Character size of the banner.
local BANNER_SIZE = 80
--- X the banner is centered on.
local SCREEN_CENTER_X = 960
--- Width of each character in the font of the banner, as a part of the
--- character size, to center it (the font has no kerning).
local CHAR_WIDTHS = {
    [" "] = 0.235, ["!"] = 0.396, ["$"] = 0.758, ["%"] = 1.395, ["&"] = 1.000,
    ["+"] = 0.656, [","] = 0.396, ["-"] = 0.656, ["."] = 0.396, ["/"] = 0.799,
    ["0"] = 0.905, ["1"] = 0.646, ["2"] = 0.727, ["3"] = 0.727, ["4"] = 0.776,
    ["5"] = 0.778, ["6"] = 0.778, ["7"] = 0.804, ["8"] = 0.778, ["9"] = 0.778,
    [":"] = 0.396, [";"] = 0.396, ["="] = 0.656, ["?"] = 0.724, ["@"] = 1.000,
    A = 0.973, B = 0.778, C = 0.824, D = 0.918, E = 0.656, F = 0.648,
    G = 0.832, H = 0.776, I = 0.396, J = 0.827, K = 0.752, L = 0.676,
    M = 0.900, N = 0.804, O = 0.940, P = 0.894, Q = 0.992, R = 0.902,
    S = 0.758, T = 0.804, U = 0.905, V = 0.854, W = 1.347, X = 0.802,
    Y = 0.760, Z = 0.799, a = 0.717, b = 0.542, c = 0.548, d = 0.670,
    e = 0.440, f = 0.440, g = 0.592, h = 0.592, i = 0.244, j = 0.583,
    k = 0.592, l = 0.440, m = 0.676, n = 0.592, o = 0.644, p = 0.614,
    q = 0.644, r = 0.614, s = 0.542, t = 0.592, u = 0.650, v = 0.650,
    w = 0.942, x = 0.592, y = 0.592, z = 0.592
}
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

--- Width of a text of the banner, in world units.
local function bannerWidth(value)
    local width = 0
    for char in value:gmatch(".") do
        width = width + CHAR_WIDTHS[char]
    end
    return width * BANNER_SIZE
end

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

--- Finds the four texts and the three voices; nothing shown yet.
function Hud:start()
    local children = getChildren(self.entity)
    self.status = children[1]
    self.stats = children[2]
    self.event = children[3]
    self.banner = children[4]
    self.startVoice = children[5]
    self.knockoutVoice = children[6]
    self.gameOverTune = children[7]
    self.playerId = nil
    self.gameOver = false
    self.shownState = nil
end

--- Writes a text of the HUD.
function Hud:setText(entity, value)
    getText(entity).value = value
end

--- Plays a voice from its start.
function Hud:play(entity)
    getAudioSource(entity).playing = true
end

--- Writes the banner, centered. Once the game is over it keeps the result,
--- until the next game starts.
function Hud:setBanner(value)
    if self.gameOver then
        return
    end
    getTransform(self.banner).position.x = SCREEN_CENTER_X
        - bannerWidth(value) / 2
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
        local victory = packet.result == server.GameResult.Victory
        local result = victory and "VICTORY" or "GAME OVER"
        self:setBanner(result .. "   Score " .. packet.finalScore)
        self.gameOver = true
        self:play(victory and self.knockoutVoice or self.gameOverTune)
    elseif packet.type == "GameStart" then
        self.gameOver = false
        self:setBanner("")
        getAudioSource(self.gameOverTune).playing = false
        self:play(self.startVoice)
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
