--[[
  logic/keeper.lua — kapus viselkedés.

  Alap: vízszintes sávban követi a labda Y-ját a saját kapu vonalán.
  Ha robot–labda távolság „érintési” küszöb alá esik: azonnali kirúgás (Kick) biztonságos
  irányba — a távolság számítás kizárólag keeperPos és ballPos között van (nem védő/játékos).
]]

require "vec2"
local Utils = dofile("logic/utils.lua")
local Lib = dofile("tactics_lib.lua")

local Keeper = {}

--- Labda pozíció érvényessége (C-ből kapott Ball() másolata).
local function is_valid_ball_object(ballPos)
    return ballPos ~= nil and type(ballPos) == "table" and ballPos.x ~= nil and ballPos.y ~= nil
end

--- Egy lépés: mozgás és szükség esetén kirúgás (tactics.lua végrehajtja a Kick-et).
function Keeper.Step(ctx)
    if ctx == nil or ctx.robotPos == nil or ctx.myGoal == nil then
        return nil
    end
    if not is_valid_ball_object(ctx.ballPos) then
        return nil
    end

    local maxSpeed = ctx.maxSpeed or 75
    local goalWidth = ctx.goalWidth or 260
    local centerY = ctx.centerY or ctx.myGoal.y

    local targetY = ctx.ballPos.y
    local keeperPos = ctx.robotPos
    local ballPos = ctx.ballPos
    local ballVel = ctx.ballVel or { x = 0, y = 0 }
    local enemyGoal = ctx.enemyGoal
    local mapWidth = ctx.mapWidth or 1100
    local team = ctx.team or "yellow"
    local robotRadius = (Lib and Lib.ROBOT_RADIUS) or 18.0
    local ballRadius = (Lib and Lib.BALL_RADIUS) or 8.0
    local minKick = (Lib and Lib.MIN_KICK_POWER) or 15.0
    local maxKick = (Lib and Lib.MAX_KICK_POWER) or 100.0

    local function clamp(v, lo, hi)
        if v < lo then return lo end
        if v > hi then return hi end
        return v
    end
    local yMin = centerY - goalWidth / 2
    local yMax = centerY + goalWidth / 2
    if targetY < yMin then targetY = yMin end
    if targetY > yMax then targetY = yMax end

    -- Ball-specific edge trigger: keeper clears immediately at contact.
    -- Strictly robot<->ball distance; never based on robot<->robot distance.
    local dBall = Utils.get_distance(keeperPos, ballPos)
    local edgeTrigger = robotRadius + ballRadius + 2.0
    if dBall < edgeTrigger then
        local attackingLeft = (team == "blue")
        local defaultForward = { x = attackingLeft and 0 or mapWidth, y = centerY }
        local kickTarget = enemyGoal or defaultForward

        -- Keep clearance safely away from own goal line.
        local safeCornerY = (ballPos.y >= centerY) and (centerY - goalWidth * 0.5) or (centerY + goalWidth * 0.5)
        local safeCornerX = attackingLeft and (ballPos.x - 260) or (ballPos.x + 260)
        if Utils.get_distance({ x = 0, y = 0 }, ballVel) < 1.0 then
            kickTarget = { x = safeCornerX, y = safeCornerY }
        end

        local kickAngle = Utils.normalize_angle(Utils.get_angle(ballPos, kickTarget))
        return {
            moveAngleDeg = Utils.get_angle(keeperPos, ballPos),
            moveSpeed = maxSpeed,
            kickAngleDeg = kickAngle,
            kickForce = clamp(90, minKick, maxKick),
            kickLabel = "KEEPER_CLEARANCE",
            keeperClearance = true
        }
    end

    local target = { x = ctx.myGoal.x, y = targetY }
    return {
        moveAngleDeg = Utils.get_angle(keeperPos, target),
        moveSpeed = maxSpeed * 0.7
    }
end

return Keeper
