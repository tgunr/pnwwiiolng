local cp = getCP("Paal Bridge")
print(cp.name)
if cp:isBridge() == false then return error("Target must be a bridge") end

local fac = cp.facilityList
local obj = fac.objectList
print("name " .. obj.name)

local i
if ARGS(1) == 'down' then
 print("destroying")
 obj:setDamageJoules(obj.m_maxDamageJoules)
 obj:calcPercentDamage()
 obj:damagedVisLevel(3204)
 obj:recalculateCapacity()
 fac:recalculateCapacity()
 cp.country = BRIDGE_DESTROYED_COUNTRY
 fac:setCountry(BRIDGE_DESTROYED_COUNTRY)
 obj:setCountry(BRIDGE_DESTROYED_COUNTRY, fac.side)
 cp.controllingCountry = cp.country
 cp:setContention(true)
elseif ARGS(1) == 'up' then
 print("repairing")
 obj:setDamageJoules(0)
 obj:calcPercentDamage()
 obj:repairVisLevel()
 obj:recalculateCapacity()
 fac:recalculateCapacity()
 cp.country = BRIDGE_BUILT_COUNTRY
 fac:setCountry(BRIDGE_BUILT_COUNTRY)
 obj:setCountry(BRIDGE_BUILT_COUNTRY, fac.side)
 cp.controllingCountry = cp.country
 cp:setContention(false)
else print("Please specify 'up' or 'down'")
end

strtQueueCPStatus(cp)
strtQueueVisLevelUpdate(obj)
