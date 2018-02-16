for cp in allCityCPs() do
 if cp.m_isSupplyCenter then
  print(cp.name)
  cp.originalCountry = cp.country
  cp.originalSide = cp.side
  cp:setRouteing(true)
  strtQueueCPStatus(cp)
  strtSaveCP(cp)
  for fac in allFacilitiesAt(cp) do
   if fac:isRDPFactory() then
    print("- "..fac.name..": own="..fac.country..", orig="..fac.originalCountry)
    fac.originalCountry = fac.country
    fac.originalSide = fac.side
    strtQueueFacilityStatus(fac)
    strtSaveFacility(fac)
   end
  end
 end
end

for unit in allStratHCUnits() do
 resetUnitSupply(unit)
end

function fixReverseCP(name)
 cp = getCP(name)
 if cp ~= nil then
  cp.originalCountry = GERMANY ; cp.originalSide = AXIS
  setCP(name, GERMANY)
  cp.country = GERMANY ; cp.side = AXIS
  cp.controllingCountry = GERMANY ; cp.controllingSide = AXIS
  strtQueueCPStatus(cp)
  strtSaveCP(cp)
  print("Fixed " .. cp.name)
 end
end

fixReverseCP("Seaford")
fixReverseCP("Eastbourne")
fixReverseCP("Brighton")
fixReverseCP("Chichester")
fixReverseCP("Gosport")
fixReverseCP("Biggin Hill")
fixReverseCP("Hornchurch")
fixReverseCP("Ipswich")
fixReverseCP("Martlesham")
fixReverseCP("Coltishall")
fixReverseCP("Wickenby")

strtBroadcastCPs()
