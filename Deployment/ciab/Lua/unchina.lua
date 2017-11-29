-- Convert all City's that are currently owned by CN
-- to their original country

for cp in allCityCPs()
do
 if cp:isEnemyLinked() then
  if cp:isCity() then
   if cp.country == WWII_CHINA then
    cp:changeOwnership(cp.originalCountry, true, true)
   end
  end
 end
end
