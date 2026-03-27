--[[
  logic/striker.lua — támadó viselkedés (támadó szerepben hívja a tactics.lua).

  Nem teljes FSM táblázat: főleg „labda felé futás” + élszintű rúgás, ha robot–labda távolság
  kicsi (sugár + puffer). A rúgás mindig a labda pozíciójából számolt irány; másik robot
  nem lehet cél — a távolság kizárólag robotPos vs. ballPos.

  Megjegyzés: a „is_valid_ball_object” itt strukturális ellenőrzés (x, y mezők), nem C-beli
  entity ID — a labdát a tactics.lua mindig API.Locate.Ball()-ból tölti.
]]

require "vec2"
local Utils = dofile("logic/utils.lua")
local Lib = dofile("tactics_lib.lua")

local Striker = {}
--- Belső állapot: beragadás számláló (lassú labda közelben).
local state = { jamFrames = 0 }

local function clamp(v, lo, hi)
    if v < lo then return lo end
    if v > hi then return hi end
    return v
end

--- Érvényes labdapozíció-e (tábla, koordináták) — nem robot másolata.
local function is_valid_ball_object(ballPos)
    return ballPos ~= nil and type(ballPos) == "table" and ballPos.x ~= nil and ballPos.y ~= nil
end

--- Van-e ellenfél a labda → kapu vonal közelében (passz/lövés sáv blokkolás).
local function is_goal_path_blocked(ballPos, enemyGoal, opponents)
    if opponents == nil then return false end
    local blockRadius2 = 35 * 35
    for _, opp in ipairs(opponents) do
        if Utils.dist2_point_segment(opp, ballPos, enemyGoal) < blockRadius2 then
            return true
        end
    end
    return false
end

--- Előre lévő csapattárs választása passzhoz (ha van tiszta sáv).
local function pick_forward_teammate(ballPos, enemyGoal, myGoal, teammates, opponents)
    if teammates == nil then return nil end
    local enemyOnRight = true
    if enemyGoal ~= nil and myGoal ~= nil then
        enemyOnRight = enemyGoal.x > myGoal.x
    end
    local best = nil
    local bestProgress = -math.huge
    for _, t in ipairs(teammates) do
        local inOffensiveHalf = enemyOnRight and (t.x >= ballPos.x) or (t.x <= ballPos.x)
        if inOffensiveHalf and Utils.is_pass_line_clear(ballPos, t, opponents, 40) then
            local progress = enemyOnRight and (t.x - ballPos.x) or (ballPos.x - t.x)
            if progress > bestProgress then
                bestProgress = progress
                best = t
            end
        end
    end
    return best
end

--- Egy lépés: mozgásirány + opcionális Kick paraméterek (a tactics.lua hívja az API-t).
function Striker.Step(ctx)
    if ctx == nil or ctx.robotPos == nil or ctx.enemyGoal == nil or ctx.myGoal == nil then
        return nil
    end
    if not is_valid_ball_object(ctx.ballPos) then
        return nil
    end

    local robotPos = ctx.robotPos
    local ballPos = ctx.ballPos
    local ballVel = ctx.ballVel or { x = 0, y = 0 }
    local enemyGoal = ctx.enemyGoal
    local myGoal = ctx.myGoal
    local maxSpeed = ctx.maxSpeed or 100
    local minKick = (Lib and Lib.MIN_KICK_POWER) or 15.0
    local maxKick = (Lib and Lib.MAX_KICK_POWER) or 100.0
    local robotRadius = (Lib and Lib.ROBOT_RADIUS) or 18.0
    local ballRadius = (Lib and Lib.BALL_RADIUS) or 8.0

    local d = Utils.get_distance(robotPos, ballPos)
    local ballFuture = {
        x = ballPos.x + (ballVel.x or 0) * 0.25,
        y = ballPos.y + (ballVel.y or 0) * 0.25
    }

    -- Anti-jam: if we are in contact and ball barely moves, dash through it.
    local ballSpeed = Utils.get_distance({ x = 0, y = 0 }, ballVel)
    if d < 10 and ballSpeed < 2 then
        state.jamFrames = state.jamFrames + 1
    else
        state.jamFrames = 0
    end
    if state.jamFrames > 2 then
        local through = {
            x = ballPos.x + (ballPos.x - robotPos.x) * 1.2,
            y = ballPos.y + (ballPos.y - robotPos.y) * 1.2
        }
        return {
            team = ctx.team or "UNKNOWN",
            moveAngleDeg = Utils.get_angle(robotPos, through),
            moveSpeed = maxSpeed
        }
    end

    local goalBlocked = is_goal_path_blocked(ballPos, enemyGoal, ctx.opponents)
    local passTarget = pick_forward_teammate(ballPos, enemyGoal, myGoal, ctx.teammates, ctx.opponents)
    local mapWidth = ctx.mapWidth or 1100
    local targetX = ((ctx.team or "yellow") == "blue") and 0 or mapWidth
    local forwardTarget = { x = targetX, y = (ctx.centerY or ballPos.y) }
    local kickAngle = Utils.normalize_angle(Utils.get_angle(ballPos, enemyGoal))
    local kickForce = 100
    local targetName = "GOAL"
    if goalBlocked and passTarget ~= nil then
        kickAngle = Utils.normalize_angle(Utils.get_angle(ballPos, passTarget))
        kickForce = 70
        targetName = "TEAMMATE"
    elseif goalBlocked then
        local cornerY = ((ctx.centerY or ballPos.y) < ballPos.y) and ((ctx.centerY or ballPos.y) - 220) or ((ctx.centerY or ballPos.y) + 220)
        local deepX = enemyGoal.x > myGoal.x and (ballPos.x + 320) or (ballPos.x - 320)
        kickAngle = Utils.normalize_angle(Utils.get_angle(ballPos, { x = deepX, y = cornerY }))
        kickForce = 80
        targetName = "CLEAR"
    end

    -- 45-degree tolerance on forward-half orientation.
    local moveAngle = Utils.get_angle(robotPos, ballFuture)
    local halfAngle = Utils.get_angle(ballPos, forwardTarget)
    local moveErr = math.abs(Utils.normalize_angle(moveAngle - halfAngle))
    local canFire = moveErr <= 45

    -- Edge-to-edge trigger with small pre-contact buffer.
    -- Strictly ball-only: this check is robot<->ball distance only.
    local edgeTrigger = (robotRadius + ballRadius + 2.0)
    if d < edgeTrigger and canFire then
        -- If ball is nearly static while touching, add slight angle noise to pop it loose.
        if ballSpeed < 1.0 then
            kickAngle = Utils.normalize_angle(kickAngle + math.random(-8, 8))
            targetName = targetName .. "_POP"
        end
        return {
            team = ctx.team or "UNKNOWN",
            targetName = targetName,
            edgeTriggerDist = d,
            moveAngleDeg = moveAngle,
            moveSpeed = maxSpeed,
            kickAngleDeg = kickAngle,
            kickForce = clamp(kickForce, minKick, maxKick)
        }
    end

    return {
        team = ctx.team or "UNKNOWN",
        moveAngleDeg = moveAngle,
        moveSpeed = maxSpeed
    }
end

return Striker

