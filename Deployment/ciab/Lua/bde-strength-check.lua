require 'localization'
require 'event-queue'

bdeStrDebug = false

if not brigadeStrengths then
 brigadeStrengths = {}
end
if not brigadeTimers then
 brigadeTimers = {}
end

function bdestr_reportOn(unit, force, forceType)
 local lastStr = brigadeStrengths[unit:unitID()][force]
 local str = unit:getSupplyPctg(force)
 local newStr = 'x' 
 local msg = ""
 if str <= 0 and lastStr > 0 then
  msg = "OUT OF" ; newStr = str
 elseif str <= 10 and lastStr > 10 then
  msg = "Critically low ("..str.."% was "..lastStr.."%) on" ; newStr = str
 elseif str <= 25 and lastStr > 25 then
  msg = "Very low (<25%) on" ; newStr = str
 elseif str <= 50 and lastStr > 50 and (lastStr - str) > 10 then
  msg = "Down to " .. str .. "%" ; newStr = str
 elseif str >= 98 and lastStr <= 50 then
  msg = "Full" ; newStr = str
 elseif str >= 75 and lastStr <= 25 then
  msg = "Back over 75%" ; newStr = str
 end
 if msg ~= "" then
  local bde = "INTEL: " .. unit:name() .. " ("..unit:homeCP().name..")"
  if host_config.arena.intermission > 0 then
   sideMessage(unit:side(), bde, msg .. " " .. forceType)
  else
   chatSendHCMessage(unit:side(), LSTR_LABELLED_GENERIC, bde, msg .. " " .. forceType)
  end
  brigadeStrengths[unit:unitID()][force] = newStr
  return true
 end
 return false
end

function bde_strength_check()
 function debugReport(msg) if bdeStrDebug == true then print(msg) end end
 if not host_config.supply.enabled then debugReport("supply disabled") ; return 0 end
 local moveIn4h = { }
 local moveIn2h = { }
 local moveIn1h = { }
 local moveIn5 = { }
 local moveNow = { }
 moveIn5[ALLIED] = "" ; moveIn5[AXIS] = "" ; moveNow[ALLIED] = "" ; moveNow[AXIS] = ""
 moveIn1h[ALLIED] = "" ; moveIn1h[AXIS] = "" ; moveIn2h[ALLIED] = "" ; moveIn2h[AXIS] = "" ; moveIn4h[ALLIED] = "" ; moveIn4h[AXIS] = ""
 for unit in allStratHCUnits()
 do
  if unit:isBrigade() and not unit:isSupplyInTransit() and not unit:offMap() and not unit:isTrainingUnit() and unit:isSpawnable() then
   if not brigadeStrengths[unit:unitID()] then
    brigadeStrengths[unit:unitID()] = {}
    brigadeStrengths[unit:unitID()][INFANTRY] = 100
    brigadeStrengths[unit:unitID()][GROUND_VEHICLE] = 100
   end
   bdestr_reportOn(unit, INFANTRY, "Infantry")
   bdestr_reportOn(unit, GROUND_VEHICLE, "Armor")
  end

  if unit:isBrigade() and not unit:isTrainingUnit() and unit:isSpawnable() then
   function moveReady(lastTimer, nextMove, minTime, rangeSeconds)
     local minSeconds = timeInSeconds(minTime)
     if lastTimer > minSeconds and nextMove <= minSeconds + rangeSeconds then
       debugReport(unit:callsign() .. " ready to move in " .. minSeconds .. " seconds")
       return true
     else
       return false
     end
   end

   local lastTimer = brigadeTimers[unit:unitID()]
   if not lastTimer then lastTimer = 99999999 end
   local nextMove = unit:nextMoveIn()
   if lastTimer and lastTimer > nextMove then
    if moveReady(lastTimer, nextMove, { seconds = 0 }, 10) then moveNow[unit:side()] = moveNow[unit:side()] .. unit:name() .. " ("..unit:callsign().." "..unit:homeCP().name.."). " ; nextMove = 0
    elseif moveReady(lastTimer, nextMove, { minutes = 5 }, 15) then moveIn5[unit:side()] = moveIn5[unit:side()] .. unit:name() .. ". " ; nextMove = 300
    elseif moveReady(lastTimer, nextMove, { hours = 1 }, 30) then moveIn1h[unit:side()] = moveIn1h[unit:side()] .. unit:callsign() .. ". " ; nextMove = 3600
    elseif moveReady(lastTimer, nextMove, { hours = 2 }, 30) then moveIn2h[unit:side()] = moveIn2h[unit:side()] .. unit:callsign() .. ". " ; nextMove = 7200
    elseif moveReady(lastTimer, nextMove, { hours = 4 }, 30) then moveIn4h[unit:side()] = moveIn4h[unit:side()] .. unit:callsign() .. ". " ; nextMove = 14400
    end
   end
   brigadeTimers[unit:unitID()] = nextMove
  end
 end

 function announce(list, unit, units)
  local fmt = locstr("bdeNextMove"..unit, units)
  if list[ALLIED] ~= "" then debugReport("[ALLIED] " .. fmt .. ":" .. list[ALLIED]) ; chatSendHCMessage(ALLIED, LSTR_LABELLED_GENERIC, fmt, list[ALLIED]) end
  if list[AXIS] ~= "" then debugReport("[AXIS] " .. fmt .. ":" .. list[AXIS]) ; chatSendHCMessage(AXIS, LSTR_LABELLED_GENERIC, fmt, list[AXIS]) end
 end

 announce(moveIn4h, 'Hours', 4)
 announce(moveIn2h, 'Hours', 2)
 announce(moveIn1h, 'Hour', 1)
 announce(moveIn5, 'Minutes', 5)
 announce(moveNow, 'Now', 0)

end

-- Run the brigade strength check once a minute
LuaEvent.AddRecurring(60, bde_strength_check, nil, 'bdeStrCheck')

