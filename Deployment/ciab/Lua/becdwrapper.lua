-- BECD callbacks.

local fbAliases = {
 ['Whitstable-Margate FB'] = 'Whistable-Margate FB',
 ['Fressenneville-Abbeville FB'] = 'Fressenneville-Abbevile FB',
 ['Maninghem-Hesdin FB'] = 'Maningham-Hesdin FB',
 ['Chauny-La Fere FB'] = 'Chauny-LaFere FB',
 ['Aalter-Tielt FB'] = 'Aalter-Tielt Firebase',
 ['Sains-Richaumont-Marle FB'] = 'Sains.Richaumont-Marle FB',
 ['Marle-Sains-Richaumont FB'] = 'Marle-Sains.Richaumont FB',
 ['Maubeuge-Beaumont FB'] = 'Maubeuge - Beaumont FB',
 ['Ath-Geraardsbergen FB'] = 'Ath-Gerrardsbergen FB',
 ['Lokeren-St.Niklaas FB'] = 'Lokeren-St. Niklaas FB',
 ['St.Niklaas-Lokeren FB'] = 'St. Niklaas-Lokeren FB',
 ['St.Niklaas-Antwerp FB'] = 'St. Niklaas-Antwerp FB',
 ['St.Niklaas-Hulst FB'] = 'St. Niklaas-Hulst FB',
 ['Hulst-St.Niklaas FB'] = 'Hulst-St. Niklaas FB',
 ['Hellevoetsluis-Spijkenisse FB'] = 'Hellevoetsluis-Spijkenisse fB',
 ['Antwerp-St.Niklaas FB'] = 'Antwerp-St. Niklaas FB',
 ['Flavion-Philipville FB'] = 'Flavion-Phillipville FB',
 ['Breda-Moerdijk FB'] = 'Breda-Moedijk FB',
 ['Givet-Philipville FB'] = 'Givet-Phillipville FB',
 ['Tienen-St.Truiden FB'] = 'Tienen-St. Truiden FB',
 ['Oosterhout-Moerdijk FB'] = 'Oosterhout-Moedijk FB',
 ['Rochefort-St.Hubert FB'] = 'Rochefort-St. Hubert FB',
 ['Hannut-St.Truiden FB'] = 'Hannut-St. Truiden FB',
 ['Waremme-St.Truiden FB'] = 'Waremme-St. Truiden FB',
 ['St.Truiden-Hannut FB'] = 'St. Truiden-Hannut FB',
 ['St.Truiden-Tienen FB'] = 'St. Truiden-Tienen FB',
 ['St.Truiden-Waremme FB'] = 'St. Truiden-Waremme FB',
 ['St.Truiden-Hasselt FB'] = 'St. Truiden-Hasselt FB',
 ['St.Hubert-Champlon FB'] = 'St. Hubert-Champlon FB',
 ['St.Hubert-Rochefort FB'] = 'St. Hubert-Rochefort FB',
 ['Gouvy-St.Vith FB'] = 'Gouvy-St. Vith FB',
 ['St.Vith-Gouvy FB'] = 'St. Vith-Gouvy FB',
 ['St.Vith-Malmedy FB'] = 'St. Vith-Malmedy FB',
 ['St.Vith-Prum FB'] = 'St. Vith-Prum FB',
 ['Malmedy-St.Vith FB'] = 'Malmedy-St. Vith FB',
 ['Hallschlag-St.Vith FB'] = 'Hallschlag-St. Vith FB',
 ['Hurtgenwald-Duren FB'] = 'Huurtgenwald-Duren FB',
 ['Erkelenz-Monchen Gladbach FB'] = 'Erkelenz-Monchen.Gladbach FB',
 ['Neiderkruchten-Monchen Gladbach FB'] = 'Neiderkruchten-Monchen.Gladbach FB',
 ['Nettetal-Monchen Gladbach FB'] = 'Nettetal-Monchen.Gladbach FB',
 ['Bouzonville-Creutzwald-la-Croix FB'] = 'Bouzonville-Creutzwald FB',
 ['Saarburg-Trier FB'] = 'Saaburg-Trier FB',
 ['Duren-Aachen FB'] = 'Duren-Aarchen FB',
 ['Bergheim-Kerpen FB'] = 'Bergeim-Kerpen FB',
 ['Grevenbroich-Monchen Gladbach FB'] = 'Grevenbroich-Monchen.Gladbach FB',
 ['Kaarst-Monchen Gladbach FB'] = 'Kaarst-Monchen.Gladbach FB',
 ['Krefeld-Monchen Gladbach FB'] = 'Krefeld-Monchen.Gladbach FB',
 ['Kaarst-Dusseldorf FB'] = 'Kaarst-Duusseldorf FB',
 ['Creutzwald-la-Croix-Bouzonville FB'] = 'Creutzwald-Bouzonville FB',
 ['Creutzwald-la-Croix-Saarlouis FB'] = 'Creutzwald-Saarlouis FB',
 ['Saarlouis-Creutzwald-la-Croix FB'] = 'Saarlouis-Creutzwald FB',
 ['Niederkassel-Meckenheim FB'] = 'Neiderkassel-Meckenheim FB',
 ['Essen-Remscheid FB'] = 'Essen-Remsheid FB',
 ['Lohmar-Niederkassel FB'] = 'Lohmar-Nierderkassel FB',
 ['Remscheid-Essen FB'] = 'Remsheid-Essen FB',
 ['Fort Rommel-Fort De Gaulle FB'] = 'Ft Rommel- Ft De Gaulle FB',
 ['Fort De Gaulle-Fort Rommel FB'] = 'Ft De Gaulle-Ft Rommel FB',
 ['Fort Montgomery-Fort Rommel FB'] = 'Ft Montgomery-Ft Rommel FB',
 ['Fort Rommel-Fort Montgomery FB'] = 'Ft Rommel-Ft Montgomery FB',
}

becd = { initialized = false, errors = false, file = nil, current_title = nil }

----------------------------------------------------------------------
-- Report an error and set errors to true.

function becd.error( text )
	becd.errors = true
	return error(becd.file .. ": " .. text)
end

----------------------------------------------------------------------
-- Invoked when becd begins processing a file.

function becd.start_file( name, title )
	becd.initialized = false
	becd.errors = false
	becd.file = name
	becd.title = title

	if not title or title == '' or title == ' ' then
		becd.errors = true
		return becd.error( "Campaign doesn't have a title, I'm not loading that." )
	end

    print( string.format( "becd: starting file '%s', title '%s'", becd.file, becd.title ) )

	systemMessage( string.format( "Loading Campaign '%s'", becd.title ) )

	-- Make sure that capture etc are disabled for now.
	if host_config.arena.lock < 7 then
		host_config.arena.lock = 7
	end

	CPMonitor:Disable()

	-- Move all HCUnits off-map.
	for unit in allStratHCUnits()
	do
		deploy( unit, "offmap", 0, false )
		resetUnitSupply( unit )
	end

	-- Reset all CPs to their original owners.
	for cp in allCityCPs()
	do
		setCP( cp, cp.originalCountry )
	end
end

----------------------------------------------------------------------
-- Invoked at the start of the CP updates.

function becd.start_cps()
	if becd.errors then return end
end

----------------------------------------------------------------------
-- Assign ownership to a given CP.

function becd.set_cp( cp_name, cp_id, country_name, country_id )
	if becd.errors then return end

    local cp = getCP( cp_id )
	if not cp or cp.name ~= cp_name then cp = getCP( cp_name ) end
	if not cp then becd.error( string.format( "Unable to find CP %s (id %s)", cp_name, cp_id ) ) end

	local ctry = getCountry( country_id )
	if not ctry or ctry == 0 then ctry = getCountry( country_name ) end
	if not ctry then
	  return becd.error( string.format( "Unable to find country %s (id %s) for %s.", country_name, country_id, cp.name ) )
	end

	setCP( cp, ctry, false, true )
end

----------------------------------------------------------------------
-- Invoked at the end of the CP updates.

function becd.end_cps()
	if becd.errors then return end

    print( "becd: finished cps" )
	strtBroadcastCPs()
end

----------------------------------------------------------------------
-- Begin TOE definitions

function becd.start_toes()
	if becd.errors then return end
end

----------------------------------------------------------------------
-- End of the TOE definitions

function becd.end_toes()
	if becd.errors then return end
end

----------------------------------------------------------------------
-- Handle TOE -> HCUnit assignment

function becd.start_hcunit_toes()
	if becd.errors then return end
end

----------------------------------------------------------------------
-- End of TOE assignments

function becd.end_hcunit_toes()
	if becd.errors then return end
end

----------------------------------------------------------------------
-- Invoked at the start of the HC Unit CP updates.

function becd.start_hcunits(...)
	return becd.start_hcunit_cps(...)
end

function becd.start_hcunit_cps()
	if becd.errors then return end

    print( "becd: starting hcunit_cps" )

    for unit in allStratHCUnits() do
      undeploy(unit)
    end
end

----------------------------------------------------------------------
-- Deploy a given HCUnit to a given CP.

function becd.set_hcunit(...)
	return becd.set_hcunit_cp(...)
end

function becd.set_hcunit_cp( hcunit_short, hcunit_id, cp_name, cp_id )
	if becd.errors then return end

	local unit = getHCUnit( hcunit_id )
	if not unit or unit:callsign() ~= hcunit_short then unit = getHCUnit( hcunit_short ) end
	if not unit then
		return becd.error( string.format( "Unable to find HCUnit %s (id %s).", hcunit_short, hcunit_id ) )
	end

	local cp = getCP( cp_id )
	if not cp or cp.name ~= cp_name then cp = getCP( cp_name ) end
	if not cp then
		return becd.error( string.format( "Unable to find CP %s (id %s) for %s.", cp_name, cp_id, unit:callsign() ) )
	end

	deploy( unit, cp, 0, false )

	resetUnitSupply( unit )
end

----------------------------------------------------------------------
-- Invoked when all of the unit updates have been completed.

function becd.end_hcunits(...)
	return becd.end_hcunit_cps(...)
end

function becd.end_hcunit_cps()
	if becd.errors then return end
end

----------------------------------------------------------------------
-- Invoked before we start changing FB ownerships.

function becd.start_fbs()
	if becd.errors then return end
end

----------------------------------------------------------------------
-- Open a given firebase.

function becd.set_fb( fb_name, fb_id, side_name, side_id )
	if becd.errors then return end

	local fb = getFacility( fb_id )
	if not fb or fb.name ~= fb_name then fb = getFacility( fb_name ) end
    if not fb or fb.name ~= fb_name then
      local alias = fbAliases[fb_name]
      if alias then fb = getFacility( alias ) end
    end
	if not fb then
		return becd.error( string.format( "Unable to find FB %s (id %s).", fb_name, fb_id ) )
	end

	-- We don't actually have the concept of assigning a side
	-- to a firebase, they're either open or closed, and they
	-- take their side from the CPs they are between.
	-- When becd tries to assert side on an FB, it's assumed
	-- that it's actually trying to open that given FB.
	if fb:isOpen() == false then
		-- You open a given firebase by blowing up it's mate.
		if fb.mate then
			if not blowFirebase( fb.mate ) then
				-- blowFirebase returns the FB on success.
				return becd.error( string.format( "Unable to open FB %s (id %s).", fb_name, fb_id ) )
			end
		end
	end

	if fb.side ~= side_id then
		warn( string.format( "Firebase %s has wrong side (%s vs %s).", fb.name, fb.side, side_id ) )
	end
end

----------------------------------------------------------------------
-- Invoked after the last FB.

function becd.end_fbs()
	if becd.errors then return end
end

----------------------------------------------------------------------
--

function becd.start_params()
	if becd.errors then return end
end

----------------------------------------------------------------------
-- Assign a value to a given parameter: not supported yet.

function becd.set_param( key, value )
	if becd.errors then return end
	---TODO: Implement me.
end

----------------------------------------------------------------------
-- Invoked when the parameter list has been exhausted.

function becd.end_params()
	if becd.errors then return end
end

----------------------------------------------------------------------
-- Invoked at the end of a script.

function becd.end_file()
    print( "becd: finished file" )

	if becd.errors then
		systemMessage( string.format( "Campaign load failed." ) )
		gmMessage( string.format( "ERRORS WHILE LOADING CAMPAIGN FILE. See logs for details." ) )
	else
		systemMessage( string.format( "'%s' campaign loaded.", becd.title ) )
	end

	CPMonitor:Enable()
end

