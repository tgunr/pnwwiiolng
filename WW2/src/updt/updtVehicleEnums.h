#pragma once

// Enumerations for vehicle updates, such as bit positions and flag values.

namespace Vehicle
{
	namespace PresentBits
	{
		enum
		{
			OneTimeBit			// Packet contains extended information on vehicle.
		,	PositionBit			// Packet contains vehicle position.
		,	TurretBit			// Packet contains turret data.
		,	FireBit				// Packet contains firing data.
		,	ClientFlags1Bit		// Packet contains first 32 bits of client flags.
		,	ClientFlags2Bit		// Packet contains second 32 bits of client flags.
		,	RidingBit			// Packet contains riding information.
		,	RideRelPosBit		// Packet includes relative-to-ride position.
		,	NewUpdateBit		// Packet describes changes since last packet.
		,	MaxBit
		};
	}
	namespace PresentFlags
	{
		enum
		{
			OneTime = 1 << PresentBits::OneTimeBit
		,	Position = 1 << PresentBits::PositionBit
		,	Turret = 1 << PresentBits::TurretBit
		,	Fire = 1 << PresentBits::FireBit
		,	ClientFlags1 = 1 << PresentBits::ClientFlags1Bit
		,	ClientFlags2 = 1 << PresentBits::ClientFlags2Bit
		,	Riding = 1 << PresentBits::RidingBit
		,	RideRelPos = 1 << PresentBits::RideRelPosBit
		,	NewUpdate = 1 << PresentBits::NewUpdateBit
		,	MaxFlag = 1 << PresentBits::MaxBit
		};
	}

	static const UINT32 FirstTimePresentFlags =
					PresentFlags::OneTime |
					PresentFlags::Position |
					PresentFlags::Turret |
					PresentFlags::Fire |
					PresentFlags::ClientFlags1 |
					PresentFlags::ClientFlags2 |
					PresentFlags::Riding |
					PresentFlags::RideRelPos
	;

	namespace StatusBits
	{
		enum
		{
			CrewDeadBit
		,	VehicleDestroyedBit
		,	DespawningBit
		};
	}
	namespace StatusFlags
	{
		enum
		{
			CrewDead = 1 << StatusBits::CrewDeadBit
		,	VehicleDestroyed = 1 << StatusBits::VehicleDestroyedBit
		,	Despawning = 1 << StatusBits::DespawningBit
		};
	}

	namespace HostBits
	{
		enum
		{
			FirstTimeBit
		,	ReallyActiveBit
		};
	}
	namespace HostFlags
	{
		enum
		{
			FirstTime = 1 << HostBits::FirstTimeBit
		,	ReallyActive = 1 << HostBits::ReallyActiveBit
		};
	}
}

