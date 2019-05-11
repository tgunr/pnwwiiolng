-- Playnet-specific configuration stuff.

function killReport(vicId, killedBy, ...)
  local killedBy = string.gsub(killedBy, '{handle\|(%d+)}', function (handle) return getCallsign(handle) end)
  local victim = getCallsign(vicId)
  systemMessage(string.format("[DEV] %s was killed by %s", getCallsign(vicId), killedBy))
end

