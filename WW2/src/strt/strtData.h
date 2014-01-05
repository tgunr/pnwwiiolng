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
struct CP;
struct FACILITY;
struct BUILDING;
struct DEPOT_LINK;

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

//same as above, only using FULL_ID
extern CPStruct* strtGetCP_FULL_ID(const FULL_ID& index);
extern FacilityStruct* strtGetFacility_FULL_ID(const FULL_ID& index);
extern ObjectStruct* strtGetObject_FULL_ID(const FULL_ID& index);
extern LinkStruct* strtGetLink_FULL_ID(const FULL_ID& index);

//Conversion routines
static inline FULL_ID strtFullObjIDToFullID(FULL_OBJ_ID fullObjID)
{
	FULL_ID fullID { fullObjID.cell.ox >> 4, fullObjID.cell.oy >> 4, fullObjID.id, 0 };
	return fullID;
}

//strtDTL initialization functions
extern void strtInitCPStructwithCP(CPStruct& newcp, CP& cp);
extern void strtInitFacilityStructwithFACILITY(FacilityStruct& newfacility, FACILITY& facility);
extern void strtInitObjectStructwithBUILDING(ObjectStruct& newobject, const BUILDING& building);
extern void strtInitLinkStructwithDEPOT_LINK(LinkStruct& newlink, const DEPOT_LINK& depotlink);

//strtWOR Initialization functions
//These functions looks up the information for strat objects from
//	the WOR parser that Marty gave us
extern bool strtInitCPfromWOR(CPStruct& cp);
extern bool strtInitFacilityfromWOR(FacilityStruct& facility);
extern bool strtInitObjectfromWOR(ObjectStruct& object);
extern bool strtInitLinkfromWOR(LinkStruct& link);

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
