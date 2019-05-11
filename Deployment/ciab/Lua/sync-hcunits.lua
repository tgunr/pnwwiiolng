require 'event-queue'

-- Wait 60 seconds before starting to sync.
local STARTUP_TIME = timeInSeconds({ minutes = 1 })
-- Interval between individual units.
local BETWEEN_UNIT_TIME = timeInSeconds({ seconds = 5 })
-- Interval between loops of the entire list.
local BETWEEN_REPEAT_TIME = timeInSeconds({ minutes = 2 })
-- Whether to allow repeats of the entire list.
local REPEAT = 0

-- Event to try and sync a hcunit and then schedule an update
-- for the next unit in the unit list.
function sync_next_unit(hcit)
    -- acquire an up-to date iterator.
    if not hcit then
        hcit = HcUnitIterator:new()
        log('W', 'No HcUnitIterator - disabling announcements.')
        if not hcit then
            return
        end
    end

    -- get the next unit from the iterator
    unit = hcit:getNext()
    if not unit then
        -- reached the end
        if REPEAT then
            log('M', 'Reached of hcunit iterator - pausing.')
            LuaEvent.Schedule(BETWEEN_REPEAT_TIME, sync_next_unit, nil)
            return
        else
            log('M', 'Reached end of hcunit iterator - stopping.')
            return
        end
    end

    log('I', 'Syncing unit#'..unit:unitID()..' '..unit:name())
    unit:announceHome()

    LuaEvent.Schedule(BETWEEN_UNIT_TIME, sync_next_unit, hcit)
end

function schedule_hc_unit_syncs()
    print("Scheduling HC Unit syncs to start in " .. STARTUP_TIME .. " seconds")
    LuaEvent.Schedule(STARTUP_TIME, sync_next_unit, nil)
end