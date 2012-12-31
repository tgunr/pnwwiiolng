#pragma once

//////////////////////////////////////////////////////////////////////
// @file omgVehicleIntro.h
// @attention AUTOMATICALLY GENERATED FROM omg/omgVehicleIntro.xml BY omg/omg-to-h.py
// @attention DO NOT MODIFY THIS FILE DIRECTLY edit omg/omgVehicleIntro.xml instead.
//////////////////////////////////////////////////////////////////////

// For VarGroup, SyncVar
#include "wwii/wwiiSyncVar.h"

namespace Vehicle
{

//! @class Vehicle::Intro
//! @brief Host/Client interface class for describing a new vehicle to be added to the viewer's vislist.
//! @details
//! Formerly called the "one time" this structure supplies the client
//! with semi-permanent details about a vehicle, sent independently
//! of the normal update stream.

//! @attention This class is automatically generated from omgVehicleIntro.xml

class Intro
{
	// Default constructor.
	public:
	Intro()
		: m_lastModified(0ULL)
		{
		}

	//! User implemented: somewhere to provide a reference to the Teul Callback for this object.
	public: static TEUL_CALLBACK* Callback(void) ;

	///! User implemented: Routine for serializing/deserializing this data.
	public: bool packer(LOGICAL mode, ACTION_OBJECT* const a, SINT32 size) ;

	// Members

	//! Game-name given to this vehicle.
	ChatHandle handle ;
	//! Squad name (or 0) for the vehicle.
	ChatHandle squadHandle ;
	//! Host's unique index for this vehicle.
	UINT32 hostIndex ;
	//! Squad ID (or 0) for the vehicle.
	UINT32 squadID ;
	//! Type of vehicle.
	WEAPON_SPEC weaponSpec ;
	//! Brigade vehicle belongs to.
	UINT32 brigadeID ;
	//! Player-selected decal, or 0.
	UINT8 decal ;
	//! Owner's rank.
	RANK_NO rank ;
} ; // class Intro

} // namespace Vehicle

