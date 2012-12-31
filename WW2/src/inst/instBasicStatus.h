#pragma once
// Created: Tue Oct  7 15:40:00 CDT 2008
//
// Describes the current status of a specific unique instance
// Shared between client and host.
//
// Copyright (C) PlayNet, Inc 1999-2008 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "inst/instStateEnums.h"

namespace Instance			//tolua_export
{							//tolua_export
	class BasicStatus
	{
	public:
		BasicStatus(const Descriptor& descriptor_
					, const UINT16 capacity_
					, const UINT16 gameTime_m_
					, const UINT32 createdAt_=teulClockTime
					, const UINT32 startedAt_=0
					, const Instance::State state_=Instance::HostCtored
					)
			: m_descriptor(descriptor_)
			, m_createdAt(createdAt_)
			, m_gameTime_m(gameTime_m_)
			, m_capacity(capacity_)
			, m_state(state_)
			, m_startedAt(startedAt_)
			, m_hasPassword(false)
			{
				m_count[WWII_NO_SIDE] = m_count[ALLIED] = m_count[AXIS] = m_count[NEUTRAL] = 0 ;
			}
		BasicStatus()
			: m_descriptor(), m_createdAt(teulClockTime), m_gameTime_m(0), m_capacity(0), m_state(Instance::HostCtored)
			, m_startedAt(0)
			, m_hasPassword(false)
		{
			m_count[WWII_NO_SIDE] = m_count[ALLIED] = m_count[AXIS] = m_count[NEUTRAL] = 0 ;
		}

		BasicStatus(ACTION_OBJECT* const a) { this->packer(TEUL_UNPACK, a) ; }
		virtual ~BasicStatus() {}

		const Descriptor& descriptor() const { return m_descriptor ; }
		UINT32 ageS() const { return (teulClockTime > createdAt()) ? (teulClockTime - createdAt()) : 0 ; }
		UINT16 allies() const { return m_count[ALLIED] ; }
		UINT16 axis() const { return m_count[AXIS] ; }
		UINT16 banned() const { return m_count[NEUTRAL] ; }
		UINT16 capacity() const { return m_capacity ; }
		UINT16 count(UINT8 forSide) const { assert( forSide < MAX_SIDES ) ; return m_count[forSide] ; }
		UINT32 createdAt() const { return m_createdAt ; }
		UINT32 gameTime_m() const { return m_gameTime_m ; }
		bool hasPassword() const { return m_hasPassword ; }
		UINT16 neutral() const { return m_count[WWII_NO_SIDE] ; }
		UINT16 playersOnSide(UINT8 forSide) const { assert( forSide < MAX_SIDES ) ; return m_count[forSide] ; }
		UINT32 startedAt() const { return m_startedAt ; }
		Instance::State state() const { return m_state ; }
		UINT32 runningForS() const { return (teulClockTime > startedAt()) ? (teulClockTime - startedAt()) : 0 ; }
		UINT32 remainingTimeS() const { return runningForS() < gameTime_m() ? gameTime_m() - runningForS() : 0 ; }

		virtual SINT32 packer(const unsigned int mode, ACTION_OBJECT* const a) ;

		// Test if the game is full
		bool isFull() const { return allies() + axis() >= m_capacity ; }
		bool isEmpty() const { return neutral() + allies() + axis() + banned() == 0 ; }
		bool isRunning() const { return m_startedAt > 0 && state() < Instance::Completed ; }

		//! Query if we are allowed to join.
		bool isJoinable() const { return state() >= Instance::HostCtored && state() < Instance::Finishing ; }
		//! Query if we are allowed to spawn.
		bool isSpawnable() const { return state() >= Instance::CanSpawnIn && state() < Instance::Closed ; }
		//! Query if we are allowed to be *in* the instance.
		bool isAvailable() const { return state() >= Instance::HostCtored && isRunning() ; }

	private:
		Descriptor	m_descriptor ;			//!< Identifies the specific instance.
		UINT32		m_createdAt ;			//!< TeulTime of when instance was created.
		UINT16		m_gameTime_m ;			//!< How long the instance is allowed to run. Based on m_startedAt.
		UINT16		m_capacity ;			//!< How many players we expect.

	protected:
		State		m_state ;				//!< Current state. @see Instance::State
		UINT16		m_count[MAX_SIDES] ;	//!< Side counts (neutral, allied, axis, locked)
		UINT32		m_startedAt ;			//!< How long since spawning was enabled.
		bool		m_hasPassword ;			//!< True if a password is required.

	private:
		friend class MMHostInstanceImpl ;
		friend class ChatHostInstanceImpl ;
		friend class CellHostInstanceImpl ;
		friend class HostInstanceImpl ;
	} ;

	//! Check-And-Set state of an instance
	extern Instance::State CASState(const char* instStr, Instance::State from, Instance::State to) ;	//tolua_export
	//! Get the current state of an instance by ID.
	extern Instance::State GetState(const char* instStr) ;												//tolua_export
	//! Spawn a new copy of the instance, with things reset.
	extern instanceid_t SpawnClone(const char* instStr) ;

}	//tolua_export

extern std::string instSpawnCloneWrapper(const char* instStr) ;	//tolua_export

