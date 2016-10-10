#pragma once
// Functions exposed ONLY to lua.

class FacilityStruct;

#include "wwii/wwiiHCUnitsIndex.h"

//tolua_begin
FacilityStruct* strtGetFacilityByName(const char* name);

UINT16 getHCiID(const HCUIDX_ITERATOR* it);
HCUnit* getHCiHCUnit(const HCUIDX_ITERATOR* it);
StratHCUnit* getHCiStratHCUnit(const HCUIDX_ITERATOR* it);
HCUIDX_ITERATOR getHCiNext(const HCUIDX_ITERATOR* it);

#ifdef TOLUA_ONLY
struct HCUNIT_INDEX {
 HCUIDX_ITERATOR find(unsigned int);
 HCUnit* operator[](unsigned int);
 StratHCUnit* operator[](unsigned int);
 HCUIDX_ITERATOR begin@beginIterator();
 HCUIDX_ITERATOR end@endIterator();
};

struct HCUIDX_ITERATOR {
 bool operator==(const HCUIDX_ITERATOR& rhs);

 static tolua_outside UINT16 getHCiID@id(const HCUIDX_ITERATOR* it);
 static tolua_outside HCUnit* getHCiHCUnit@hcUnit(const HCUIDX_ITERATOR* it);
 static tolua_outside StratHCUnit* getHCiStratHCUnit@stratHCUnit(const HCUIDX_ITERATOR* it);
 static tolua_outside HCUIDX_ITERATOR getHCiNext@next(const HCUIDX_ITERATOR* it);
};

#endif

static inline const char* getGameName(const ChatHandle handle)
{
    return chatGetCallsign(handle);
}
static inline ChatHandle getHandle(const char* gameName)
{
    if ( NULL == gameName ) return 0ULL;
    return chatGetHandle(gameName);
}

static inline const Rank* getRank(unsigned int i) { return Rank::lookup(i); }

//tolua_end
