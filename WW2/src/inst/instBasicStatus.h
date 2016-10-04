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
#include "inst/instDescriptor.h"
#include "wwii/wwiiOwnershipEnums.h"
#include <assert.h>

extern UINT32 teulClockTime;

namespace Instance			//tolua_export
{							//tolua_export
	class BasicStatus
	{
	public:
		BasicStatus(const Descriptor& _descriptor,
			const UINT8 _capacity,
			const UINT16 _gameTime_m,
			const UINT32 _createdAt = teulClockTime,
			const UINT32 _startedAt = 0,
			const Instance::State state_ = Instance::HostCtored
		)
			: m_descriptor(_descriptor)
			, m_createdAt(_createdAt)
			, m_gameTime_m(_gameTime_m)
			, m_capacity(_capacity)
			, m_state(state_)
			, m_startedAt(_startedAt)
		{
			m_count[WWII_NO_SIDE] = m_count[ALLIED] = m_count[AXIS] = m_count[NEUTRAL] = 0;
		}
		BasicStatus()
			: m_descriptor(), m_createdAt(teulClockTime), m_gameTime_m(0), m_capacity(0), m_startedAt(0)
		{
			m_count[WWII_NO_SIDE] = m_count[ALLIED] = m_count[AXIS] = m_count[NEUTRAL] = 0;
		}
		BasicStatus(struct ACTION_OBJECT*);

		const Descriptor& descriptor() const { return m_descriptor; }
		UINT32 ageS() const { return (teulClockTime > createdAt()) ? (teulClockTime - createdAt()) : 0; }
		UINT32 allies() const { return m_count[ALLIED]; }
		UINT32 axis() const { return m_count[AXIS]; }
		UINT32 banned() const { return m_count[NEUTRAL]; }
		UINT16 capacity() const { return m_capacity; }
		UINT32 count(UINT8 forSide) const { assert(forSide < MAX_SIDES); return m_count[forSide]; }
		UINT32 createdAt() const { return m_createdAt; }
		UINT32 gameTime_m() const { return m_gameTime_m; }
		UINT32 neutral() const { return m_count[WWII_NO_SIDE]; }
		UINT32 playersOnSide(UINT8 forSide) const { assert(forSide < MAX_SIDES); return m_count[forSide]; }
		UINT32 startedAt() const { return m_startedAt; }
		UINT32 runningForS() const { return (teulClockTime > startedAt()) ? (teulClockTime - startedAt()) : 0; }
		UINT32 remainingTimeS() const { return runningForS() < gameTime_m() ? gameTime_m() - runningForS() : 0; }

		virtual SINT32 packer(LOGICAL mode, struct ACTION_OBJECT* const a);

		// Test if the game is full
		bool isFull() const { return allies() + axis() >= m_capacity; }
		bool isEmpty() const { return neutral() + allies() + axis() + banned() == 0; }
		bool isRunning() const { return m_startedAt > 0; }

	private:
		Descriptor	m_descriptor;			//!< Identifies the specific instance.
		UINT32		m_createdAt;			//!< TeulTime of when instance was created.
		UINT16		m_gameTime_m;			//!< How long the instance is allowed to run. Based on m_startedAt.
		UINT16		m_capacity;				//!< How many players we expect.

	protected:
		State       m_state;                //!< Current state. @see Instance::State
		UINT16		m_count[MAX_SIDES];		//!< Side counts (neutral, allied, axis, locked)
		UINT32		m_startedAt;			//!< How long since spawning was enabled.
	};

}	//tolua_export
