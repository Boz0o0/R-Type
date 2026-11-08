--- The lobby screen, shown while out of a room. Its texts are the children
--- of the entity (see world/prefabs/lobby-menu.json).
local Lobby = {}

local State = server.State
local Reason = server.RejectReason

--- The fixed lines, in their order on screen; the rooms come after them.
local QUICK_MATCH, CREATE_PUBLIC, CREATE_PRIVATE, JOIN_CODE = 0, 1, 2, 3
local CHOICES = 4
--- Rooms shown at once; the list scrolls with the choice.
local VISIBLE_ROOMS = 7

--- Colors of the lines.
local TEXT_COLOR = { r = 255, g = 255, b = 255 }
local CHOSEN_COLOR = { r = 90, g = 220, b = 255 }

--- The key help at the bottom.
local HELP = "Up/Down or stick: choose   Enter or A: confirm"
    .. "   Esc or Start: quit   (in a room, they come back here)"

--- Why the server refused a room request: any reason it sends is shown.
local REFUSED = {
    [Reason.ServerFull] = "Server full",
    [Reason.GameInProgress] = "Game already started",
    [Reason.VersionMismatch] = "Wrong game version",
    [Reason.InvalidName] = "Invalid player name",
    [Reason.PartyFull] = "Room full",
    [Reason.InvalidPartyCode] = "Invalid room code",
    [Reason.RoomNotFound] = "Room closed",
    [Reason.BadCredentials] = "Wrong username or password",
    [Reason.AccountExists] = "Username already taken",
    [Reason.AlreadyLoggedIn] = "Account already in use",
    [Reason.InvalidPassword] = "Invalid password",
    [Reason.TooManyAttempts] = "Too many attempts, wait a minute",
}

--- The buttons of config.json the menu reads. The first ones and the
--- letters do nothing but take the hint away, like every key.
local KEYS = { "Shift", "Fire", "Left", "Right", "Next", "Up", "Down",
    "Confirm", "Erase" }
for code = string.byte("A"), string.byte("Z") do
    KEYS[#KEYS + 1] = string.char(code)
end
for digit = 0, 9 do
    KEYS[#KEYS + 1] = tostring(digit)
end

--- The buttons of KEYS pressed this frame, each one once.
local function keysPressed()
    local pressed = {}
    for _, key in ipairs(KEYS) do
        if input.getButtonDown(key) then
            pressed[#pressed + 1] = key
        end
    end
    return pressed
end

--- Sets the color of a text.
local function setColor(text, color)
    text.color.r = color.r
    text.color.g = color.g
    text.color.b = color.b
end

--- Whether two room lists show the same rooms with the same players.
local function sameRooms(left, right)
    if #left ~= #right then
        return false
    end
    for index, room in ipairs(left) do
        if room.roomId ~= right[index].roomId
            or room.players ~= right[index].players then
            return false
        end
    end
    return true
end

--- Text of a room line: its number, its players, and whether it is full.
local function roomText(room)
    local text = "Room " .. room.roomId .. "   " .. room.players .. "/"
        .. server.MAX_PLAYERS .. " players"
    if room.players >= server.MAX_PLAYERS then
        text = text .. "   full"
    end
    return text
end

--- Starts on quick match, with the party code of the command line to join.
function Lobby:start()
    self.texts = getChildren(self.entity)
    self.selected = QUICK_MATCH
    self.code = ""
    self.message = ""
    self.hint = ""
    self.previousVertical = 0
    self.rooms = {}
    self.partyCode = arguments.partyCode
end

--- Converts a controller stick into one menu step.
function Lobby:controllerKey()
    local vertical = input.getAxis("StickVertical")
    local key = nil
    if vertical > 0 and self.previousVertical <= 0 then
        key = "Up"
    elseif vertical < 0 and self.previousVertical >= 0 then
        key = "Down"
    end
    self.previousVertical = vertical
    return key
end

--- Replaces the public rooms. The same room stays chosen if it is still
--- there; if it closed, the choice goes back to quick match.
function Lobby:setRooms(rooms)
    if sameRooms(rooms, self.rooms) then
        return
    end
    local chosenRoom = nil
    if self.selected >= CHOICES then
        chosenRoom = self.rooms[self.selected - CHOICES + 1].roomId
    end
    self.rooms = rooms
    if chosenRoom then
        self.selected = QUICK_MATCH
        for index, room in ipairs(self.rooms) do
            if room.roomId == chosenRoom then
                self.selected = CHOICES + index - 1
            end
        end
    end
end

--- Enter on the chosen line: sends its request, or shows a hint if the code
--- line has no code yet.
function Lobby:confirm()
    if self.selected >= CHOICES then
        server.joinRoom(self.rooms[self.selected - CHOICES + 1].roomId, "")
    elseif self.selected == QUICK_MATCH then
        server.joinRoom(0, "")
    elseif self.selected == CREATE_PUBLIC then
        server.createRoom(false)
    elseif self.selected == CREATE_PRIVATE then
        server.createRoom(true)
    elseif self.code == "" then
        self.hint = "Type the code of the room first"
    else
        server.joinRoom(0, self.code)
    end
end

--- Handles a key: Up and Down choose a line, Enter confirms it. On the code
--- line, the digits type the code and Backspace erases. Only digits: they
--- are the same keys on every layout, and the server makes codes of digits.
function Lobby:press(key)
    self.hint = ""
    local typing = self.selected == JOIN_CODE
    if key == "Up" then
        self.selected = math.max(self.selected - 1, 0)
    elseif key == "Down" then
        self.selected = math.min(self.selected + 1, CHOICES + #self.rooms - 1)
    elseif key == "Confirm" then
        self:confirm()
    elseif typing and key == "Erase" then
        self.code = string.sub(self.code, 1, -2)
    elseif typing and string.match(key, "^%d$")
        and #self.code < server.MAX_PARTY_CODE_LENGTH then
        self.code = self.code .. key
    end
end

--- Text of a fixed line; the code line shows the code typed, and a cursor
--- while chosen.
function Lobby:choiceText(choice)
    if choice == QUICK_MATCH then
        return "Quick match"
    end
    if choice == CREATE_PUBLIC then
        return "Create a public room"
    end
    if choice == CREATE_PRIVATE then
        return "Create a private room"
    end
    local typing = self.selected == JOIN_CODE
    return "Join with a code: " .. self.code .. (typing and "_" or "")
end

--- First room shown (from 0), so that the chosen one is on screen.
function Lobby:firstVisibleRoom()
    if self.selected < CHOICES or self.selected - CHOICES < VISIBLE_ROOMS then
        return 0
    end
    return self.selected - CHOICES - VISIBLE_ROOMS + 1
end

--- Writes every text. Hidden, they are emptied once and then left alone.
function Lobby:refresh(visible)
    if not visible and self.hidden then
        return
    end
    self.hidden = not visible
    local function setLine(text, value, color)
        text.value = visible and value or ""
        if color then
            setColor(text, color)
        end
    end
    local function marked(line, text)
        return (self.selected == line and "> " or "  ") .. text
    end
    local function colorOf(line)
        return self.selected == line and CHOSEN_COLOR or TEXT_COLOR
    end

    setLine(getText(self.texts[1]), "LOBBY")
    for choice = 0, CHOICES - 1 do
        setLine(getText(self.texts[choice + 2]),
            marked(choice, self:choiceText(choice)), colorOf(choice))
    end
    local header = "Public rooms"
    if #self.rooms > VISIBLE_ROOMS then
        header = header .. " (" .. #self.rooms .. ")"
    end
    setLine(getText(self.texts[CHOICES + 2]),
        #self.rooms == 0 and "No public room yet" or header)
    local first = self:firstVisibleRoom()
    for index = 0, VISIBLE_ROOMS - 1 do
        local text = getText(self.texts[CHOICES + 3 + index])
        local room = self.rooms[first + index + 1]
        if room then
            local line = CHOICES + first + index
            setLine(text, marked(line, roomText(room)), colorOf(line))
        else
            setLine(text, "", TEXT_COLOR)
        end
    end
    local messageIndex = CHOICES + 3 + VISIBLE_ROOMS
    setLine(getText(self.texts[messageIndex]),
        self.message == "" and self.hint or self.message)
    setLine(getText(self.texts[messageIndex + 1]), HELP)
end

--- In the lobby, joins the party code of the command line once, then
--- handles the keys. Then shows the rooms and the message.
function Lobby:update(dt)
    if server.state() == State.InLobby then
        if self.partyCode ~= "" then
            server.joinRoom(0, self.partyCode)
            self.partyCode = ""
        end
        local controllerKey = self:controllerKey()
        if controllerKey then
            self:press(controllerKey)
        end
        for _, key in ipairs(keysPressed()) do
            self:press(key)
        end
    end
    local state = server.state()
    self:setRooms(server.rooms())
    local reason = server.roomRejectReason()
    if state == State.Joining then
        self.message = "Joining..."
    elseif state == State.Leaving then
        self.message = "Leaving the room..."
    elseif reason then
        self.message = REFUSED[reason] or "Refused by the server"
    else
        self.message = ""
    end
    self:refresh(state == State.InLobby or state == State.Joining
        or state == State.Leaving)
end

return Lobby
