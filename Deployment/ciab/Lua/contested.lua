local cp
if targeted_cp ~= nil then cp = getCP(targeted_cp)
elseif ARGS(1) ~= nil then cp = getCP(ARGS(1)) end
if cp == nil then
 return error("Please specify or target a CP")
else
 print("Querying "..cp.name.."/"..cp.uOID)
 if cp:isContested() then print("Contested") else print("Not contested") end
end


