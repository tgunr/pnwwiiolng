#pragma once
// Date: Fri Dec  2 18:12:40 CST 2005
//
// Handle strat sync packets
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "strt/strtConstants.h"

#pragma pack(push, 1)
struct CP_SINFO
{
	UINT16				uOID;		// Database ID
	UCHAR				owner;		// list of countries indicating ownership
	UCHAR				control;	// list of countries indicating control
	UINT32				flags;		// flags including contention
};

struct STRT_SYNC_DATA
{
	SINT16	sx, sy;				// supercell co-ordinates of this data
	UINT16	numEntries;			// number of entries in the message
	UINT16	startID;				// id that the message starts at
};

struct STRT_SYNC_CPS : public STRT_SYNC_DATA
{
	CP_SINFO info[STRT_CPS_PER_SYNC];
};

struct STRT_SYNC_FACILITIES : public STRT_SYNC_DATA
{
	UINT16	uOID[STRT_FACILITIES_PER_SYNC];
	UCHAR	country[STRT_FACILITIES_PER_SYNC];		// list of countries indicating ownership
	UINT8	captureStage[STRT_FACILITIES_PER_SYNC];
	UINT16	captureCounter[STRT_FACILITIES_PER_SYNC];
	UINT32	captureStateChanged[STRT_FACILITIES_PER_SYNC];
};

struct STRT_SYNC_BUILDINGS : public STRT_SYNC_DATA
{
	UCHAR	vislevel[STRT_BUILDINGS_PER_SYNC];		// list of countries indicating ownership
};
#pragma pack(pop)

extern void strtProcessSyncCPs(const STRT_SYNC_CPS& syncCPs);
extern void strtProcessSyncFacilities(const STRT_SYNC_FACILITIES& syncFacilities);
extern void strtProcessSyncBuildings(const STRT_SYNC_BUILDINGS& syncBuildings);

