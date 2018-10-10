-- Save the current map state.

function saveMapStateToFile(filename)
  local output = ""
  for cp in allCityCPs() do

   -- Save CP states
   output = output .. 'setCP("'..cp.name..'", '..cp.country..")\n"

   -- Save all the firebase states
   for link in allLinksFrom(cp) do
    linkback = link.endDepot.m_outgoingLink
    if link and link.firebase and linkback and linkback.firebase then
     local fb = nil
     if link.firebase:isOpen() then fb = linkback.firebase
     elseif linkback.firebase:isOpen() then fb = link.firebase end
     if fb ~= nil then output = output .. "blowFirebase(" .. fb.uOID .. ")\n"
     end
    end
   end

  end

  -- Save brigade deployments
  local cpName
  for unit in allStratHCUnits() do
   if unit:homeCP() then cpName = unit:homeCP().name else cpName = 'offmap' end
   output = output .. 'deploy("'..unit:callsign()..'", "'..cpName..'", '..unit:nextMoveIn()..')' .. "\n"
  end

  if filename then
    output = output .. "print('Map state restored')\n"
    local f = assert(io.open(filename, "w"))
    f:write(output)
    f:close()
  else
    print(output)
  end
end

function autosaveMapState(filename)
-- if wwii_config.arena.lock > 0 then return end
 print("Saving map state to " .. filename)
 saveMapStateToFile(filename)
end

local mapfilename = "map-state"
local mapfile = "/playnet/wwiiol/lua/"..mapfilename..".lua"

function loadMapState(filename)
 require 'map-state'
end

LuaEvent.AddRecurring(120, autosaveMapState, mapfile, 'saveMapState')

function saveTheMap()
  saveMapStateToFile(mapfile)
end

