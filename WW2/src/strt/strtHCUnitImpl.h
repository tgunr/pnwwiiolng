#pragma once
// Created: Sat Jul  7 21:58:47 CDT 2007
//
// Non-public inner workings of the StratHCUnit
//
// Copyright (C) PlayNet, Inc 1999-2007 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include <list>
#include <map>

class StratHCUnit;
class ResupplyTicket;

//////////////////////////////////////////////////////////////////////
// A docket is a collection of resupply tickets. They are stored individually
// but are best processed by collating the individual entries by weap_spec
// into a RESUPPLY_MANIFEST
typedef std::list<ResupplyTicket*> RESUPPLY_DOCKET;
typedef std::map< UINT32, RESUPPLY_DOCKET > RESUPPLY_QUEUE;
typedef std::map<WEAPON_SPEC, UINT32> RESUPPLY_MANIFEST;
typedef std::list<ResupplyTicket*> TRANSFERS;

//////////////////////////////////////////////////////////////////////
// The actual impl

class StratHCUnitImpl
{
public:
	// We consume one docket every 15 seconds at normal timing
	static const TeulTimeMS SECONDS_PER_DOCKET = 15;
	static const TeulTimeMS DOCKET_RATE_MS = SECONDS_PER_DOCKET * 1000;
	StratHCUnitImpl(StratHCUnit& unit)
		: m_unit(unit)
		, m_resupplyQueue()
		, m_totalTickets(0)
		, m_nextServing(0)
		, m_nextServiceAt(teulClock64 + DOCKET_RATE_MS)
		, m_inTransit(false)
		, m_lastTimeInSupply(teulTime())
		, m_transfers()
		{}

	// Calculate what the docket number (rounded up) would be at a standard tick
	UINT32 getDocketFor(UINT32 seconds) const
	{
		return m_nextServing + (seconds + (UINT32)SECONDS_PER_DOCKET - 1) / (UINT32)SECONDS_PER_DOCKET;
	}

	// Determine the interval between docket consumption for this brigade.
	UINT32 docketRate_s() const { return (UINT32)(docketRate_ms() / 1000); }
	TeulTimeMS docketRate_ms() const;	// Based on DOCKET_RATE_MS

	UINT32 nextServiceIn_s() const { return (UINT32)((m_nextServiceAt > teulClock64) ? (m_nextServiceAt - teulClock64) / 1000 : 0); }

	static UINT32 ThrottleKey(const FacilityStruct* facility, const WEAPON_SPEC spec)
	{
		return ( facility->uOID << 16 ) | ( spec.asUINT16() & 0xffff );
	}

	// Throttle handling
	void decrementThrottle(UINT32 key);
	void incrementThrottle(UINT32 key);

//	void resetThrottleWindow(const FacilityStruct* facility, const WEAPON_SPEC spec)
//	{
//		m_throttles.erase(ThrottleKey(facility, spec));
//	}

	UINT32 throttles(const FacilityStruct* facility, const WEAPON_SPEC spec) const
	{
		THROTTLE_MAP::const_iterator it = m_throttles.find(ThrottleKey(facility, spec));
		return ( it != m_throttles.end() ) ? it->second.Check() : 0;
	}

	// The unit we derive from
	StratHCUnit&	m_unit;

	// The tickets waiting to be resupplied
	RESUPPLY_QUEUE	m_resupplyQueue;
	// The count of individual tickets across all dockets.
	// not the same as m_resupplyQueue.size()
	UINT32			m_totalTickets;

	// In order to allow flexible resupply schedules that
	// adapt to the brigades current situation, we provide
	// a ticket system. Tickets are grouped into dockets,
	// each docket covering a theoretical window of time.
	// The actual time when this docket is reached depends
	// on the circumstances of the brigade affecting
	// docketRate_ms and thus increasing or slowing the
	// rate at which tickets are actually pulled from dockets.
	UINT32			m_nextServing;
	TeulTimeMS		m_nextServiceAt;
	bool			m_inTransit;

	// When was this unit last in supply.
	UINT32			m_lastTimeInSupply;

	TRANSFERS		m_transfers;

#ifdef IS_STRAT_HOST
	struct Throttle
	{
		UINT32 Check() const
		{
			if ( m_used > 0 && m_ends > 0 && m_ends <= teulTime() )
				m_used = 0;
			return m_used;
		}
		void Decrement()
		{
			Check();
			if ( m_used > 0 )
				--m_used;
		}
		void Increment(UINT32 ends)
		{
			Check();
			m_used++;
			m_ends = ends;
		}
		UINT32 Ends() const { return m_ends; }
		UINT32 Used() const { return m_used; }

	private:
		UINT32			m_ends;	// When this throttle expires
		mutable UINT32	m_used;	// Number of this throttle in use
	};
	typedef std::map<UINT32, Throttle> THROTTLE_MAP;
	THROTTLE_MAP	m_throttles;
#endif
};
