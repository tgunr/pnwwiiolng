function getStats (cp, side)
--  local nair = cp:getPlayers(side, _AIRCRAFT)
  local ninf = cp:getPlayers(side, _INFANTRY)
--  local narm = cp:getPlayers(side, _GROUND_VEHICLE)
--  local nsea = cp:getPlayers(side, _SEA_VEHICLE)
  if ninf >= 1 then
   return ' ' .. cp.name .. '(' .. ninf .. ')' ;
  end
  return ''
end

local cp
local cps = 0
local allied = ""
local axis = ""
for cp in allCityCPs() do
  cps = cps + 1
  allied = allied .. getStats(cp, ALLIED)
  axis = axis .. getStats(cp, AXIS)
end
if allied == "" then allied = " None" end
if axis == "" then axis = " None" end
print("ALLIED:" .. allied)
print("AXIS:" .. axis)

