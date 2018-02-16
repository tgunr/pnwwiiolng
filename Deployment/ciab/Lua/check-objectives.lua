local count = 0
local errors = 0

for cp in allCityCPs() do
 if cp:isDefendObjective(ALLIED) then
  if not cp:isAttackObjective(AXIS) then
   warn(cp.name .. " lopsided AO")
   cp:clearObjective(ALLIED)
   errors = errors + 1
  end
 elseif cp:isDefendObjective(AXIS) then
  if not cp:isAttackObjective(ALLIED) then
   warn(cp.name .. " lopsided AO")
   cp:clearObjective(AXIS)
   errors = errors + 1
  end
 elseif cp:isContested() then
  warn(cp.name .. " contested without ao")
  errors = errors + 1
  for fac in allFacilitiesAt(cp) do
   if fac.side ~= cp.side then
    warn(fac.name .. " causing contention")
    setFacility(fac, cp.country, false)
   end
  end
  cp:setTableStates()
 end
 strtSaveCP(cp)
 count = count + 1
end

print("Checked " .. count .. " CPs with " .. errors .. " errors")
