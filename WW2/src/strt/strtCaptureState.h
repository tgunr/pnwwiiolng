#pragma once
//////////////////////////////////////////////////////////////////////
// CaptureState class
//
// Describes the capture/no-capture/progress of capture of the
// capture building the player is current in (or was in moments ago).
//
// IsCapturable() == false && IsPending() == false
//  :- Building is not going to be capturable any time soon.
// IsCapturable() == false && IsPending() == true
//  :- GetPendingSeconds() will return a non-zero number of
//     seconds in which the building will /become/ capturable.
// IsCapturable() == true
//  :- The building can now be captured by the enemy.
// GetProgress()
//  :- Returns the percentage towards the non-owning side.
//     (This is an optimization that allows the host to avoid
//     having to build versions for both sides)
// GetLastContribution()
//  :- Returns +ve rate indicator if the building is being
//     captured by the enemy; -ve rate indicator if the building
//     is being captured by the current owner or 0 if stalled.
//     This describes the LAST capture-tick.

#include "teul/teulPackers.h"

#pragma pack(push, 1)
class CaptureState
{
private:
	enum Stage
	{
		CapDisabled			// Equivalent to no-table, no-radio;
							// no ao on town, or military facility
							// cap requirements not met yet.
							// CapBar should show ownership.

	,	CapPending			// Equivalent to has-table, no-radio;
							// m_counter describes time until will
							// be capturable.
							// CapBar should show ownership.

	,	CapEnabled			// Equivalent to has-table, has-radio;
							// m_counter describes the percentage
							// of the bar owned by your side.
							// I receive 0% -> 10% -> 25% -> 75% -> 100%
							// Enemy would receive 100% -> 75% -> 25% -> 10% -> 0%
							// m_rate will be 0 for no progress or initial update;
							// m_rate will be +/-1 for "auto progress",
							// m_rate will be +/-2 for "human progress",
							// m_rate will be +/-3 for "'sweet' progress",

	,	CapCaptured			// Indicates end of CapEnabled, no other
							// data is supplied.
	};
	enum { StageBits = 2 };

private:
	UINT32	m_stage;		// Current stage (Disabled/Pending/Enabled)
	UINT32	m_counter;		// Stage-dependent counter.
							// CapDisabled :- Always 0
							// CapPending :- seconds until cappable
							// CapEnabled :- percentage towards capture
	bool	m_spawnable;	// Is it/will it be spawnable by us?

	enum { RateBits = 7 };
	SINT16	m_rate;	// +ve = our capture, -ve = decapture
						// Always zero on first update.

		// Default ctor
public:	CaptureState()
		: m_stage(CapDisabled)
		, m_counter(0)
		, m_spawnable(false)
		, m_rate(0)
#if platBuild == BUILD_CLIENT
		, m_valid(false)
#endif
		{}
		// Network ctor
public:	CaptureState(struct ACTION_OBJECT* a)
		: m_stage(CapDisabled)
		, m_counter(0)
		, m_spawnable(false)
		, m_rate(0)
		{ packer(TEUL_UNPACK, a); }

	// Network packer
public:	void packer(LOGICAL mode, struct ACTION_OBJECT* a)
	{
		a->packer(mode, &m_stage, StageBits);
		a->packer(mode, &m_counter, 16);
		a->packer(mode, &m_rate, RateBits);
		a->packer(mode, m_spawnable, 1);
	}

	// True/False: Can this building be captured?
public:	bool IsCapturable() const { return m_stage == CapEnabled; }

	// True/False: Is the table for this building pending?
public:	bool IsPending() const { return m_stage == CapPending; }

	// Time in seconds until building becomes capturable (0 = never)
public:	UINT32 GetPendingSeconds() const { return (IsPending() ? m_counter : 0); }

	// True/False: Is/Will this building be spawnable for us?
	// (Does not indicate you can spawn here right now)
public: bool IsSpawnable() const { return m_spawnable; }

	// True/False: Is the capture completed?
public: bool IsComplete() const { return m_stage == CapCaptured; }

	// Get the capture percentage (towards enemy ownership). 0% = owner fully owns.
public: UINT32 GetProgress() const { return (IsCapturable() ? m_counter : 0); }

	// Returns the rate (value) and direction (sign) of the last capture tick.
	// 0 = stalled.
	// +ve = Non-owner pulse
	// -ve = Owner pulse
public:	SINT32 GetLastContribution() const
	{
		if ( IsCapturable() == false || IsComplete() ) return 0;
		return (SINT32) m_rate;
	}

#if platBuild == BUILD_SERVER
	// Host function to populate
public:	void populate(const class FacilityStruct* facility, UINT32 forSide, SINT32 rate, bool spawnable);
	// Only used by the host
public:	UINT64	m_fidKey;
public:	UINT32	m_forSide;
public: void invert(const CaptureState& rhs);
#else
public: bool m_valid;
public: UINT32 GetStage() const { return m_stage; }
#endif
};
#pragma pack(pop)

