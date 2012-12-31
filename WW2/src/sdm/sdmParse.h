#pragma once
// Created: Fri Mar  9 10:45:43 2001
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Eric "Meat" Hall

#include "strt/strtDTL.h"

//Open File return values
#define SDMFILE_READ_ERR		-1
#define SDMNO_STRAT_DATA		1
#define SDMFILE_TRANS_ERR		2		// Translation errors

//Parser "Modes"
#define SDMPARSE_CP			0x0001
#define SDMPARSE_FACILITY		0x0002
#define SDMPARSE_BUILDING		0x0004
#define SDMPARSE_DEPOT_LINK	0x0008
#define SDMPARSE_WAYPOINT		0x0010
#define SDMPARSE_ALL			0x001F

// Initializes the parser for file whose name is the string pointed ot by <path>
// mode is currently not supported
//returns 0 on successful read
extern SINT32 sdmOpenFile(const SCHAR* path,UINT16 mode);

//Accessor functinos to get at data in the file.  Names should be self-describing,
//  will comment in more detail when time permits

extern const struct STRT_DTL_HEADER& sdmGetDtlHeader() ;

extern UINT16 sdmGetNumCP(void);
extern CP* sdmGetNextCP(void);

extern UINT16 sdmGetNumFACILITY(void);
extern FACILITY* sdmGetNextFACILITY(void);
extern FACILITY* sdmGetFACILITYArray(void);

extern UINT16 sdmGetNumBUILDING(void);
extern BUILDING* sdmGetNextBUILDING(void);
extern BUILDING* sdmGetBUILDINGArray(void);

extern UINT16 sdmGetNumDEPOT_LINK(void);
extern DEPOT_LINK* sdmGetNextDEPOT_LINK(void);

extern UINT16 sdmGetNumWAYPOINT(void);
extern WAYPOINT* sdmGetNextWAYPOINT(void);

extern UINT32 sdmGetTotalStratObjects(void);
extern UINT32 sdmGetMaxStratId(void);

//closes file and frees all memory used by parser
extern SINT32 sdmCloseFile(void);

// Clean up after the SDM routines and exit it
// there were any cumulatively fatal errors.
extern void sdmFinish() ;

#include <string>
extern const std::string& sdmTranslateName(const std::string& from) ;
