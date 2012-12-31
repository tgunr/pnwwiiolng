#pragma once
// Created: Thu Jan 17 22:20:26 CST 2008
//
// Structure host uses to notify the client of sortie results
//
// Copyright (C) PlayNet, Inc 1999-2008 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "miss/missionTypes.h"			// MAX_HIT_ARRAY
#include "miss/missHitArrayNode.h"		// HIT_ARRAY_NODE
#include "wwii/wwiiRTBCodes.h"			// For CODE_RTB
#include "weap/weapSerial.h"				// WEAPON_SERIAL
#include "chat/chatGroupPermissionTypes.h"	// For ChatPermissions

//This is the structure that is sent to the front end upon completion of
// the mission
struct MISSION_RESULTS
{
	ChatHandle	handleID ;						// "Chat Handle" of the player
	UINT32 		missionID;						//so FRONT_END can tell if they were on a mission or not
	UINT32		elapsedTime;					//in seconds
	UINT16		kills;							//claimed kills this sortie
	UINT16		numHits;						//number of different types of vehicles hit.
	sortieid_t	sortieID ;						// sortie this was for.
												//the number of elements in the hit Array
	HIT_ARRAY_NODE	hitArray[MapMiss::MAX_HIT_ARRAY];	//one entry per vehicle type

	LOGICAL		missionSuccess;
	LOGICAL		promoted;						// Got a promotion
	LOGICAL		missionInvalid ;				// TRUE if mission is still valid

	WWII_SIDE	m_side;							//current side of the originating facility
	CODE_RTB	rtb;							//rtb code received from FRONT_END
	FULL_OBJ_ID	rtbFacility ;					// full object id of facility vehicle was
												// returned to, if any (nullObjID indicates failure)
	WEAPON_SERIAL	rtbReservedWeapon ;			// If they rtb'd, where to?

	RANK_NO		rank;		//in case it changed
	UINT16		captureEvents ;					// How many capture events this player caused
	UINT16		captureAssists ;				// How many captures you assisted with
	UINT32		mobileSpawnees ;				// People who spawned* through* you
	UINT32		aisHit ;
	UINT32		bridgesHit ;
	UINT32		bridgesRepaired ;
	UINT32		factorysHit ;
	UINT32		firebasesHit ;

	LOGICAL		rankIsKOTH ;			// Rank is KingOfTheHill
	UINT32		pointsScored ;
	UINT32		pointsAccumulated ;
	UINT32		pointsNextRank ;
	RANK_NO		nextRank ;

	UINT32		m_resupplies ;				// Number of times we resupplied someone
	UINT32		m_resuppliesScored ;		// How many earned score

	UINT16		m_guardingKills ; 			// Kills you scored from inside a friendly cap bldg
	UINT16		m_capperKills ;				// People you killed who were inside a cap bldg

	ChatHandle	m_killedBy ;				// Packed chat handle of the player given kill credit
											// *or* 0LL
	UINT16		m_guardingPoints ;			// Points for guarding tables
	UINT16		m_umsKills ;				// Unmanned Mobile Spawns killed
	RANK_NO		m_realRank ;				// What rank you'd really have (if sub restricted)
	RANK_NO		m_maxRank ;					// Max rank your* account* allows

	UINT32		accountGoodUntil ;			// teulTime() + minutes remaining on account
	ChatPermissions	chatPermissions ;		// Latest player permission mask

	UINT16		m_hitArraySize ;			// How many entries in hit array.

	UINT16		m_assists ;					// Number of assists scored.
} ;

