-- if ARGS(1) then cp = getCP(ARGS(1)) else cp = targeted_cp end
-- if cp == nil then return error("Specify or target a CP please") end

local count = 0
function doCp(cp)
 local fac
 for fac in allFacilitiesAt(cp) do
  local bldg = fac.objectList
  while bldg ~= nil do
    strtQueueVisLevelUpdate(bldg)
    count = count + 1
    bldg = bldg.next
  end
 end
end

for cp in allCityCPs() do
 doCp(cp)
end

print("Queued " .. count .. " updates")

