#pragma once
// Created: Wed Apr 18 13:44:06 2001
//
// These are the messages the front end sends and receives in congress with
// the map server to negotiate spawning
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson

namespace Teul
{
	class Connection;
};

struct FULL_OBJ_ID;
struct WEAPON_SPEC;
struct WEAPON_SERIAL;
struct FLOAT64VECTOR;

//Callback installer
extern SINT32 mmClientInstallCallbacks(void);

//this function sends the post request to the mmhost
extern SINT32 mmSendPostRequest(SINT32 connection, const FULL_OBJ_ID& facility, const WEAPON_SPEC& weapon
							, LOGICAL immediateSpawn);

//this function sends the post list request to the mmhost
extern SINT32 mmSendPostListRequest(SINT32 connection, const FULL_OBJ_ID& facility);

//this function sends the join request to the mmhost
extern SINT32 mmSendJoinRequest(SINT32 connection, const WEAPON_SERIAL& weaponSerial);

//this function sends the join perm response to the mmhost
extern SINT32 mmSendAcceptCrew(SINT32 connection, UINT32 joinRequestID);

//this function sends a kill for a join request
extern SINT32 mmSendJoinPermKill(SINT32 connection, UINT32 joinRequestID);

//this function sends a cancel for a post, join etc
extern SINT32 mmSendVehicleCancel(SINT32 connection, const WEAPON_SERIAL& weaponSerial);

//this function sends a client spawn
extern SINT32 mmSendClientSpawn(SINT32 connection, const WEAPON_SERIAL& weaponSerial, LOGICAL wantMobileSpawn);

//this function sends for a list of joiners (crew) and puts the commander lists for
//joiners to see.
extern SINT32 mmSendRequestCrew(SINT32 connection, const WEAPON_SERIAL& weaponSerial, UINT32 numCrewNeeded);

extern SINT32 mmSendDirectSpawn(SINT32 connection, const WEAPON_SPEC& weapon, const FLOAT64VECTOR* location);

// tell the host we have despawned
extern void mmSendDespawn(Teul::Connection* conn);

