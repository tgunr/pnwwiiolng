-- Script to cancel all active objectives
local cp = nil
if ARGS(1) ~= nil and ARGS(1) ~= "" then
 cp = strtGetCPByName(ARGS(1))
 if cp == nil then
  error("Unknown CP: "..ARGS(1))
  return 0
 end
elseif targeted_cp ~= nil then
 cp = targeted_cp
else
 error("remove-objective <cp>")
 return 0
end

done = 0
local side, name
if cp:isAttackObjective(ALLIED) then
 side = ALLIED
 name = "allied attack"
elseif cp:isAttackObjective(AXIS) then
 side = AXIS
 name = "axis attack"
elseif cp:isDefendObjective(ALLIED) then
 side = ALLIED
 name = "allied defense"
elseif cp:isDefendObjective(AXIS) then
 side = AXIS
 name = "axis defense"
else
 error("No objective found on " + cp.name)
end

cp:clearObjective(side)
