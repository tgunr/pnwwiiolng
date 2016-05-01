----------------------------------------------------------------------
-- stratlib.lua
--
--	Defines formulae for various strat calculations that need to be
--	more flexible than simple variables.
--
----------------------------------------------------------------------

require 'cpevents'
require 'events'

require 'auto-ao-placement'
require 'ewscheck'
require 'bde-strength-check'
require 'threat-level'

require 'capture-functions'
require 'cp-monitor'

require 'save-map'

require 'becdwrapper'		-- Functions for loading BECD campaigns

----------------------------------------------------------------------
-- Simple variables
----------

---- What percentage of Attack Objectives must be engagements (on towns
---- with an enemy brigade/division in it) before the HCs can place non-
---- engagement AOs?
--engagementObjectivesPercentage = 65
engagementObjectivesPercentage = 25	-- First soft-cap at 3 AO/side

---- How many players are required to be present to trip "High" EWS
---- Note that "trucks" are counted as _INFANTRY
ewsHighThreshold[_INFANTRY] = 8
ewsHighThreshold[_GROUND_VEHICLE] = 4
ewsHighThreshold[_AIRCRAFT] = 4
ewsHighThreshold[_SEA_VEHICLE] = 3

---- Enable the "INTEL" system telling players where to fight?
---- Recommended: false (for campaigns)
ewsCheckEnabled = true

---- Interval between EWS checks
---- Recommended: timeInSeconds({ minutes = 15 })
ewsCheckInterval = timeInSeconds({ minutes = 5 })

----------------------------------------------------------------------
-- HCUnits/Brigades/Divisions

-- After a brigade has been moved how long until the next move?
-- of a unit within the division

brigadeMoveTime_s = timeInSeconds({ hours = 1 })
brigadeFallbackTime_s = timeInSeconds({ minutes = 30 })

-- For a division?
divisionMoveTime_s = timeInSeconds({ hours = 1 })
divisionFallbackTime_s = timeInSeconds({ minutes = 30 })

-- If you get kicked off the map?
unitReformTime_s = timeInSeconds({ hours = 12 })

----------------------------------------
-- HCUnit/Pocket controls:

-- How much to slow down unit moves inside a pocket.
-- The brigadeMoveTime_s etc will be multiplied by this value.

pocketMovementDelayFactor = 2.0

-- After this amount of time, moves inside a pocket will take
-- pocketMovementDelayFactor longer.

pocketTimeToSlowMoves_s = timeInSeconds({ hours = 1 })

-- How long after becoming frontline + out of supply
-- should a town surrender?

pocketTimeToSurrender_s = timeInSeconds({ hours = 6 })

-- How often the CPMonitor runs it's cycle (which controls
-- the frequency with which CPs are checked for surrender)

cpMonitorInterval = timeInSeconds({ seconds = 15 })

-- Maximum number of towns that can turn, regardless of side
-- per CPMonitor run.

pocketTurnTownsAtATime = 1

----------------------------------------------------------------------
-- Automatic attack objective controls

-- true/false - should automatic attack objective placement be enabled
gameAOPlacementEnabled = true
-- How long should we wait after AOs become spare before notifying the HCs
gameAONoticeDelay = timeInSeconds({ minutes = 5 })
-- How long between warnings?
gameAOWarningTime = timeInSeconds({ minutes = 1 })
-- How many warnings?
gameAOWarnings = 10
-- How many times to warn a second spare?
gameAOReWarnings = 4
-- What fraction of available AOs must be placed?
gameAOMinimumRatio = 1
gameAOMinimum = 0

----------------------------------------------------------------------
-- Functions to call once strat is ready for data

function stratSetup ()
  -- Where do units get rout'd to?
  routDestination[WWII_ENGLAND] = getCP(locstr('ukTraining'))
  routDestination[WWII_FRANCE] = getCP(locstr('frTraining'))
  routDestination[WWII_GERMANY] = getCP(locstr('deTraining'))
  routDestination[WWII_USA] = getCP(locstr('ukTraining'))

  if routDestination[WWII_ENGLAND] == nil or
     routDestination[WWII_FRANCE] == nil or
     routDestination[WWII_GERMANY] == nil or
     routDestination[WWII_USA] == nil then
   error("*** routDestination table not initialized correctly")
  end
end

----------------------------------------------------------------------
-- Functions
----------

----------------------------------------------------------------------
-- maxBridgeObjectives([population])
--	Return the current maximum allowed bridge objectives
--
-- INPUTS:
--	[population]	Use specified number of players instead of chat population
----------------------------------------------------------------------

function maxBridgeObjectives (population)
	-- If population is not specified, use the current chat population
	if population == nil then population = getPopulation() end

	local minimum = 4
	local populationPer = 95
	local extra = math.floor(population / populationPer)

	return minimum + extra
end

----------------------------------------------------------------------
-- maxCaptureObjectives()
--	Return the current maximum allowed capture objectives
----------------------------------------------------------------------

--function maxCaptureObjectives (population)
--	-- If population is not specified, use the current chat population
--	if population == nil then population = getPopulation() end
--
--	-- Minimum number of AOs
--	local minimum = 3
--	-- Baseline number of players required per AO
--	-- HIGHER values mean more players for each AO
--	local populationPer = 75
--
--	-- Sliding curve increment that increases the
--	-- players required for each AO as the population rises.
--	-- LOWER values mean more plaeyrs for each AO.
--	local playerIncrement = 17
--
--	local increment = population / playerIncrement
--	local target = math.floor(population / (populationPer + increment))
--
--	if target < minimum then return minimum end
--
--	-- Otherwise what we have is good
--	return target
--end

function maxCaptureObjectives (population)
  if population == nil then population = getPopulation() end

--  if population >= 720 then return 7 + math.floor((population - 720) / 100) end
--  if population >= 610 then return 6 end   -- 12 objectives total, 25.4 players/side/obj
--  if population >= 480 then return 5 end   -- 10 objectives total, 24.5 players/side/obj
--  if population >= 350 then return 4 end   -- 8 objectives total, 21 players/side/obj
--  if population >= 220 then return 3 end   -- 6 objectives total, 18 players/side/obj
   if population >= 790 then return 8 + math.floor((population - 790) / 100) end
   if population >= 680 then return 7 end   -- 14 total, 48 p/s/o
   if population >= 570 then return 6 end   -- 12 total, 48 p/s/o
   if population >= 460 then return 5 end   -- 10 total, 49 p/s/o
   if population >= 280 then return 4 end   -- 8 total, 50 p/s/o
   if population >= 190 then return 3 end   -- 6 total, 51 p/s/o
   if population >=  85 then return 2 end   -- 4 total, 55 p/s/o
-- return 2 -- this is for beta testing
   return 1  -- 2 total
end

function countsAsEngagement(target)
	-- If the town has units present, it's a hard AO.
	if target:numUnits() > 0 then return 1 end

	-- If the town has at least one friendly link with a brigade present,
	-- it's a "No Man's Land" AO.
	local i = 0
	while i < target.numLinks do
		link = target:link(i) 
		i = i + 1
		cp = link.endDepot.cp
		if target.side == cp.side then
			if cp:numUnits() > 0 then
				return 1
			end
		end
	end

	-- An exception for factory towns to avoid potential abuse.
	if target:isSupplyCenter() then return 1 end

	-- No brigade in this CP or nearby, it's a softcap.
	return 0
end

----------------------------------------------------------------------
-- enagagementObjectivesQuota(availableAOs)
-- Given that a side has a certain number of AOs available, determine
-- how many of those MUST be engagements before the system will allow
-- a softcap to be placed.
-- Note: The value of "0" is a special case, meaning there is no
-- requirement to place an engagement.
----------------------------------------------------------------------

function engagementObjectivesQuota(availableAOs)
  -- Note: when only one ao is available,
  -- the return value will be zero, allowing
  -- a softcap to be placed.

  if availableAOs == 1 then return 1 end
  return availableAOs - 1
end

----------------------------------------------------------------------
-- RDP bombing penalty to resupply timers
----------------------------------------------------------------------

function rdpPenaltyPercentage (forCountry)
  if forCountry == USA then
    return (rdpPenaltyPercentage(ENGLAND) + rdpPenaltyPercentage(FRANCE)) / 2
  end
  -- 100% factory damage translates to 100% delay in resupply
  -- changed for campaign 81 BLOODY BATTLES on 03-21-2012 per DOC
  return 1.0 + factoryDamageFactor[forCountry] * 1.00
  -- return 1.0
end

----------------------------------------------------------------------
-- resupplyTime (weapon, StratHCUnit, facility)
--      Return the number of seconds it should take to resupply
--      the specified weapon at the specified facility
--      "weapon" is a weapon spec, i.e. consisting of:
--       weapon.wCountry/wCategory/wClass/wType

-- Potentially interesting cp fields are:
--- cp:isEnemyLinked()  (means we're front line)
--- cp:isFriendlyLinked()
--- cp:isContested() (contested)
--- cp:isUnderAttack() (EWS is on)
--- cp:isSupplyCenter() (does the CP have factories?)
-- Potentially interesting cp functions are:
--- cp:isAttackObjective(forSide)  e.g. cp:isAttackObjective(ALLIED)
--- cp:isBridge()
--- cp:isCaptureObjective(forSide)
--- cp:isCity()
--- cp:isDefendObjective(forSide)
--- cp:isFallingBack()
--- cp:isHolding([whichSide])
--- cp:isObjective([forSide])
--- cp:isOverrun([forSide])
--- cp:hasABs(whichCountry)
--- cp:isSupplied()
--- cp:isSupplyCenter()  (is active supply center)

function resupplyTimeFn (weapon, unit, facility, unitResupplyTimer)
--  local cp = facility.cp
  local interval = host_config.supply.resupplyTime_m * 60
  local rdpEffect = rdpPenaltyPercentage(weapon.wCountry)

  return interval * rdpEffect
end

trainingDeploymentDelay = 900 * 24 * 60 * 60

if getHCUnit("trngukarmy") then
 deploy("trngukarmy", locstr('ukFort'), trainingDeploymentDelay)
 deploy("trngukair", locstr('ukFort'), trainingDeploymentDelay)
 deploy("trnguknavy", locstr('ukFort'), trainingDeploymentDelay)
 deploy("trngfrarmy", locstr('frFort'), trainingDeploymentDelay)
 deploy("trngfrair", locstr('frFort'), trainingDeploymentDelay)
 deploy("trngfrnavy", locstr('frFort'), trainingDeploymentDelay)
 deploy("trngdearmy", locstr('deFort'), trainingDeploymentDelay)
 deploy("trngdeair", locstr('deFort'), trainingDeploymentDelay)
 deploy("trngdenavy", locstr('deFort'), trainingDeploymentDelay)
 deploy("trngusarmy", locstr('ukFort'), trainingDeploymentDelay)
 deploy("trngusair", locstr('ukFort'), trainingDeploymentDelay)
 deploy("trngusnavy", locstr('ukFort'), trainingDeploymentDelay)
end

