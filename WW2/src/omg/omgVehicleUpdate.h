#pragma once

//////////////////////////////////////////////////////////////////////
// @file omgVehicleUpdate.h
// @attention AUTOMATICALLY GENERATED FROM omg/omgVehicleUpdate.xml BY omg/omg-to-h.py
// @attention DO NOT MODIFY THIS FILE DIRECTLY edit omg/omgVehicleUpdate.xml instead.
//////////////////////////////////////////////////////////////////////

// For VarGroup, SyncVar
#include "wwii/wwiiSyncVar.h"

namespace Vehicle
{

//! @class Vehicle::Update
//! @brief Host/Client interface classes for describing player vehicles.
//! @details
//! Various components required to describe the position, type and movement
//! of a player vehicle.

//! @attention This class is automatically generated from omgVehicleUpdate.xml

class Update
{
	// Default constructor.
	public:
	Update()
		: m_lastModified(0ULL)
		, m_data(m_lastModified)
		, m_model(m_lastModified)
		{
		}

	//! User implemented: somewhere to provide a reference to the Teul Callback for this object.
	public: static TEUL_CALLBACK* Callback(void);

	//! Last modified time of the structure or a member var group.
	protected: TeulTimeMS m_lastModified;

	//! Get milisecond timestamp of last modification.
	//! @return Local teulClock64 timestamp of the last modification of a managed member of the structure.
	public: const TeulTimeMS& lastModified() const { return m_lastModified; }

	//! How many bits required to describe number of groups changed.
	public: UINT32 groupsChangedBits() const { return 8; }

	///! User implemented: Routine for serializing/deserializing this data.
	public: bool packer(PackMode mode, ACTION_OBJECT* const a, const TeulTimeMS& lastPack);

	// Members

	//! @struct Data
	//! @brief Flags or flag-like values that may change fairly often.
	public:
	struct Data : public WWII::VarGroup
	{
		Data(TeulTimeMS& parent)
			: VarGroup(parent)
			, oneTimeBits(this)
			, clientFlags1(this)
			, clientFlags2(this)
			, fireMask(this)
			, ridingVeh(this)
			, ridingSeat(this)
			{}
		Data& operator = (const Data& rhs)
		{
			m_lastModified = rhs.m_lastModified;
			oneTimeBits.operator=(rhs.oneTimeBits);
			clientFlags1.operator=(rhs.clientFlags1);
			clientFlags2.operator=(rhs.clientFlags2);
			fireMask.operator=(rhs.fireMask);
			ridingVeh.operator=(rhs.ridingVeh);
			ridingSeat.operator=(rhs.ridingSeat);
			return *this;
		}
		virtual void groupPacker(PackMode mode, ACTION_OBJECT* const a, const TeulTimeMS& lastPack, UINT32& changes)
		{
			UINT32 fieldNo = 0;
			oneTimeBits.packer(mode, a, lastPack, changes, fieldNo++);
			clientFlags1.packer(mode, a, lastPack, changes, fieldNo++);
			clientFlags2.packer(mode, a, lastPack, changes, fieldNo++);
			fireMask.packer(mode, a, lastPack, changes, fieldNo++);
			ridingVeh.packer(mode, a, lastPack, changes, fieldNo++);
			ridingSeat.packer(mode, a, lastPack, changes, fieldNo++);
		}
		//! Vehicle's OneTimeBits.
		WWII::SyncVar<UINT16,0> oneTimeBits;
		//! Client's vis bits.
		WWII::SyncVar<UINT32,0> clientFlags1;
		//! Client's animation bits.
		WWII::SyncVar<UINT32,0> clientFlags2;
		//! Turret firing mask.
		WWII::SyncVar<UINT8,0> fireMask;
		//! Host index of vehicle being ridden.
		WWII::SyncVar<UINT32,0> ridingVeh;
		//! Client's index for seat.
		WWII::SyncVar<UINT8,0> ridingSeat;
	} m_data;
	const Data& data() const { return m_data; }
	Data& data() { return m_data; }

	//! @struct Model
	//! @brief Position, orientation and speed of the vehicle.
	public:
	struct Model : public WWII::VarGroup
	{
		Model(TeulTimeMS& parent)
			: VarGroup(parent)
			, posX(this)
			, posY(this)
			, posZ(this)
			, attX(this)
			, attY(this)
			, attZ(this)
			, velX(this)
			, velY(this)
			, velZ(this)
			, accX(this)
			, accY(this)
			, accZ(this)
			, rotX(this)
			, rotY(this)
			, rotZ(this)
			{}
		Model& operator = (const Model& rhs)
		{
			m_lastModified = rhs.m_lastModified;
			posX.operator=(rhs.posX);
			posY.operator=(rhs.posY);
			posZ.operator=(rhs.posZ);
			attX.operator=(rhs.attX);
			attY.operator=(rhs.attY);
			attZ.operator=(rhs.attZ);
			velX.operator=(rhs.velX);
			velY.operator=(rhs.velY);
			velZ.operator=(rhs.velZ);
			accX.operator=(rhs.accX);
			accY.operator=(rhs.accY);
			accZ.operator=(rhs.accZ);
			rotX.operator=(rhs.rotX);
			rotY.operator=(rhs.rotY);
			rotZ.operator=(rhs.rotZ);
			return *this;
		}
		virtual void groupPacker(PackMode mode, ACTION_OBJECT* const a, const TeulTimeMS& lastPack, UINT32& changes)
		{
			UINT32 fieldNo = 0;
			posX.packer(mode, a, lastPack, changes, fieldNo++);
			posY.packer(mode, a, lastPack, changes, fieldNo++);
			posZ.packer(mode, a, lastPack, changes, fieldNo++);
			attX.packer(mode, a, lastPack, changes, fieldNo++);
			attY.packer(mode, a, lastPack, changes, fieldNo++);
			attZ.packer(mode, a, lastPack, changes, fieldNo++);
			velX.packer(mode, a, lastPack, changes, fieldNo++);
			velY.packer(mode, a, lastPack, changes, fieldNo++);
			velZ.packer(mode, a, lastPack, changes, fieldNo++);
			accX.packer(mode, a, lastPack, changes, fieldNo++);
			accY.packer(mode, a, lastPack, changes, fieldNo++);
			accZ.packer(mode, a, lastPack, changes, fieldNo++);
			rotX.packer(mode, a, lastPack, changes, fieldNo++);
			rotY.packer(mode, a, lastPack, changes, fieldNo++);
			rotZ.packer(mode, a, lastPack, changes, fieldNo++);
		}
		//! X position.
		WWII::SyncVar<SINT32,0> posX;
		//! Y position.
		WWII::SyncVar<SINT32,0> posY;
		//! Z position.
		WWII::SyncVar<SINT32,0> posZ;
		//! X attitude.
		WWII::SyncVar<SINT32,0> attX;
		//! Y attitude.
		WWII::SyncVar<SINT32,0> attY;
		//! Z attitude.
		WWII::SyncVar<SINT32,0> attZ;
		//! X velocity.
		WWII::SyncVar<SINT32,0> velX;
		//! Y velocity.
		WWII::SyncVar<SINT32,0> velY;
		//! Z velocity.
		WWII::SyncVar<SINT32,0> velZ;
		//! X acceleration.
		WWII::SyncVar<SINT32,0> accX;
		//! Y acceleration.
		WWII::SyncVar<SINT32,0> accY;
		//! Z acceleration.
		WWII::SyncVar<SINT32,0> accZ;
		//! Rotational X (formerly attvel).
		WWII::SyncVar<SINT32,0> rotX;
		//! Rotational Y (formerly attvel).
		WWII::SyncVar<SINT32,0> rotY;
		//! Rotational Z (formerly attvel).
		WWII::SyncVar<SINT32,0> rotZ;
	} m_model;
	const Model& model() const { return m_model; }
	Model& model() { return m_model; }

	//! Host's index for this vehicle.
	UINT32 hostIndex;
}; // class Update

} // namespace Vehicle

