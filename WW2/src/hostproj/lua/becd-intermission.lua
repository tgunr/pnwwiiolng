-- <attribution>
--! @author Oliver 'kfs1' Smith
--! @brief  Loads a BECD campaign configuration.
--! @see Battleground Europe Campaign Designer
--! Copyright (C) PlayNet Inc, 2010. All rights reserved.
--! Permission to modify/redistribute this code is
--! granted so long as this attribution is included in
--! this exact wording.
-- </attribution>


--! becd class / object / table / goodness.
becd = {}

--! Assign ownerships from the CP List
function becd:set_cp_ownerships()

	for cp, country in pairs(self.cps)		-- or self['cps'] if you will
	do
		if cp and country then				-- ignore any empty entries caused by trailing commas
			setCP(cp, country)				-- 'setCP' can handle a CPStruct*, a CP Name, a CP OID or FullOBJID
			rebuildCP(cp)					-- Repair all damageable buildings.
		end
	end

end

--! Open firebases that are listed, default FB state is closed.
function becd:open_firebases()

	for _, fb in pairs(self.fbs)
	do
		if fb then							-- ignore any empty entries caused by trailing commas
			openFirebase(fb)
		end
	end

end

--! Deploy units onto the map.
function becd:deploy_hc_units()

	--Default all units to offmap (training).
	for unit in allHCUnits()
	do
		deploy(unit, "offmap")
	end

	-- Now deploy to their real destinations.
	for unit, destination in pairs(self.hcunits)
	do
		if unit and destination then		-- ignore any empty entries caused by trailing commas
			deploy(unit, destination)		-- Move to it's desination
			resetSupply(unit)				-- Restore full supply.
		end
	end
end

--! Assign fallbacks to HCUnits
function becd:set_hc_unit_fallbacks()

	for unit, fallback in pairs(self.fallbacks)
	do
		if unit and fallback then			-- ignore any empty entries caused by trailing commas
			setHCUnitFallbackDestination(unit, fallback)
		end
	end
end

--! Apply server properties.
function becd:apply_server_properties()

	for category, entries in pairs(self.properties)
	do
		if entries then
			for key, value in pairs(entries)
			do
				if key then			-- ignore any empty entries caused by trailing commas
					host_config.category.key = value
				end
			end
		end
	end
end

--! Run before the Load starts.

function becd:start_script()

	-- Announce what we're doing to all players
	-- if there is anything to announce.
	if self.title then
		systemMessage("Loading " .. self.title)
	end

	if self.description then
		systemMessage(self.description)
	end

end

--! Load the script

function becd:load_script()

	-- Set town ownerships.
	self:set_cp_ownerships()

	-- Deploy brigades
	self:deploy_hc_units()

	-- Set open firebases
	self:open_firebases()

	-- Set HC Unit fallbacks
	self:set_hc_unit_fallbacks()

	-- Apply server properties.
	self:apply_server_properties()
end

--! Run after the script is loaded and any other
--! work/cleanup has been done.

function becd:end_script()

	adminMessage("Campaign loaded.")

end

--! Run the entire script

function becd:run()

	self:start_script()

	self:load_script()

	self:end_script()

end

