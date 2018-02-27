function resetUnit (unit)
 -- Wipe the resupply queue and set all equipment back to capacity
 resetUnitSupply(unit)
 return 1
end

local unit
local unitsChanged = 0
for unit in allStratHCUnits() do
 unitsChanged = unitsChanged + resetUnit(unit)
end

strtBroadcastCPs()

