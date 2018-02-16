-- If ARGS(1) is empty (no arguments) then we assume
-- they have a CP targetted on the map which will be
-- present as "targeted_cp".

local usage = "Usage: destroy [<cp>]"
local cpName = ARGS(1)
if cpName == nil and targeted_cp == nil then
 error(usage)
 return 0
end

function destroy (cp)
 local fac = cp.facilityList
 if fac == nil then print("No facility list for "..cp.name) ; return 0 end
 local hits = 0
 while fac ~= nil do
  local bldg = fac.objectList
  while bldg ~= nil do
      strtDestroyObject(bldg)
      hits = hits + 1
   bldg = bldg.next
  end
  fac = fac.next
 end
 if hits == 0 then
  print("* Nothing to destroy at " .. cp.name)
 elseif hits == 1 then
  print("Destroyed 1 object at " .. cp.name)
 else
  print("Destroyed " .. hits .. " objects at " .. cp.name)
 end
end

if ARGS(1) == nil then
  destroy(targeted_cp)
else
 for i = 1,10 do
  cpName = ARGS(i)
  if cpName ~= nil then
   cp = strtGetCPByName(cpName)
   if cp == nil then
    error("Unrecognized CP, '"..cpName.."'.")
    return 0
   end
   destroy(cp)
  end
 end
end

