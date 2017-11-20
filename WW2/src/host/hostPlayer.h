#pragma once
// Created: Tue Nov  8 13:43:34 CST 2005
//
// Definition of the cell host version of a Player.
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "wwii/wwiiFullObjID.h"

#include "miss/missFlags.h"
#include "wwii/wwiiExitCodes.h"	// For PLAYER_EXIT_CODE

class HostVehicle;
namespace Server { class PersonaXfer; class PersonaLogin; }

class Player : public wwiiPlayer
{
public:
	enum PERSONA_STATE
	{
		_UNINITIALIZED = -1,
		_FREE,
		_TX_MAP,
		_RESERVED
	};

public:
	void activate() { active = true; }

	// Check the player's situation to see if their vehicle needs to
	// be marked for AWS.
	void awsCheck();

	void changeFPSBuilding(const FULL_OBJ_ID newBuildingID, bool isInCaptureBuilding);

	UINT16 getCurrentFramingMode();
	UINT16 getPreferredFramingMode() { return m_framingMode; }

	bool isInGracePeriod(UINT32 gracePeriod_s, bool includeFirstConnection=true) const;

	void processUpdateValues(UINT32 updateNo, UINT32 prime);

	void resetUpdateSeq();

	void setFramingMode(UINT16 mode);
	void setVehicle(HostVehicle* const vehicle_);

	FULL_OBJ_ID fpsBuildingID() const { return m_fpsBuildingID; }

public:
	static SINT32 Create(const Server::PersonaLogin& xfer, class PlayerStatusNode& psn);
	Player() : wwiiPlayer(), vehicle(nullptr), m_antiHack(nullptr) {}
	Player(const Server::PersonaXfer& xfer);
	~Player();

public:
	TeulTimeMS				m_lastTimeRx;			// Timestamp on the player's client as of last rx.
	TeulTimeMS				m_lastTimeRxed;			// Host timestamp for the rx'ed value.
	TeulTimeMS m_nextMemReq = 0; // The next time to send out a memory request

	TeulTimeMS				m_hostClientOffset_ms;		// difference between the host and client monotonic clock
	TeulTimeMS				m_ibdTimeReachedMaxAccumDelay; 	// track how long we are at the max inbound
	TeulTimeMS				m_obdTimeReachedMaxAccumDelay; 	// track how long we are at the max outbound

	HostVehicle*			vehicle;
	class SpeedhackCheck* m_antiHack;

	PERSONA_STATE			state;					// enum of condition at this moment
	UINT32 					stationID;
	PLAYER_EXIT_CODE		exitState;

	// Track the number of times this user has connected/reconnected
	UINT32					m_numConnects;			// How many times this persona has (re)connected
	UINT32					m_connectedAt;			// Time they first connected
	UINT32					m_lastConnected;		// Time they last (re)connected

	UINT32					m_nextVelocityCheck;
	UINT32					m_awsTrigger;

	UINT32					m_expectUpdateNo;
	UINT32					m_updatePrime;
	UINT16					m_ejectionCountdown;
	UINT16					m_framingMode;
	UINT16					m_updatesSinceRX;
	UINT16					m_updatesFailed;
	SINT16					m_lastIndex;			// Last index we were on
	TeulTimeMS				m_timeToLive;
	UCHAR					station;

	bool					active : 1;
	bool					sendClose : 1;
	bool					m_hasSentUpdate : 1;
	bool					m_badUpdateSeq : 1;
	bool					m_badUpdatePrime : 1;
	bool					m_hasExited : 1;		// If _exitPlayer has been invoked
	bool					m_onAWS : 1;
	bool					m_isDespawning : 1;
	bool					m_hasDied : 1;			// Has send us a death packet.

	TeulTimeMS				m_timeTillSend;		// Time until we need to send next cells

	FULL_OBJ_ID				exitFacility;
	FULL_OBJ_ID				m_fpsBuildingID;
        SINT32                          	m_exitTeulIndex;
};

