#pragma once
// Created: Sat May 29 23:15:50 CDT 2010
// Originating Programmer: Oliver "kfsone" Smith

//! Structure that describes one vehicle's previous view of another.
//! When a subject is added to a viewer's vis list, a Viewing is
//! created to track the state of the subject. This lets us perform
//! deltas between updates, and also determine which vehicles are
//! most in need of a fresh update.

#include "updt/updtVehiclePosition.h"

class Viewing
{
public:
	//! Constructor
	Viewing(const HostVehicle& vehicle, SINT64 priority)
		: m_vehicle(vehicle)
		, m_index(vehicle.index)
		, m_lastSent(0ULL)
		, m_priority(priority)
		, m_fireMask(0)
		, m_visBits(0)
		, m_stateFlags0(0)
		, m_stateFlags1(0)
		, m_turretsPending(~0)
		, m_oneTimeBits(0)
		, m_seatNumber(0)
		, m_rideIndex(0)
		, m_hideCallsign(false)
		, m_oneTimeBitsChanged(false)
		, m_position()
//		, m_attitude()
//		, m_velocity()
//		, m_accel()
		{
		}

	//! Assignment operator
	Viewing& operator=(const Viewing& rhs)
	{
		memcpy(this, &rhs, sizeof(*this));
		return *this;
	}

	//! Less-than operator for STL containers, compares priorities.
	bool operator < (const Viewing& rhs) const { return m_priority < rhs.m_priority; }

	//! SortieID of this sortie.
	sortieid_t sortieID() const { return m_index; }

	// We take a pointer to a vehicle structure and the index
	// that the vehicle had when we saw it. This assumes that
	// vehicle objects are persistent in memory. We can then
	// be lazy about pointers and just check that the vehicle
	// has retained the same vehicle number (index) between
	// our observations.

	const HostVehicle&	m_vehicle;				//!< Vehicle we are viewing.
	sortieid_t			m_index;				//!< Index of vehicle at last view.

	TeulTimeMS			m_lastSent;				//!< Timestamp of last send.

	SINT64				m_priority;				//!< Rating of how urgently needs updating.

	// Vehicle bits
	UINT32				m_fireMask;				//!< Which turrets are firing.
	UINT32				m_visBits;				//!< Visible damage states.
	UINT16				m_stateFlags0;			//!< Animation state flags.
	UINT16				m_stateFlags1;			//!< Animation state flags.
	UINT16				m_turretsPending;		//!< Turrets that need updates.
	UINT16				m_oneTimeBits;			//!< Per-sortie bits.
	UINT16				m_seatNumber;			//!< Position on the vehicle.
	UINT32				m_rideIndex;			//!< Vehicle we're riding.

	bool				m_hideCallsign : 1;		//!< Whether callsign needs special hiding.
	bool				m_oneTimeBitsChanged : 1;	//!< Perhaps 'onetimebits' doesn't cut it any more.

	VEHICLE_POSITION	m_position;				//!< Position last time viewed.
//	Vector				m_attitude;				//!< Orientation.
//	Vector				m_velocity;				//!< Speed + Direction of movement.
//	Vector				m_accel;				//!< Acceleration.
};

#include <vector>
using Viewings = std::vector<Viewing>;

