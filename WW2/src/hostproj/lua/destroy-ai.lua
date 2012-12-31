-- If ARGS(1) is empty (no arguments) then we assume
-- they have a CP targetted on the map which will be
-- present as "targeted_cp".

local hits = 0
local cp

function destroyAI (cp)
 local fac = cp.facilityList
 if fac == nil then print("No facility list for "..cp.name) ; return 0 end
 while fac ~= nil do
  local bldg = fac.objectList
  while bldg ~= nil do
    if bldg.fullObjID.objType == BUILDING_AUTOGUNNER then
     strtDestroyObject(bldg)
     hits = hits + 1
    end
   bldg = bldg.next
  end
  fac = fac.next
 end
end

if ARGS(1) == 'all' then
 for cp in allCityCPs() do
  destroyAI(cp)
 end
 print("Destroyed "..hits.." objects at "..cp.name)
 return 0
end

local usage = "Usage: destroy [<cp>]"
local cpName = ARGS(1)
if cpName == nil and targeted_cp == nil then
 error(usage)
 return 0
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
   destroyAI(cp)
  end
 end
end

print("Destroyed "..hits.." objects at "..cp.name)

