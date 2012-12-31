--[[ 
   Lua timed event queues

   Copyright (C) PlayNet Inc, 2010
   Original Author: Oliver "kfs1" Smith <oliver@kfs.org>
   All rights granted to PlayNet

   Usage:
    To schedule a one-time event:
  		LuaEvent.Schedule( triggerTime, handlerFn, myData )
  			triggerTime :- An absolute time in seconds since epoch,
  							(tested against teulClockTime)
  			handlerFn :- A function (not name) that will be called with 'myData'
  			myData :- Data to be passed to the handlerFn
  			eventId :- Optional, lets you assign a name to an event so that
  					   it can only be quued once.
  		Returns the event.
  
  	To schedule a recurring event
  		LuaEvent.AddRecurring( interval, handlerFn, myData [, eventId]  )
  			interval :- time in seconds between invocations
  			handlerFn :- A function that will be called with myData as
  						the first argument and the event as the second.
  			eventId :- Any user defined value to uniquely identify this event
  
    To cancel an event, sent the "cancel" property to true
  
    To cancel a recurring event by it's eventId
    	LuaEvent.CancelRecurring( eventId )
  
    To Process the queue
  		LuaEvent.ProcessQueue()
]]--

-- Our public interface.
LuaEvent = { }

-- Our private data (queues)
local eventQueue = {}
local recurring = {}

----------------------------------------------------------------------
-- Add a recurring event to the queue.
----------------------------------------------------------------------

function LuaEvent.AddRecurring(withInterval, handler, withData, eventId)
	local intervalTime = tonumber(math.ceil(withInterval))
	if intervalTime == nil or intervalTime == 0 then
		error("LuaEvent.AddRecurring: Invalid interval: "..withInterval)
		return nil
	end

	if type(handler) ~= 'function' then
		error("LuaEvent.AddRecurring: Invalid argument (#2): Got "..type(handler)..", expected 'function'")
		return nil
	end

	local event =
	{
		-- Interval between iterations
		['interval'] = intervalTime
		-- The first call is always delayed by 'interval'
		-- so that it doesn't get called immediately
	,	['due'] = teulClockTime + intervalTime
		-- Function to be called (will be passed the event)
	,	['handler'] = handler
	,	['data'] = withData
	,	['cancel'] = false
	,	['eventId'] = eventId
	}

	if eventId ~= nil then
		for i in pairs(recurring)
		do
			if recurring[i].eventId ~= nil and recurring[i].eventId == eventId then
				recurring[i] = event
				return event
			end
		end
	end

	table.insert(recurring, event)

	return event
end


----------------------------------------------------------------------
-- Cancel a recurring event by it's eventId
----------------------------------------------------------------------

function LuaEvent.CancelRecurring(eventId)
	-- Quietly ignore a request with a nil id.
	if eventId == nil then return false end

	for i in pairs(recurring)
	do
		if recurring[i].eventId ~= nil and recurring[i].eventId == eventId then
			recurring[i].cancel = true
			return true
		end
	end

	return false
end

----------------------------------------------------------------------
-- Add a one time event to the queue.
----------------------------------------------------------------------

function LuaEvent.Schedule(atTime, handler, withData)
	local eventTime = tonumber(math.ceil(atTime))

	-- Validate arguments.
	if eventTime == nil or eventTime == 0 then
		error("LuaEvent.Schedule: Invalid time for event schedule: "..atTime)
		return nil
	end
	if eventTime > 0 and eventTime < timeInSeconds({ days = 365 }) then
		eventTime = teulClockTime + eventTime
	end

	if eventTime < teulClockTime then
		return error("LuaEvent.Schedule: Invalid time for event (in past): "..eventTime)
	end

	if type(handler) ~= 'function' then
		error("LuaEvent.Schedule: Invalid argument (#2): Got "..type(handler)..", expected 'function'")
		return nil
	end

    local event =
    {
        -- The time that this event is scheduled for.
        ['due'] = tonumber(math.ceil(eventTime))
        -- Whether this event has been cancelled or not.
    ,   ['cancel'] = false
    ,   ['handler'] = handler
    ,   ['data'] = withData
	,	['interval'] = 0
    }

    if event.due < teulClockTime then
		pcall(handler, withData, event)
		return nil
    end

	local idx
	for idx = 1, #eventQueue + 1
	do
		-- Insert a new record if we reached the end or
		-- found an slot scheduled for a time greater than ours.
		if idx > #eventQueue or eventQueue[idx].due > event.due then
			table.insert(eventQueue, idx, { ['due'] = event.due, events = {} })
		end
		if eventQueue[idx].due == event.due then
			table.insert(eventQueue[idx].events, event)
			return event
		end
	end

	error("This should never happen")
end

----------------------------------------------------------------------
-- Test if the queue is populated and ready for events to be pulled.
----------------------------------------------------------------------

function LuaEvent.QueueReady()
	if #eventQueue == 0 then return false end
	if eventQueue[1].due > teulClockTime then return false end
	return true
end

----------------------------------------------------------------------
-- Check for and execute pending events
----------------------------------------------------------------------

function LuaEvent.ProcessQueue()
	-- Process the recurring queue
	local newRecurring = {}
	for i in pairs(recurring)
	do
		if recurring[i] then
			if recurring[i].cancel == false and recurring[i].due <= teulClockTime then
				local status, msg = pcall(recurring[i].handler, recurring[i].data, recurring[i])
				if status ~= true then log('A', 'eventq', msg) end
				recurring[i].due = teulClockTime + recurring[i].interval
			end
			if recurring[i].cancel == false then
				table.insert(newRecurring, recurring[i])
			end
		end
	end

	-- Overwrite the recurring list.
	recurring = newRecurring

	-- Now process the one time queue.
	while LuaEvent.QueueReady()
	do
		for _, event in pairs(eventQueue[1].events)
		do
			if event.cancel == false then
				pcall(event.handler, event.data)
			end
		end
		table.remove(eventQueue, 1)
	end
end

--[[
-- To test the functionality...

teulClockTime = 0

function PrintQueue()
	local str, idx = "", 0
	for idx = 1, #eventQueue
	do
		queue = eventQueue[idx]
		str = "Q:"..idx..": due="..queue.due..":"
		for _, event in pairs(queue.events)
		do
			if event.cancel == true then
				str = str .. " [cancelled]"
			else
				str = str .. " " .. event.data
			end
		end
		print(str)
		str = ""
	end
end

function Spew(x) print("spewing: " .. x) end

LuaEvent.Schedule(10, Spew, "10")
LuaEvent.Schedule(15, Spew, "15")
LuaEvent.Schedule(5, Spew, "5a")
LuaEvent.Schedule(5, Spew, "5b").cancel = true
LuaEvent.Schedule(5, Spew, "5c")
LuaEvent.Schedule(12, Spew, "12")
LuaEvent.Schedule(3, Spew, "3")

LuaEvent.AddRecurring(1, Spew, "r1")
LuaEvent.AddRecurring(5, Spew, "r5")
LuaEvent.AddRecurring(2, Spew, "r2")

PrintQueue()

while #eventQueue > 0
do
	print("Tick: " .. teulClockTime)
	LuaEvent.ProcessQueue()
	teulClockTime = teulClockTime + 4
end


]]--
