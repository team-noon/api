--[[
  logic/utils.lua — geometriai segédfüggvények a taktikai réteghez.

  Szögek: fokban (°), a tacapi.lua radiánra vált a C API felé.
  Cél: távolság, irány, szakasz–pont távolság (passz „sáv” ellenőrzéshez).

  Megjegyzés (labda vs. robot): itt nincs külön „isBall” függvény — a labdát a tactics.lua
  a API.Locate.Ball() hívásból kapja, és táblaként adja tovább; a Striker/Keeper modulok
  a ballPos koordináták érvényességét ellenőrzik. Az állapotgép (szerepkörök) a labda
  távolságához kötődik, nem másik robot pozíciójához.
]]

local Utils = {}

--- Szög normalizálása [-180, 180) tartományba (fok).
function Utils.normalize_angle(angle_deg)
    if angle_deg == nil then return 0 end
    return ((angle_deg + 180) % 360) - 180
end

--- Két pont euklideszi távolsága a síkon.
function Utils.get_distance(p1, p2)
    if p1 == nil or p2 == nil then return math.huge end
    local dx = p2.x - p1.x
    local dy = p2.y - p1.y
    return math.sqrt(dx * dx + dy * dy)
end

--- Irányszög (fok): p1 → p2 vektor szöge az x tengelyhez képest.
function Utils.get_angle(p1, p2)
    if p1 == nil or p2 == nil then return 0 end
    local dx = p2.x - p1.x
    local dy = p2.y - p1.y
    local rad = math.atan2(dy, dx)
    return Utils.normalize_angle(rad * 180 / math.pi)
end

--[[
  Pont–szakasz négyzetes távolsága: P vetítése AB-re [0,1] intervallumra, majd P–C távolság².
  Passz vonal tisztaságához: ellenfél „közel van-e a passz vonalhoz”.
]]
function Utils.dist2_point_segment(p, a, b)
    if p == nil or a == nil or b == nil then return math.huge end
    local abx = b.x - a.x
    local aby = b.y - a.y
    local apx = p.x - a.x
    local apy = p.y - a.y
    local ab2 = abx * abx + aby * aby
    if ab2 < 1e-12 then
        return apx * apx + apy * apy
    end
    local t = (apx * abx + apy * aby) / ab2
    if t < 0 then t = 0 end
    if t > 1 then t = 1 end
    local cx = a.x + abx * t
    local cy = a.y + aby * t
    local ex = p.x - cx
    local ey = p.y - cy
    return ex * ex + ey * ey
end

--- Igaz, ha egy ellenfél sincs a [fromPos, toPos] szakaszhoz `clearance`-nél közelebb.
function Utils.is_pass_line_clear(fromPos, toPos, opponents, clearance)
    if fromPos == nil or toPos == nil then return false end
    if opponents == nil then return true end
    local c = clearance or 40
    local c2 = c * c
    for _, opp in ipairs(opponents) do
        if Utils.dist2_point_segment(opp, fromPos, toPos) < c2 then
            return false
        end
    end
    return true
end

return Utils

