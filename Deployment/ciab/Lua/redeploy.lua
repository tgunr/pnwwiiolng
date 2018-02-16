-- Redeployment script
-- Takes two arguments:
--  "unit" "destination"

local unitID = ARGS(1)
local destName = ARGS(2)
local usage = "Usage: redeploy <unit> [<destination> or target CP on map]"

if unitID == nil then
 error(usage)
 return 0
end

if destName == nil and targeted_cp == nil then
 error(usage)
 return 0
end

local dest 
if destName ~= nil then
 dest = getCP(destName)
else
 dest = targeted_cp
end

if dest == nil then
 error("Cannot find CP '"..destName.."'")
 error(usage)
 return 0
end

deployDivision(unitID, dest.name, 0)

