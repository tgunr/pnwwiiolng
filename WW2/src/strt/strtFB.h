#pragma once
// Created: Tue Jan 16 12:59:44 2001
//
// All the firebase calls for the strat system
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson

//#define FRIENDLY_FB_WAVE

enum STRAT_FIREBASE_INIT_MODE
{
	ALLIED_OWNED=1,
	AXIS_OWNED,
	RANDOM_OWNED,
	ALL_OPEN,
	DB_CHECKED,
	//Add new ones here
	MAX_STRAT_FIREBASE_INIT_MODE
} ;

//tolua_begin

//This function sets all firebases open that need to be open across the
//	Strat System
//	If mode is 1, then all Allied Firebases are opened
//	If mode is 2, then all Axis Firebases are opened
//	If mode is 3, then firebases are randomly opened
extern LOGICAL strtInitStratSystemFirebases(CPStruct* stratSystem);

//This function Sets a firebase on if all conditions are correct (competing
//	CPs, etc), AND the Firebase is controlled by the side indicated by "side"
//extern LOGICAL strtSetFBOnControl(FacilityStruct* firebase, WWII_SIDE side);

//This fucntion opens or closes a firebase at the beginning of the game
//	NOTE: this should NOT be used to open or close a firebase during
//		the running of the strat system
//	if mode is TRUE, then the firebase is opened
//	if mode is FALSE, the firebase is closed
extern void strtInitFirebase(FacilityStruct& firebase, bool isOpen);

//This function closes a firebase that is currently open in a RUNNING
//	strat system
extern void  strtCloseFirebase(FacilityStruct& closingFirebase);

//This function opens a firebase that is currently closed in a RUNNING
//	strat system
extern void strtOpenFirebase(FacilityStruct& openingFirebase);

//This function is called to clear the firebases on BOTH ENDS of a PATH
//	This function acts on both link the link its passed, and that links
//	partner
//extern LOGICAL strtClearFirebases(LinkStruct* link);
extern void strtClearFirebases(LinkStruct& link);

// Determine whom the new owner should be for an FB
extern WWII_COUNTRY strtNewFBOwner(FacilityStruct& fb) ;

//tolua_end

