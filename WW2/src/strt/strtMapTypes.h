#pragma once
// Date: April 18 2000
//
// These are all the data types that wll be utilized by the Strat System.
// All structures and enumerated types should go here, as well as any defines
// that impact the entire system.
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Marty "Vandal" Poulin

#include "mm/mmSpawnTypes.h"
#include "miss/missionTypes.h"
#include "wwii/wwiiRTBCodes.h"

struct SPAWN_DENY
{
	SINT16	reasonCode;		//The reason they can't spawn
};

struct MM_POST_REQUEST
{
	PermissionMask	permMask;
	ChatHandle		handle;

	UINT32			customerID;	// Customer ID, so the mmhost can find the person
								//	again

	UINT32			missionID;	// Mission ID of the user

	FULL_OBJ_ID		facility;	//The facility from which they wish to
								//spawn
	WEAPON_SPEC		weapon;		//The weaponType they want to take
	LOGICAL			multicrew;		// if TRUE we are multicrew wating for a crew
	LOGICAL			immediateSpawn;	// If true, the person is spawned immediately

	UINT32			brigadeID;	// Brigade ID of the user
};

struct MM_POST_ACCEPT
{
	UINT32		customerID;		//Customer ID, so the mmhost can find the person
								//	again
	FULL_OBJ_ID	facility;
	WEAPON_SERIAL weaponSerial;	//The weapon serial, the bane of my
								//	existence
	SINT16		portal;
	LOGICAL		multicrew;		// if TRUE we are multicrew wating for a crew
	LOGICAL		immediateSpawn;	// If true, the person is spawned immediately

};

struct MM_POST_DENY
{
	UINT32		customerID;	//Customer ID, so the mmhost can find the person
							//	again
	WEAPON_SPEC	weapon;		//The weapon that was denied
	UINT32		reasonCode;	//The reason it was denied
};

//This message tells the strat system to cancel a post
struct MM_POST_CANCEL
{
	WEAPON_SERIAL weaponSerial;
};

//This message requests a list of posted vehicles at a facility
struct MM_POST_LIST_REQUEST
{
	ChatHandle	crewHandle;
	UINT32 	crewID;
	UINT32	missionID;
	FULL_OBJ_ID	facility;
	UINT32	brigadeID;
};

//This message asks for information on a posted weapon
struct MM_POST_INFO_REQUEST
{
	ChatHandle	handle;		// Compressed chat handle of sender
	UINT32	customerID;
	WEAPON_SERIAL weaponSerial;
	LOGICAL join;	//Whether we are going to use this information to join
					//	a vehicle or cancel a request
};

//This message asks for information on a posted weapon
struct MM_POST_INFO_UPDATE
{
	UINT32	customerID;
	WEAPON_SERIAL weaponSerial;
	LOGICAL multicrew;
};

//this message is information on a posted weapon
struct MM_POST_INFO_RESPONSE
{
	UINT32	customerID;
	MM_POSTED_WEAPON postedWeapon;
	LOGICAL join;	//Whether we are going to use this information to join
					//	a vehicle or cancel a request
};

struct MM_JOIN_LIST_RESPONSE
{
	playerid_t	commandID;
	playerid_t	crewID;
	ChatHandle	crewHandle;
};

//The mmhost uses this message to assign another person to that
//	weapon
struct MM_JOIN_REQUEST
{
	WEAPON_SERIAL	weaponSerial;
	UINT32	customerID;
};

//This reply comes back if a join request is accepted
struct MM_JOIN_ACCEPT
{
	UINT32			customerID;
	MM_POSTED_WEAPON	postedWeapon;	//The final posted weapon
};


//This reply comes back if a join request is denied
struct MM_JOIN_DENY
{
	UINT32 customerID;
	UINT32 reasonCode;
};

//This message comes to the mmhost if a post expires
struct MM_POST_EXPIRED
{
	UINT32 customerID;
	WEAPON_SERIAL weaponSerial;
	bool cancelled;
};

//This message comes from the mmhost asking to spawn a specific vehicle
struct MM_SPAWN_GRANT_REQUEST
{
	WEAPON_SERIAL	weaponSerial;
	FULL_OBJ_ID		facility;
	UINT32			missionID;
	UINT32			playerID;
};

struct MM_SPAWN_GRANT_RESPONSE
{
	WEAPON_SERIAL	weaponSerial;
	UINT32			serverIP;
	UINT32			commander;
	UINT32			crew;
	SINT32 			capacity;
	SINT32			code;
};

struct MM_SPAWN_DENY
{
	WEAPON_SERIAL	serial;
	UINT32			code;
};

//This message tells the strat system to cancel a spawn
struct MM_SPAWN_CANCEL
{
	UINT32	customerID;
	FULL_OBJ_ID	facility; //The facility the mmhost wants information on
	WEAPON_SERIAL weaponSerial;
};

//Message telling the map server to tell the commander front end that his
//	crew has bailed
struct MM_CREW_BAILED
{
	UINT32 comanderID;
	UINT32 crewID;
};

//This is what the map/mission host uses to get a peek at a
//	facility's overall worth and current operating capacity
#pragma pack(push, 1)
struct MM_CAPACITY_REQUEST
{
	UINT32	tag;			//Something echoed back, so the mmHost knows what to do
							//	with the response
	UINT32	missionID;		//Echoed back again
	UINT32	customerID;		//Echoed back
	FULL_OBJ_ID	facility;	//The facility the mmhost wants information on
	CODE_RTB	rtb;		//RTB code
};

//The response from the MM_CAPACITY_REQUEST.  All fields are echoed back,
//	and the capacity and totalRPs are listed
struct MM_CAPACITY_RESPONSE
{
	UINT32	tag;	//Something echoed back, so the mmHost knows what to do
					//	with the response
	UINT32	missionID;	//Echoed back again
	UINT32	customerID;	//Echoed back
	FULL_OBJ_ID	facility; //The facility the mmhost wants information on
	SINT32	capacity;	//The capacity of the facility.  If negative, there's
						//	an error
	UINT32	totalRPs;	//The total # of RPs it takes to rebuild the facility
	FULL_OBJ_ID	rtbFacility;
	WEAPON_SERIAL	rtbReservedWeapon;
	bool	vehicleDelivered;	// Was the weapon delivered to brigade (manually supplied)
};
#pragma pack(pop)

