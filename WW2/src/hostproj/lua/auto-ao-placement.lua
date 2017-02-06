--[[
    Auto AO Placement System
    $Id: auto-ao-placement.lua 3317 2010-04-14 22:05:05Z osmith@CRS $

    Each time the server notices spare AO capacity for a side,
    it calls 'gameSpareAOCapacity(forSide)'.
    This system checks for AOs to be placed above a certain
    threshold, so that players have plenty to do. If the AO numbers
    fall below the threshold, it endeavours to find a fair and
    relatively even fight. Some weight is given to the whereabouts
    of players in game, with some biasing against armor.
]]--

----------------------------------------------------------------------
----------------------------------------------------------------------
-- Initialization

if not (gameAOPlacementEnabled or gameAOPlacements) then
	gameAOPlacements = { ALLIED = nil, AXIS = nil }
	gameAONotice = { ALLIED = gameAOWarnings, AXIS = gameAOWarnings }
	gameAOLastCap = {}
	gameAOLastCap[ALLIED] = 0
	gameAOLastCap[AXIS] = 0
	gameAOLastSpare = {}
	gameAOLastSpare[ALLIED] = 0
	gameAOLastSpare[AXIS] = 0
	gameAOMinInfantryStrength = 60		-- Must be 60% or higher
end

----------------------------------------------------------------------
----------------------------------------------------------------------
-- true/false, check if this cp is a valid ao origin

function _validOriginForAO(cp, forSide)

	-- Citys only
	if not cp:isCity() then return false end
	-- Must be our town
	if cp.side ~= forSide then return false end
	-- Frontline towns
	if cp.numLinks <= 0 or cp:isEnemyLinked() == false then return false end
	-- Needs to have a brigade to attack with
	if cp:numUnits() <= 0 then return false end
	-- Ignore "out of play" towns
	if cp.country == WWII_CHINA or cp.country == WWII_RUSSIA then return false end
	-- Ignore the 'offmap' towns
	if cp.uOID == routDestination[ENGLAND].uOID or
				cp.uOID == routDestination[FRANCE].uOID or
				cp.uOID == routDestination[GERMANY].uOID or
				cp.uOID == routDestination[USA].uOID then
		return false
	end
	return true

end

----------------------------------------------------------------------
----------------------------------------------------------------------
-- Given a city, link and remote city, score the potential for an AO
--  Origin := the town we would be attacking *from*
--  Link := The link from that town to target
--  Target := the town that would be attacked
--  Country := The attacking country

function _scoreAOPotential(unit, origin, link, target, country)

	-- Make sure it's an enemy city
	if target.side == origin.side then return 0 end
	if target:isBridge() then return 0 end

	-- And we should skip anything that's an objective
	if target:isObjective() then return 0 end

	-- Never place a softcap as our sole AO.
	if g_objectiveStats:attacks(origin.side) == 0 and not countsAsEngagement(target) then return 0 end

	-- Never pick a town with heavy armor ews unless the favor is returned
	if target:hasHeavyArmorEWS() == true and origin:hasHeavyArmorEWS() == false then return 0 end

	local infStr = unit:getSupplyPctg(INFANTRY)
	if infStr < gameAOMinInfantryStrength then return 0 end

	-- Make sure it's an enemy city
	-- Start the score with the number of infantry present.
	local attackingInf = target:getPlayers(origin.side, INFANTRY)
	local defendingInf = target:getPlayers(target.side, INFANTRY)
	local infAverage = (attackingInf + defendingInf) / 2
	local score = target.m_value + (infStr / 10) + math.floor(infAverage)

	if origin:isObjective() then score = score + 1 end
	score = score + origin.numLinks - target.numLinks
	if target:numUnits() == 0 then score = score + origin.m_value end

	score = score + math.floor(attackingInf / ewsHighThreshold[INFANTRY]) * 10

	local str = "Init score = " .. score

	-- Try to pick a fair fight
	score = score + origin.numArmyBases - target.numArmyBases
	if origin:numUnits() >= target:numUnits() then
		str = str .. " " ..origin:numUnits().."v"..target:numUnits()
		score = score + 5 + origin:numUnits() - target:numUnits()
	end

	-- You pay for having armor at the town
	local attackingArmor = target:getPlayers(origin.side, GROUND_VEHICLE)
	local defendingArmor = target:getPlayers(target.side, GROUND_VEHICLE)
	if attackingArmor > defendingArmor then
		str = str .. " aa>da"
		score = score + (attackingArmor / 10)
	end
	local armorBonus = math.floor(unit:getSupplyPctg(GROUND_VEHICLE) / 20)
	if armorBonus > 2 then
		str = str .. " +ab" .. armorBonus
		score = score + armorBonus
	end

	-- Avoid launching attacks from pockets
	if origin:isSupplied() == false then
		str = str .. " !sup"
		score = score / 2
		if origin:isFriendlyLinked() == false then
			str = str .. " !fl"
			score = score / 2
		end
	end

	-- But an enemy pocket is a nice target
	if target:isFriendlyLinked() == false then
		if target:isSupplied() == false then
			str = str .. " +pocket"
			score = score * 1.5
			-- HUGE bonus for retaking our towns
			if target.originalSide == origin.side then
				score = score * 1.25
			end
		else
			str = str .. " +1/2pocket"
			score = score * 1.25
		end
	end

	-- Bonus for Factory Towns
	if target.m_isSupplyCenter then
		-- Particularly so if the enemy is holding one of ours!
		if target.originalSide == origin.side then
			str = str .. " +scos"
			score = score * 2
		else
			str = str .. " +sc"
			score = score * 1.25
		end
	end

	if link.firebase ~= nil and link:hasEnemyFirebase(origin.side) == false then
		-- Also favor towns where we have the firebase: but not if it is
		-- under attack
		if link.firebase.m_onRepairQueue == false then
			str = str .. " +fb"
			score = score * 1.5		-- Its not under attack
		end
	elseif link.firebase then -- It has an enemy firebase, lower the score
		str = str .. " efb"
		score = score * 0.75
	else
		str = str .. " nofb"
		score = score * 1.3
	end

	-- Is Origin under attack?
	if origin:isDefendObjective(origin.side) then
		score = score * 2
		-- Is the destination the home of the attacking unit?
		local attacker = origin:attackingUnit()
		if attacker ~= nil and attacker:homeCPID() == origin.fullObjID then
			-- Big bonus for attacking our attacker.
			score = score * 3
		end
	end

	-- For multi-country sides: favor the "prefered" country
	if score > 0 and origin.country == country then str = str .. " +ctry" ; score = score * 1.5 end

	score = math.floor(score)

	log('D', 'autoao', "Considering " .. unit:name() .. " : " .. origin.name .. " => " .. target.name .. " (" ..score ..")" .. str)

	return score
end

----------------------------------------------------------------------
-- Pick the underdog country of a given side

function gameAOChooseAlliedCountry(forSide)

	if forSide == AXIS then return WWII_GERMANY end

	local cp, neighbor
	local french = 0
	local brit = 0

	for cp in allCityCPs() do
		if cp:isAttackObjective(ALLIED) then
			for neighbor in allNeighbors(cp) do
				if neighbor.country == WWII_FRANCE then
					french = french + 1
				elseif neighbor.country == WWII_ENGLAND then
					brit = brit + 1
				end
			end
		end
	end

	if french == 0 or brit > french then return WWII_FRANCE else return WWII_ENGLAND end

end

----------------------------------------------------------------------
-- Check if a unit is a worthwhile candidate

function _isCandidateUnit(unit)
	if unit:isBrigade() == false then return false end
	if unit:offMap() == true then return false end
	local cp = unit:homeCP()
	if cp == nil then return false end
	if cp.numLinks <= 0 or cp:isEnemyLinked() == false then return false end
	if cp.country == WWII_CHINA or cp.country == WWII_RUSSIA then return false end
	if unit:movementType() ~= BRANCH_LAND then return false end
	return true
end

----------------------------------------------------------------------
-- AO Placement Candidate Selection

function gameAOCandidateSelection (forSide)

	local cp, link
	local country = gameAOChooseAlliedCountry(forSide)

	local result = {
		candidates = {},
		maxScore = 0,
		numCandidates = 0,
		best = {}
	}
	local scores = {}
	local byScore = {}
	-- Look for someone to place an AO
	-- Look for frontline, friendly towns with HC Units,
	-- then search their neighbors for something to attack.
	for unit in allStratHCUnits() do
		if unit:side() == forSide and _isCandidateUnit(unit) then
			local cp = unit:homeCP()
			for link in allLinksFrom(cp) do
			-- check all of the links
				local rcp = link.endDepot.cp
				-- Skip towns that score nil point
				local score = _scoreAOPotential(unit, cp, link, rcp, country)
				if score > 0 and (scores[rcp] == nil or score > scores[rcp] or (unit:country() == country and result.candidates[rcp]:country() ~= country)) then 
					result.candidates[rcp] = unit
					scores[rcp] = score
					if not byScore[score] then byScore[score] = {} end
					table.insert(byScore[score], rcp)
					if score > result.maxScore then result.maxScore = score end
				end
			end
		end
	end

	if result.maxScore > 0 then
		result.best = byScore[result.maxScore]
		for i in pairs(result.best) do
			result.numCandidates = result.numCandidates + 1
		end
	end

	return result

end

----------------------------------------------------------------------
----------------------------------------------------------------------
-- Helper: Clear any pending placements for a particular side

function _cancelPlacements(forSide)
	gameAOPlacements[forSide] = nil
	gameAONotice[forSide] = gameAOWarnings 
end

----------------------------------------------------------------------
----------------------------------------------------------------------
-- Helper: schedule a placement for a particular side
-- If 'hadWarning' is false, this is a first warning

function _schedulePlacement(forSide, time, warnings)
	gameAOPlacements[forSide] = teulClockTime + time
	gameAONotice[forSide] = warnings
end

----------------------------------------------------------------------
-- Called when a side has spare attack objective capacity to provide
-- an opportunity for automatic placement of AOs by the system.

function gameSpareAOCapacity (forSide)

	-- You must set gameAOPlacementEnabled to true to enable this functionality
	if gameAOPlacementEnabled == false or host_config:isArenaOpen() == false then
		log('D', 'autoao', "not running")
		return _cancelPlacements(forSide)
	end

    -- Make sure certain values are initialized
	if not gameAOLastSpare[forSide] or not gameAOLastCap[forSide] then
		log('D', 'autoao', "initializing")
		return _cancelPlacements(forSide)
	end

	-- Get stats on whats currently going on
	local capacity = g_objectiveStats:attackCapacity()
	local placed = g_objectiveStats:attacks(forSide)
	local required = math.floor(capacity * gameAOMinimumRatio)
    if gameAOMinimumRatio == 0 and gameAOMinimum > 0 then required = gameAOMinimum end
	local spare = capacity - placed
	local shortFall = required - placed
	if shortFall <= 0 then shortFall = 0 end

	-- Generate a presentable description of availability status
	local stats = locstr('aoStats', capacity, _sideNameAdj[forSide], placed, spare, shortFall)

	-- 'bumped' tracks whether AO capacity has increased over last time or not
	local bumped = ( spare > gameAOLastSpare[forSide] or capacity > gameAOLastCap[forSide] )
	local isBumped ; if bumped then isBumped = "bumped" else isBumped = "no change" end
	log('D', 'autoao', "capacity = "..capacity..", placed = "..placed..", required = "..required..", spare = "..spare..", shortFall = "..shortFall..", "..isBumped)

	-- Update the record
	gameAOLastSpare[forSide] = spare
	gameAOLastCap[forSide] = capacity

	if placed >= required then
		-- They are in good standing - more AOs placed than required
		if bumped then	-- Tell them as availability changes anyway
			log('D', 'autoao', "placed >= required, bumped")
			chatSendHCMessage(forSide, LSTR_LABELLED_GENERIC, locstr('aoInfo'), stats)
		else
			log('D', 'autoao', "placed >= required, not bumped")
		end
		_cancelPlacements(forSide)
		return
	end

	-- If there is no scheduled placement, then schedule a first warning
	if gameAOPlacements[forSide] == nil or gameAOPlacements[forSide] < teulClockTime - 30 then
		chatSendHCMessage(forSide, LSTR_LABELLED_GENERIC, locstr('aoNotice'), stats)
		log('D', 'autoao', stats .. " considering auto-placement")
		local noticeDelay = gameAONoticeDelay
		_schedulePlacement(forSide, noticeDelay, gameAOWarnings)	-- false := first warning
		return
	end

	-- Don't pass this point until after the scheduled time
	if teulClockTime < gameAOPlacements[forSide] then
		return
	end

	-- Have they been given notice yet?
	if gameAONotice[forSide] > 0 then
		log('D', 'autoao', stats .. " working auto-placement schedule")
		-- Make a nice version of it
		local timeLeft = gameAOWarningTime * gameAONotice[forSide]
		local interval
		if timeLeft > 90 then
			interval = locstr('aoTimeMinutes', math.ceil(timeLeft/60))
		else
			interval = locstr('aoTimeSeconds', timeLeft)
		end
		chatSendHCMessage(forSide, LSTR_LABELLED_GENERIC, locstr('aoAttention'), stats)
		chatSendHCMessage(forSide, LSTR_LABELLED_GENERIC, locstr('aoAttention'), locstr('aoNewObjectiveSoon', interval))
		gameAOListCandidates(forSide)
		_schedulePlacement(forSide, gameAOWarningTime, gameAONotice[forSide] - 1)
		return
	end

	-- Get a list of candidates
	local options = gameAOCandidateSelection(forSide)
	if options.maxScore == 0 then
		log('M', 'autoao', "Nowhere for an AO")
		_schedulePlacement(forSide, gameAOWarningTime, 2)
		return
	end

	-- Ok - Get a random candidate from the best match
	local pick = math.floor(math.random()*options.numCandidates) + 1
	local candidate = options.best[pick]

	-- If we somehow fail to find a candidate...
	if candidate == nil then
		_schedulePlacement(forSide, gameAOWarningTime, 2)
		log('W', 'autoao', stats .. " Unable to find a candidate!")
		return
	end

	-- Write this info to the log
	log('I', 'autoao', stats .. " Placing automatic attack objective on "..candidate.name)

	-- Request AO Placement
	chatSendSideMessage(forSide, nullObjID, LSTR_SYSTEM, LSTR_GENERIC, locstr('aoPlacement', candidate.name))
	candidate:placeAttackObjective(options.candidates[candidate])

	_cancelPlacements(forSide)

	placed = placed + 1

	if placed < required then
		-- You have more? Oh no you don't.
		_schedulePlacement(forSide, gameAOWarningTime, gameAOReWarnings)
	end
end

----------------------------------------------------------------------
----------------------------------------------------------------------
-- Tell the HC of a side what candidates the engine is considering

function gameAOListCandidates (forSide)
	local options = gameAOCandidateSelection(forSide)
	local str = ""
	if options.numCandidates > 0 then
		for i = 1,options.numCandidates do
			local rcp = options.best[i]
			if i > 1 then str = str .. ", " end
			str = str .. rcp.name
		end
		chatSendHCMessage(forSide, LSTR_GENERIC, locstr('aoCandidates', str))
	else
		log('D', 'autoao', "No candidates for an AO")
	end
end

----------------------------------------------------------------------
----------------------------------------------------------------------
-- For luaClient: print the current AO candidates

function testAOPlacement(forSide)
	local options = gameAOCandidateSelection(forSide)
	if options.numCandidates == 0 then
		warn("No candidates")
		return
	end

	local rcp, i
	local size = 0
	local str = ""
	for i in options.best do
		rcp = options.best[i]
		if i > 1 then str = str .. ", " .. rcp.name else str = str .. rcp.name end
	end
	warn(options.numCandidates .. " candidates: " .. str)
end

