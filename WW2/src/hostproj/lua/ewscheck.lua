-- Check the map for towns where one side has a numeric infantry
-- advantage and pick the best 1 or 2 for each side to announce
-- to players to give them an idea of where they are needed.
-- Set "announceToSide" to "true" if you want the message sent
-- as an intel report to all players

require 'localization'

local intelSpamEnabled = true

----------------------------------------------------------------------
-- Entry point
function runEWSCheck(forSide, announceToSide)
	-- Check that 'forSide' makes sense
	if forSide ~= ALLIED and forSide ~= AXIS then
		warn("runEWSCheck(forSide, announceToSide) does not recognize side " .. forSide)
		return false
	end

	-- Highest 'threat' seen so far; we set this to non-zero to start with so that
	-- the script won't even consider towns below a given threshold
	-- Set really low mostly to avoid softcaps more than anything.
	local maxThreat = 6

	-- Which two CPs had the highest threats
	local cp1 = nil
	local cp2 = nil	

	-- Start run ... Go over all the citys and see what their threats are like
	for cp in allCityCPs() do
		local threat = ewsCheck_calculateThreat(forSide, cp, cp1, maxThreat)
		-- Higher threat than the previous best? If so, shuffle cp to 2nd cp and place me
		if threat >= maxThreat then
			cp2 = cp1
			cp1 = cp
			maxThreat = threat
		end
	end

	-- Did we find a candidate?
	if maxThreat == 0 or cp1 == nil then return false end

	local message
	local name1 = cp1.name
	local name2 = nil
	if cp2 == nil then
		message = LSTR_SOLDIERS_NEEDED_AT
	else
		message = LSTR_SOLDIERS_NEEDED_AT_AND
		name2 = cp2.name
	end

	-- If announceToSide is true, send a message to all players on the entire side
	if announceToSide == false then return true end
	-- What prefix should the message have?
  	if intelSpamEnabled == true or host_config.arena.intermission > 0 then
      chatSendSideMessage(forSide, nullObjID, sideLString(forSide), message, name1, name2)
	else
		chatSendHCMessage(forSide, message, name1, name2)
	end

	return true
end

----------------------------------------------------------------------
-- Check to see if a new CP beats our previous best in terms of
-- need for additional soldiers. If this is a good candidate,
-- returns a numerical value for that demand.
function ewsCheck_calculateThreat(side, cp, prevCp, prevBest)

	-- Needs to be a brigade here and some infantry ews
	if not(cp:hasInfantryEWS()) then return 0 end

	-- Don't consider towns where the AO is being cleared
	if cp.m_clearing[cp:attacker()] then return 0 end

	-- Check for an enemy presence that ignores silly softcaps
	local opposition = cp:getPlayers(otherSide(side), _INFANTRY)
	if opposition < 4 then return 0 end

	-- Check if we have a near-matching presence
	local presence = cp:getPlayers(side, _INFANTRY)
	if presence == 0 or presence > opposition * 0.75 then return 0 end

	-- Factor in armor
	opposition = opposition + (cp:getPlayers(otherSide(side), _GROUND_VEHICLE) / 3)
	presence = presence - (cp:getPlayers(side, _GROUND_VEHICLE) / 3)

	-- Determine how outnumbered the enemy are
	local threat = opposition - presence

	-- Bump the need for obvious boons to actually fighting here
	if cp:isContested() then threat = threat + 3 end
	if cp:isObjective() then threat = threat * 2.1 end

	-- If we exceed the previous threat, we're golden
	if threat > prevBest then return threat end

	-- Try not to blindly replace the previous need if we match out
	if threat == prevBest
	and (prevCp == nil or presence > prevCp:getPlayers(side, _INFANTRY)) then
		return threat
	end

	-- Otherwise, ignore this candidate
	return 0
end

