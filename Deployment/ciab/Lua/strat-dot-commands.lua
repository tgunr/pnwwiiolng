----------------------------------------------------------------------
-- Module implements functions for replacing strat host dot (;) commands
-- with localizable Lua implementations.
--
-- Provides the "Strat" namespace/object with the following helper
-- functions:
--
--   processed, object =
--     Strat.FindStratObject(playerId, arguments, fullObjId, allowBridges)
-- Looks up the fullObjID in the entire strat database, and attempts
-- to find the right CPStruct/FacilityStruct/ObjectStruct/LinkStruct
-- Under some conditions, it may report an error to the player for you,
-- in which case 'processed' will be true.
--   
--   Strat.Error, Strat.Caution, Strat.Note, Strat.Tell, Strat.Beta
-- e.g.
--   Strat.Error(playerId, MSG_LSTRING_ID [, text1 [, text2]])
-- If you do not know the LSTRING_IDs you can use
--   Strat.Error(playerId, LSTR_GENERIC, "Some text to display")
-- or
--   Strat.Error(playerId, LSTR_LABELLED_GENERIC, "Label", "After Label")

if not Strat then
 Strat = { Dot = {} }
end

function Strat.Error(playerId, msg, text1, text2)
  chatSendLocalizedMessage(playerId, LSTR_ERROR, msg, text1, text2)
end

function Strat.Caution(playerId, msg, text1, text2)
  chatSendLocalizedMessage(playerId, LSTR_CAUTION, msg, text1, text2)
end

function Strat.Note(playerId, msg, text1, text2)
  chatSendLocalizedMessage(playerId, LSTR_NOTE, msg, text1, text2)
end

function Strat.Tell(playerId, msg, text1, text2)
  chatSendLocalizedMessage(playerId, LSTR_NONE, msg, text1, text2)
end

function Strat.Beta(playerId, msg, text1, text2)
  chatSendLocalizedMessage(playerId, LSTR_BETA, msg, text1, text2)
end

function Strat.FindStratObject(playerId, arguments, fullObjID, allowBridges)
  local obj = nil
  if arguments == nil or arguments == "" then
    obj = getStratObject(oobmessage.fullObjID)
  else
    obj = getCP(arguments)
    if obj == nil then
      obj = getFacility(arguments)
    end

    if obj == nil then
	  Strat.Error(playerId, LSTR_INVALID_CP_NAME, arguments)
      return (true, nil)
    end
  end

  -- Translate bridges into CP objects
  if obj and obj.objClass == CLASS_BUILDING and obj.type == BUILDING_BRIDGE and obj.facility then
    if allowBridges == false then
	  Strat.Caution(playerId, LSTR_CANNOT_USE_BRIDGE)
      return (true, nil)
    end
    return (false, obj.facility.cp)
  end

  return (false, obj)
end

-- Include dot command bodies here.

require 'dot-own'

