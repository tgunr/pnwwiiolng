-- Implements the variations of ;own.

-- Reporting for a firebase.
local function _firebase(playerId, obj)
  Strat.Caution(playerId, LSTR_GENERIC, "Not implemented yet.")
end

-- Reporting for a factory.
local function _factory(playerId, obj)
  Strat.Caution(playerId, LSTR_GENERIC, "Not implemented yet.")
end

-- Reporting for a bridge.
local function _bridge(playerId, obj)
  Strat.Caution(playerId, LSTR_GENERIC, "Not implemented yet.")
end

-- Reporting for a CP/bridge (will call _bridge)
local function _cp(playerId, obj)
  Strat.Caution(playerId, LSTR_GENERIC, "Not implemented yet.")
end

-- Main body.
Strat.Dot['own'] = function (oobmessage, arguments)
  local playerId = oobmessage.m_originatorID
  local processed, obj = Strat.FindStratObject(playerId, arguments, oobmessage.m_cpID, true)
  if processed then return end

  if tolua.type(obj) == "FacilityStruct" then
    if obj.type == FACILITY_FIREBASE then
      return _firebase(playerId, obj)
    end
    if obj.type == FACILITY_FACILITY_FACTORY and obj.cp and obj.cp.m_isSupplyCenter then
      return _factory(playerId, obj)
    end
  end
  if tolua.type(obj) == "CPStruct" then
    return _cp(playerId, obj)
  end

  -- We didn't recognize what you clicked on.
  Strat.Error(playerId, LSTR_SPECIFY_TOWN)
end

