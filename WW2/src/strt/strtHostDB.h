#pragma once
// Date: Mon Dec 12 18:43:43 CST 2005
//
// stratd specific host functions
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

class CPStruct;
class FacilityStruct;
class WeaponEntry;
class ObjectStruct;
class LinkStruct;
class HCUnit;

// Functions that Lua doesn't need to see.

extern bool strtInitHostDB();

//tolua_begin

// Capture states
enum CAP_STATE { CAP_START = 0, CAP_ILLEGAL = 1, CAP_CAPTURED = 2, CAP_SAVED = 3, MAX_CAP_STATES };

extern void strtProcessDBQueue();

extern void strtSaveCP(CPStruct* cp);
extern void strtSaveFacility(FacilityStruct* facility);

extern void strtdbSaveGameWorld(CPStruct* cp);
extern void strtdbPopulateVehTypesTable();
extern void strtdbReadVehicleAccess();
extern void strtdbSaveVehicleCVCs();
extern void strtdbSaveVehicleAccess();
extern void strtSaveFactoryOutput(ObjectStruct* factory);
extern bool strtdbFetchLuaScript(const char* name, std::string& obj);
extern void strtdbSaveConfigVar(const char* name, const char* value);
extern void strtStartNewDay();

// Load/update the TOE Template headers and vehice lists.
extern void strtdbLoadTOETemplates();

// Re-load vehicle levels from the database into the TOEs
extern void strtdbRestoreTOESupplies();

extern void strtCaptureLog(FacilityStruct* facility, UINT32 state, UINT32 fromCountry
							, const ChatHandle handle, UINT32 customerID, UINT32 country, UINT32 missionID
							, const HCUnit* unit, UINT32 started, UINT32 seconds
							, bool wasContested, bool tookControl, bool tookOwnership);

//tolua_end
