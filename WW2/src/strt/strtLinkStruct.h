#pragma once
// Created: Wed Dec  7 21:25:07 CST 2005
//
// Defines the LinkStruct class
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

//tolua_begin

class LinkStruct
{
public:
	enum { StratClass = CLASS_LINK };

	// Determine if this link has an active enemy firebase for side "forSide"
	bool hasFirebase() const { return firebase != nullptr; }
	bool hasEnemyFirebase(WWII_SIDE forSide) const;
	void setEnabled(bool newEnabled) { m_enabled = newEnabled; }

	bool processFirebases();

	bool isEnabled() const { return m_enabled; }

public:
//FULL_OBJ_ID must be the first field in the structure.
//DO NOT MOVE!!!!!!!!!! -Meat
	FULL_OBJ_ID		fullObjID;
	bool			modified;
	char			name[MAX_STRT_NAME_LENGTH];
	const char*		filename;
	FULL_OBJ_ID		startDepotID;
	FULL_OBJ_ID		endDepotID;
	FacilityStruct* startDepot;		//Starting Depot
	FacilityStruct* endDepot;		//Ending Depot
	UINT32			capacity;		//The Max RPs that can be on the link
	UINT32			latency;		//How long it takes for the convoy to
									//	travel the link
	UINT32 			distance;		//The length of the link

	/*These &&-ed together give us the status of the link*/
	bool			playerValve;	//The player controlled valve
	bool			m_enabled;		//The computer controlled valve
	bool			m_convoy;		//Whether the link contains a convoy

	//For firebases
	FacilityStruct* firebase;		//The firebase at the end of the link
	FULL_OBJ_ID		firebaseID;

	UINT32			m_uid;			// Databases's unique ID for me
};

//tolua_end

