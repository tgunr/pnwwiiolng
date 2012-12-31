----------------------------------------------------------------------
-- PlayNet ZeroMQ Instancing Management Interface.
-- Copyright (C) 2011, PlayNet Inc, All Rights Reserved.
-- Not for redistribution.
--
-- Originating Programmer: Oliver "kfsone" Smith
-- Created: 2011/04/27 20:01.13 CDT
--
--[[
	Provides the "Instancing" interface for implementing
	a Lua-based ZeroMQ Instance client or server.

	Instancing.Config defines the default address,
	along with properties of the known interfaces.
	
	Instancing:Start(<role> [, <address>])
		Configures the Instancing object and establishes
		the communications connections.
		role := "Client" or "Server"
		address := overrides the default server address.

	Instancing:SetInstance(<numeric instance UniqueID>)
		In the Client role, tells ZeroMQ only to receive
		events for the specified instance. Provide a
		value of "0" if you are not in an instance.

	Instancing.getMessage(<zmqSocket> [, <zmqSocketOptions>])
		Reads the next complete message from the specified
		zmqSocket and returns it as an array of parts, even
		if there is only one message part.

	Instancing:SetInstanceState(<instanceNo>, <stateName>, <value>, broadcast = true | false, forward = true | false)
		Changes the value of <stateName> for the given instance.
		If broadcast is true, the change will be forwarded upstream,
		i.e. from Client to Server or Server to Client.
		Forward controls whether the message will be forwarded on
		from the upstream.
		Thus a Client calling
			Instancing:SetInstanceState(1, "lastuser", "oliver", false, false)
		will only change the state locally, whereas
			Instancing:SetInstanceState(1, "lastuser", "oliver", true, false)
		will update the server but not forward it on to instance
		occupants, local state will not be changed until the event
		is received back from the server.
			Instancing:SetInstanceState(1, "lastuser", "oliver", true, true)
		will update the server and forward on to instance members,
		only updating the local state when the event arrives back.

	Instancing.runCode(<code string>)
		Safely executes the given code.
		Returns (Instancing.SuccessCode, <return value>) on success
		Otherwise (Instancing.ErrorCode, <error message>)

	See InstancingClient.lua for Client specific functions,
	See InstancingServer.lua for Server specific functions.
]]--

--require 'zmq'		-- For the ZeroMQ API
-- game.LoadLib("zmq") -- For the ZeroMQ API

require 'string'	-- For the string format function
require 'table'		-- For table.insert

Instancing =
{
	--------- --------- --------- --------- --------- ---------
	-- CONFIGURATION
	--------- --------- --------- --------- --------- ---------

	--------- --------- --------- --------- --------- ---------
	-- Server parameters and default values.
	Config =
	{
	  ['DefaultAddress']	=	'192.168.0.61'
	, ['Interfaces'] =
	  {
		['Console']	=	{ ['Port'] = '27020', ['Type'] = { ['Client'] = zmq.REQ, ['Server'] = zmq.REP } }
	  , ['Command']	=	{ ['Port'] = '27021', ['Type'] = { ['Client'] = zmq.PUSH, ['Server'] = zmq.PULL } }
	  , ['Events']	=	{ ['Port'] = '27022', ['Type'] = { ['Client'] = zmq.SUB, ['Server'] = zmq.PUB } }
	  }
	}

	-- Number of IO threads to tell ZeroMQ to use.
,	IOThreads		=	{ ['Client'] = 1, ['Server'] = 2 }
	
,	Role			=	nil		-- Will be either "Client" or "Server".
,	Context			=	nil		-- ZeroMQ context wrapper.
,	HostAddress		=	nil		-- Host address we connected to.
,	ConsoleSock		=	nil		-- Will be the bi-directional socket to the lua command process.
,	CommandSock		=	nil		-- Will be the send-only socket to the lua trigger processor.
,	EventSock		=	nil		-- Will be the read-only socket for broadcasted events.
,	CurrentInstance	=	nil		-- The instance we've subscribed to.

,	SuccessCode		=	'OK'	-- Returned to indicate things went well
,	ErrorCode		=	'ERR'	-- Returned to indicate something failed.

,	InstanceStates	=	{ }
,	InstanceId		=	nil		-- Current instance (formatted)
,	InstanceNo		=	nil		-- Current instance (number)

,	Running			=	false	-- Gets set to true by Start

	--------- --------- --------- --------- --------- ---------
	-- Functions
	-- - Functions marked 'private' are not for use outside of
	--   the class itself.
	-- - Functions marked 'public' are usable by anyone.
	-- - Functions are called using Instancing:FunctionName()
	-- - Except where marked "static" (names start with lowercase)
	--------- --------- --------- --------- --------- ---------

	--------- --------- --------- --------- --------- ---------
	-- Return true/false whether we are the client.
,	IsClient = function(self)
		return self.Role and self.Role == "Client"
	end

	--------- --------- --------- --------- --------- ---------
	-- Return true/false whether we are the server.
,	IsServer = function(self)
		return self.Role and self.Role == "Server"
	end

	--------- --------- --------- --------- --------- ---------
	-- Turn an instance's unique ID into a hex 16-char string
	-- public: static
,	formatInstanceNo = function (instanceNo)
		return string.format("%016u", tonumber(instanceNo))
	end

	--------- --------- --------- --------- --------- ---------
	-- Create and connect a socket for a given interface based on our current role.
	-- private:
,	_OpenInterface = function (self, ifName)
		-- Gotta have a context to do anything.
		if self.Context == nil then
			return error("Tried to call OpenInterface without a Context")
		end

		local interface = self.Config.Interfaces[ifName]
		if not interface then
			return error("Unknown interface name: " .. ifName)
		end

		local ifType = interface.Type[self.Role]
		if not ifType then
			return error("Interface " .. ifName .. " not supported in " .. self.Type .. " role")
		end

		local sock = self.Context:socket(ifType)
		if not sock then
			return error("Unable to create socket for " .. ifName .. " interface")
		end

		-- The host always binds ports, the client always connects to them.
		local uri = "tcp://" .. self.HostAddress .. ":" .. interface.Port
		if self:IsClient() then
			sock:connect(uri)
		else
			sock:bind(uri)
		end

		return sock
	end

	--------- --------- --------- --------- --------- ---------
	-- Tune the Events interface to a specific instance ID.
	-- public:
,	SetInstance = function (self, instanceNo)
		if not self.Context then
			return { Instancing.ErrorCode, "Tried to call Instancing.SetInstance without a Context" }
		end

		-- Numeric argument required.
		if type(instanceNo) ~= "number" then
			return { Instancing.ErrorCode, "SetInstance requires a numeric instance argument." }
		end

		local instanceId = self.formatInstanceNo(instanceNo)

		-- Only mess with ZeroMQ stuff if we have an EventSock connected.
		-- Also, the server doesn't do any sub/unsub stuff.
		if self:IsClient() then
			if self.InstanceId ~= nil then
				-- Purge the old state information.
				self.InstanceStates[self.InstanceNo] = nil
	
				if self.EventSock ~= nil then
					-- Unsubscribe from whatever instance we were previously in.
					self.EventSock:setopt(zmq.UNSUBSCRIBE, self.InstanceId)
					self.InstanceId = nil
					self.InstanceNo = nil
				end
			end
			if self.EventSock ~= nil then
				self.EventSock:setopt(zmq.SUBSCRIBE, instanceId)
				print("subscribed EventSock to " .. instanceId)
			end
		end

		self.InstanceId = instanceId
		self.InstanceNo = instanceNo

		return { Instancing.SuccessCode, "Instance " .. instanceNo .. "(" .. instanceId .. ") selected." }
	end
			
	--------- --------- --------- --------- --------- ---------
	-- Tear up the ZeroMQ communications system and open the interfaces.
	-- public:
,	Start  = function (self, role, address)
		print(string.format("D Instancing:Start(%s, %s)", tostring(role), tostring(address)))

		-- Validate the role argument, must be client or server.
		if not role or (role ~= "Client" and role ~= "Server") then
			return error("Invalid use of Instancing.Start. Usage: Instancing:Start(\"Client\"|\"Server\" [, \"host.ip.addr\" ]")
		end

		-- Establish the role.
		self.Role = role
		print("D Role is " .. self.Role)

		-- Use the default address if none is supplied
		if not address then
			address = self.Config.DefaultAddress
			print("D Using Default Address of " .. address)
		end

		self.HostAddress = address
		print("D Using server address " .. self.HostAddress)

		-- Start a ZMQ thread if one isn't already started.
		if not self.Context then
			self.Context = zmq.init(self.IOThreads[self.Role])
		end

		-- Create the interfaces.
		self.ConsoleSock = self:_OpenInterface("Console")
		self.CommandSock = self:_OpenInterface("Command")
		self.EventSock   = self:_OpenInterface("Events")

		-- On the client, we need to subscribe to something
		-- if we're going to receive events;
		-- Start with a default instance of 0 (the campaign).
		if self:IsClient() and self.EventSock then
			self:SetInstance(0)
		end

		self.Running = true

		print("I " .. self.Role .. " ready.")
	end

	--------- --------- --------- --------- --------- ---------
	-- Retrieve a message, or a set of multi-part messages,
	-- as an array of message parts.
	-- optional argument 'zmqOptions' can be set to zmq.NOBLOCK
	-- if you want to avoid blocking.
	-- Returns NIL if no messages were received.
	-- public: static
,	getMessage = function (zmqSocket, zmqOptions)
		local parts = {}
		local msg
		while true
		do
			msg = zmqSocket:recv(zmqOptions)
			if not msg then break end
			table.insert(parts, msg)
			print("part #" .. #parts .. ": " .. tostring(msg))
			
			-- ZMQ will set the "RCVMORE" value on a socket
			-- if the message was a non-terminal part of a
			-- multi-part message.
			-- If RCVMORE is not set, it's time to stop.
			if zmqSocket:getopt(zmq.RCVMORE) == 0 then break end
		end
		
		-- No parts, no array.
		if #parts == 0 then return nil end
		
		return parts
	end

	--------- --------- --------- --------- --------- ---------
	-- Publish a state change, non-blocking.
	--  broadcast: whether the message should be
	--    transmitted "upstream" (i.e. Client->Server,
	--	  or Server->Client).
	--  forward: when broadcasting, determines if
	--    we should re-broadcast the event.
	-- If "broadcast" is true, we will send the
	-- message "upstream" (Client->Server, Server->Client)
	-- with the value of "forward" as the remote argument
	-- to "broadcast".
	-- public:
,	SetInstanceState = function (self, instanceNo, state, value, broadcast, forward)
		if instanceNo == nil
		or state == nil or type(state) ~= "string"
		or broadcast == nil
		or forward == nil
			then return { Instancing.ErrorCode, "Incorrect usage of SetInstanceState: expects instanceNo, state, value, broadcast, forward" }
		end
		if type(instanceNo) == "string" then
			local no = tonumber(instanceNo)
			if no == nil or no == 0 then
				return { Instancing.ErrorCode, "Incorrect usage of SetInstanceState: unrecognized instanceNo " .. instanceNo .. "." }
			end
			instanceNo = no
		end
		if not self.Context then
			return { Instancing.ErrorCode, "Tried to call SetInstanceState without a Context." }
		end

		-- Convert to hex format for better hashing.
		local instanceId = self.formatInstanceNo(instanceNo)

		-- Check to see if the value needs modifying.
		local instanceStates = self.InstanceStates[instanceId]
		if instanceStates then
			local currentValue = instanceStates[stateName]
			if tostring(currentValue) == tostring(value) then
				return { Instancing.SuccessCode, "-- No change made" }
			end
		end

		-- broadcast should be either true or false, anything else we
		-- consider to be false.
		if type(broadcast) ~= "boolean" then broadcast = false end
		if broadcast == false or type(forward) ~= "boolean" then forward = false end

		if broadcast == true then
			if self:IsServer() == true then
				-- Server should never tell clients to forward state.
				forward = false
			end

			-- Client and Server implement SendEvent themselves,
			-- which means we can't access it as self.Event
			local bcastFn = self['SendEvent']
			if bcastFn then
				-- Always pass "false" in the forward column.
				-- We can't use : notation, so we have to manually
				-- pass the "self" argument.
				print("forwarding")
				print( unpack( bcastFn(self, instanceNo, string.format("return Instancing:SetInstanceState(%u, %q, %q, %s, false)", instanceId, state, value, tostring(forward))) ) )
			end
		end

		-- Only update state on the server or when we are the
		-- final recipient of the state.
		if self:IsServer() or broadcast == false then
			instanceStates = self.InstanceStates[instanceId]
			if not instanceStates then
				instanceStates = { ['id'] = instanceId, ['no'] = instanceNo }
				self.InstanceStates[instanceId] = instanceStates
			end

			instanceStates[state] = value

			return { Instancing.SuccessCode, "-- State change applied" }
		else
			return { Instancing.SuccessCode, "-- State change forwarded" }
		end
	end

	--------- --------- --------- --------- --------- ---------
	-- make sure we're dealing with a number

, 	formatInstanceId = function(instanceId)
		return tonumber(instanceNo)
	end

	--------- --------- --------- --------- --------- ---------
	-- Run an arbitrary piece of Lua code.
	-- Returns <success code>, <return value> on success,
	-- otherwise <error code>, <error message>.
	-- public: static
,	runCode = function(code)
		local success, retval = pcall(loadstring(code))

		-- Ensure retval is something to avoid complaints about nil.
		if retval == nil then retval = "" end

		if success then
			return { Instancing.SuccessCode, retval }
		else
			return { Instancing.ErrorCode, retval }
		end
	end
}

