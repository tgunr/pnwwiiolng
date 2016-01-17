#pragma once
// Created: Tue Apr 17 13:57:03 2001
//
// These are all the* shared* types for spawning and spawn negotiaion.
// Thus, there will probably be* only* messages in here.
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson

#include "weap/weapDefs.h"		// For MAX_WEAP_STATIONS
#include "weap/weapSerial.h"
#include "chat/chatHandle.h"	// For MAX_HANDLE_SIZE

enum
{
	MM_JOIN_FAIL_CANT_FIND_POSTED_WEAPON,
	MM_JOIN_FAIL_POSTER_WEAPON_FULL,
	MM_JOIN_FAIL_POSTER_DENIED_ACCESS,
	MM_JOIN_FAIL_POST_EXPIRED,
	MM_JOIN_FAIL_UNKNOWN_ERROR,
	//Add more reasoncodes here
	MM_MAX_JOIN_DENY_REASONCODE
};

enum POST_ACCEPT_TYPE
{
	MM_ACCEPT_NEW
,	MM_ACCEPT_UPDATE
,	MM_ACCEPT_RTB_RESERVED
};

//-----------------------------STRUCTURE SECTION--------------------------

enum COMMANDED_WEAPON_ACTION
{
	COMMANDED_WEAPON_ADD,
	COMMANDED_WEAPON_DELETE
};

struct COMMANDED_WEAPON
{
	COMMANDED_WEAPON_ACTION action;
	ChatHandle		handle;
	WEAPON_SERIAL	weaponSerial;
};

//_-----------------------------MESSAGE SECTION---------------------------
//From the front end to the mmHost requesting the post of a vehicle
struct FE_POST_REQUEST
{
	FULL_OBJ_ID	facility;	//The facility from which they would like to
							//	take the weapon
	WEAPON_SPEC	weapon;		//The weapon they want to take
	LOGICAL		immediateSpawn;	// If true, the person is spawned immediately
};

//The message received by the front end apprising them of an accepted post
struct FE_POST_ACCEPT
{
	WEAPON_SERIAL	weaponSerial;	//The weapon serial, the bane of my
									//	existence
	LOGICAL			newVehicle;
	SINT16			portal;
	LOGICAL			mobileSpawnAvailable;
};

//The message from the front end that cancels a post
struct FE_POST_CANCEL
{
	FULL_OBJ_ID	facility;
	WEAPON_SERIAL weaponSerial;
};

//The message received by the front end apprising them of an denied post
struct FE_POST_DENY
{
	WEAPON_SPEC	weapon;		//The weapon that was denied
	UINT32		reasonCode;	//The reason it was denied
	LOGICAL		loseWeapon;	// Whether you lose the weapon or not
};

//This message requests a list of posted vehicles at a facility
struct FE_POST_LIST_REQUEST
{
	FULL_OBJ_ID	facility;
};

//This message contains a list of posted vehicles at a facility
struct FE_POST_LIST_RESPONSE
{
	FE_POST_LIST_RESPONSE(const FULL_OBJ_ID& _facility, SINT32 _numWeapons, COMMANDED_WEAPON* const _commandedWeapon)
		: facility(_facility), numWeapons(_numWeapons), commandedWeapon(_commandedWeapon) {}
	FE_POST_LIST_RESPONSE()
		: facility(nullObjID), numWeapons(0), commandedWeapon(nullptr) {}

	FULL_OBJ_ID	facility;
	SINT16	numWeapons;		//Number of weapons posted
	COMMANDED_WEAPON* commandedWeapon;	//Allocated array of posted weapons
};

//This message is sent asking to join a posted vehicle.  The mmhost then
//	will broker the request between the two front ends AND the strat
//	system
struct FE_JOIN_REQUEST
{
	WEAPON_SERIAL	weaponSerial;
};

//This message is sent asking to join a posted vehicle.  The mmhost then
//	will broker the request between the two front ends AND the strat
//	system
struct FE_CREW_REQUEST
{
	WEAPON_SERIAL	weaponSerial;
	SINT32			numCrewNeeded;
};

struct POSTED_WEAPON;

struct MM_POSTED_WEAPON
{
  MM_POSTED_WEAPON(const WEAPON_SERIAL& _weaponSerial, SINT16 _portal, UINT32 _commander, UINT32 _missionID, UINT32 _crew)
    : weaponSerial(_weaponSerial), portal(_portal), commander(_commander), missionID(_missionID), crew(_crew) {}
  MM_POSTED_WEAPON()
    : weaponSerial(NULL_WEAP_SERIAL), portal(0), commander(0), missionID(0), crew(0) {}

	void copy(const POSTED_WEAPON& rhs);

	WEAPON_SERIAL	weaponSerial;	//Marty's WEAPON_SERIAL,
	SINT16			portal;		//the portal from which it'll spring

	UINT32			commander;	//The commander of this vehicle
	//Used for secondary verification of spawn ability
	UINT32			missionID;	//The missionID of the commander
	UINT32			crew;		//The crewmember of this vehicle
};

//This reply comes back if a join request is accepted
struct FE_JOIN_ACCEPT
{
	MM_POSTED_WEAPON	postedWeapon;	//The final posted weapon
};

//This reply comes back if a join request is denied
struct FE_JOIN_DENY
{
	UINT32 reasonCode;
};

//This message comes from the mmhost if a post expires
struct FE_POST_EXPIRED
{
	WEAPON_SERIAL weaponSerial;
};

//This message asks a front end if he wants a player to join his posted
//	vehicle
struct FE_JOIN_PERM_REQUEST
{
	UINT32		joinRequestID;	//The ID this join request has been given
	ChatHandle	chatHandle;
};

//This is the response as to whether the front end will allow them to join
struct FE_CREW_ACCEPT
{
	UINT32	joinRequestID;
};

//This is the kill message for a player
struct FE_JOIN_PERM_KILL
{
	UINT32	joinRequestID;
};

//This is the message telling the mmhost that the client wants to spawn
struct FE_CLIENT_SPAWN
{
	WEAPON_SERIAL	weaponSerial;
	UCHAR			wantMobileSpawn;
};

//This is the notification that the front end gets when it is spawning
#pragma pack(push, 1)
class FESpawnNotify
{
public:
	FESpawnNotify(WEAPON_SERIAL const& _ws = NULL_WEAP_SERIAL
					, UINT32 _vehicle = 0
					, FLOAT64 _x = 0, FLOAT64 _y = 0, FLOAT64 _z = 0
					, UCHAR _decal = 0
					, bool isHighCommander = false, bool isBuilder = false, bool isMissionleader = false);
	FESpawnNotify(ACTION_OBJECT* const a);
	~FESpawnNotify() {}
	SINT32 packer(LOGICAL mode, ACTION_OBJECT* a);

public:
	WEAPON_SERIAL	weaponSerial;	// What you're spawning
	UINT32			spawnVehicleID;	// 0 or weap-serial of vehicle you are spawning at
	FLOAT64			x, y, z;		// If spawnVehicleID != 0 - spawning coordinates
	UCHAR			decal;			// 0 for no decal
	UCHAR			isHighCommander;
	UCHAR			isBuilder;
	UCHAR			isMissionLeader;	// equals isMyMissionLeader too!
};
#pragma pack(pop)

//Used by the mmHost to notify a commander that his crew has bailed
struct FE_CREW_BAILED
{
	UINT32 joinRequestID;
	UINT32 reasonCode;
};

