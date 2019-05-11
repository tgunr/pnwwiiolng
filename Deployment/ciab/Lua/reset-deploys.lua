function resetUnit (unit)
 local change = 0
 unit:deploy(unit:homeCP(), 0, "SYSTEM", false, false)
 -- Wipe the resupply queue and set all equipment back to capacity
 resetUnitSupply(unit)

 return change
end

systemMessage("*GM RESETTING ALL MOVE TIMERS")

local unit
local unitsChanged = 0
for unit in allStratHCUnits() do
 unitsChanged = unitsChanged + resetUnit(unit)
end

strtBroadcastCPs()
