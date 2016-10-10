#pragma once
// Date: Wed Apr  5 04:29:40 2000
//
// This is the main header file for the Strat System.
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson

class CPStruct;
class Supercell;

// Number of CPs per pass that the main loop will process, effectively
// spreading out the processing load of CPs over a longer time
// Thoughts: maybe make this a percentage, or maybe a time-delay?
#define		CPSPERCALL 18

// The main Strat Call that the Convoy System makes.  This is the
// procedure that actually runs a certain amount of CPs before returning.
extern bool strtMain(UINT32 currentTime);

//tolua_begin

extern const char* HostLanguage;

//Broadcasts the CP_STATUS messages for every cp to everybody
extern void strtBroadcastCPs(void);

extern CPStruct* strtGetCPList(void);
extern Supercell* strtGetCellList(void);

extern FULL_OBJ_ID g_fullObjID;

extern bool strtIsStratInitialized();

//tolua_end

extern void strtInitializeStratSystem();
extern void strtShutdownStratSystem();

