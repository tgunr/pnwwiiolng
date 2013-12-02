#pragma once
// Created: Mon May  8 13:46:07 2000
//
// This is the header file for the main Convoy System
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson

#include "strt/strtProtos.h"
struct WEAPON_SPEC;
class WeaponCounter;
struct STRAT_UPDATE;

//Initializes the convoy system
extern LOGICAL cnvyInit(void);

//This is the call the Strat System will make to create a convoy.  It takes
//	all the information to create a convoy and moves it to the appropriate
//	lists
extern LOGICAL cnvyCreateConvoy(WWII_SIDE owner, WWII_COUNTRY country, LinkStruct* link, struct PAYLOADSTRUCT* payload);

//This will be the function that the strat System calls to free a convoy after
//	its supplies have been transported to the depot
extern LOGICAL cnvyFreeConvoy(struct CONVOYSTRUCT* convoy);

//This function takes RPs and returns a payload for the item
extern PAYLOADSTRUCT* cnvyMakeRPPayload(UINT32 RPs);

//Get the map host connection
extern SINT32 cnvyGetMapConnection(void);
//Set the map connection
extern SINT32 cnvySetMapConnection(SINT32 connection);

//tolua_begin
//This function adds a weapon to the Expire List
extern bool cnvyAddWeaponToExpireList(struct POSTED_WEAPON& weapon, UINT32 uExpireSeconds);
//This function removes a weapon from the Expire List
extern void cnvyRemoveWeaponFromExpireList(struct POSTED_WEAPON& weapon);

// Request the begining of processing for a factory cycle
// Seconds specifies time until first processing; afterwards host_config.factoryPeriod is used
extern LOGICAL cnvyBeginFactoryProduction(ObjectStruct* object, UINT32 seconds);

//This function interrupts the waiting in the main convoy loop.  Its
//	gonna be used to make sure that the incremental strat additions get
//	processed normally
extern LOGICAL cnvyInterrupt(void);

//this checks the value of the flag
extern LOGICAL cnvyInterrupted(void);

//this resets the interrupt flag
extern LOGICAL cnvyClearInterrupt(void);

//Various processing functions to run the whole of the convoy and
//	strat system

//This function processes convoys, if necessary
extern void cnvyProcessConvoys();

//This function processes strat updates, if necessary
extern LOGICAL cnvyProcessStratUpdates(UINT32 currentTime);

// Request expiry of an objective at a later time
extern bool cnvyScheduleObjectiveExpiry(CPStruct* cp, WWII_SIDE side, UINT32 expirySeconds);

extern STRAT_UPDATE* cnvyScheduleBrigadeReformation(UINT32 brigadeID, UINT32 delay);

extern void cnvyCancelBrigadeEvent(struct STRAT_UPDATE* update);

extern void cnvyUpdateTime();
//tolua_end
