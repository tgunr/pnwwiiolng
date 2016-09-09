--
-- Common initialization file for lua in a server
-- default file can be overridden with LUA_RC_FILE
--

require 'debug'

ENGLAND = WWII_ENGLAND ; FRANCE = WWII_FRANCE ; GERMANY = WWII_GERMANY ; CHINA = WWII_CHINA ; USA = WWII_USA ; NO_COUNTRY = WWII_NO_COUNTRY
AIRCRAFT = _AIRCRAFT ; INFANTRY = _INFANTRY ; GROUND_VEHICLE = _GROUND_VEHICLE ; SEA_VEHICLE = _SEA_VEHICLE

if not game then game = { ['adjustTime'] = 0, ['debug'] = 0 } end

gluehelp = {}

----------------------------------------------------------------------
-- Check "gluehelp" for any help on a given function name.

function help(onWhat)
  if not onWhat or onWhat == "list" then
    local commands = {}
    for k, _ in pairs(gluehelp)
	do
	  table.insert(commands, k)
	end
	table.sort(commands)
	print("Commands: " .. join(', ', commands))
    return
  end
  local helpString = gluehelp[onWhat]
  if not helpString then
    print("Unrecognize command (or no help available for) " .. onWhat)
	return
  end
  print(onWhat .. helpString)
end

function getCallStack(n)
  local str = ""
  for k,v in pairs(debug.getinfo(n))
  do
    local val = ''
    if type(v) ~= 'function' then val = " " .. v end
    if str ~= "" then str = str .. "; " end
    str = str .. k..": <"..type(v)..">"..val
  end
  return str
end

----------------------------------------------------------------------
-- Script-based logging
--   level: D=Debug, I=Info, N=Nominal, M=Minor, W=Warning, A=Alert

gluehelp['log'] = "(L, facil, msg) Reports 'msg' to the log file with level 'L' under facility 'facil'." ;
function log(level, facil, msg)
  -- Don't print debug messages unless it's enabled.
  if level == 'D' then
    if game.debug ~= true and game.debug ~= facil then
      return
    end
  end
  -- Obtain stack info on the caller.
  local info = debug.getinfo(2)
  local file, line, func
  if info.lastlinedefined > 0 then file = info.short_src else file = '<lua>' end
  line = info.currentline
  if info.name ~= nil then func = info.name else func = '<'..info.what..'>' end

  writeToLog(file, func, line, level, facil, msg)
end

----------------------------------------------------------------------
-- Turn a list of strings into a sep-separated string
--   sep: separator
--   l: list of strings to join
-- returns
--   s: joined up string
----------------------------------------------------------------------

function join(sep, l)
  local len = table.getn(l)
  if len == 0 then return "" end
  local s = l[1]
  for i = 2, len do s = s .. sep .. l[i] end
  return s
end

----------------------------------------------------------------------
-- Iterator: All CITY Choke Points (CPs)
----------------------------------------------------------------------

gluehelp['allCityCPs'] = "() Iterates through all CPs that are flagged as citys (i.e. not bridges)" ;
function allCityCPs ()
  local nextCP = strtGetCPList()
  return function ()
    local cp
    repeat
      cp = nextCP
      if nextCP ~= nil then nextCP = nextCP.next end
    until cp == nil or cp:isCity()
    return cp
  end
end

----------------------------------------------------------------------
-- Iterator: All Choke Points (CPs)
----------------------------------------------------------------------

gluehelp['allCPs'] = "() Iterates through all CPs, including bridges"
function allCPs ()
  local nextCP = strtGetCPList()
  return function ()
    local cp = nextCP
    if nextCP ~= nil then nextCP = nextCP.next end
    return cp
  end
end

----------------------------------------------------------------------
-- Iterator: *All* facilities [of an optional type]
----------------------------------------------------------------------

gluehelp['allFacilities'] = "([facilityType]) Iterates through all facilities. If facilitytype is specified, limits itself to matching facilities"
function allFacilities (facilityType)
  local nextCP = strtGetCPList()
  local facility = nil
  return function ()
    if facilityType == nil then
      repeat
        if facility == nil then
          if nextCP == nil then return nil end
          facility = nextCP.facilityList
          nextCP = nextCP.next
        else
          facility = facility.next
        end
      until facility ~= nil
      return facility
    end
    repeat
      if facility == nil then
        if nextCP == nil then return nil end
        facility = getFirstFacility(nextCP, facilityType)
        nextCP = nextCP.next
      else
        facility = facility.next
      end
    until facility ~= nil
    return facility
  end
end

----------------------------------------------------------------------
-- Iterator: *All* facilities at a specific CP
----------------------------------------------------------------------

gluehelp['allFacilitiesAt'] = "(cp) Iterates through all facilities at the given CP"
function allFacilitiesAt (atCP)
  local cp = getCP(atCP)
  if cp == nil then log('A', 'glue', "Unable to locate CP " .. atCP) ; return end
  local nextFac = cp.facilityList
  return function ()
   if nextFac == nil then
    return nil
   else
    local fac = nextFac
    nextFac = nextFac.next
    return fac
   end
  end
end

----------------------------------------------------------------------
-- All valid links for a particular CP

gluehelp['allLinksFrom'] = "(cp) Iterates through all outgoing links from the given CP"
function allLinksFrom(atCP)
 local cp = getCP(atCP)
 if cp == nil then return log('A', 'glue', "Unable to locate CP " .. atCP) end
 if not cp:isCity() then return nil end
 local i = 0
 return function ()
  while i < cp.numLinks do
   local link = cp:link(i)
   i = i + 1
   if link ~= nil and link.endDepot ~= nil and link.endDepot.cp ~= nil then
    return link
   end
  end
  return nil
 end
end

----------------------------------------------------------------------
-- All neighboring CPs of a particular CP

gluehelp['allNeighbors'] = "(atCP) Iterates through all CPs that have links to the specified CP"
function allNeighbors(atCP)
 local cp = getCP(atCP)
 if cp == nil then return log('A', 'glue', "Unable to locate CP " .. atCP) end
 if not cp:isCity() then return nil end
 local i = 0
 return function ()
  while i < cp.numLinks do
   local link = cp:link(i)
   i = i + 1
   if link ~= nil and link.endDepot ~= nil and link.endDepot.cp ~= nil then
    return link.endDepot.cp
   end
  end
  return nil
 end
end

----------------------------------------------------------------------
-- Iterator: All spawnable facilities
----------------------------------------------------------------------

gluehelp['allSpawnableFacilities'] = "() Iterates through all player-spawnable facilities"
function allSpawnableFacilities ()
  local nextCP = strtGetCPList()
  local facility = nil
  return function ()
    repeat
      if facility == nil then
        if nextCP == nil then return nil end
        facility = nextCP.facilityList
        nextCP = nextCP.next
      else
        facility = facility.next
      end
    until isSpawnableFacility(facility)
    return facility
  end
end

----------------------------------------------------------------------
-- Iterate over all Strat HC Units (divisions/brigades only)
----------------------------------------------------------------------

gluehelp['allStratHCUnits'] = "() Iterates through all Strat HC Units (brigades + divisions)"
function allStratHCUnits ()
  local it = hcUnits:beginIterator()
  local last = hcUnits:endIterator()
  return function ()
    local hcUnit, unit
    repeat
      if it == nil or it == last then return nil end
      hcUnit = getHCiHCUnit(it)
      unit = getHCiStratHCUnit(it)
      it = getHCiNext(it)
    until hcUnit:isBrigade() == true or hcUnit:isDivision() == true
    return unit
  end
end

----------------------------------------------------------------------
-- Determine if a country is a useable country
----------------------------------------------------------------------

gluehelp['activeCountry'] = "(country) Returns true if the specified country is an active (playable) country"
function activeCountry (country)
  if country == WWII_ENGLAND then return true end
  if country == WWII_FRANCE then return true end
  if country == WWII_GERMANY then return true end
  if country == WWII_USA then return true end
  return false
end

----------------------------------------------------------------------
-- Return true if a facility is suitable for spawning;
-- does not determine if the facility could currently
-- be spawned at
----------------------------------------------------------------------

gluehelp['isSpawnableFacility'] = "(facility) Returns true if facility is a player-spawnable facility (does not test that the facility is /currently/ spawnable)"
function isSpawnableFacility (facility)
  if facility == nil then return false end
  if not activeCountry(facility.country) then return false end
  if not facility:isMilitaryFacility() then return false end
  if not facility.m_spawnable then return false end
  return true
end

----------------------------------------------------------------------
-- Get the first facility of a particular type from a CP
-- Allows following of facility.typeNext chains
----------------------------------------------------------------------

gluehelp['getFirstFacility'] = "(cp) Returns the first facility under the specified CP"
function getFirstFacility (atCP, facilityType)
  if facilityType == nil then return nil end
  if atCP == nil then return log('A', 'glue', "getFirstFacility called with nil cp") end
  local cp = getCP(atCP)
  if cp == nil then return log('A', 'glue', "Unknown/nil cp:"..atCP) end
  return cp.facilityTypeIndex[facilityType]
end

----------------------------------------------------------------------
-- Parse a time struct containing time members
-- e.g. timeInSeconds( { days = 5, hour = 1, minutes = 30 })
-- units can be days, day, hours, hour, minutes, minute, seconds or
-- second. The singular versions (day/hour/minute/second) will
-- always expand to one single second regardless of what value
-- you supply with them, thus timeInSeconds({ minute = 5 }) will
-- return a value of 60.
----------------------------------------------------------------------

gluehelp['timeInSeconds'] = "({ [day[s] = value], [hour[s] = value], [minute[s] = value], [second[s] = value] }) Converts the combined specified units into seconds. E.g. timeInSeconds{day=1, hours=5}"
function timeInSeconds (time)
  if not time then return 0 end
  local seconds = 0
  function parseMember (plural, single, mult)
   if plural and plural > 0 then return plural * mult end
   if single and single == 1 then return mult end
   return 0
  end
  seconds = seconds + parseMember(time.years, time.year, 365*24*60*60)
  seconds = seconds + parseMember(time.days, time.day, 24*60*60)
  seconds = seconds + parseMember(time.hours, time.hour, 60*60)
  seconds = seconds + parseMember(time.minutes, time.minute, 60)
  seconds = seconds + parseMember(time.seconds, time.second, 1)
  return seconds
end

----------------------------------------------------------------------
-- getPopulation -- returns the combined axis + allied populations
----------------------------------------------------------------------

gluehelp['getPopulation'] = "() Returns the total server population"
function getPopulation ()
  return chatGetPopulation(ALLIED) + chatGetPopulation(AXIS)
end

gluehelp['getLowerPopulation'] = "() Returns the lower of the two sides' population"
function getLowerPopulation()
  return math.min(chatGetPopulation(ALLIED), chatGetPopulation(AXIS))
end

----------------------------------------------------------------------
-- Wrappers for manipulating brigades and divisions
----------------------------------------------------------------------

----------------------------------------------------------------------
-- Given a HC Unit short-name, OID or pointer, return a pointer to the
-- Strat HC Unit.

gluehelp['getHCUnit'] = "(unit) Translates 'unit' (short name or ID number) to a StratHCUnit"
function getHCUnit(identifier) return getHcUnit(identifier) end

function getHcUnit(identifier)
 if type(identifier) == 'string' then
  return StratHCUnit:Lookup(identifier)
 elseif type(identifier) == 'number' then
  return StratHCUnit:Lookup(identifier)
 elseif tolua.type(identifier) == 'StratHCUnit' then
  return identifier
 elseif identifier ~= nil then
  log('A', 'glue', "Invalid HCUnit identifier ("..tolua.type(identifier).."): " .. identifier)
 else
  log('A', 'glue', "HCUnit identifier is nil")
 end
end

----------------------------------------------------------------------
-- Look up a side by it's name or value

gluehelp['getSide'] = "(side) Translates 'side' to a WWII_SIDE value"
function getSide(side)
  if tolua.type(side) == 'number' then
    return side
  elseif type(side) == 'string' then
    if side == 'ALLIED' or side == 'Allied' or side == 'allied' then return 1
    elseif side == 'AXIS' or side == 'Axis' or side == 'axis' then return 2
    elseif side == 'NO_SIDE' or side == 'no_side' or side == 'none' then return 0
    elseif side == 'NEUTRAL' or side == 'Neutral' or side == 'neutral' then return 3
    end
  end
  log('A', 'glue', "Invalid side: " .. side)
  return 0
end
   
----------------------------------------------------------------------
-- Look up a country
----------------------------------------------------------------------

gluehelp['getCountry'] = "(country) Translates 'country' (a number or 2-letter abbreviation) to a WWII_COUNTRY value"
function getCountry(country)
  if tolua.type(country) == 'number' then
    return country
  elseif type(country) == 'string' then
    if country == 'UK' or country == 'uk' or country == 'ENGLAND' or country == 'England' then
      return ENGLAND
    elseif country == 'FR' or country == 'fr' or country == 'FRANCE' or country == 'France' then
      return FRANCE
    elseif country == 'DE' or country == 'de' or country == 'GERMANY' or country == 'Germany' then
      return GERMANY
    elseif country == 'CN' or country == 'cn' or country == 'CHINA' or country == 'China' then
      return CHINA
	elseif country == 'US' or country == 'us' or country == 'USA' or country == 'usa' then
	  return USA
    else
      return error("Unrecognized country: " .. country)
    end
  else
   return error("Unrecognized country: " .. type(country))
  end
end

---------------------------------------------------------------------
-- Function that looks for a CP by either the name or the oid.
-- If no target is specified then it attempts to use the
-- variable "targeted_cp".
---------------------------------------------------------------------

gluehelp['getCP'] = "(entity) Looks up a CP; entity can be a CP name, numeric ID or a facility or building (ObjectStruct). If no entity is provided, attempts to use the CP the user has selected." 
function getCP(target)
 if target == nil then
  if targeted_cp == nil then return error("getCP called with NIL target") end
  return targeted_cp
 end

 if type(target) == 'number' then
  return strtGetCPByOID(target)
 elseif type(target) == 'string' then
  return strtGetCPByName(target)
 elseif tolua.type(target) == 'CPStruct' then
  return target
 elseif tolua.type(target) == 'FacilityStruct' then
  return target.cp
 elseif tolua.type(target) == 'ObjectStruct' then
  if target.facility == nil then return nil end
  return target.facility.cp
 else
  error("getCP called with unknown target type of "..tolua.type(target))
 end
end

----------------------------------------------------------------------
-- Function that looks for a facility either by name or oid.
-- If not target is specified than it attempts to use the variable
-- "targeted_fac".
----------------------------------------------------------------------

gluehelp['getFacility'] = "(entity) Looks up a facility; entity can be a facility name, numeric ID or building. If no entity is provided, attempts to use the facility the user has selected."
function getFacility(target)
 if target == nil then
  if targeted_fac == nil then return error("getFac called with NIL target") end
  return targeted_fac
 end

 if type(target) == 'number' then
  return strtGetFacilityByOID(target)
 elseif type(target) == 'string' then
  return strtGetFacilityByName(target)
 elseif tolua.type(target) == 'FacilityStruct' then
  return target
 elseif tolua.type(target) == 'ObjectStruct' then
  return target.facility
 else
  error("getFac called with unknown target type of "..tolua.type(target))
 end
end

----------------------------------------------------------------------
-- Looks up an entity by it's full object id and returns whatever the
-- object is or nil.

gluehelp['getStratObject'] = "(fullObjId) Looks up the relevant strat object type for the given ID"
function getStratObject(fullObjId)
  if fullObjId == nil then return nil end

  local stratObj = strtGetFullObjID(fullObjId)
  if stratObj == nil then return nil end

  -- Cast to the correct type
  if stratObj.objClass == CLASS_CP then return tolua.cast(stratObj, "CPStruct") end
  if stratObj.objClass == CLASS_FACILITY then return tolua.cast(stratObj, "FacilityStruct") end
  if stratObj.objClass == CLASS_BUILDLING then return tolua.cast(stratObj, "ObjectStruct") end
  if stratObj.objClass == CLASS_LINK then return tolua.cast(stratObj, "LinkStruct") end

  -- Nothing we recognize.
  return nil
end

----------------------------------------------------------------------
-- Returns true if there are any incoming enemy firebases.

gluehelp['hasEnemyFirebases'] = "(cp) Returns true if the specified CP has incoming enemy firebases"
function hasEnemyFirebases(targetCP)
  local cp = getCP(targetCP)
  if cp == nil then return error("Unknown cp: " .. targetCP) end
  if not cp:isCity() or cp.numLinks <= 0 then return false end
  for link in allLinksFrom(cp) do
   if link.firebase ~= nil and link.endDepot ~= nil and link.endDepot.cp.side ~= cp.side then
    local mate = link.endDepot.m_outgoingLink
    if mate.firebase.open and mate.firebase.side ~= cp.side then
     return true
    end
   end
  end
  return false
end

----------------------------------------------------------------------
-- Returns true if the town has firebases.

gluehelp['hasFirebases'] = "(cp) Returns true if the specified CP has firebases connecting to/from it"
function hasFirebases(targetCP)
  local cp = getCP(targetCP)
  if cp == nil then return error("Unknown cp: " .. targetCP) end
  if not cp:isCity() or cp.numLinks <= 0 then return false end
  for link in allLinksFrom(cp) do
   if link.firebase ~= nil then return true end
  end
  return false
end

----------------------------------------------------------------------
-- Set the ownership of a CP

gluehelp['setCP'] = "(cp, country, tellPlayers, force) Changes ownership of CP. If tellPlayers is set to 'true', players will be notified - otherwise it is done silently"
function setCP(targetCP, country, announce, force)
 local cp = getCP(targetCP)
 if cp == nil then return error("Cannot find CP "..targetCP) end
 local ctry = getCountry(country)
 if type(ctry) ~= 'number' then return error("Unrecognized country "..country) end
 if force or cp.country ~= ctry then
  cp:changeOwnership(ctry, true, not(announce))
 end
 strtSaveCP(cp)
 return cp
end

----------------------------------------------------------------------
-- Set the ownership of a facility

gluehelp['setFacility'] = "(facility, country, tellPlayers) Changes ownership of a Facility. If tellPlayers is 'true', players will be notified - otherwise it is done silently"
function setFacility(targetFac, country, announce)
 local fac = getFacility(targetFac)
 if fac == nil then return error("Cannot find facility "..targetFac) end
 local ctry = getCountry(country)
 if type(ctry) ~= 'number' then return error("Unrecognized country "..country) end
 if fac.country ~= ctry then
  strtCaptureFacility(fac, ctry, fac.country, not(announce))
 end
 strtSaveFacility(fac)
 return fac
end

----------------------------------------------------------------------
-- Blow a firebase

gluehelp['blowFirebase'] = "(fb) Blows up the specified firebase"
function blowFirebase(targetFB)
 local fb = getFacility(targetFB)
 if fb == nil then return error("Invalid FB " .. targetFB) end
 if fb.type ~= FACILITY_FIREBASE then
  return error("Called blowFirebase with non-FB facility " .. fb.name)
 end
 if fb:isOpen() then
  strtProcessFirebase(fb)
 end
 strtSaveFacility(fb)
 return fb
end

----------------------------------------------------------------------
-- Blow a firebase

gluehelp['openFirebase'] = "(fb) Closes the opposing firebase"
function blowFirebase(targetFB)
 local fb = getFacility(targetFB)
 if fb == nil then return error("Invalid FB " .. targetFB) end
 if fb.type ~= FACILITY_FIREBASE then
  return error("Called openFirebase with non-FB facility " .. fb.name)
 end
 if fb:isOpen() == false and fb.mate then
  strtProcessFirebase(fb.mate)	-- Facility is closed, trigger the mate.
 end
 strtSaveFacility(fb)
 return fb
end

----------------------------------------------------------------------
-- Returns the Unix concept of 'current time', which is the number of
-- seconds since Jan 1st 1970. You can use this value to measure
-- durations or check for elapsed time.

gluehelp['currentTime'] = "() Returns the teulKit 'clockTime' value, which is the number of seconds since Jan 1st 1970"
function currentTime()
 if game.adjustTime then
  return teulClockTime + math.floor(game.adjustTime)
 else
  return teulClockTime
 end
end

----------------------------------------------------------------------
-- Set the fallback destination of an HC Unit
-- (targetCP can be nil if you want to deny the unit a fixed fallback)

gluehelp['setHcUnitFallbackDestination'] = "(hcUnit, cp) Set the fallback destination of the given HC Unit"
function setHcUnitFallbackDestination(hcUnit, targetCP)
 local unit = getHcUnit(hcUnit)
 if unit == nil then return error("Cannot find HCUnit " .. unit) end

 -- You can specify targetCP as 'nil' if you want to prevent the
 -- unit from falling back to a fixed destination.
 local cp = nil
 if targetCP ~= nil then
  cp = getCP(targetCP)
  if cp == nil then return error("Cannot find destination CP " .. targetCP) end
 end

 hcUnit.m_fallbackCP = cp
end

----------------------------------------------------------------------
-- Deploys a named division and all of its member brigades at a named cp
-- Unit can be unit short name, OID or object
-- If "transferSupply" is true, equipment will be trickled

gluehelp['deployDivision'] = "(unit, cp, [moveDelayInSeconds, [transferTrickleSupply]]) Immediately redeploys a division, all of its brigades and all their supply to the specified CP. Specifying a moveDelay will prevent players redeploying it immediately. If transferTrickleSupply is true, supply will behave as in a manual move" 
function deployDivision(hcUnit, destCP, nextMoveDelay_s, transferSupply)
 local unit = getHcUnit(hcUnit)
 if unit == nil then return error("Cannot find unit " .. hcUnit) end

 if not nextMoveDelay_s or nextMoveDelay_s < 0 then nextMoveDelay_s = 0 end

 deploy(unit, destCP, nextMoveDelay_s, transferSupply)
 if unit:isDivision() then
  local brigade = unit:next()
  while brigade ~= nil do
   deploy(brigade, destCP, nextMoveDelay_s, transferSupply)
   brigade = brigade:next()
  end
 end

 return true
end

----------------------------------------------------------------------
-- Deploy a named unit at a named cp
-- If "transferSupply" is true, equipment will be trickled

gluehelp['deploy'] = "(unit, cp, [nextMoveDelayInSeconds, [transferTrickleSupply]]) Immediately redeploy a unit, moving all supply immediately unless transferTrickleSupply is true"
function deploy(unitName, destCP, nextMoveDelay_s, transferSupply)
 local unit = getHcUnit(unitName)
 if unit == nil then return error("Cannot find unit " .. unitName) end

 if not nextMoveDelay_s or nextMoveDelay_s < 0 then nextMoveDelay_s = 0 end

 local cp
 if destCP == "offmap" then
  cp = routDestination[unit:country()]
  if cp == nil then return error("Cannot find rout destination for "..unitName) end
 else
  cp = getCP(destCP)
  if cp == nil then return error("Unable to find target destination CP " .. destCP) end
 end
 if cp == nil then return error("Cannot find cp " .. destCP) end

 unit:deploy(cp, nextMoveDelay_s, "SYSTEM", transferSupply, true)
 unit.m_fallbackCP = nil

 return true
end

----------------------------------------------------------------------
-- Deploy a named unit at a named cp
-- If "transferSupply" is true, equipment will be trickled

gluehelp['undeploy'] = "(unit) Move a brigade off-map and prevent it's redeployment for 10-years or so."
function undeploy(unitName)
 local unit = getHcUnit(unitName)
 if unit == nil then return error("Cannot find unit " .. unitName) end

 local cp = routDestination[unit:country()]
 if cp == nil then return error("Cannot find rout destination for "..unitName) end

 unit:deploy(cp, timeInSeconds{ years = 3 }, "SYSTEM", false, true)

 unit.m_fallbackCP = nil

 return true
end

----------------------------------------------------------------------
-- Validates the fallback destination of the *externally specified*
-- testUnit (set to short name, OID or StratHCUnit object of the brigade
-- you want to test)

function checkFallback(toCP)
 local unit = getHcUnit(testUnit)
 local cp = getCP(toCP)
 if unit == nil then return error("Couldn't lookup unit " .. testUnit)
 else
  local fallback = unit:findFallback()
  if fallback == nil then
   if toCP ~= "offmap" then warn("setup#"..setupNum.." no fallback") end
  elseif fallback.uOID ~= cp.uOID then
   warn("setup#"..setupNum.." failed, fallback to "..fallback.name.." not "..toCP)
  end
 end
 setupNum = setupNum + 1
 return true
end

----------------------------------------------------------------------
-- Place an capture attack objective (city only)

gluehelp['placeCaptureObjective'] = "(cp, brigade) Places an AO on the specified CITY with the specified brigade as the owner"
function placeCaptureObjective(target, withBrigade)
 if target == nil then error("placeCaptureObjective requires a target cp") end
 if withBrigade == nil then error("placeCaptureObjective requires a brigade") end
 local cp = getCP(target)
 if cp == nil then return error("placeCaptureObjective called with unrecognized cp: "..target) end
 if cp:isBridge() then return error(cp.name..": tried to set capture objective on bridge") end
 local bde = getHcUnit(withBrigade)
 if bde == nil then return error("Invalid brigade: " .. withBrigade) end

 if cp:isAttackObjective(bde:side()) then return true end

 if cp:isObjective() and not clearCaptureObjective(target) then
  return error(cp.name..": unable to remove existing objective")
 end

 cp:setAttackObjective(true, bde)

 return true
end

----------------------------------------------------------------------
-- Withdraw capture from a particular city

gluehelp['clearCaptureObjective'] = "(cp) Removes a capture objective from the specified city CP"
function clearCaptureObjective(target)
 local cp = getCP(target)
 if cp == nil then return error("clearAttackObjective called with unrecognized cp: "..target) end
 if cp:isBridge() then return error(cp.name..": tried to clear capture objective from bridge") end

 local attacker = wwiiOtherSide(cp.side)
 if cp:isDefendObjective(attacker) then
  if cp:isContested() then
   warn(cp.name..": contested defense objective; town will be uncontested first")
  end
  cp:clearObjective(cp.side)
  if cp:isObjective(attacker) then
   warn(cp.name..": clear objective did not withdraw defense objective!")
   cp:clearObjective(attacker)
  end
 end

 if cp:isObjective() then
  warn(cp.name..": invalid objective configuration, forcing removal")
  cp:clearObjective(ALLIED)
  cp:clearObjective(AXIS)
  if cp:isObjective() then
   return error(cp.name..": unable to force objective removal!")
  end
 end

 return true
end

----------------------------------------------------------------------
-- Place an attack objective on a cp/bridge.

gluehelp['placeAttackObjective'] = "(cp, brigade) Places an attack AO on the specified CP"
function placeAttackObjective(target, withUnit)
 local cp = getCP(target)
 if cp == nil then return error("placeAttackObjective called with unrecognized cp: "..target) end
 local unit = getHcUnit(withUnit)
 if unit == nil then return error("placeAttackObjective called with unrecognized unit: "..withUnit) end
 if cp:isBridge() then
  if cp:isObjective(unit:side()) then cp:clearObjective(unit:side()) end
 else
  if cp.side == unit:side() then return error(cp.name..": cannot place attack on friendly town") end
 end

 cp:setAttackObjective(true, unit)

 return true
end

----------------------------------------------------------------------
-- Reset the nominated StratHCUnit's supply back to capacity

gluehelp['resetUnitSupply'] = "(unit) Reset all supply back to capacity levels for the given unit"
function resetUnitSupply (forUnit)
 local unit = getHcUnit(forUnit)
 if unit == nil then return error("Unrecognized StratHCUnit: "..forUnit) end
 -- Remove all supply tickets and wipe all equipment (overstock etc)
 unit:purgeResupply(true)
 -- Put all equipment back
 unit:restartSupply(0, 0, false)
end

----------------------------------------------------------------------
-- Force rebuild of a town

gluehelp['rebuildCP'] = "(cp) Force all buildings in the specified CP to fully repair"
function rebuildCP(target)
 local cp = getCP(target)
 if cp == nil then return error("unrecognized CP for rebuild: "..target) end
 strtForceRebuildCP(cp)
 return true
end

----------------------------------------------------------------------
-- Get the name of a side

gluehelp['getSideName'] = "(side) Returns the displayable name for a given side"
function getSideName(forSide)
 local side = getSide(forSide)
 return _sideName[side]
end

----------------------------------------------------------------------
-- Get the posessive form of a side's name

gluehelp['getSideName'] = "(side) Returns the adjectival name for a given side"
function getSideNameAdj(forSide)
  local side = getSide(forSide)
  return _sideNameAdj[forSide]
end

----------------------------------------------------------------------
-- Get a weapon spec based on a weapon name or weap description

function getWeaponSpec(key)
 if tolua.type(key) == 'WEAPON_SPEC' then
  return key
 elseif type(key) == 'table' then
  local spec = weapCreateSpec("")
  spec.wCountry = key[1]
  spec.wCategory = key[2]
  spec.wClass = key[3]
  spec.wType = key[4]
  return spec
 elseif type(key) == 'string' then
  return weapFindWeaponByName(key)
 else
  error("Unknown weapon spec key type: "..type(key))
 end
end

----------------------------------------------------------------------
-- Tell me what side is opposite to this one

gluehelp["otherSide"] = "(forSide) returns the enemy side to a given side. For ALLIED returns AXIS, for AXIS returns ALLIED, for NEUTRAL returns NEUTRAL"
function otherSide(forSide)
 return wwiiOtherSide(forSide)
end

----------------------------------------------------------------------
-- Return the LSTR_ for a side.

function sideLstring(side)
 local sideNo = getSide(side)
 if not sideNo then error("Invalid side: " .. side) end
 if side == ALLIED then return LSTR_ALLIED end
 if side == AXIS then return LSTR_AXIS end
 return LSTR_SYSTEM
end

-- My shift-key is sticky ;-P
function sideLString(side) return sideLstring(side) end

----------------------------------------------------------------------
-- Send a message to all the players on a side

gluehelp["sidemessage"] = "(side, prefix, message, ...) sends a localized message to players on a given side. You'll need to know the LSTRING_IDs to use this."
function sideMessage(side, prefix, message, ...)
 local sideLstr
 if side == ALLIED or side == LSTR_ALLIED then sideLstr = LSTR_ALLIED
 elseif side == AXIS or side == LSTR_AXIS then sideLstr = LSTR_AXIS
 else
  return error("Called sideMessage with unknown side: " .. side)
 end

 if prefix == nil then
  return error("Called sideMessage with nil prefix")
 end

 if type(prefix) == "number" then
   chatSendSideMessage(side, nullObjID, sideLstr, prefix, message)
 elseif message == nil then
   chatSendSideMessage(side, nullObjID, sideLstr, LSTR_GENERIC, prefix)
 else
   chatSendSideMessage(side, nullObjID, sideLstr, LSTR_LABELLED_GENERIC, prefix, message)
 end
end

----------------------------------------------------------------------
-- Disable or enable supply, returning the previous value

function setSupply(state)
 local oldState = (host_config.supply.enabled == 1)
 if state then host_config.supply.enabled = 1 else host_config.supply.enabled = 0 end
 return oldState
end

function setTrickle(state)
 local oldState = (host_config.supply.trickleEnabled == 1)
 if state then host_config.supply.trickleEnabled = 1 else host_config.supply.trickleEnabled = 0 end
 return oldState
end

----------------------------------------------------------------------
-- Send a message to all players, identified as "SYSTEM"

gluehelp["systemMessage"] = "(message) Sends a message as SYSTEM: to all players."
function systemMessage(message)
 if message == nil then return error("Called systemMessage() with nil message???") end
 print("SYSTEM: "..message)
 chatSendBroadcastAll(LSTR_SYSTEM, LSTR_GENERIC, message)
end

----------------------------------------------------------------------
-- Warn all players

gluehelp["warnAll"] = "(message) Sends a message as WARNING: to all players."
function warnAll(message)
 if message == nil then return error("Called warnAll() with nil message???") end
 print("WARNING:ALL:"..message)
 chatSendBroadcastAll(LSTR_WARNING, LSTR_GENERIC, message)
end

----------------------------------------------------------------------
-- Mark a CP as overrun

gluehelp["setOverrun"] = "(forCP) Marks the specified CP as being overrun"
function setOverrun(forCp)
 local cp = getCP(forCp)
 if cp == nil then return error("Invalid cp: " .. forCp) end
 cp:markOverrunReported()
 cp:markOverrunReported()
 cp:markOverrunReported()
 cp:markOverrunReported()
 cp:markOverrunReported()
 cp:setAlert(CP_STAT_OVERRUN)
end

function addWeapon(unitName, weaponName, units)
 local unit = getHcUnit(unitName)
 if unit == nil then return error("Unknown HC Unit " .. unitName) end
 local weap = weapFindWeaponByName(weaponName)
 if weap.wCountry == 0 then return error("Unknown weapon " .. weaponName) end
 unit:depositWeapons(weap, units, false, nil)
 print("Added " .. units .. " " .. weaponName .. " to " .. unit:name())
end

----------------------------------------------------------------------
-- Interface to the "strtCaptureAndLog" call which does the same as a
-- regular capture but it logs it as though a player has done it.
-- Unfortunately, you have to look up all the player details yourself
-- for now.

function captureAndLog(theFac, thePlayer, customerID, theMissionID, theHCUnit)
 local fac = getFacility(theFac)
 if fac == nil then return error("Unknown facility: " .. theFac) ; end
 local handle = getHandle(thePlayer)
 if handle == nil then return error("Invalid gamename/handle: " .. thePlayer) end
 local unit = getHCUnit('104atr')
 if unit == nil then return error("Unknown HC Unit: " .. theHCUnit) ; end
 local seconds = 60
 strtCaptureAndLog(fac, handle, customerID, theMissionID, unit, seconds)
 print("Captured " .. fac.name)
end

----------------------------------------------------------------------
-- Advance the capture progress bar (if capture is enabled) for
-- a given facility in favor of the specified side, as long as it
-- doesn't cause it to become captured.

gluehelp["advanceCaptureProgress"] = "(forFacility, amount, forSide) Adds capture progress for a facility for a given side. E.g to add 10% to the ALLIED cap of your current facility: advanceCaptureProgress('%here', 10, ALLIED)"
function advanceCaptureProgress(forFac, amount, forSide)
 local fac = getFacility(forFac)
 if fac == nil then return error("Unknown facility: " .. forFac) end
 if fac.m_captureBuilding == nil then
   return error("'"..fac.name.."' does not have a recognized capture building associated with it.")
 end
 if fac:isCapturable() == false then return error("Facility '"..fac.name.."' is not capturable.") end

 if type(amount) ~= 'number' then return error("Invalid amount: " .. amount) end

 local side = getSide(forSide)
 if side == 0 then return error("Invalid side: " .. forSide) end

 local progress = fac:getCaptureProgress()
 local newProgress = progress + amount
 if newProgress <= 0.0 or newProgress >= 100.0 then
  return error("Advancing '"..fac.name.."' progress ("..progress.."%) by "..amount.."% would cause capture. Use capture functions instead.")
 end

 -- Make a capture contribution
 local country
 if side == ALLIED then
  country = ENGLAND
 elseif side == AXIS then
  country = GERMANY
 elseif side == NEUTRAL then
  country = CHINA
 end

 local contrib = Strat.CaptureContribution(Strat.CaptureContributionEvent.Active, fac, getHandle("SYSTEM"), 0, 0, 0, HCUnit:GetCommandUnit(country, BRANCH_LAND))
 fac:setCaptureProgress(newProgress, contrib)

 print("Advanced '"..fac.name.."' progress to "..newProgress.."%")
end

----------------------------------------------------------------------
-- Set the game time to a given hour and minute in hh:mm format.

gluehelp["setGameTime"] = "(timestr) changes the game time, e.g. setGameTime('23:50')"
function setGameTime(timestr)
  local hour, minute = string.match(timestr, "^%s*(%d+)[:.](%d+)")
  if hour == nil or hour == "" or minute == nil or minute == "" then
    return error("Invalid time string passed to setGameTime: "..timestr)
  end
  hour = math.floor(hour)
  minute = math.floor(minute)
  if hour < 0 or hour > 23 then
    return error("Invalid hour in time string: "..timestr)
  end
  if minute < 0 or minute > 59 then
    return error("Invalid minute in time string: "..timestr)
  end

  local seconds = timeInSeconds({ ['hours'] = hour, ['minutes'] = minute })
  wthrSetGameTime(seconds)
  cnvyUpdateTime()

  print("Time set to "..hour..":"..minute)
  
end

----------------------------------------------------------------------
-- Clear all attack objectives.

gluehelp["clearAllObjectives"] = "() Remove any/all attack/defend objectives."
function clearAllObjectives()
  for cp in allCPs() do
   if cp:isAttackObjective(ALLIED) then cp:clearObjective(ALLIED) end
   if cp:isAttackObjective(AXIS) then cp:clearObjective(AXIS) end
   if cp:isDefendObjective(ALLIED) then cp:clearObjective(ALLIED) end
   if cp:isDefendObjective(AXIS) then cp:clearObjective(AXIS) end
  end
  print("Done")
end

function clearAllAOs() clearAllObjectives() end
function clearAllAos() clearAllObjectives() end

----------------------------------------------------------------------
-- Face off two brigades

gluehelp["faceoff"] = "(town1, town2, unit1, unit2) Establishes a face-off pair of AOs between the two towns/units."

function faceoff(arg1, arg2, arg3, arg4, arg5)
  function usage(msg)
    warn("faceoff: invalid usage: "..msg)
    warn("faceoff expects two linked towns and two brigades (one from each side). e.g. faceoff('Antwerp', 'Schilde', 'sas', '1pzkg1'")
  end

  local leftCp, leftBde
  local rightCp, rightBde

  function interpretArg(arg)
    if type(arg) == 'string' then
      local val = StratHCUnit:Lookup(arg)
      if val ~= nil then return val end
      val = strtGetCPByName(arg)
      if val ~= nil then return val end
    elseif tolua.type(arg) == 'StratHCUnit' then
      return arg
    elseif tolua.type(arg) == 'CPStruct' then
      return arg
    end
    usage("Unrecognized CP/brigade: "..arg)
    return nil
  end

  if arg1 == nil or arg2 == nil or arg3 == nil or arg4 == nil then
   return usage("Not enough parameters")
  end
  if arg5 ~= nil then
   return usage("Too many parameters")
  end

  item = interpretArg(arg1)
  if item == nil then return end
  if tolua.type(item) == 'StratHCUnit' then leftBde = item else leftCp = item end

  item = interpretArg(arg2)
  if item == nil then return end
  if tolua.type(item) == 'StratHCUnit' then
   if leftBde == nil then leftBde = item else rightBde = item end
  else
   if leftCp == nil then leftCp = item else rightCp = item end
  end

  item = interpretArg(arg3)
  if item == nil then return end
  if tolua.type(item) == 'StratHCUnit' then
   if leftBde == nil then leftBde = item elseif rightBde == nil then rightBde = item else return usage("Too many brigades given") end
  else
   if leftCp == nil then leftCp = item elseif rightCp == nil then rightCp = item else return usage("Too many CPs given") end
  end

  item = interpretArg(arg4)
  if item == nil then return end
  if tolua.type(item) == 'StratHCUnit' then
   if leftBde == nil then leftBde = item elseif rightBde == nil then rightBde = item else return usage("Too many brigades given") end
  else
   if leftCp == nil then leftCp = item elseif rightCp == nil then rightCp = item else return usage("Too many CPs given") end
  end

  if leftCp.uOID == rightCp.uOID then
    return warn("Please use two different CPs...")
  end
  if leftBde == rightBde then
    return warn("Please use two different brigades...")
  end

  if leftBde:side() == rightBde:side() then
    return warn("Brigades must be on opposite sides.")
  end

  if leftCp:linkTo(rightCp) == nil then
    return warn("CPs must have a link between them.")
  end

  -- Change the CP ownerships.
  setCP(leftCp, leftBde:country(), true, true)
  setCP(rightCp, rightBde:country(), true, true)

  deploy(leftBde, leftCp, 0, false)
  deploy(rightBde, rightCp, 0, false)

  placeCaptureObjective(leftCp, rightBde)
  placeCaptureObjective(rightCp, leftBde)

  print("Faceoff of "..leftBde:callsign().." at "..leftCp.name..
		" vs "..rightBde:callsign().." at "..rightCp.name.." is ready!")
end

----------------------------------------------------------------------
-- Report on game time and weather

local Weather = { ['skyPatterns']  = { 'Sunny', 'Overcast', 'Chance of rain' } }

gluehelp["weatherReport"] = "() displays current server-side weather information"
function weatherReport()
  -- Query the weather system for the clock and convert to a HH:MM format.
  local gameTime = wthrConvertTimeToString(wthrGetGameTime())
  local skyPattern = wthrGetWeatherPattern()
  local rainPattern = wthrGetFogPattern()

  print("Game Time: " .. gameTime .. ". " ..
	"Sky: " .. Weather.skyPatterns[skyPattern] .. " ("..skyPattern.."/"..rainPattern.."). ")
end

----------------------------------------------------------------------
-- Generate next weather forecast

gluehelp["advanceDay"] = "() causes the strat host to advance to the start of the next day."
function advanceDay()
  strtStartNewDay()
  cnvyUpdateTime()
  local expect = wthrGetWeatherPattern()
  if expect == 0 then expect = "sunny" else expect = "overcast" end
end

gluehelp["changeWeather"] = "() changes the weather patterns on the host"
function changeWeather()
  local current = wthrGetWeatherPattern()
  if current == 1 then -- sunny
    systemMessage("Changing weather to overcast/chance of rain")
    wthrSetWeatherPattern(3)
    wthrSetFogPattern(5)
  else
    systemMessage("Changing weather to sunny")
    wthrSetWeatherPattern(1)
  end
  cnvyUpdateTime()
end

----------------------------------------------------------------------
-- Shorten the delay before a facility becomes capturable.

gluehelp["skipCaptureTimer"] = "(facilityName) tells the strat host to advance the capture counter for a given facility. You can use skipCaptureTimer('%here') as a short-cut for your current facility."
function skipCaptureTimer(facname)
  local fac = getFacility(facname)
  if fac == nil then return error("Unknown facility name, '"..facname.."'.") end
  if fac.m_captureBuilding == nil then
    return error("'"..fac.name.."' does not have a recognized capture building associated with it.")
  end
  if fac:isCapturePending() == false then
   return error("'"..facname.."' is not in capture-pending state.")
  end
  fac.m_captureCounter = 2
  fac.m_captureStateChanged = teulClockTime
  strtQueueFacilityStatus(fac)
  print("Timer set to 2 seconds.")
end

----------------------------------------------------------------------
-- For testing: add some time to the time system.

function addSeconds(n)
  game.adjustTime = game.adjustTime + n
  print("Lua time now adjusted by " .. game.adjustTime .. " seconds")
end

function resetSeconds()
  game.adjustTime = 0
  print("Lua time now back to normal.")
end

----------------------------------------------------------------------
-- Returns a string value of something you input.

local stringTranslations = {
	['string']	= function (value) return value end
,	['number']	= function (value) return value end
,	['boolean']	= function (value) if value then return '<true>' else return '<false>' end end
}

stringUserdataTranslations = {
	['CPStruct'] = function (cp) return '<cp:'..cp.name..'>' end
,	['FacilityStruct'] = function (fac) return '<fac:'..fac.name..'>' end
,	['ObjectStruct'] = function (obj) return '<obj:'..obj.name..'>' end
,	['FULL_OBJ_ID'] = function (fid)
		return '<fid:'..fid.cell.ox..','..fid.cell.oy..'/'..fid.id..'>'
	end
}

function toString(value)
	if not value then return '<not set>' end
	local fn = stringTranslations[type(value)]
	if fn then return fn(value) end
	fn = stringUserdataTranslations[tolua.type(value)]
	if fn then return fn(value) end
	return value
end

----------------------------------------------------------------------
-- Force all RDP cycles to 100%.

gluehelp['advanceRDP'] = "() Trigger the advance of all RDP cycles." ;
function advanceRDP()
  dbDo("UPDATE wwiionline.wwii_country SET rdp_percent = 100, rdp_produced = rdp_goal WHERE rdp_goal > 0")
  log("I", "strat", "RDP cycles forced to 100%, should take effect within 120 seconds.")
end

----------------------------------------------------------------------
-- Rebuild everything on the map.

gluehelp['rebuildMap'] = "() Rebuild all CPs and bridges on the map." ;
function rebuildMap()
  for cp in allCPs() do
    strtForceRebuildCP(cp)
  end
end

----------------------------------------------------------------------
-- Restore full supply to all units on the map.

gluehelp['restoreAllSupply'] = "() Flush all resupply tickets and reset all brigades to full supply." ;
function restoreAllSupply()
 for unit in allStratHCUnits() do
  -- Remove all supply tickets and wipe all equipment (overstock etc)
  unit:purgeResupply(true)
  -- Put all equipment back
  unit:restartSupply(0, 0, false)
 end
end

