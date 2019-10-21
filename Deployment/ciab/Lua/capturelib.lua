----------------------------------------------------------------------
-- capturelib.lua
--  Library of capture-related helper functions
----------------------------------------------------------------------

----------------------------------------------------------------------
-- Small helper function that checks the state of a facility
--  fac == the facility
--  expectCtry == who should own it?
--  expectSide == which side (sanity)
--  expectVis == vis level you expect of the table
--  expectRebuild == whether you expect it to be rebuilding
--  expectOpen == whether we we expect it to be open
--  occasion = description of when you are testing

function test_facility_state (fac, expectCtry, expectSide, expectVis, expectRebuild, expectOpen, occasion)

 if fac == nil then
  return "FAIL", "Facility is nil in test_facility_state " .. occasion
 end

 if fac.captureTable:visLevel() ~= expectVis then
  return "FAIL", fac.name.." table is "..
		fac.captureTable:visLevel()..
		" not "..expectVis.." "..occasion
 end

 local rebuilding = (fac.captureTable.repairTime > 0)

 if isRebuilding ~= expectedRebuild then
  return "FAIL", fac.name.." table build state wrong "..occasion
 end

 if fac.country ~= expectCtry then
  return "FAIL", fac.name.." doesnt belong to "..expectCtry.." "..occasion
 end

 if fac.side ~= expectSide then
  return "FAIL", fac.name.." has invalid side "..occasion
 end

 if fac:isOpen() and not expectOpen then return "FAIL", fac.name.." is open "..occasion end
 if not fac:isOpen() and expectOpen then return "FAIL", fac.name.." is closed "..occasion end

 return "OK", nil

end

----------------------------------------------------------------------
-- Test the state of a CP
-- Arguments are:
--  cp => cp to test,
--  occasion => the occasion of the test
--  owner => Which country should own
--  controller => Which country should control
--  contention => true/false: expect contention?
--  defABs => number of defender ABs expected
--  defFacs => number of defender facilities (including ABs) expected
--  attABs => number of attacker ABs expected
--  attFacs => number of attacker facilities expected
--  attackSide => side expected to have attack objective
--  defendSide => side expected to have defend objective
--  enemyLinked => true/false: expect enemy link (front line)?
--  supplied => true/false: expect town to be supplied?

function test_cp_state (arg)

 local cp = arg.cp
 if cp == nil then return "FAIL", "test_cp_state passed nil cp " .. occasion end
 if cp.facilityList == NULL then return "FAIL", cp.name.." has nil facilitylist "..occasion end

 -- Check the ownership of the CP if an owner has been provided
 if arg.owner ~= nil then
  if cp.country ~= arg.owner then return "FAIL", cp.name.." does not belong to "..arg.owner.." "..occasion end
  if cp.side ~= wwiiSide(arg.owner) then return "FAIL", cp.name.." side does not match country "..occasion end
 end

 -- If controller was supplied, check that the controller makes sense
 if arg.controller ~= nil then
  if cp.controllingCountry ~= arg.controller then
   return "FAIL", cp.name.." not controlled by "..arg.controller.." "..occasion
  elseif cp.controllingSide ~= wwiiSide(arg.controller) then
   return "FAIL", cp.name.." conSide does not match conCountry "..occasion
  end
 end

 -- Check that control makes sense
 if cp.armyBasesOwned[cp.controllingSide] == 0 then
  return "FAIL", cp.name.." controlling side owns 0 armybases".. occasion
 end

 -- Investigate contention flags and state
 if not cp:isContested() and cp.side ~= cp.controllingSide then
  return "FAIL", cp.name.." has no contention but different owner/controller "..occasion
 end

 if arg.contention ~= nil and arg.contention ~= cp:isContested() then
  return "FAIL", cp.name.." contention is not "..arg.contention.." "..occasion
 end

 if not cp:isContested() and cp.facilitiesOwned[cp.side] < cp.ownableFacilities then
  return "FAIL", cp.name.." contention/facility count mismatch "..occasion
 end

 if cp.facilitiesOwned[cp.side] > cp.ownableFacilities then
  return "FAIL", cp.name.." wrong owned facility count for "..cp.country.." "..occasion
 end

 -- Check the expected values for abs and facs
 if arg.defABs ~= nil and arg.defABs ~= cp.armyBasesOwned[cp.side] then
  return "FAIL", cp.name.." defender does not have "..arg.defABs.." abs "..occasion
 end
 if arg.defFacs and arg.defFacs ~= cp.facilitiesOwned[cp.side] then
  return "FAIL", cp.name.." defender does not have "..arg.defFacs.." facilities "..occasion
 end

 local attacker = wwiiOtherSide(cp.side)
 if arg.attABs ~= nil and arg.attABs ~= cp.armyBasesOwned[attacker] then
  return "FAIL", cp.name.." attacker does not have "..arg.attABs.." abs "..occasion
 end
 if arg.attFacs ~= nil and arg.attFacs ~= cp.facilitiesOwned[attacker] then
  return "FAIL", cp.name.." attacker does not have "..arg.attFacs.." facilities "..occasion
 end

 -- Count all the allegedly owned facilites and make sure they make sense
 local facilities = cp.facilitiesOwned[0] + cp.facilitiesOwned[1] + cp.facilitiesOwned[2] + cp.facilitiesOwned[3]
 if facilities > cp.ownableFacilities then
  return "FAIL", cp.name.." facility tally ("..facilities..") exceeds total "..cp.ownableFacilities.." "..occasion
 end

 -- And the same for the armybases
 local abs = cp.armyBasesOwned[0] + cp.armyBasesOwned[1] + cp.armyBasesOwned[2] + cp.armyBasesOwned[3]
 if abs > cp.numArmyBases then
  return "FAIL", cp.name.." AB tally ("..abs..") exceeds total "..cp.numArmyBases.." "..occasion
 end

 -- Check the objective flags
 if arg.objective ~= nil and arg.objective ~= cp:isObjective() then
  return "FAIL", cp.name.." is not an objective "..occasion
 end
 if arg.attackSide ~= nil and not cp:isAttackObjective(arg.attackSide) then
  return "FAIL", cp.name.." no attack objective for side #"..arg.attackSide.." "..occasion
 end
 if arg.defendSide ~= nil and not cp:isDefendObjective(arg.defendSide) then
  return "FAIL", cp.name.." no defend objective for side #"..arg.defendSide.." "..occasion
 end

 -- Check there are no dual objectives
 if cp:isCity() then
  if cp:isAttackObjective(cp.side) and cp:isAttackObjective(attacker) then
   return "FAIL", cp.name.." has dual attack objectives "..occasion
  end
  if cp:isDefendObjective(cp.side) and cp:isDefendObjective(attacker) then
   return "FAIL", cp.name.." has dual defend objectives "..occasion
  end
  if ( cp:isAttackObjective(cp.side) and not cp:isDefendObjective(attacker) )
  or ( cp:isDefendObjective(cp.side) and not cp:isAttackObjective(attacker) )
  then
   return "FAIL", cp.name.." unbalanced objectives "..occasion
  end
 end

 -- Check the enemy linked state
 if arg.enemyLinked ~= nil and arg.enemyLinked ~= cp:isEnemyLinked() then
  if arg.enemyLinked then return "FAIL", cp.name.." is not enemy linked "..occasion
  else return "FAIL", cp.name.." is enemy linked "..occasion end
 end

 -- Check the supplied state
 if arg.supplied ~= nil and arg.supplied ~= cp:isSupplied() then
  if arg.supplied then return "FAIL", cp.name.." is out of supply "..occasion
  else return "FAIL", cp.name.." is in supply "..occasion end
 end

 return "OK", "cp looks good"
end

