local unit
local units = ""
for unit in allStratHCUnits() do
 unit:purgeResupply(false)
 unit:restartSupply(0, 1800, false)
 units = units .. unit:callsign() .. " "
end

print("Reset " .. units)
