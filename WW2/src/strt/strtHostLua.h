#pragma once
// Wrapper functions for C->Lua function calls.

// Queries
struct WEAPON_SPEC;
class FacilityStruct;
class CPStruct;
class StratHCUnit;

#include "wwii/wwiiOwnershipEnums.h"

extern UINT32 stratBridgeObjectiveLimit(void);
extern UINT32 stratCaptureObjectiveLimit(void);
extern UINT32 stratResupplyTime(const WEAPON_SPEC& weapon, StratHCUnit* unit, FacilityStruct* facility, UINT32 resupplyTimer);
extern double stratRDPSupplyPenalty(WWII_COUNTRY forCountry);
extern UINT32 stratEWSHighThreshold(void);
extern UINT32 stratEngagementObjectivesQuota(UINT32 attackLimit);
extern void stratProcessLuaEventQueue();

extern bool strtCPEventTrigger(CPStruct&, const char* trigger);

// Run a generic, single-argument, game event
extern bool strtGameEventTrigger(const char* trigger, WWII_SIDE forSide);

extern void stratInitializeLua();
extern bool stratStartLua();
extern bool stratStartIntermission();

