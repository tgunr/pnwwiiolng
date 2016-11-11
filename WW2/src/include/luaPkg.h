#pragma once
// Functions exposed ONLY to lua.

class FacilityStruct;
class StratHCUnit;

using HcUnitVector = std::vector<StratHCUnit*>;
extern HcUnitVector& strtGetDivisionsAndBrigades();

//tolua_begin
FacilityStruct* strtGetFacilityByName(const char* name);

class HcUnitIterator
{
//tolua_end
	HcUnitVector::iterator m_it;
//tolua_begin
public:
	HcUnitIterator() : m_it(strtGetDivisionsAndBrigades().begin()) {}
	StratHCUnit* getNext() { return m_it == strtGetDivisionsAndBrigades().end() ? nullptr : *(m_it++); }
};

static inline const char* getGameName(const ChatHandle handle)
{
    return chatGetCallsign(handle);
}
static inline ChatHandle getHandle(const char* gameName)
{
    if ( NULL == gameName ) return 0ULL;
    return chatGetHandle(gameName);
}
//tolua_end
