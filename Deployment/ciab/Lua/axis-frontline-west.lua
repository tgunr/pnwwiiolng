-- Move the Axis frontline one town west.
-- Used for NO MANS LAND intermission.

local frontLine = {}
local frontLines = 0

for cp in allCityCPs()
do
 if cp.side == ALLIED and cp.numLinks > 0 and cp:isEnemyLinked() == true then
  table.insert(frontLine, cp)
 end
end

for i in frontLine do
 local cp = frontLine[i]
 print(cp.name)
 setCP(cp, GERMANY)
 frontLines = frontLines + 1
end

print("FrontLines = "..frontLines)
