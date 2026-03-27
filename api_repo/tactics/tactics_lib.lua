--[[
  tactics_lib.lua — közös konstansok a Lua taktikához (sebesség, rúgás, „sugarak”).

  Ezeket a Striker / tactics.lua tölti be: a C fizika numerikus értékeit itt finomhangoljuk
  olvasható nevekkel (pl. MAX_KICK_POWER), nem varázsszámokkal a kódban.
]]

local Lib = {}

-- Motor: robot gyorsabb legyen, mint a guruló labda (általános cél).
Lib.MAX_SPEED = 100.0
Lib.ACCELERATION = 140.0

-- Rúgás: felső korlát és „edge trigger” sugár (tactics.lua / striker egyeztet).
Lib.MAX_KICK_POWER = 100.0
Lib.BASE_KICK_POWER = 85.0
Lib.OPPONENT_PRESSURE_DIST = 45.0
Lib.MIN_KICK_POWER = 15.0
Lib.ROTATION_SPEED = 180.0
Lib.ROBOT_RADIUS = 18.0
Lib.BALL_RADIUS = 8.0

return Lib

