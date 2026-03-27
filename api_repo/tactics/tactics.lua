--[[
  tactics.lua — központi taktikai vezérlő minden robothoz (OnInit / OnUpdate).

  Roles (magas szinten, nem külön táblában):
  - Kapus (player == KEEPER_ID): Keeper modul → mozog az alapvonalon, és védi a kaput.
  - Mezőny: egy „támadó” (attackerId) — a labdához legközelebbi játékos; többiek support position(supporterId: második legközelebbi).
  - Szerepcsere: attackerLock, hogy ne villogjon a szerep két robot között.

  Labda azonosítás: a C oldal a c_api(12)-vel adja a labda pozícióját; itt ballObj néven
  { id = "ball", type = "ball", x, y } táblát adunk át a Striker/Keeper-nek — így a rúgási
  logika kizárólag ezt a „labda” objektumot használja távolsághoz, nem másik robotot.

  Robot–robot: az applyAvoidance csak a mozgás irányát finomítja közeli játékoshoz képest;
  ütközés nem vált taktikai „állapotot”, és önmagában nem indít Kick-et.
]]

local Utils = dofile("logic/utils.lua")
local Striker = dofile("logic/striker.lua")
local Keeper = dofile("logic/keeper.lua")
local Lib = dofile("tactics_lib.lua")

local BORDER_STRIP_WIDTH = 100
local FIELD_LENGTH = 900
local FIELD_WIDTH = 600
local GOAL_WIDTH = 260

local CENTER_X = BORDER_STRIP_WIDTH + FIELD_LENGTH / 2.0
local CENTER_Y = BORDER_STRIP_WIDTH + FIELD_WIDTH / 2.0
local MAP_WIDTH = BORDER_STRIP_WIDTH * 2 + FIELD_LENGTH

local KEEPER_ID = 0
local maxSpeed = (Lib and Lib.MAX_SPEED) or 100
local PLAYERS = 4
local OPPS = 4

local ENEMY_GOAL = { x = CENTER_X + FIELD_LENGTH / 2.0, y = CENTER_Y }
local MY_GOAL = { x = CENTER_X - FIELD_LENGTH / 2.0, y = CENTER_Y }
local goalLogCooldown = 0
local speedBoostLogCooldown = 0

local attackerId = nil
local attackerLock = 0
local supporterId = nil

--- Saját / ellenfél kapu becslése (kapus pozíció + opcionálisan OppGoal API).
local function UpdateGoals()
    -- Determine team side from our keeper position for robust half-awareness.
    local keeper = API.Locate.Teammate(KEEPER_ID)
    local leftGoal = { x = CENTER_X - FIELD_LENGTH / 2.0, y = CENTER_Y }
    local rightGoal = { x = CENTER_X + FIELD_LENGTH / 2.0, y = CENTER_Y }
    if keeper ~= nil then
        if keeper.x <= CENTER_X then
            MY_GOAL = leftGoal
            ENEMY_GOAL = rightGoal
        else
            MY_GOAL = rightGoal
            ENEMY_GOAL = leftGoal
        end
    else
        -- Fallback to API OppGoal if keeper data is unavailable.
        local oppGoal = API.Locate.OppGoal()
        if oppGoal ~= nil then
            ENEMY_GOAL = { x = oppGoal.x, y = oppGoal.y }
            MY_GOAL = { x = 2 * CENTER_X - oppGoal.x, y = oppGoal.y }
        end
    end

    -- Blue-team enforcement: if our goal is on the left, always attack right side.
    if MY_GOAL.x <= CENTER_X then
        ENEMY_GOAL = { x = CENTER_X + FIELD_LENGTH / 2.0, y = CENTER_Y }
    else
        ENEMY_GOAL = { x = CENTER_X - FIELD_LENGTH / 2.0, y = CENTER_Y }
    end
end

--- Robot indulás: szerepkör logolása (Lua globális `player` az aktuális index).
function OnInit()
    if player == KEEPER_ID then
        print("Robot inicializálva! ID: " .. player .. " Szerepkör: keeper")
    else
        print("Robot inicializálva! ID: " .. player .. " Szerepkör: striker")
    end
end

--- Minden tick: célok frissítése, labda/leképezés, majd szerep szerinti viselkedés.
function OnUpdate(dt)
    UpdateGoals()
    goalLogCooldown = math.max(0, goalLogCooldown - (dt or 0))
    speedBoostLogCooldown = math.max(0, speedBoostLogCooldown - (dt or 0))
    if goalLogCooldown <= 0 then
        print("Targeting Enemy Goal at: " .. tostring(ENEMY_GOAL.x) .. ", " .. tostring(ENEMY_GOAL.y))
        goalLogCooldown = 1.0
    end
    if speedBoostLogCooldown <= 0 then
        print("SPEED BOOST ACTIVE")
        speedBoostLogCooldown = 1.0
    end
    local ball = API.Locate.Ball()
    local ballVel = API.Locate.BallVel()
    -- Labda „burkoló”: csak a C-ből jött Ball() pozíciót adjuk tovább (ne robot koordinátát).
    local ballObj = ball and { id = "ball", type = "ball", x = ball.x, y = ball.y } or nil
    local me = API.Locate.Teammate(player)
    local meVel = API.Locate.TeammateVel(player)
    if ball == nil or me == nil then return end
    local robotRadius = (Lib and Lib.ROBOT_RADIUS) or 18.0
    local ballRadius = (Lib and Lib.BALL_RADIUS) or 8.0
    local edgeTrigger = robotRadius + ballRadius + 2.0

    -- Kitérő kormányzás: közeli játékos mellett finomítjuk az irányt (nem Kick, nem szerepcsere).
    local function applyAvoidance(desiredAngleDeg, desiredSpeed)
        if desiredAngleDeg == nil then return desiredAngleDeg, desiredSpeed end
        local nearest = nil
        local nearestDist = math.huge
        for id = 0, PLAYERS - 1 do
            if id ~= player then
                local tp = API.Locate.Teammate(id)
                if tp ~= nil then
                    local dd = Utils.get_distance(me, tp)
                    if dd < nearestDist then
                        nearestDist = dd
                        nearest = tp
                    end
                end
            end
        end
        for id = 0, OPPS - 1 do
            local op = API.Locate.Opp(id)
            if op ~= nil then
                local dd = Utils.get_distance(me, op)
                if dd < nearestDist then
                    nearestDist = dd
                    nearest = op
                end
            end
        end

        local ballDist = Utils.get_distance(me, ballObj)
        if nearest ~= nil and nearestDist < robotRadius * 1.8 and ballDist > edgeTrigger then
            local dr = desiredAngleDeg * math.pi / 180
            local dx, dy = math.cos(dr), math.sin(dr)
            local ox, oy = nearest.x - me.x, nearest.y - me.y
            local od = math.max(1e-6, math.sqrt(ox * ox + oy * oy))
            ox, oy = ox / od, oy / od
            local side = (dx * oy - dy * ox) >= 0 and 1 or -1
            local px, py = -oy * side, ox * side
            local mixx = dx + px * 0.8
            local mixy = dy + py * 0.8
            local avoidAngle = Utils.normalize_angle(math.atan2(mixy, mixx) * 180 / math.pi)
            print("ROBOT-ROBOT COLLISION DETECTED - IGNORING")
            return avoidAngle, math.max(desiredSpeed or 0, maxSpeed * 0.8)
        end
        return desiredAngleDeg, desiredSpeed
    end

    if player == KEEPER_ID then
        local k = Keeper.Step({
            robotPos = me,
            ballPos = ballObj,
            ballVel = ballVel,
            enemyGoal = ENEMY_GOAL,
            myGoal = MY_GOAL,
            goalWidth = GOAL_WIDTH,
            centerY = CENTER_Y,
            maxSpeed = maxSpeed,
            mapWidth = MAP_WIDTH,
            team = (MY_GOAL.x > CENTER_X) and "blue" or "yellow"
        })
        if k ~= nil then
            local moveAngle, moveSpeed = applyAvoidance(k.moveAngleDeg, k.moveSpeed)
            API.Robot.Move(moveAngle, moveSpeed)
            if k.kickAngleDeg ~= nil and k.kickForce ~= nil then
                if k.keeperClearance then
                    print("KEEPER CLEARANCE")
                end
                API.Robot.Kick(Utils.normalize_angle(k.kickAngleDeg), k.kickForce)
            end
        end
        return
    end

    -- Decide single main attacker (closest to ball) with a small lock to avoid flicker.
    attackerLock = math.max(0, attackerLock - (dt or 0))
    if attackerLock <= 0 then
        local bestId = nil
        local bestD = math.huge
        for id = 0, PLAYERS - 1 do
            if id ~= KEEPER_ID then
                local p = API.Locate.Teammate(id)
                if p ~= nil then
                    local d = Utils.get_distance(p, ball)
                    if d < bestD then
                        bestD = d
                        bestId = id
                    end
                end
            end
        end

        -- Hysteresis: don't swap unless clearly better.
        if attackerId == nil then
            attackerId = bestId
            attackerLock = 0.5
        elseif bestId ~= nil and bestId ~= attackerId then
            local curPos = API.Locate.Teammate(attackerId)
            local curD = curPos and Utils.get_distance(curPos, ball) or math.huge
            if bestD + 10 < curD then
                attackerId = bestId
                attackerLock = 0.5
            end
        end
    end

    -- Secondary role: dedicated advanced supporter (2nd closest field robot).
    local ordered = {}
    for id = 0, PLAYERS - 1 do
        if id ~= KEEPER_ID then
            local p = API.Locate.Teammate(id)
            if p ~= nil then
                ordered[#ordered + 1] = { id = id, d = Utils.get_distance(p, ball) }
            end
        end
    end
    table.sort(ordered, function(a, b) return a.d < b.d end)
    supporterId = (ordered[2] and ordered[2].id) or nil

    if attackerId == player then
        print("Robot [" .. player .. "] is the main attacker")
    end

    local teammates = {}
    for id = 0, PLAYERS - 1 do
        if id ~= player and id ~= KEEPER_ID then
            local t = API.Locate.Teammate(id)
            if t ~= nil then teammates[#teammates + 1] = t end
        end
    end
    
    local opponents = {}
    for id = 0, OPPS - 1 do
        local o = API.Locate.Opp(id)
        if o ~= nil then opponents[#opponents + 1] = o end
    end

    -- Non-attacker robots: striker+supporter split to avoid crowding.
    if attackerId ~= nil and player ~= attackerId then
        if supporterId ~= nil and player == supporterId then
            local attackingRight = ENEMY_GOAL.x > MY_GOAL.x
            local rx = attackingRight and (MAP_WIDTH * 0.6) or (MAP_WIDTH * 0.4)
            local receiver = { x = rx, y = CENTER_Y }
            local ang = Utils.get_angle(me, receiver)
            print("SUPPORTER: Moving to offensive position")
            API.Robot.Move(ang, maxSpeed)
        else
            local holdDist = 80
            local toGoalAng = Utils.get_angle(ball, MY_GOAL)
            local rad = toGoalAng * math.pi / 180
            local target = { x = ball.x + math.cos(rad) * holdDist, y = ball.y + math.sin(rad) * holdDist }
            API.Robot.Move(Utils.get_angle(me, target), maxSpeed * 0.7)
        end
        return
    end

    local s = Striker.Step({
        playerId = player,
        team = (MY_GOAL.x > CENTER_X) and "blue" or "yellow",
        dt = dt,
        robotPos = me,
        robotVel = meVel,
        ballPos = ballObj,
        ballVel = ballVel,
        enemyGoal = ENEMY_GOAL,
        myGoal = MY_GOAL,
        centerY = CENTER_Y,
        mapWidth = MAP_WIDTH,
        teammates = teammates,
        opponents = opponents,
        maxSpeed = maxSpeed,
        alignDist = 50,
        attackDist = 20,
        behindDist = 20,
        facingEpsDeg = 12
    })

    if s == nil then return end
    if s.moveAngleDeg ~= nil and s.moveSpeed ~= nil then
        local moveAngle, moveSpeed = applyAvoidance(s.moveAngleDeg, s.moveSpeed)
        API.Robot.Move(moveAngle, moveSpeed)
    end
    if s.kickAngleDeg ~= nil and s.kickForce ~= nil then
        if s.edgeTriggerDist ~= nil then
            print("EDGE TRIGGER: Kicking ball from distance " .. tostring(s.edgeTriggerDist))
        end
        print("INSTANT KICK: Team " .. tostring(s.team or "UNKNOWN") .. " toward " .. tostring(s.targetName or "TARGET"))
        print("POWER KICK: " .. tostring(s.kickLabel or s.kickAngleDeg))
        API.Robot.Kick(s.kickAngleDeg, s.kickForce)
    end
end