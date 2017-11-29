function resetUnit (unit)
 local change = 0
 -- Move to its home location
 if unit:homeCP().uOID ~= unit:startCP().uOID then
  if unit:side() ~= unit:startCP().side then
   error(unit:callsign() .. " cannot deploy to " .. unit:startCP().name)
   return 0
  end
  unit:deploy(unit:startCP(), 0, "SYSTEM", false, false)
  change = 1
 end

 -- Wipe the resupply queue and set all equipment back to capacity
 resetUnitSupply(unit)

 return change
end

local unit
local unitsChanged = 0
for unit in allStratHCUnits() do
 unitsChanged = unitsChanged + resetUnit(unit)
end

strtBroadcastCPs()

