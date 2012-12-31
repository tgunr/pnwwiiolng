-- Monitor CP states and perform various external actions on them.

CPMonitor = { ['cps'] = nil, enabled = true }

cpMonitorInterval = 15

--------------------------------------------------------------------
-- Update the lua representation.

function CPMonitor:UpdateStatus(lcp)
	if self.enabled == false then return end

	self:Populate()

	if type(lcp) == 'userdata' then
		local luacp = self.cps[lcp]
		if luacp == nil then
			error("No CPmon entry for " .. toString(lcp))
			return
		end
		lcp = luacp
	end

	local surrenderCandidate = false
	if activeCountry(lcp.cp.country) == true
	and lcp.cp.side == lcp.side
	and lcp.cp.controllingSide == lcp.side
	and lcp.cp:isEnemyLinked() == true
	and lcp.cp:isSupplied() == false
	then
		surrenderCandidate = true
	end

	self:UpdateSurrender(lcp, surrenderCandidate)

	-- Make sure surrender flag is correct.
	if lcp.cp:isSurrendering() ~= lcp.surrenderFlagged then
		log('D', 'cpmon', 'Adjusting surrender flag of ' .. lcp.cp.name)
		lcp.cp:setSurrender(lcp.surrenderFlagged)
	end

	-- Finally, update our side tracking.
	lcp.side = lcp.cp.side
end

--------------------------------------------------------------------
-- Update the Surrender status of a given CP.

function CPMonitor:UpdateSurrender(lcp, isCandidate)
	if isCandidate == lcp.surrenderCandidate then
		self:ProcessSurrender(lcp, false)
	else
		-- Change of state.
		lcp.surrenderCandidate = isCandidate

		if isCandidate == true then
			log('I', 'cpmon', lcp.cp.name .. ' flagged for surrender consideration')
		else
			log('I', 'cpmon', lcp.cp.name .. ' left surrender state')
		end

		lcp.lastAnnounce = timeInSeconds({ days = 365 })
		lcp.surrenderFlagged = false
	end
end

--------------------------------------------------------------------
-- Process the Surrender state of a CP.

CPMonitor.announcementTimes =
{
	timeInSeconds({ minutes = 2 })
,	timeInSeconds({ minutes = 5 })
,	timeInSeconds({ minutes = 20 })
,	timeInSeconds({   hours = 1 })
,	timeInSeconds({   hours = 4 })
,	timeInSeconds({	  hours = 8 })
}

function CPMonitor:ProcessSurrender(lcp, actionable)
	if lcp.surrenderCandidate == false then return end

	log('D', 'cpmon', 'Checking surrender state of ' .. lcp.cp.name)

	if not lcp.surrenderFlagged then
		if not actionable then
			log('D', 'cpmon', 'Queueing candidate ' .. lcp.cp.name)
			table.insert(self.surrenders, lcp)
		else
			log('I', 'cpmon', 'Marking ' ..lcp.cp.name.. ' as surrender candidate')
			lcp.surrenderCandidateSince = currentTime()
			lcp.surrenderFlagged = true
			for _, thisTime in ipairs(self.announcementTimes)
			do
				if thisTime < pocketTimeToSurrender_s then
					lcp.lastAnnounce = thisTime + 1
				end
			end
		end
		return
	end

	local turnAt = lcp.surrenderCandidateSince + pocketTimeToSurrender_s
	if turnAt <= currentTime() then
		if actionable == true then
			self:ExecuteSurrender(lcp)
		else
			log('D', 'cpmon', 'Queueing surrender for ' .. lcp.cp.name)
			table.insert(self.surrenders, lcp)
		end

		return
	end

	-- Check if we need to send reminder messages.
	local remainingTime    = turnAt - currentTime()
	for _, thisTime in ipairs(self.announcementTimes)
	do
		if thisTime >= lcp.lastAnnounce then return end
		if remainingTime <= thisTime then
			remainingMinutes = math.floor((remainingTime + cpMonitorInterval) / 60)
			if remainingMinutes >= 120 then
				chatSendSideMessage(lcp.side, nullObjID, sideLString(lcp.cp.side), LSTR_CP_SURRENDER_IN_HOURS, lcp.cp.name, math.floor(remainingMinutes / 60))
			else
				chatSendSideMessage(lcp.side, nullObjID, sideLString(lcp.cp.side), LSTR_CP_SURRENDER_IN_MINS, lcp.cp.name, remainingMinutes)
			end
			lcp.lastAnnounce = thisTime
			return
		end
	end
end

--------------------------------------------------------------------
-- Execute the actual surrender.

function CPMonitor:ExecuteSurrender(lcp)
	local cp = lcp.cp

	local action, country
	if cp.side ~= cp.originalSide then
		action = "Resetting"
		country = cp.originalCountry
	else
		action = "Turning"
		if cp.side == ALLIED then
			country = getCountry("DE")
		else
			country = getCountry("FR")
		end
	end

	log('I', 'cpmon', 'Executing surrender: ' .. action .. ' ' .. cp.name .. ' to ' .. _countryName[country])

	chatSendSideMessage(cp.side, nullObjID, sideLString(cp.side), LSTR_CP_SURRENDERED_ENEMY, cp.name)

	cp:setSurrender(false)
	lcp.surrenderCandidate = false
	lcp.surrenderCandidateSince = nil
	lcp.lastAnnounce = nil
	lcp.surrenderFlagged = false

	local oldSide = lcp.side

	setCP(cp, country, false, true) 

	-- Make absolutely sure it has turned before we tell the enemy.
	if cp.side ~= oldSide then
		chatSendSideMessage(cp.side, nullObjID, sideLString(cp.side), LSTR_CP_SURRENDERED_US, cp.name)
	else
		log('A', 'cpmon', cp.name .. " FAILED TO SURRENDER, lcp.cp.side = " .. cp.side ", lcp.side = " .. oldSide)
	end
end

-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
-- CP Monitor routines.
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --

--------------------------------------------------------------------
-- Populate the Monitor.cps structure.

function CPMonitor:Populate()
	if self.cps ~= nil then return end

	log('D', 'cpmon', 'Populating CP list')

	self.cps = {}
	for cp in allCityCPs()
	do
		local lcp = {
				['cp'] = cp
			,	['side'] = cp.side
			,	['surrenderCandidate'] = false
			,	['surrenderCandidateSince'] = nil
			,	['lastAnnounce'] = nil
			,	['surrenderFlagged'] = false
		}
		self.cps[cp] = lcp
	end
end

--------------------------------------------------------------------
-- Run the Monitor.

function CPMonitor:Run()
	if self.enabled == false then return end

	-- Make sure the index is populated.
	self:Populate()

	log('D', 'cpmon', 'Processing CPs')

	-- Nobody has surrendered.
	self.surrenders = {}

	-- Process each CP.
	for _, lcp in pairs(self.cps)
	do
		self:UpdateStatus(lcp)
	end

	local maxTurn = pocketTurnTownsAtATime
	if not maxTurn then maxTurn = #self.cps end

	while #self.surrenders > 0 and maxTurn > 0
	do
		local n = math.floor(math.random() * #self.surrenders) + 1
		log('D', 'cpmon', 'Picked cp #'..n..'/'..#self.surrenders..' for surrender/candidacy')

		local lcp = self.surrenders[n]
		table.remove(self.surrenders, n)

		self:ProcessSurrender(lcp, true)

		maxTurn = maxTurn - 1
	end
end

--------------------------------------------------------------------
-- Temporarily disable CP monitor

function CPMonitor:Disable()
	self.enabled = false
end

--------------------------------------------------------------------
-- Re-enable CP monitor

function CPMonitor:Enable()
	if self.enabled == true then return end
	self.enabled = true
	self:Run()
end

--------------------------------------------------------------------
-- Register the periodic event.

gmMessage("* Restarting CP/Surrender Monitor ["..cpMonitorInterval.." second interval]")
if pocketTimeToSurrender_s then
	gmMessage("- CPs surrender after " .. toString(pocketTimeToSurrender_s) .. " seconds")
else
	gmMessage("X CP surrender-after time not set")
end
if pocketTurnTownsAtATime then
	gmMessage("- Maximum of " .. toString(pocketTurnTownsAtATime) .. " towns turned per-side per run")
else
	gmMessage("X pocketTurnTownsAtATime not set")
end

require 'event-queue'
LuaEvent.AddRecurring(cpMonitorInterval, CPMonitor.Run, CPMonitor, 'CPMonitor')
