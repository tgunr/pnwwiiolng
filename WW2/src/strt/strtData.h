#pragma once
// Date: Wed May 31 12:33:44 2000
//
// The strtData module is going to be a wrapper module so I can obscure
// all the strat System Data from the rest of the modules.  Sorta a set
// of globals with accessor functions.
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson

#include "strt/strtProtos.h"

struct MANIFESTSTRUCT;
struct CONVOYSTRUCT;
struct PAYLOADSTRUCT;
struct FULL_ID;

//tolua_begin

extern void strtEnemyLink(CPStruct& cp, bool state);

extern CPStruct* strtInitializeStratData();
extern bool strtVerifyStratSystem(CPStruct* cpList, UINT32 testLevel);
extern void strtResetObject(ObjectStruct& object);

extern Supercell* strtGetSupercellFromPosition_cm(const SINT32 x_cm, const SINT32 y_cm);
extern Supercell* strtGetSupercellFromPosition_m(const SINT32 x_m, const SINT32 y_m);
extern Supercell* strtGetSupercell(const SINT16 x, const SINT16 y);
extern Supercell* strtGetSupercellFULL_OBJ_ID(const FULL_OBJ_ID& fullObjID);
extern CPStruct* strtGetCP(const FULL_OBJ_ID& index);
extern CPStruct* strtGetCPorBridge(const FULL_OBJ_ID& index);
extern FacilityStruct* strtGetFacility(const FULL_OBJ_ID& index);
extern ObjectStruct* strtGetObject(const FULL_OBJ_ID& index);
extern LinkStruct* strtGetLink(const FULL_OBJ_ID& index);
extern FULL_OBJ_ID* strtGetFullObjID(const FULL_OBJ_ID& index);
extern CPStruct* strtGetCPFromTarget(const FULL_OBJ_ID& index);

//same as abive, only using FULL_ID
extern CPStruct* strtGetCP_FULL_ID(const FULL_ID& index);
extern FacilityStruct* strtGetFacility_FULL_ID(const FULL_ID& index);
extern ObjectStruct* strtGetObject_FULL_ID(const FULL_ID& index);
extern LinkStruct* strtGetLink_FULL_ID(const FULL_ID& index);

//Initialization functions for the strat system
extern void strtInitCP(CPStruct* cp);
extern void strtInitFacility(FacilityStruct* facility);
extern void strtInitObject(ObjectStruct* object);
extern void strtInitLink(LinkStruct* link);
extern void strtInitConvoy(CONVOYSTRUCT* convoy);
extern void strtInitPayload(PAYLOADSTRUCT* payload);
extern void strtInitManifest(MANIFESTSTRUCT* manifest);

//Conversion routines
extern FULL_ID strtFullObjIDToFullID(FULL_OBJ_ID fullObjID);

//strtDTL initialization finctions
extern LOGICAL strtInitCPStructwithCP(CPStruct* newcp, struct CP* cp);
extern LOGICAL strtInitFacilityStructwithFACILITY(FacilityStruct* newfacility, struct FACILITY* facility);
extern LOGICAL strtInitObjectStructwithBUILDING(ObjectStruct* newobject, struct BUILDING* building);
extern LOGICAL strtInitLinkStructwithDEPOT_LINK(LinkStruct* newlink, struct DEPOT_LINK* depotlink);

//strtWOR Initialization functions
//These functions looks up the information for strat objects from
//	the WOR parser that Marty gave us
extern LOGICAL strtInitCPfromWOR(CPStruct& cp);
extern LOGICAL strtInitFacilityfromWOR(FacilityStruct& facility);
extern LOGICAL strtInitObjectfromWOR(ObjectStruct& object);
extern LOGICAL strtInitLinkfromWOR(LinkStruct& link);

// get the total cps that are loaded
extern UINT32 strtNumCPs(void);

extern CPStruct* strtGetCPByName(const char* name);

extern void strtAddCPoid(CPStruct* newEntry, UINT16 entryNum);
extern void strtAddFacilityoid(FacilityStruct* newEntry, UINT16 entryNum);

// Find a CP by its database uoid
extern CPStruct* strtGetCPByOID(UINT16 uOID);
// Find a facility by its database uoid
extern FacilityStruct* strtGetFacilityByOID(UINT16 uOID);
// Locate the next facility, starting at uStartOID, which matches the string name
extern FacilityStruct* strtNextMatchingFacility(const char*, UINT16);

extern UINT32 strtReportDataErrors();

extern UINT32 g_maxCPOID;

//tolua_end
