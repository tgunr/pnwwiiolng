local targetCountry = getCountry(ARGS(1))
if targetCountry == nil then return error("Please specify a country") end

function changeCP (cp)
 if string.find(cp.name, "^Fort") then return 0 end
 if string.find(cp.name, "^Camp") then return 0 end
 if cp:isObjective(AXIS) then cp:clearObjective(AXIS) end
 if cp:isObjective(ALLIED) then cp:clearObjective(ALLIED) end
 if cp.country == targetCountry then return 0 end 
 cp:changeOwnership(targetCountry, true, false)
 return 1
end

systemMessage("Changing map to "..target)

changed = 0
i = 0
for cp in allCityCPs() do changed = changed + changeCP(cp) end

systemMessage("Map change finished: Changed "..changed.." towns")

