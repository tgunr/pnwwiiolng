local area51 = {
  { "Fort Rommel", GERMANY }
, { "Fort Montgomery", ENGLAND }
, { "Fort De Gaulle", FRANCE }
, { "Camp Victoria", ENGLAND }
, { "Camp Gretchen", GERMANY }
, { "Camp Yvette", FRANCE }
} ;


for i in area51
do
 local cp = getCP(area51[i][1])
 if cp == nil then return error(area51[i][1] .. " does not compute") end
 local ctry = getCountry(area51[i][2])
 if ctry == nil then return error(area51[i][1] .. " invalid country " .. area51[i][2]) end
 setCP(cp, ctry, true, true)
 cp:setSupplyCenter()
 cp:setRouteing(true)
end

