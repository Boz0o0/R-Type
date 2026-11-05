--- The login screen, shown until the server lets us in. Its texts are the
--- children of the entity (see world/prefabs/login-menu.json).
local Login = {}

local State = server.State
local Reason = server.RejectReason

--- The lines, in their order on screen.
local USERNAME, PASSWORD, LOG_IN, CREATE_ACCOUNT = 0, 1, 2, 3
local LINES = 4

--- Colors of the lines.
local TEXT_COLOR = { r = 255, g = 255, b = 255 }
local CHOSEN_COLOR = { r = 90, g = 220, b = 255 }

--- The key help at the bottom.
local HELP = "Up/Down/Tab: choose   Shift: upper case   Enter: confirm"
    .. "   Esc: quit"

--- Why the server refused the account. The other reasons end the
--- connection, and the HUD shows them.
local REFUSED = {
    [Reason.InvalidName] = "Invalid player name",
    [Reason.BadCredentials] = "Wrong username or password",
    [Reason.AccountExists] = "Username already taken",
    [Reason.AlreadyLoggedIn] = "Account already in use",
    [Reason.InvalidPassword] = "Invalid password",
    [Reason.TooManyAttempts] = "Too many attempts, wait a minute",
}

--- Sets the color of a text.
local function setColor(text, color)
    text.color.r = color.r
    text.color.g = color.g
    text.color.b = color.b
end

--- Starts on the username, or on the password when the username comes from
--- the command line.
function Login:start()
    self.texts = getChildren(self.entity)
    self.selected = USERNAME
    self.username = arguments.username
    self.password = ""
    self.message = ""
    self.hint = ""
    if #self.username > server.MAX_PLAYER_NAME_LENGTH then
        self.username = ""
    end
    if self.username ~= "" then
        self.selected = PASSWORD
    end
end

--- The field typed in on the selected line and its maximum length, if it is
--- one.
function Login:field()
    if self.selected == USERNAME then
        return "username", server.MAX_PLAYER_NAME_LENGTH
    end
    if self.selected == PASSWORD then
        return "password", server.MAX_PASSWORD_LENGTH
    end
    return nil
end

--- The buttons of config.json the menu reads. The first ones do nothing but
--- take the hint away, like every key.
local KEYS = { "Shift", "Fire", "Left", "Right", "Up", "Down", "Next",
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

--- The character a key types (upper case with Shift), if it types one.
local function characterOf(key, shift)
    if string.match(key, "^%u$") then
        return shift and key or string.lower(key)
    end
    if string.match(key, "^%d$") then
        return key
    end
    return nil
end

--- Enter on the selected line: the login asked for, if one is complete. The
--- password is not kept once sent.
function Login:confirm(createAccount)
    if self.username == "" then
        self.hint = "Type your username first"
        self.selected = USERNAME
        return nil
    end
    if self.password == "" then
        self.hint = "Type your password first"
        self.selected = PASSWORD
        return nil
    end
    local password = self.password
    self.password = ""
    return {
        username = self.username,
        password = password,
        createAccount = createAccount,
    }
end

--- Handles a key: Up, Down and Tab move between the lines, letters and
--- digits are typed in the selected field, Backspace erases. Enter goes from
--- the username to the password, and elsewhere logs in or registers. Returns
--- the login asked for, if the key confirmed one.
function Login:press(key, shift)
    local request = nil
    self.hint = ""
    local field, limit = self:field()
    local character = characterOf(key, shift)
    if key == "Up" then
        self.selected = math.max(self.selected - 1, 0)
    elseif key == "Down" then
        self.selected = math.min(self.selected + 1, LINES - 1)
    elseif key == "Next" then
        self.selected = (self.selected + 1) % LINES
    elseif key == "Confirm" then
        if self.selected == USERNAME then
            self.selected = PASSWORD
        else
            request = self:confirm(self.selected == CREATE_ACCOUNT)
        end
    elseif field ~= nil and key == "Erase" then
        self[field] = string.sub(self[field], 1, -2)
    elseif field ~= nil and character ~= nil and #self[field] < limit then
        self[field] = self[field] .. character
    end
    return request
end

--- Text of a line, with a cursor on the selected field. The password is
--- shown as stars.
function Login:lineText(line)
    local cursor = self.selected == line and "_" or ""
    if line == USERNAME then
        return "Username: " .. self.username .. cursor
    end
    if line == PASSWORD then
        return "Password: " .. string.rep("*", #self.password) .. cursor
    end
    if line == LOG_IN then
        return "Log in"
    end
    return "Create an account"
end

--- Writes every text. Hidden, they are emptied once and then left alone.
function Login:refresh(visible)
    if not visible and self.hidden then
        return
    end
    self.hidden = not visible
    local function setLine(text, value)
        text.value = visible and value or ""
    end
    setLine(getText(self.texts[1]), "R-TYPE")
    for line = 0, LINES - 1 do
        local chosen = self.selected == line
        local text = getText(self.texts[line + 2])
        setLine(text, (chosen and "> " or "  ") .. self:lineText(line))
        setColor(text, chosen and CHOSEN_COLOR or TEXT_COLOR)
    end
    setLine(getText(self.texts[6]), self.hint == "" and self.message or self.hint)
    setLine(getText(self.texts[7]), HELP)
end

--- Handles the keys while logged out (the ones after a login are not for
--- the menu), then shows the message.
function Login:update(dt)
    if server.state() == State.LoggedOut then
        local shift = input.getButton("Shift")
        for _, key in ipairs(keysPressed()) do
            local request = self:press(key, shift)
            if request then
                server.logIn(request.username, request.password,
                    request.createAccount)
                break
            end
        end
    end
    local state = server.state()
    local reason = server.rejectReason()
    if state == State.Connecting then
        self.message = "Connecting..."
    elseif state == State.LoggedOut and reason then
        self.message = REFUSED[reason] or "Refused by the server"
    else
        self.message = ""
    end
    self:refresh(state == State.LoggedOut or state == State.Connecting)
end

return Login
