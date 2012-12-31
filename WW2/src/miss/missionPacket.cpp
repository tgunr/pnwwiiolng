#include <syscfg.h> // Path is project specific.
// Created: Wed Aug 24 10:20:15 CDT 2005
//
// Standard member functions of MissionPacket
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "miss/missionTypes.h"
#include "miss/missionPacket.h"
#include "wwii/wwiiRTBCodes.h"

//////////////////////////////////////////////////////////////////////
// Reset all sortie stats data

void
MissionPacket::discardSortieData()
{
	resetHitArray() ;

	scoring().hits = 0 ;
	scoring().kills = 0 ;
	scoring().captures = 0 ;
	scoring().defends = 0 ;
	scoring().aiKills = 0 ;
	scoring().aiRepairs = 0 ;
	scoring().bridgeKills = 0 ;
	scoring().bridgeRepairs = 0 ;
	scoring().factoryKills = 0 ;
	scoring().firebaseKills = 0 ;
	scoring().resupplies = 0 ;
	scoring().resuppliesScored = 0 ;
	scoring().guardingKills = 0 ;
	scoring().cappingKills = 0 ;
	scoring().umsKills = 0 ;
	scoring().guardingPoints = 0 ;
	scoring().spawnees = 0 ;
	scoring().uniqueHits = 0 ;
	scoring().vehtypesHit = 0 ;
	scoring().points = 0 ;

	vehicle().killedBy = 0ULL ;

	m_killScore = 0.0f ;
	wasSuccessful = false ;
	m_altF4ed = false ;
	m_vehicleResupplied = false ;
	m_saved = false ;
/*
	m_killedBy = 0ULL ;
	///TODO:SCORING: Replace
	rtbPromotion = false ;
	spawnTime = returnTime = 0 ;
	spawnCapacity = despawnCapacity = 0 ;
	missionScore = 0 ;
*/
}

//////////////////////////////////////////////////////////////////////
// Clear out any scoring information

void
MissionPacket::resetHitArray()
{
	///TODO:SCORING: Reimplement.
}

