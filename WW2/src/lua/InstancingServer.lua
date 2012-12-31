--
-- Lua server code
--

require 'Instancing'

--------- --------- --------- --------- --------- ---------
function Instancing.SendEvent(self, instanceNo, code)
	if self.Context == nil then
		return { Instancing.ErrorCode, "Called SendEvent without a Context." }
	end

	if self.EventSock == nil then
		return { Instancing.ErrorCode, "EventSock not connected." }
	end

	local instanceId = self.formatInstanceNo(instanceNo)

	print( "sending event " .. instanceId .. ", " .. code )

	-- Send the instanceId as the first part of the message,
	-- the code as the second.
	self.EventSock:send(instanceId, zmq.SNDMORE)
	self.EventSock:send(code, zmq.NOBLOCK)

	return { Instancing.SuccessCode, "-- event passed to zmq, data may not actually have been sent yet." }
end

-- Start the communications stuff.
Instancing:Start("Server")

local parts

function drainConsole(zmqSocket)
	while true
	do
		local parts = Instancing.getMessage(zmqSocket, zmq.NOBLOCK)
		if parts == nil or #parts == 0 then
			return
		end

		if #parts > 1 then
			print("W multi-part message on console socket.")
		else
			print("D --con-> " .. parts[1])
			local result = Instancing.runCode(parts[1])
			print("D <--con- " .. result[1] .. ", " .. result[2])
			zmqSocket:send(result[1], zmq.SNDMORE)
			zmqSocket:send(result[2], zmq.NOBLOCK)
		end
	end
end

function drainCommand(zmqSocket)
	while true
	do
		local parts = Instancing.getMessage(zmqSocket, zmq.NOBLOCK)
		if parts == nil or #parts == 0 then
			return
		end

		if #parts > 1 then
			print("W multi-part message on command socket.")
		else
			print("D --cmd-> " .. parts[1])
			local result = Instancing.runCode(parts[1])
			print("D result: " .. result[1] .. ", " .. tostring(result[2]))
		end
	end
end
	
-- Set up a poller for the incoming sockets.
zmq.poller = require "zmq/poller"
poller = zmq.poller(2)

poller:add(Instancing.CommandSock, zmq.POLLIN, drainCommand)
poller:add(Instancing.ConsoleSock, zmq.POLLIN, drainConsole)

-- Ok, now sit and watch.
while Instancing.Running
do
	-- Wait upto 100ms for events.
	poller:poll(100 * 1000)

	-- TODO:PERSIST: If we have a timer/event queue, process it here.
end

print("N shutting down.")

