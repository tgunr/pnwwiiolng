#pragma once
// Created: Wed Nov 1 16:55:26 2000
//
//	This is where all tyepes for universal game specific object structures
//	that	will be transmited accross the network must reside or be
//	included.  All pack routines must have an entry for a reference
//	object in this file.
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Marty "Vandal" Poulin

#include "weap/weapSerial.h"
#include "updt/updtEnums.h"
#include "updt/updtTurrets.h"

#if platBuild != BUILD_SERVER
	#include "wwii/wwiiTypes.h"
#endif

#include "strt/strtTypes.h"

#include "updt/updtFraming.h"
extern UpdateFraming framings[UpdateFraming::_MODES];

#include "updt/updtVehiclePosition.h"

// The number of vehicles sent per update packet is typically lower
// than the total number of vehicles being displayed on the client
// machine; the framing describes the number of levels we have, how
// many updates are sent for each level, provides the vehicles-per-level
// and the total vehicles. It must also stipulate whether a large
// packet is required for this framing format.
//
// NOTE: Levels also control the way the data is exchanged for the
// vehicles in that level.

struct VEHICLE_HIT
{
	SINT32	timeStamp;		//timestamp
	UINT32	shooter;
	UINT32	target;
	UINT32	NumComponents;
	UCHAR	compID[VEH_MAX_OBJ_HIT];
	UINT16	Damage[VEH_MAX_OBJ_HIT];
	UCHAR	NumHits[VEH_MAX_OBJ_HIT];
};

#pragma pack(push, 1)
struct TURRET_POS
{
	turret_t x;
	turret_t y;
};

struct RELATIVE_POSITION
{
	SINT16	relX;
	SINT16	relY;
	SINT16	relZ;
	SCHAR	yaw;
};

struct VEHICLE_VEL_HIGH // Velocity vector
{
	SINT16	velX;
	SINT16	velY;
	SINT16	velZ;
};

struct VEHICLE_ACCEL_HIGH	// Acceleration vector
{
	SINT16	accelX;
	SINT16	accelY;
	SINT16	accelZ;
};

struct VEHICLE_ATTVEL_HIGH	// Velocity vector
{
	SINT16	velX;
	SINT16	velY;
	SINT16	velZ;
};

struct VEHICLE_ATT_HIGH
{
	SINT16	attX;
	SINT16	attY;
	SINT16	attZ;
};
#pragma pack(pop)

struct TURRET_STRUCT
{
	// when valid data is received from a turret the mask is set to 1
	// when an individual turrest timestamp expires (rolls over) the mask is
	// set to 0
	TeulTimeMS		clock64[MAX_TURRETS];
	TURRET_POS	pos[MAX_TURRETS];
	UINT16		valid;
};

struct FIRE_STRUCT
{
	// when valid data is received from a turret the mask is set to 1
	// when an individual turrest timestamp expires (rolls over) the mask is
	// set to 0
	TeulTimeMS	masterClock64;
	TeulTimeMS	clock64[MAX_GUNS];
	UINT16	valid;
	UINT16	state;		// indicates if the gun has fired this frame?
};

//////////////////////////////////////////////////////////////////////
//! Vehicle Info structure defines elements of vehicles that are
//! shared between clients via the host.
//! @warning THIS STRUCTURE IS USED ON THE HOST, AND FOR EVERY REMOTE VEHICLE
//! @warning INSTANCE THE CLIENT CAN SEE.

struct SHARED_VEHICLE_INFO
{
	WEAPON_SPEC spec() const { return wSerial.spec; }

	TeulTimeMS			hostPosTimeStamp;

	UINT32				index; 		// assigned at initialization of list = array index
	UINT32				resolution;
	UINT32				rideIndex;
	UINT32				visBits;		// used for texture progressive damage scheme
	UINT32				squad;			// squad # 0 is none

#if platBuild != BUILD_SERVER
		///TODO: Move this elsewhere, we don't need one for every vehicle.
	SINT32				timerInfo;		// Whether or not* I* am depsawning
#endif

	UINT16				stateFlags0;	// indication of states like  and gear up/down.
	UINT16				stateFlags1;	// extended state info for more complicated vehicles
	UINT16				timeStamp;		// delta of mili-secs of FRONT_END
	UINT16				seatNumber;
	UINT16				posTimeStamp;	//	Last time we got a position update
	UINT16				globalFlags;
	UINT16				oneTimeBits;
#if platBuild == BUILD_SERVER
	UINT16				cmdrTimeStamp;  // On the host we need to keep track of the commander independently
#endif

//	UCHAR				loadOut;
	UCHAR				country;
	UCHAR				side;
	UCHAR				decal;
	UINT32				brigadeID;		// ID of the HCUnit this came from

	char				callsign[MAX_HANDLE_SIZE]; // SIZE includes null byte

	VEHICLE_POSITION	position;
	RANK_NO				rank;

	RELATIVE_POSITION	relPos;

	VEHICLE_VEL_HIGH	velocity;		// Velocity vector
	VEHICLE_ACCEL_HIGH	acceleration;	// Acceleration vector
	VEHICLE_ATTVEL_HIGH	rotational;		// anglular velocity
	VEHICLE_ATT_HIGH	attitude; 		// orientation of the veh

#if platBuild == BUILD_SERVER
	FULL_OBJ_ID			fpsBuildingID;
#else
	REL_OBJ_ID          fpsBuildingID;  // Full object ID of building FPS is in/on
#endif
	// Set this to true if the troop has an fpsBuildingID != 0 and they meet
	// the requirements to be able to capture the building (current, being
	// inside certain areas of it).
	bool                fpsInCaptureBuilding;

	struct FIRE_STRUCT	fire;
	struct TURRET_STRUCT	turret;

//	DAMAGE_HEAD			damage;

#if platBuild == BUILD_SERVER
	ChatHandle			squadHandle;
#else
	char				squadName[MAX_HANDLE_SIZE]; // SIZE includes null byte
#endif

	// one time structures...
	WEAPON_SERIAL		wSerial;	// Partially populated weapon serial for spec

#if platBuild == BUILD_SERVER
	bool				m_hideCallsign;
	ChatHandle			packedCallsign;
	class HostVehicle*	m_vehicle;
#endif
};

struct VEHICLE_TURRETS
{
	UINT16			numTurrets;
	UINT16			turretMask;
};

struct TURRET_HEADER
{
	SHARED_VEHICLE_INFO*		vehInfo;
	VEHICLE_TURRETS 	vt;
};

//////////////////////////////////////////////////////////////////////
// The vehicle transport header
// Essentially: Non-persistent data that is marshalled and transported
// Shared client/host.

struct VEHICLE_UPDATE_HEADER
{
#if platBuild == BUILD_SERVER
	SHARED_VEHICLE_INFO* vehInfo;
	class Player*		player;
#else
	SHARED_VEHICLE_INFO	vehInfo;
#endif

	PRESENT_FLAGS		present;
	UINT32				fireMask;	// indicates if the gun has fired this frame?

	VEHICLE_TURRETS		vt;
	VEHICLE_POSITION 	position;	// Position

	UINT32				resolution;
	UINT32				updateNo;
	UINT32				updatePrime;

	sortieid_t			killedBy;
};

namespace Teul { class Packet; }

struct WORLD_UPDATE
{
#pragma pack(push,1)
	struct Header
	{
		/// @brief X, Y, Z in cm of the center of the update.
		/// @detail All vehicle positions will be relative to this.
		VEHICLE_POSITION center;

		/// @brief 8 bits of randomness to ensure encryption of some kind.
		UINT8			random1;
		/// @brief Despawn Timer: -1 = can't, 0 = immediate, +ve = number of seconds
		SINT8			timerInfo;
		/// @brief Framing mode used for this update.
		UINT8			framingMode;
		/// @brief Number of vehicles in this update packet.
		UINT8			numVehicles;

		/// @brief Regardless of what's in this update, this is how many vehicles you are supposed to be seeing.
		UINT16			actualVehiclesInRange;
	} header;
#pragma pack(pop)

	/// @brief List of vehicles being removed.
	UINT32			numRemove;
	UINT32			removeList[MAX_VEH_REMOVE];

#if platBuild == BUILD_SERVER
	char			m_state[64];
	// Host version.
	class	HostVehicle*	myVehicle;
	class	Viewing*	viewings[MAX_VEH_PER_UPDATE];
	Teul::Packet*		packet;
#else

	// Client version.
	VEHICLE_UPDATE_HEADER	vehUpdate[MAX_VEH_PER_UPDATE];

#endif

	SINT32	packRemoveList(PackMode mode, ACTION_OBJECT* const a);
};

// if a fireTimeStamp is valid and hapened btween updateSent and now
// it must be sent now, otherwise the current state is sent.

// if a turret TimeStamp is valid and hapened between updateSend
// and now it is either sent or qued by seting its bit in turretsPending.
// if a coresponding bit in turests pending is set it is sent on some
// future update.
