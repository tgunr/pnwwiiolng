#pragma once
// Created: Tue Dec  7 17:23:46 EST 2004
//
// Enumerations etc for client/cell/map mobile spawn exchanges
//
// Copyright (C) 2004 Playnet, Inc., All Rights Reserved.
// Not to be redistributed under any form except as part of
// a compiled library.
//
// Originating Programmer: Oliver "kfsone" Smith

// Mobile spawn flags: Spawner states
enum MSPAWN_STATE	// (Mobile Spawn STate)
{
	MSST_UNDEPLOYED = 0			// Vehicle not deployed
,	MSST_DEPLOYED				// Vehicle deployed
,	MSST_DEPLOYING				// Vehicle requesting deployment
} ;

// Mobile spawn deployment responses
enum MSPAWN_RESULT	// (Mobile Spawn Deployment Results)
{
	MSDR_INVALID = 0		// If you set this, you are scum
,	MSDR_GRANTED			// Deployment was granted, you are deployed
,	MSDR_FAILED				// Failed, internal error
,	MSDR_VEHICLE			// Cannot deploy this vehicle type
,	MSDR_DISABLED			// Mobile spawning is disabled
,	MSDR_DENIED				// You are prohibited from spawning
,	MSDR_NO_MISSION			// You must be on a mission
,	MSDR_DEAD				// Must be alive to deploy spawn
,	MSDR_RANK				// Rank too low to mobile spawn
,	MSDR_OWNER				// Lost ownership of origin facility
,	MSDR_TFR				// [out of] Target Facility Range
,	MSDR_EFR				// [out of] Enemy Facility Range
,	MSDR_1PER				// There can be only one (spawn per mission)
,	MSDR_EOC				// Enemy must Own or Control target
,	MSDR_LINK				// Must be a supply link between origin/target
,	MSDR_VLINK				// Supply link is severed
,	MSDR_MOVING				// Cannot deploy while moving
,	MSDR_STATIONARY			// Cannot deploy while stationary (paratroops)
,	MSDR_DISPLACED			// [From deployed state] You have been displaced
,	MSDR_UNDEPLOYED			// Successfully undeployed
,	MSDR_LAPSED				// Your connection timed out
,	MSDR_NOT_COMMANDER		// Crew cannot deploy mobile spawn
,	MSDR_MISSION_EXPIRED	// Mission has expired
,	MSDR_TARGET_DESTROYED	// Firebase target has been destroyed
,	MSDR_TARGET_INVALID		// Your target is not a valid target
,	MSDR_INVALID_STATE		// Can't do that in this state

,	MSPAWN_RESULT_MAX
} ;

