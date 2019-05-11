----------------------------------------------------------------------
-- Capture Contribution processing:
--
-- Cell hosts periodically submit a "Strat.CaptureContribution" to the
-- strat host. These indicate an individual player is currently standing
-- in a building that has capture enabled. The strat host places these
-- in a queue for later (batched) processing by the Lua engine.
--
-- Periodically the strat host calls lua->ProcessCaptureContributions()
-- which is implemented here.
--
-- HOST FUNCTIONS:
--
-- Strat.CaptureContribution functions:
--  fac = Strat.CaptureContribution:GetNextActiveFacility(fac)
--		 Allows you to iterate over all facilities that are
--		 currently capturable. This provides the opportunity to
--		 perform actions on facilities with no players in them.
--
--	[true | false] Strat.CaptureContribution:IsQueueEmpty()
--		 Returns whether or not there are any capture contributions
--		 remaining in the queue.
--
--	contrib = Strat.CaptureContribution:GetQueueFront()
--		 Returns the contribution at the front of the queue.
--
--	Strat.CaptureContribution:QueuePop()
--		 Removes the contribution from the front of the queue.
--
--	status = contrib:status()	Returns the status of the contrib
--								(see below; you may get two contribs for one player)
--  fac = contrib:facility()	Returns the facility being contributed to
--	handle = contrib:handle()	Returns the ChatHandle of the contributor
--	id = contrib:playerID()		Returns the playerID of the contributor
--	id = contrib:sortieID()		Returns the player's sortieID
--	id = contrib:missionID()	Returns the player's missionID
--	unit = contrib:unit()		Returns the player's HCUnit.
--
--  Statuses:
--		Strat.CaptureContributionEvent.Active := The player started a contribution,
--		Strat.CaptureContributionEvent.Interrupted := The player's contribution was interrupted.
--		Strat.CaptureContributionEvent.Left := The player left the building
--		Strat.CaptureContributionEvent.Killed := The player was killed
--
-- CURRENT IMPLEMENTATION:
--
--	A Lua table, P.activeData, is created for tracking all active
--  facilities: when we last processed them, and whether or not they had
--	any 'cap'pers or 'def'enders since the last cycle.
--
--	The contribution queue is scanned and totaled into the facilityData
--	table.
--
--  Finally we iterate over the P.activeData table. Facilities that
--  have come due for processing are then adjusted based on:
--   if the facility has had any cappers, we increment the progress.
--   otherwise if the facility has had any defenders, we decrement progress.
--   otherwise we slowly bleed off capture points.

require 'event-queue'

captureLogLevel = 1

-- Library of CaptureContribution processing and related routines

-- Private data
local P = {
	activeData = {}
,	statuses = {
		[Strat.CaptureContributionEvent.Active] = "active"
	,	[Strat.CaptureContributionEvent.Interrupted] = "interrupted"
	,	[Strat.CaptureContributionEvent.Left] = "left"
	,	[Strat.CaptureContributionEvent.Killed] = "killed"
	}
	-- How long would 100% auto-uncap take in seconds?
,	autoUncapTime = timeInSeconds({ minutes = 10 })
	-- Maximum number of people we bother tabulating
,	maxCapContributors = 12
	-- After max contributors, points is based on a target time
,	minCaptureTime = 60
	-- Table of target times for a given number of players
,	captureTimeTable =
	{
		225		-- 1 guy
	,	205		-- 2
	,	180		-- 3
	,	160		-- 4
	,	135		-- 5
	,	120		-- 6
	,	105		-- 7
	,	90		-- 8
	,	 90		-- 9
	,	 75		-- 10
	,	 60		-- 11
	,	 60		-- 12
	}
}

function output(level, text)
  if level <= captureLogLevel then print(text) end
end

----------------------------------------------------------------------
----------------------------------------------------------------------
-- Declaration of the Lua class that describes our tracking of an
-- active facility.
----------------------------------------------------------------------
----------------------------------------------------------------------

FacilityData = {
	new = function (self, interval, uncapDelay) return {
		-- When we should next process
		nextTick = teulClockTime + interval,

		-- Record of the contributers
		history = { ['cap'] = {}, ['def'] = {} },

		-- When we can next announce a cap action
		nextAnnounce = 0,
	}
	end
}

----------------------------------------------------------------------
----------------------------------------------------------------------
-- Reset the counters for a given facility data entry
----------------------------------------------------------------------
----------------------------------------------------------------------

function P.resetCounters(entry)
	-- Number of cappers/defenders contributing since last processing
	entry.counted = { ['cap'] = 0, ['def'] = 0 }
	-- Number of cappers/defenders killed since last processing
	entry.killed = { ['cap'] = 0, ['def'] = 0 }
	-- List of players who have sent contribs
	entry.players = {}
end

----------------------------------------------------------------------
----------------------------------------------------------------------
-- Analyze a contribution and if it is applicable, update the stats
-- for the facility it applies to.
----------------------------------------------------------------------
----------------------------------------------------------------------

function P.contribute(contrib)

	-- Start by checking that the facility is currently capturable.

	if contrib:facility():isCaptureEnabled() == false then
		return false
	end

	-- Look the facility up in the facilityData table; if it's not
	-- present, then the facility is not being tracked and we stop
	-- processing.

	local data = P.activeData[contrib:facility()]
	if not data then
		return false
	end

	-- Compare the Player's side (derived from their brigade)
	-- with that of the facility. If they are different (~=) then
	-- the player is capturing, otherwise the player is defending.

	local contribType
	if contrib:unit():side() ~= contrib:facility().side then
		contribType = "cap"
	else
		contribType = "def"
	end

	-- Get a string version of the status.
	local status = contrib:status()
	local statusName = P.statuses[status]

	-- Print a log message
	output(2,
			"c:"..getGameName(contrib:handle())..
				" fac:"..contrib:facility().name..
				" contributing "..statusName.." "..contribType)

	-- Add or update the entry for this user
	local entry = data.history[contribType][contrib:playerID()]
	if status == Strat.CaptureContributionEvent.Active then
		if entry ~= nil then
			if entry.updated <= teulClockTime - host_config.capture.tickInterval_s then
				entry.contrib = contrib
				entry.contribs = entry.contribs + 1
				entry.updated = teulClockTime
			end
		else
			data.history[contribType][contrib:playerID()] = {
										['contrib'] = Strat.CaptureContribution()
										, ['updated'] = teulClockTime
										, ['contribs'] = 0
										, ['started'] = teulClockTime
									}
			data.history[contribType][contrib:playerID()].contrib = contrib
		end
		if data.players[contrib:playerID()] ~= true then
			data.counted[contribType] = data.counted[contribType] + 1
			data.players[contrib:playerID()] = true
		end
	else
		-- Look to see if there is an entry to update
		if entry ~= nil then
			-- Update their contrib
			entry.contrib = contrib
			entry.updated = teulClockTime
		end

		-- Count the number of players killed.
		if status == Strat.CaptureContributionEvent.Killed then
			data.killed[contribType] = data.killed[contribType] + 1
		end

		-- Decrease the count for the side if we had bumped it
		if data.players[contrib:playerID()] then
			data.counted[contribType] = data.counted[contribType] - 1
		end
	end

	-- Returning 'true' because work was done.
	return true

end

----------------------------------------------------------------------
----------------------------------------------------------------------
-- Find the primary contributor (currently present) in this
-- history index
----------------------------------------------------------------------
----------------------------------------------------------------------

function P.findPrimaryContributor(history)

	-- Ignore entries that haven't been updated since the last tick.
	local updateCutoff = teulClockTime - host_config.capture.tickInterval_s

	local contributor = { ['updated'] = 0, ['contribs'] = 0, ['started'] = teulClockTime }
	local lastHope = contributor

	for i, entry in pairs(history)
	do
		if lastHope.contrib == nil or
				(entry.updated == lastHope.updated and entry.contribs > lastHope.contribs) or
				(entry.updated > lastHope.updated)
		then
			-- If we don't have a last hope yet, this guy will do.
			lastHope = entry
		end
		-- Now check if they're still here and so viable for actually winning.
		if entry.updated >= updateCutoff and entry.contribs >= contributor.contribs then
			if entry.contribs > contributor.contribs then
				-- I have more contributions than you, I win
				contributor = entry
			elseif entry.started < contributor.started then
				-- I have the same contributions, but I started before you
				contributor = entry
			end
		end
	end

	-- If we didn't find a winner, try the last resort
	if contributor.contrib == nil then
		return lastHope
	else
		return contributor
	end

end

----------------------------------------------------------------------
----------------------------------------------------------------------
-- Audit trail the list of players who helped this contribution
----------------------------------------------------------------------
----------------------------------------------------------------------

auditContributorList = true

function P.getContributorList(history)
	if auditContributorList == false then return "" end

	-- Ignore entries that haven't been updated since the last tick.
	local updateCutoff = teulClockTime - host_config.capture.tickInterval_s
	local names = ""
	for i, entry in pairs(history)
	do
		if entry.updated >= updateCutoff then
			names = names .. " " .. getGameName(entry.contrib:handle()) .. ":" .. entry.contribs
		end
	end

	return names
end

----------------------------------------------------------------------
----------------------------------------------------------------------
-- Calculate scaling factor based on population
----------------------------------------------------------------------
----------------------------------------------------------------------

function P.ProgressScaleForSide(side)
	local axisPop = chatGetPopulation(AXIS)
	local alliedPop = chatGetPopulation(ALLIED)

	local over = math.max(axisPop, alliedPop)
	local under = math.min(axisPop, alliedPop)
	if ( under == 0 ) then
		return 1.0
	end
	local overpop = over / under;


	if ( ((axisPop > alliedPop) and (side == AXIS) ) or ((alliedPop > axisPop) and (side == ALLIED)) ) then
		return math.max(1.0 - (overpop - 1.0), 0.5)
	else
		return math.min(overpop, 1.5)
	end
end

----------------------------------------------------------------------
----------------------------------------------------------------------
-- Calculate the cap/uncap points for a given number of players.
----------------------------------------------------------------------
----------------------------------------------------------------------

function P.ProgressRateForPlayers(players, side)
	local totalCapTicks = 100 * host_config.capture.tickInterval_s
	local progress = 0

	-- No players, return the system uncap rate.
	if players == 0 then
		return totalCapTicks / P.autoUncapTime
	end

	-- More than the max?
	if players > P.maxCapContributors then
		progress = totalCapTicks / P.minCaptureTime
	else
		progress = totalCapTicks / P.captureTimeTable[players]
	end

	return progress * P.ProgressScaleForSide(side)
end

----------------------------------------------------------------------
----------------------------------------------------------------------
-- Apply the accumulated facilityData to a specific facility.
----------------------------------------------------------------------
----------------------------------------------------------------------

function P.processFacility(facility, facilityData)
	-- If the facility is no-longer capturable, then remove it from
	-- the queue.

	if facility:isCaptureEnabled() == false then
		P.activeData[facility] = nil
		return false
	end

	---TODO: If I just add one point, it then takes 100 * tickInterval_s to capture,
	---TODO: which made capture take 500s.
	---TODO: Should be exposed through host_config.capture instead?
	---TODO: Side balance adjustment?

	-- Query the current progress for this facility.

	-- Number of captures and defenses that weren't subsequently
	-- withdrawn because of the player dying/leaving/exiting.
	local cappers = facilityData.counted.cap
	local defenders = facilityData.counted.def
	-- Current progress as a %age of capture. 0 = no capture, 100 = change of owner
	local progress = facility:getCaptureProgress()

	local newProgress

	if cappers > 0 then
		-- The facility had people submit capture contributions;
		---CURRENT: Does not take into account presence of defenders
		---CURRENT: or factor side balance into the equation.

		-- Search the history to find a currently present player we
		-- can credit this capture progression to. The database only
		-- supports one player being named as "the capper".

		local capper = P.findPrimaryContributor(facilityData.history.cap)

		-- Check we could find someone. Future-proofs against cases
		-- we might not expect where nobody appears to be capping.

		if capper.contrib ~= nil then
			-- Increment progress.
			local side = capper.contrib:unit():side();
			newProgress = progress + P.ProgressRateForPlayers(cappers, side)

			-- Get printable name of the capper

			local playerList = P.getContributorList(facilityData.history.cap)

			-- Audit trail.
			output(1,
					"f:"..facility.name..
					" capture contribution credited to "..getGameName(capper.contrib:handle())..
					" => " .. newProgress .. "% players:"..cappers..playerList)

			-- Update the capture progress and credit the last capture contributee we saw.
			facility:setCaptureProgress(newProgress, capper.contrib)
		end

	elseif progress > 0 and progress < 100 then
		-- There were no cappers, and the facility has non-zero/non-complete progress.

		local capper = P.findPrimaryContributor(facilityData.history.def)
		local side = facility.side;

		if defenders > 0 and capper.contrib ~= nil then
			-- There were some defenders present

			newProgress = progress - P.ProgressRateForPlayers(defenders, side)

			local playerList = P.getContributorList(facilityData.history.def)

			output(1,
					"f:"..facility.name..
					" defense contribution credited "..getGameName(capper.contrib:handle())..
					" => " .. newProgress .. "% players:"..defenders..playerList)

			facility:setCaptureProgress(newProgress, capper.contrib)
		elseif host_config:isArenaLocked(AL_CAPTURE) == false then
			-- The building is unoccupied. Slowly "bleed off" the capture progress.

			newProgress = progress - P.ProgressRateForPlayers(0, side)
			if newProgress < 0 then newProgress = 0 end

			output(2, "f:"..facility.name.." no contributions "..newProgress.. "%")

			-- Create a contribution that represents SYSTEM.

			local contrib = Strat.CaptureContribution(
								Strat.CaptureContributionEvent.Active
								, facility
								, getHandle("SYSTEM")
								, 0, 0, 0
								, HCUnit:GetCommandUnit(facility.country, BRANCH_LAND))

			-- Reduce the progress.
			facility:setCaptureProgress(newProgress, contrib)
		end
	end

	facilityData.players = {}
	return true
end

----------------------------------------------------------------------
----------------------------------------------------------------------
-- ENTRY POINT:
-- This is the function called periodically by the Strat Host.
----------------------------------------------------------------------
----------------------------------------------------------------------

function ProcessCaptureContributions()

	-- We build a table of facilities that received contributions;
	-- In this first iteration we count the number of capture and
	-- defense contribution but we only look at whether these
	-- numbers are 0 or not.
	--- CURRENT: In this iteration, defenders do not prevent captures.

    local contributions = {}
	local numContributions = 0

	-- Ensure there are entries for all active facilities.

	local facility = nil
	for facility in allActiveFacilities() do

		if not P.activeData[facility] then

			-- There wasn't an entry for this one; create one
			-- with a processing delay of 1 second,
			-- with no uncapDelay.

			P.activeData[facility] = FacilityData:new(1, 0)

			P.resetCounters(P.activeData[facility])

		end

	end

	-- Now, consume all the contributions waiting in the queue
	-- and summarize them into the facilityData table for later
	-- processing.

	while Strat.CaptureContribution:IsQueueEmpty() == false do

		-- Get the first contribution on the queue.
		local contrib = Strat.CaptureContribution:GetQueueFront()

		-- Process it.
		if P.contribute(contrib) then
			numContributions = numContributions + 1
		end

		-- Remove it from the queue.
		Strat.CaptureContribution:QueuePop()

	end

	local facilityData = nil
	local numFacilities = 0
	local numFacilitiesProcessed = 0

	-- The facilityData data should now be populated with
	-- contribution statistics. Scan over them all and process
	-- each facility who's next "tick" has come due.

	for facility, facilityData in pairs(P.activeData)
	do
		numFacilities = numFacilities + 1

		-- Only process each facility every so often.
		if facilityData.nextTick <= teulClockTime then

			-- Process the data.
			if P.processFacility(facility, facilityData) == true then
				-- Returns true when the facility wants another tick scheduling.
				P.activeData[facility].nextTick = P.activeData[facility].nextTick + host_config.capture.tickInterval_s
				P.resetCounters(P.activeData[facility])
			end

			numFacilitiesProcessed = numFacilitiesProcessed + 1
		end
	end

	-- Print a summary report when anything was done.
    if numFacilities > 0 and numFacilitiesProcessed > 0 then
		output(2,
				"Processed " ..
				numFacilitiesProcessed .. "/" .. numFacilities  ..
				" facilities with " .. numContributions .. " contributions")
	end
end

-- Add ProcessCaptureContributions to the once-a-second event queue.
if not DebugCaptureContributions then
	LuaEvent.AddRecurring(1, ProcessCaptureContributions, nil, 'ProcessCaptureContributions')
end

----------------------------------------------------------------------
----------------------------------------------------------------------
-- Advanced functions/helpers. Do not modify unless you're sure!
----------------------------------------------------------------------
----------------------------------------------------------------------


----------------------------------------------------------------------
-- Iterator for active facilities

function allActiveFacilities ()
	local nextFac = Strat.CaptureContribution:GetNextActiveFacility(nil)
	return function ()
		local fac = nextFac
		if fac ~= nil then nextFac = Strat.CaptureContribution:GetNextActiveFacility(fac) end
		return fac
	end
end

