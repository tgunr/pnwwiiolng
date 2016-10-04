#pragma once
// Copyright 1999 Playnet Inc, All rights reserved
// Not to be distributed in API form except as part of a compiled
// library
//
// Originating Programmer: Martin Poulin

#include "miss/missionPacket.h"
#include "teul/teulTime.h"
#include "wwii/wwiiExitCodes.h"		// For PLAYER_EXIT_CODE
#include <list>

// force clients out after 35 seconds
//#define FORCE_CLIENT_TIMEOUT 30000
static const TeulTimeMS FORCE_CLIENT_TIMEOUT = 35 * 1000;

#define MAX_HOST_VEHICLES 6000

#define	WWIIOL_ARENA	1
#define REINIT_SERVER_LEVEL 5

#define ACTIVE_TO_REALLY_ACTIVE_TIME_S 21

extern bool g_serverReserved;
extern bool g_clustered;

class HostVehicle;
class Player;
struct VEHICLE_HOLDER;
struct HIT_VEHICLE_NODE;
struct WEAP_BIAS;
struct UPDATE_LIST;

typedef std::list<Player*> PLAYER_LIST;

struct VEHICLE_HOLDER
{
	HostVehicle* vehicle;
	UINT32	serial;

	// for the broadcast list we need these
	UINT16 	timeStamp;
	UINT16	posTimeStamp;
	UINT16	turretMask;
	UINT16	fireMask;
	SINT8	numTurrets;
	UINT32	present;	// present flags from update
};

#ifdef NEW_SERVER_INFO
struct NEW_SERVER_INFO
{
	NEW_SERVER_INFO* prev;
	NEW_SERVER_INFO* next;
	VEH_BROADCAST_LIST	bcastList;
	UINT64	id;
	SINT32	offset;
	SINT32	syncCount;
	HASH_D1*	syncSent;
};
#endif

struct MY_SERVER_INFO
{
	UINT32 recvVehBcastErrors;
	UINT32 initStartTime;
	UINT32 packetTag;
};

extern SINT32			authConnection;
extern SINT32 updateBcastRecv, updateBcastSend;
extern MY_SERVER_INFO msi;
extern LOGICAL serverResync;

extern SINT32 hostGetMapConnection(void);
extern UINT32 hostGetStratAddress(void);

extern void	hostActivateVehicle(HostVehicle* newVehicle, bool checkPlayers);
extern HostVehicle* hostFindVehicleLocal( UINT32 indexValue );
extern SINT32 hostJoinVehicle(Player& persona, const struct VEHICLE_REQUEST& vRequest);
extern HostVehicle* hostCreateRemoteVehicle(const Instance::Descriptor& instance_, sortieid_t index, WEAPON_SPEC spec);
extern void hostRemoveRemoteVehicle(UINT32 serial, PLAYER_EXIT_CODE exitCode);
struct VEHICLE_UPDATE_HEADER;
extern bool hostAddVehicleToBroadcast(HostVehicle* vehicle, VEHICLE_UPDATE_HEADER* vh);

extern void	hostActivatePlayer(Player* persona);

extern SINT32 hostPlayerLeave(Player& player, struct PLAYER_EXIT& pe);

extern void hostSmiteCustomer(UINT32 customerID);

extern void hostForceVehExit(HostVehicle* vehicle);

extern void hostDeleteToastVehicle(HostVehicle* vehicle);

extern void hostBuildingVisLevelChange(class ObjectStruct* object, const instanceid_t inInstance);
extern void hostChangeFPSBuilding(Player* persona, const FULL_OBJ_ID newBuildingID);

extern void* teulGetPersonaData(SINT32 teulIndex);
static inline Player* hostGetPlayer(SINT32 teulIndex) { return (Player*)teulGetPersonaData(teulIndex); }
extern void hostGiveCaptureScoreToPlayers(const FULL_OBJ_ID buildingID, WWII_SIDE forSide);
extern void hostSendCaptureStatesToOccupants(const class CaptureState& incoming);

#include "host/hostPlayers.h"

/*

The problems here are pretty straight forward.

1. 	How do we add and delete people or vehicles from and array without having
	to search the array for empty nodes.  How do we attach these entries to
	vehicles simple?

1a:	Reference them also from a linked list that manages released nodes
	by putting them at the head of the list for recycling.  The same next/prev
	header entries are also used to link personas to vehicles, and vehicles to
	locations.

2.	How do we  ensure that a recycled ID doest get mistaken for the previous
	ID?

2a:	Split the ID into 2 parts,  i.e. the lower 14 bits could actually designate the
	location in the array, and the upper 2 bits that are incremented each time
	the node changes hands to provide uniqueness.  This way if an event comes
	in for a defunct vehicle the ID will not match the ID value referenced in the
	array.


3.	How do we inform the client that a change has occurred in one of the vehicles
	on its request list.

3a:	We send an extra bit that is transmitted with each update that tells weather
	the vehicle is "dirty".  If the vehicle is dirty the client will send an info
	request for the updated info.  When the info request for that vehicle is obtained
	the
*/

