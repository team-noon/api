--[[
  tacapi.lua — vékony Lua réteg a C `c_api` függvény körül (game.c).

  Szögkonvenció: a taktika fokban számol; Move/Kick hívás előtt radiánra váltunk,
  mert a C oldal cos/sin közvetlenül radiánnal dolgozik.

  Parancs kódok (első argumentum): 0 állapot, 10–16 pozíció/sebesség, 20 mozgás, 21 rúgás, stb.
]]

require "vec2"

API = {} -- Tactics API

local DEG2RAD = math.pi / 180

-- Események (helykitöltő / jövőbeli bővítéshez)
API.Event = {}
API.Event.Type = {stateChange = 0, message = 1}

function API.Event.Count()
    return 0
end

function API.Event.Poll()
    return nil
end

-- TODO

-- Játékállapot (C: case 0)
API.Game = {}

function API.Game.State() -- Get Game State
    return c_api(0)
end

-- TODO

-- Objektumok (C: 1x esetek)
API.Locate = {}

function API.Locate.Teammate(id) -- Get Teammate Position
    return Vec2:New(c_api(10, id))
end

function API.Locate.Opp(id) -- Get Opponent Position
    return Vec2:New(c_api(11, id))
end

function API.Locate.Ball() -- Get Ball Position
    return Vec2:New(c_api(12))
end

function API.Locate.OppGoal() -- Get Ball Position
    return Vec2:New(c_api(13))
end

-- Robot vezérlés (C: 2x esetek)
API.Robot = {}

function API.Robot.Move(direction, speed)
    -- src/game.c expects `dir` in radians (it uses cos/sin directly).
    c_api(20, direction * DEG2RAD, speed)
end

function API.Robot.Kick(angle, force)
    -- src/game.c expects `angle` in radians.
    c_api(21, angle * DEG2RAD, force)
end

-- Sebességek (C: 14–16)
API.Locate.TeammateVel = function(id)
    return Vec2:New(c_api(14, id))
end

API.Locate.OppVel = function(id)
    return Vec2:New(c_api(15, id))
end

API.Locate.BallVel = function()
    return Vec2:New(c_api(16))
end

-- Utolsó ütközés óta eltelt idő (proxy) — C: case 22
API.Robot.CollisionAge = function()
    return c_api(22)
end

-- Kommunikáció (C: 3x) — részben TODO
API.Comms = {}

function API.Comms.Send(id, message) -- Send Message to Teammate
    c_api(30, id, message)
end

-- receive options