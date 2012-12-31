print("- Loading InstancingClient")
require "InstancingClient"

local ic = Instancing

function check( fnCallReturn, expect )
	write( fnCallReturn )
	if expect == nil or expect == true then
		expect = ic.SuccessCode
	else
		expect = false
	end
	if type(fnCallReturn) == "table" then
		if fnCallReturn[1] ~= expect then
			return error("FAILED.")
		end
	end
end

print("- Checking GetInstanceState and GetStateValue handle no selection.")
write( ic:GetInstanceState() )
write( ic:GetStateValue("foo") )

print("- Selecting instance 10.")
write( ic:SetInstance(10) )

print("- Checking GetInstanceState works.")
write( ic:GetInstanceState() )

print("- Checking there is no value for foo.")
write( ic:GetStateValue("foo") )

print("- Checking state set.")
write( ic:SetState("foo", "bar") )

print("- Checking nothing is set.")
write( ic:GetStateValue("foo") )

print("- Now we call GetEvents")
write( ic:GetEvents(0, 1) )

print("- Lets see that GetStateValue('foo') returns now, eh?")
write( ic:GetStateValue("foo") )

print("- Lets destroy the instance.")
write( ic:SendLua("Instancing.InstanceStates[10] = nil") )

