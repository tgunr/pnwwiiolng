-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
--! @file	victory-conditions-system.lua
--! @brief	System for implementing the Victory Condition rules.
--! @detail
--! Run periodically to determine if there is a clear winner
--! based on victory conditions.
--!
--! Each condition is described within its own object under
--! the VictoryConditions.Condition namespace.
--!
--!  Condition.Rule()	provides the code for testing the rule,
--!  Condition.Value	how many points this condition earns
--!
--! The name of the Condition object is used to create a
--! localization string and also stored in the database.
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --

--! Victory Condition namespace.
--! Once victory is delcared, the winner is described in
--! VictoryConditions.Victor.
--! Individual Victory Condition descriptions have their own
--! sub-namespace, VictoryConditions.Condition

VictoryConditions = {
	['Victor'] = nil				--!< Details of side declared winner.
,	['Victory'] =   10				--!< Must have this many points to win.
,	['Condition'] = {}				--!< Namespace for condition rules.
}

-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
--! Totals of ownership etc.
--! Creates a structure where you can say
--! totals[ALLIED].originals etc to get the
--! total for a given side.

--! Creates a new stats array for you.
function VictoryConditions.NewStats()
	return {
		['originals']	=	0		--!< CPs held at map start.
	,	['current']		=	0		--!< CPs held currently.
	,	['value']		=	0		--!< Value of current CPs.
	,	['linked']		=	0		--!< Towns with factory links.
	,	['factories']	=	0		--!< Factory towns remaining.
	,	['airfields']	=	0		--!< Number of airfields.
	,	['unitsOnMap']	=	0		--!< Number of HC Units on the map
	}
end

-- The condition rules are stored ina  separate lua file.
require 'victory-conditions'

-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
-- -- BEGIN MAIN CODE -- --
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --

local totals = { }
totals[ALLIED] = VictoryConditions.NewStats()
totals[AXIS]   = VictoryConditions.NewStats()

-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
--! Calculate the factors to various victory conditions.
--! Populates the "allies" and "axis" tables with counts
--! of how many CPs they own, how many factories
--! remaining, etc.
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --

function VictoryConditions.Count()

	-- Repeat the enclosed loop for every city CP
	-- setting 'cp' to the current CP.
	for cp in allCityCPs() do

	  -- Only consider CPs that have links and are owned
	  -- by one of the combatant side.
	  if cp.numLinks > 0 and (cp.side == ALLIED or cp.side == AXIS) then
	  
		-- Count original ownerships.
		totals[cp.originalSide].originals = totals[cp.originalSide] + 1
	  
		-- Count each CP to the owning side,
		-- or both sides if contested.
		totals[cp.side].current = totals[cp.side].current + 1
		if cp.side ~= cp.controllingSide then
			local enemy = otherSide(cp.side)
			totals[enemy].current = totals[enemy].current + 1
		end

		totals[ALLIED].value = totals[ALLIED].value + cp.armybasesOwned[ALLIED]
		totals[AXIS].value = totals[AXIS].value + cp.armybasesOwned[AXIS]

		-- Count airfields.
		for fac in allFacilitiesAt(cp) do
		  if fac.side == cp.side then
		    if fac.type == FACILITY_AIRFIELD then
			  totals[cp.side].airfields = totals[cp.side].airfields + 1
			end
          end
		end

		-- Note the ':', required to call a function-of-a-thing.
		if cp:isSupplyCenter() then
		  -- Contains factories and belongs to original owner.
		  totals[cp.side].factories = totals[cp.side].factories + 1
		elseif cp:isSupplied() then
		  -- Count towns with factory links.
		  totals[cp.side].linked = totals[cp.side].linked + 1
		end

	  end

	end		-- End of 'for' loop

	-- Now count all of the deployed HC Units.
	for unit in allStratHCUnits() do
	  if unit:isBrigade() or unit:isDivision() then
	    if not ( unit:offMap() or unit:isTrainingUnit() ) then
		  totals[unit:side()].unitsOnMap = totals[unit:side()].unitsOnMap + 1
		end
      end
	end

end		-- End of function

-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
--! Evaluate all of the Victory Condition routines.
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --

function VictoryConditions.Calculate()

	-- If victory has been declared, end of story.
	if VictoryConditions.Victor ~= nil then
		return
	end

	-- If we are in intermission or arena locked, don't do it either.
	if host_config.arena.intermission > 0 or host_config.arena.lock > 0 then
		return
	end

	-- First we need to call the Count function to get
	-- the current stats.
	VictoryConditions.Count()

	local alliedWins = ""
    local alliedScore = 0
	local axisWins = ""
    local axisScore = 0

	-- Go through all of the condition modules, evaluating
	-- each for a possible win by each side.
	-- pairs breaks Condition into a list of key:value elements.
	for name, condition in pairs(VictoryConditions.Condition) do
	
	  -- Does the rule apply to the ALLIES?
	  if condition:Rule(ALLIED) then
		alliedWins = alliedWins .. "," .. name
	    alliedScore = alliedScore + condition.Value
	  end

	  -- Does it apply to the AXIS?
	  if condition:Rule(AXIS) then
		axisWins = axisWins .. "," .. name
		axisScore = axisScore + condition.Value
	  end

    end

	-- Update the database
	dbDo("REPLACE INTO `wwii`.`victory` (`key`, `value`) "..
			"VALUES "..
				"('alliedScore', "..alliedScore.."),"..
				"('axisScore', "..axisScore.."),"..
				"('alliedResults', '"..alliedWins.."'),"..
				"('axisResults', '"..axisWins.."')")

	if alliedScore > VictoryConditions.Victory and alliedScore > axisScore then
		VictoryConditions.Declare(ALLIED, "Allied", alliedWins, alliedScore)
	elseif axisScore > VictoryConditions.Victory and axisScore > alliedScore then
		VictoryConditions.Declare(AXIS, "Axis", axisWins, axisScore)
	end
end

-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
--! What to do when a side is declared the victor.
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --

function VictoryConditions.Declare(forSide, message, wins, score)
	-- Make sure nothing messes with us while we're working.
	VictoryConditions.Victor = { ['side'] = forSide, ['wins'] = wins, ['score'] = score }

	--- TODO: Replace with localized strings below.
	chatSendSideMessage(forSide, nullObjID, LSTR_SYSTEM, LSTR_GENERIC, "CONGRATULATIONS: " .. message .. " forces have been declared VICTORIOUS!")
	chatSendSideMessage(otherSide(forSide), nullObjID, LSTR_SYSTEM, LSTR_GENERIC, message .. " forces have been declared victorious.")

	--[[
		-- Have the client handle 'VICTORY' specially; have it interpret 'wins'
		-- as a comma separate list of victory-condition descriptions.
		chatSendSideMessage(forSide, nullObjID, LSTR_VICTORY, LSTR_OUR_VICTORY, wins, score)
		chatSendSideMessage(otherSide(forSide), nullObjID, LSTR_VICTORY, LSTR_ENEMY_VICTORY, wins, score)
	]]--

	-- Record to database.
	dbDo("REPLACE INTO `wwii`.`victory` (`key`, `value`) VALUES ('victor', '"..forSide.."')")

end

