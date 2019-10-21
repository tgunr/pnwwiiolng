function changeCP (cp)
 if string.find(cp.name, "^Fort") then return 0 end
 if string.find(cp.name, "^Camp") then return 0 end
 if cp:isObjective(AXIS) then cp:clearObjective(AXIS) end
 if cp:isObjective(ALLIED) then cp:clearObjective(ALLIED) end
 if cp.country == cp.originalCountry and cp:isContested() == false then return 0 end
 cp:changeOwnershipFromScript(cp.originalCountry, true, true)
 return 1
end

function resetUnit (unit)
 local change = 0
 if unit:startCP() == nil then
  if unit:division() ~= nil and unit:division():startCP() ~= nil then
   unit:deploy(unit:division():startCP(), 0, "SYSTEM", false, false)
   return 1
  end
  print(unit:callsign() .. " has no startCP!")
  return 0
 end
 -- Move to its home location
 if unit:homeCP() == nil or unit:homeCP().uOID ~= unit:startCP().uOID then
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

systemMessage("*GM RESETTING ALL CPs, BRIGADES and DIVISIONS")

CPMonitor:Disable()

local cp
local cpsChanged = 0
for cp in allCityCPs() do
 cpsChanged = cpsChanged + changeCP(cp)
end

local unit
local unitsChanged = 0
for unit in allStratHCUnits() do
 unitsChanged = unitsChanged + resetUnit(unit)
end

if getHCUnit("uktrngd") then
  deploy("uktrngd", "Fort Montgomery", trainingDeploymentDelay)
  deploy("uktrngb", "Fort Montgomery", trainingDeploymentDelay)
  deploy("frtrngd", "Fort De Gaulle", trainingDeploymentDelay)
  deploy("frtrngb", "Fort De Gaulle", trainingDeploymentDelay)
  deploy("detrngd", "Fort Rommel", trainingDeploymentDelay)
  deploy("detrngb", "Fort Rommel", trainingDeploymentDelay)
end

strtBroadcastCPs()

CPMonitor:Enable()

systemMessage("*All CPs set to original owners, Brigades/Divisions to original locations and supply levels.")

print("Map set to initial conditions: Changed "..cpsChanged.." towns, "..unitsChanged.." units.")

