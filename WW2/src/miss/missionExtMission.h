#pragma once
// Created: Sun Mar  6 20:44:58 CST 2005
//
// Defines the class for dealing with external
// missions
//
// Copyright (C) PlayNet, Inc 2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "teul/teulPackers.h"

class Mission;

// Packet for describing a new mission to be created
// these are primarily created externally
#define EXTM_DESCRIPTION_LEN 255
class NewMission
{
public:
	NewMission()
		: missionID(0), country(WWII_NO_COUNTRY), brigadeID(0), missionType(0)
		, postedAt(0), postAt(0), launchAt(0)
		, launchCP_oid(0), launchFac_oid(0), targetCP_oid(0), targetFac_oid(0)
		, priority(0), weapon()
		, instanceID(0)
		{
			memset(posterCallsign, 0, sizeof(posterCallsign));
			memset(description, 0, sizeof(description));
		}

	NewMission(ACTION_OBJECT* const a) { a->offset = packer(TEUL_UNPACK, a); }

	SINT32 packer(LOGICAL mode, ACTION_OBJECT* const a) { return teulPackVoid(mode, a->packed, a->offset, this, sizeof(*this)); }

	Mission* post();					// execution function if appropriate

	//// Member fields ////
public:
	// ID of the missions
	UINT32	missionID;
	// Country and side
	WWII_COUNTRY	country;
	UINT32	brigadeID;
	// Type of mission
	UINT32	missionType;
	// Who posted it (playerID)
	UINT32	postedByID;
	char	posterCallsign[MAX_HANDLE_SIZE];
	// When it was posted
	SINT32	postedAt;
	// When the mission is to be posted on the host
	SINT32	postAt;
	// When the mission should launch
	SINT32	launchAt;
	// Launch CP and Facility
	UINT16	launchCP_oid;
	UINT16	launchFac_oid;
	// Target CP and Facility
	UINT16	targetCP_oid;
	UINT16	targetFac_oid;
	// Description of the mission
	SCHAR	description[EXTM_DESCRIPTION_LEN+1];
	// Mission priority
	UCHAR	priority;
	// Vehicle for the mission
	WEAPON_SPEC	weapon;
	instanceid_t instanceID;
};

