----------------------------------------------------------------------
-- Event handlers
----------------------------------------------------------------------

require 'event-queue'

----------------------------------------------------------------------
-- Code called once per minute

local fbSaveTime = currentTime() + 15

function eventMinuteTick(dummy)
  if game.debug == true then print("eventMinuteTick [debug]") else print("eventMinuteTick") end

  local savefbs = false
  if fbSaveTime >= currentTime() then
    fbSaveTime = currentTime() + (10 * 60)
    savefbs = true
  end

  -- Check all CPs for lopsided AOs and AOs without contention
  for cp in allCityCPs() do

    if cp:isDefendObjective(ALLIED) and not cp:isAttackObjective(AXIS) then
      -- Allied defense with no axis attack? that's not good
      warn(cp.name.." has lopsided AO")
      cp:clearObjective(ALLIED)
      strtSaveCP(cp)
    elseif cp:isDefendObjective(AXIS) and not cp:isAttackObjective(ALLIED) then
      -- Axis defense with no allied attack? that's bad too
      warn(cp.name.." has lopsided AO")
      cp:clearObjective(AXIS)
      strtSaveCP(cp)
    elseif cp:isContested() and not(cp:isDefendObjective(ALLIED) or cp:isDefendObjective(AXIS)) then
      -- Should not be possible to be contested without having an AO
      warn(cp.name .. " contested without AO")

      for fac in allFacilitiesAt(cp) do
        if fac.side ~= cp.side then
          warn(fac.name .. " causing contention in non-AO town")
          setFacility(fac, cp.country, false)
        end

        if savefbs == true and fac.type == FACILITY_FIREBASE then
          strtSaveFacility(fac)
        end
      end
      -- Make sure all the tables get cleared
      cp:setTableStates()
      strtSaveCP(cp)
    end

  end
end

----------------------------------------------------------------------
-- Periodically run the ews check routine.

function ewsCheckHandler(_, event)
  if ewsCheckEnabled == true and ewsCheckInterval > 0 then
    runEWSCheck(ALLIED, true)
    runEWSCheck(AXIS, true)
  end

  if ewsCheckInterval >= 60 then
   event.interval = ewsCheckInterval
  else
   event.interval = 60
  end
end

----------------------------------------------------------------------
-- Queue the recurring event handlers.

-- Run the minutely tick.
LuaEvent.AddRecurring(60, eventMinuteTick, nil, 'eventMinuteTick')

-- Run the ewsCheckHandler 1 minute from now (self-adjusts)
LuaEvent.AddRecurring(60, ewsCheckHandler, nil, 'ewsCheck')

----------------------------------------------------------------------
-- Queue the recurring event handlers.

-- Run the minutely tick.
LuaEvent.AddRecurring(60, eventMinuteTick, nil, 'eventMinuteTick')

-- Run the ewsCheckHandler 1 minute from now (self-adjusts)
LuaEvent.AddRecurring(60, ewsCheckHandler, nil, 'ewsCheck')

-- Every 2s re-broadcast a CP.
local nextCP = nil
function forceFacilityUpdates()
  if nextCP == nil then
    nextCP = strtGetCPList()
  end

  -- Skip any bridges.
  while nextCP:isBridge() do
    nextCP = nextCP.next
    if nextCP == nil then return end
  end

  strtQueueCPStatus(nextCP)
  for fac in allFacilitiesAt(nextCP) do
    strtQueueFacilityStatus(fac)
  end

  nextCP = nextCP.next
end

LuaEvent.AddRecurring(2, forceFacilityUpdates, nil, 'forceFacUpdates')

