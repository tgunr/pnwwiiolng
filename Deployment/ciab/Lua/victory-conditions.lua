-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
--! @file	victory-conditions.lua
--! @brief	Contains the actual Victory Condition rules.
--! @detail
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

-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
--! Victory Condition: Capture X% of the map.
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --

VictoryConditions.Condition.percentage_of_map_owned = {
	['Value'] = 10						-- Something we can squeeze stuff around.
,	['Rule'] = function (self, side)

		-- Percentage we must reach to win.
		local totalOwnableCPs = totals[ALLIED].originals + totals[AXIS].originals
		local cpsOwned = totals[side].current
		local percentageOwned = cpsOwned * 100 / totalOwnableCPs
		
		local victory = 92			-- CONFIGURABLE: Percentage to win.

		return ( percentageOwned >= victory )

	end
}

-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
--! Victory Condition: Enemy has no factories remaining.
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --

VictoryConditions.Condition.enemy_factories_captured = {
	['Value'] = 25						-- Worth more than total map ownership?
,	['Rule'] = function (self, side)

		-- Check the factory count for the other side.
		return ( totals[otherSide(side)].factories <= 0 )

	end
}

-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
--! Victory Condition: Enemy has no supply.
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --

VictoryConditions.Condition.enemy_supply_cutoff = {
	['Value'] = 50						-- Worth a lot, force a win.
,	['Rule'] = function (self, side)

		return ( totals[otherSide(side)].linked <= 0 )

	end
}

-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
--! Victory Condition: Enemy has no units left on the map.
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --

VictoryConditions.Condition.enemy_routed = {
	['Value'] = 50						-- Worth a lot, force a win.
,	['Rule'] = function (self, side)

		return ( totals[otherSide(side)].unitsOnMap <= 0 )

	end
}

-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
--! Victory Condition: Enemy has few remaining linked towns.
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --

VictoryConditions.Condition.supply_constriction = {
	-- Score equivalent to theater ownership
	['Value'] = VictoryConditions.Condition.cp_percentage_owned.Value
,	['Rule'] = function (self, side)

		local threshold = 10			-- CONFIGURABLE.

		return ( totals[otherSide(side)].linked <= threshold )

	end
}

-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
--! Victory Condition: Enemy has no remaining airfields.
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --

VictoryConditions.Condition.airfields_denied = {
	['Name'] = "Enemy Airfields Denied"
,	['Value'] = 5
,	['Rule'] = function (self, side)

		return ( totals[otherSide(side)].airfields <= 0 )

	end
}

