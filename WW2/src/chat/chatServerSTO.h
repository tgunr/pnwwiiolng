#pragma once
// Created: Fri Aug  6 17:14:44 CDT 2010
//
//! Generic STO class for chat host derived classes.
//
// Copyright (C) PlayNet, Inc 1999-2010 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "wwii/wwiiServerTrackedObject.h"

class ServerSTO : public ServerTrackedObject
{
public:
	ServerSTO(UINT16 withType, UINT16 withSubType
				, const WorldPosition_m& withPosition
				, const AttitudeVector& withAttitude
				, UINT32 withFlags
				, UINT32 withLifespan
				, UINT32 withCreatorID
				, WWII_SIDE withSide
				, UINT32 withMissionID
				)
		: ServerTrackedObject(0, withType, withSubType
								, withPosition, withAttitude
								, withFlags, withLifespan
								)
		, m_creatorID(withCreatorID)
		, m_side(withSide)
		, m_missionID(withMissionID)
		{
		}
	ServerSTO(ACTION_OBJECT* const a) ;

protected:
	enum EventType { Creation, Destruction } ;
	void _audit(EventType) ;

public:
	virtual bool activate() ;
	virtual void deactivate() ;

	virtual bool applyJoules(const SINT64& /*joules*/, class Player* /*player*/=NULL) { return false ; }

	virtual void extractPlayer(const Player* /*player*/) { }

	virtual const char* typeName() const { return "generic" ; }

	virtual bool needsBroadcastRemoval() const { return lifespan() > 1 && age() < lifespan() ; }

public:
	UINT32		creatorID() const { return m_creatorID ; }
	WWII_SIDE	side() const { return m_side ; }
	UINT32		missionID() const { return m_missionID ; }

private:
	UINT32			m_creatorID ;
	WWII_SIDE		m_side ;
	UINT32			m_missionID ;
} ;

