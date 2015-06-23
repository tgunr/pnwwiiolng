#pragma once

/// @file	updt/updtVehicle.h
/// @author	Oliver 'kfs1' Smith <osmith@playnet.com
/// @brief	Shared client/host transports for vehicle data.
///
/// Defines the base classes Vehicle::Update, Vehicle::Position,
/// Vehicle::FireData and Vehicle::PositionData.

#include "teul/teulNetTimeMS.h"
#include "weap/weapSpec.h"
#include "updt/updtVehicleEnums.h"
#include "updt/updtEnums.h"				// For MAX_GUNS and MAX_TURRETS etc.
#include "updt/updtTurrets.h"			// For turret_t

namespace Vehicle
{
	template<typename _Type>
	struct Vector
	{
		Vector() {}
		Vector(_Type _x, _Type _y, _Type _z) : m_x(_x), m_y(_y), m_z(_z) {}

		_Type		m_x;
		_Type		m_y;
		_Type		m_z;

		virtual SINT32 packer(LOGICAL mode, struct ACTION_OBJECT* a, const struct Position& base) { return -EINVAL; }
	};

	/// @struct Vehicle::Position
	/// @brief Shared client/host transport of vehicle position in cm.
	struct Position : public Vector<SINT32>	// Coordinates in CM.
	{
		Position() {}
		Position(SINT32 _x, SINT32 _y, SINT32 _z) : Vector<SINT32>(_x, _y, _z) {}

		SINT32 x_cm() const { return m_x; }
		SINT32 y_cm() const { return m_y; }
		SINT32 z_cm() const { return m_z; }
		SINT32 x_m() const { return x_cm() / 100; }
		SINT32 y_m() const { return y_cm() / 100; }
		SINT32 z_m() const { return z_cm() / 100; }
		SINT32 x_oct() const { return x_cm() / 800; }
		SINT32 y_oct() const { return y_cm() / 800; }
//		SINT32 x_sc() const { return x_cm() / SUPERCELL_SIZE_M; }
//		SINT32 y_sc() const { return y_cm() / SUPERCELL_SIZE_M; }

		virtual SINT32 packer(LOGICAL mode, struct ACTION_OBJECT* a, const Position& base);
	};

	/// @struct Vehicle::Acceleration
	/// @brief Shared client/host transport of vehicle acceleration vector.
	struct Acceleration : public Vector<SINT16>
	{
		virtual SINT32 packer(LOGICAL mode, struct ACTION_OBJECT* a, const Position& base);
	};

	/// @struct Vehicle::Attitude
	/// @brief Shared client/host transport of a vehicle's attitude.
	struct Attitude : public Vector<SINT16>
	{
		virtual SINT32 packer(LOGICAL mode, struct ACTION_OBJECT* a, const Position& base);
	};

	/// @struct Vehicle::AttVel
	/// @brief Shared client/host transport of a vehicle's angular velocity.
	/// Name (AttVel) inherited from old structures.
	struct AttVel : public Vector<SINT16>
	{
		virtual SINT32 packer(LOGICAL mode, struct ACTION_OBJECT* a, const Position& base);
	};

	/// @struct Vehicle::Velocity
	/// @brief Shared client/host transport of a vehicle's velocity vector.
	struct Velocity : public Vector<SINT16>
	{
		virtual SINT32 packer(LOGICAL mode, struct ACTION_OBJECT* a, const Position& base);
	};

	/// @struct Vehicle::FireData
	/// @brief Shared client/host transport of fire events.
	struct FireData
	{
		Teul::NetTimeMS m_clock[MAX_GUNS];	///< When each gun was last fired.
		UINT16			m_mask;			///< Flags indicating which guns fired.

		bool isTurretFiring(size_t turret) const
		{
			const bool hasFired = (m_mask & (1<<turret));					// Must be flagged true.
			const bool isValid  = (m_clock[turret] >= teulClock64 - 1000);	// Must be within last 1s.
			return (isValid & hasFired);
		}
		const TeulTimeMS& clock(size_t turret) const
		{
			return m_clock[turret].m_time;
		}
		TeulTimeMS& clock(size_t turret) { return m_clock[turret].m_time; }

		SINT32 packer(LOGICAL mode, struct ACTION_OBJECT* a);
	};

	/// @struct Vehicle::TurretData
	/// @brief Shared client/host transport of a vehicle's turret movements.
	struct TurretData
	{
		turret_t		m_x[MAX_TURRETS];	///< X value this frame.
		turret_t		m_y[MAX_TURRETS];	///< Y value this frame.
		UINT16			m_mask;			///< Flags indicating which turrets moved.

		SINT32 packer(LOGICAL mode, struct ACTION_OBJECT* a);
	};

	/// @struct Vehicle::RideData
	///	@brief Shared client/host transport of a vehicle's riding status.
	struct RideData
	{
		UINT32			rideIndex() const { return m_rideIndex; }
		UINT32			seatNumber() const { return m_seatNumber; }
		const Position&	relpos() const { return m_relpos; }
		Position&		relpos() { return m_relpos; }
		SINT16			yaw() { return m_yaw; }

		UINT32			m_rideIndex;		///< HostID of vehicle being ridden.
		UINT32			m_seatNumber;		///< Client's index for which position.
		Position		m_relpos;			///< Relative position.
		turret_t		m_yaw;

		SINT32 packer(LOGICAL mode, struct ACTION_OBJECT* a, bool includeRelpos);
	};

	/// @struct	Vehicle::Intro
	/// @brief	Persistent details of the vehicle itself.
	/// Formerly called the "One Time".
	struct Intro
	{
		WEAPON_SPEC		m_weaponSpec;	///< Type of vehicle.

		UINT32			m_oneTimeBits;	///< Vehicle's OneTimeBits.
		UINT32			m_brigadeID;	///< Brigade vehicle belongs to.
		UCHAR			m_decal;		///< Player selected vehicle decal.
		RANK_NO			m_rank;		///< Vehicle's current rank.

		UINT32			m_squadID;		///< Squad ID or 0 for none.

		ChatHandle		m_handle;		///< Vehicle's name.
		ChatHandle		m_squadHandle;	///< SquadHandle or 0 for none.

		SINT32 packer(LOGICAL mode, struct ACTION_OBJECT* a);
	};

	/// @struct Vehicle::Update
	/// @brief Shared client/host transport of a vehicle description.
	struct Update
	{
		UINT32				hostIndex() const { return m_hostIndex; }
		UINT32				present() const { return m_present; }
		UINT8				country() const { return intro().m_weaponSpec.wCountry; }
		UINT8				side() const { return wwiiSide(country()); }
		UINT32				oneTimeBits() const { return intro().m_oneTimeBits; }
		const WEAPON_SPEC&	weaponSpec() const { return intro().m_weaponSpec; }
		WEAPON_SPEC&		weaponSpec() { return intro().m_weaponSpec; }
		UINT32				brigadeID() const { return intro().m_brigadeID; }
		UCHAR				decal() const { return intro().m_decal; }
		RANK_NO				rank() const { return intro().m_rank; }
		UINT32				clientFlags1() const { return m_clientFlags1; }
		UINT32				clientFlags2() const { return m_clientFlags2; }
		const ChatHandle&	handle() const { return intro().m_handle; }
		ChatHandle&			handle() { return intro().m_handle; }
		UINT32				squadID() const { return intro().m_squadID; }
		const ChatHandle&	squadHandle() const { return intro().m_squadHandle; }
		ChatHandle			squadHandle() { return intro().m_squadHandle; }

		const FireData&		fireData() const { return m_fireData; }
		FireData&			fireData() { return m_fireData; }
		const TurretData&	turretData() const { return m_turretData; }
		TurretData&			turretData() { return m_turretData; }
		const RideData&		rideData() const { return m_rideData; }
		RideData&			rideData() { return m_rideData; }
		const Intro&		intro() const { return m_intro; }
		Intro&				intro() { return m_intro; }

		const Position& position() const { return m_position; }
		Position& position() { return m_position; }
		const Acceleration& accel() const { return m_accel; }
		Acceleration& accel() { return m_accel; }
		const Attitude& attitude() const { return m_attitude; }
		Attitude& attitude() { return m_attitude; }
		const AttVel& attvel() const { return m_attvel; }
		AttVel& attvel() { return m_attvel; }
		const Velocity& velocity() const { return m_velocity; }
		Velocity& velocity() { return m_velocity; }


		UINT32			m_hostIndex;	///< Host's ID for this vehicle.
		UINT32			m_present;		///< Vehicle::PresentFlags indicating what parts are being packed.

		UINT32			m_clientFlags1; ///< Client's state/vis-damage bits.
		UINT32			m_clientFlags2; ///< Client's state/vis-damage bits.

protected:
		Intro			m_intro;		///< Vehicle type, player, etc.
		Position		m_position;		///< Absolute world position.
		Attitude		m_attitude;		///< Attitude vector.
		Velocity		m_velocity;		///< Velocity vector.
		Acceleration	m_accel;		///< Acceleration.
		AttVel			m_attvel;		///< Rotational vector.
		FireData		m_fireData;		///< Firing events.
		TurretData		m_turretData;	///< Turret movements.
		RideData		m_rideData;		///< Data about riding of vehicle.

public:
		/// Both the client and the host associate timestamps with
		/// any given update. This value is NOT transmitted
		/// across the network.
		TeulTimeMS		mloc_timeStamp;	// loc = local.
		/// Both the client and the host track the last time they
		/// saw a position update for a vehicle. This value is
		/// NOT transmitted across the network.
		TeulTimeMS		mloc_posTimestamp;	// loc = local.

		SINT32 packer(LOGICAL mode, struct ACTION_OBJECT* a, const Vehicle::Position& base);
	};
}

