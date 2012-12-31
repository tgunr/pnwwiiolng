----------------------------------------------------------------------
-- Report on the threat level of defense-ao towns.

require 'localization'
require 'event-queue'

local threatReports = {
  [ALLIED] = { ['lastTime'] = 0, ['lastReport'] = "" }
, [AXIS]   = { ['lastTime'] = 0, ['lastReport'] = "" }
, ['minReportInterval'] = timeInSeconds({ seconds = 90 })
, ['minRepeatInterval'] = timeInSeconds({ minutes = 5 })
, ['debug'] = nil
}

function threatLevel(forSide, force)

  if threatReports.debug then
    chatSendLocalizedMessage(3204, LSTR_NOTE, LSTR_GENERIC, "running threatLevel for " .. getSideName(forSide))
  end

  -- Test if a given CP is a candidate for reporting.
  function isCandidate(cp, side)
    if cp:isDefendObjective(side) == false then return false end
    if cp:hasHeavyInfantryEWS() then return true end
    if cp:hasInfantryEWS() then
      if cp:hasHeavyArmorEWS() then return true end
      if cp:isContested() then return true end
    end
    return false
  end

  local side = getSide(forSide)

  local minReportAge = currentTime() - threatReports.minReportInterval
  if not force and threatReports[side].lastTime > minReportAge then
    if threatReports.debug then print("ignoring, last report too recent") end
    return
  end

  local report = ""
  for cp in allCityCPs()
  do
    if isCandidate(cp, side) then
      local description = ""
      if cp:hasHeavyInfantryEWS() then
        description = description .. locstr('threatHeavyInfantry')
      else
        description = description .. locstr('threatLightInfantry')
      end
	  if cp:hasArmorEWS() then
		description = description .. locstr('threatSeparator')
		if cp:hasHeavyArmorEWS() then
			description = description .. locstr('threatHeavyArmor')
		else
			description = description .. locstr('threatLightArmor')
		end
      end

      if threatReports.debug then print(cp.name .. ": " .. description) end

      if report ~= "" then report = report .. ", " end
      local name = cp.name
      if cp:isContested() then name = "*" .. cp.name .. "*" end
      report = report .. name .. " " .. description
      
    end
  end

  if not report or report == "" then
    if threatReports.debug then print("Nothing to report.") end
    return
  end

  if not force and report == threatReports[side].lastReport then
    local repeatCutoff = currentTime() - threatReports.minRepeatInterval
    if threatReports[side].lastTime > repeatCutoff then
      if threatReports.debug then print("Report hasn't changed") end
      return
    end
  end

  threatReports[side].lastTime = currentTime()
  threatReports[side].lastReport = report

  if not force then
    chatSendHCMessage(side, LSTR_THREAT_REPORT, report)
  else
    chatSendLocalizedMessage(3204, LSTR_NOTE, LSTR_THREAT_REPORT, report)
  end
end

function threatLevels()
 threatLevel(ALLIED, nil)
 threatLevel(AXIS, nil)
end

-- Run the events every 15 seconds.
LuaEvent.AddRecurring(15, threatLevels, nil, 'threatLevels')

