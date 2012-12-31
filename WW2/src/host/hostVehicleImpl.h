#pragma once
// Created: Mon Jan 21 13:23:29 CST 2008
//
// Private implementation detail of the host vehicle class
//
// Copyright (C) PlayNet, Inc 1999-2008 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "inst/instDescriptor.h"
#include "chat/chatLocalityUpdate.h"

//////////////////////////////////////////////////////////////////////
// Private definitions/variables

class HostVehicle ;

#include <set>
#include <stack>
#include <vector>
#include <map>

typedef std::vector<UINT32> REMOVE_LIST ;

// List of callsigns we've resupplied and when we resupplied them.
typedef std::map<ChatHandle, UINT32> RESUPPLIES ;

class CPStruct ;

// Private C++ segment of the HostVehicle
class HostVehicleImpl
{
	HostVehicle*	m_vehicle ;
	PLAYER_LIST		m_players ;

	LocalityUpdate	m_locality ;
	CPStruct*		m_nearCP ;

	// Last person to resupply me
	UINT32			m_lastResupplier ;

	RESUPPLIES		m_resupplies ;

	// When we will be considered stopped
	UINT32			m_stopped ;

	// Capture parameters
	bool			m_captureActive : 1 ;		// True when we've sent a capture tick.
	TeulTimeMS		m_nextCaptureTick ;
	UINT32			m_lastCaptureFacility ;	// uOID if the last facility we were capping

public:
	HostVehicleImpl() {}
	~HostVehicleImpl() ;

	void reset() ;
	void resetCaptureTick() ;

	HostVehicle& vehicle() { return *m_vehicle ; }
	const HostVehicle& vehicle() const { return *m_vehicle ; }

	friend class HostVehicle ;

	friend void Host::Vehicles::AllocateArray( UINT32 numVehicles ) ;
} ;

