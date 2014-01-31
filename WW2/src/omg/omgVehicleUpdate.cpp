#include <syscfg.h> // Path is project specific.
//////////////////////////////////////////////////////////////////////
// @file omgVehicleUpdate.cpp
// @attention AUTOMATICALLY GENERATED FROM omg/omgVehicleUpdate.xml BY omg/omg-to-h.py
// @attention DO NOT MODIFY THIS FILE DIRECTLY edit omg/omgVehicleUpdate.xml instead.
//////////////////////////////////////////////////////////////////////
// Implementation functions for Vehicle::Update

#include "omg/omgVehicleUpdate.h"

Vehicle::Update()
	: m_lastModified(0ULL)
	, m_data(m_lastModified)
	, m_model(m_lastModified)
	, m_hostIndex(0)
{
}

struct TEUL_CALLBACK* Vehicle::Update::Callback()
{
	static TEUL_CALLBACK* callback;
	return callback;
}

bool Vehicle::Update::packer(LOGICAL mode, ACTION_OBJECT* const a, const TeulTimeMS& lastPack)
{
	packer(mode, m_hostIndex);
	SINT32 changeOffset = a->offset;
	const UINT32 numChangeBits = groupsChangedBits();
	UINT32 changedBits = 0;
	a->packer(mode, changedBits, numChangeBits);
	m_data.groupPacker(mode, a, lastPack, changedBits);
	if ( mode == TEUL_PACK )
	{
		teulPackUINT32(TEUL_PACK, a->packed, changeOffset, &changedBits, groupsChangedBits());
	}
}
Vehicle::Update::Data(TeulTimeMS& parent)
	: VarGroup(parent)
	, oneTimeBits(this)
	, clientFlags1(this)
	, clientFlags2(this)
	, fireMask(this)
	, ridingVeh(this)
	, ridingSeat(this)
	{}


Vehicle::Update::Data& operator = (const Data& rhs)
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

void Vehicle::Update::Data::groupPacker(LOGICAL mode, ACTION_OBJECT* const a, const TeulTimeMS& lastPack, UINT32& changes)
{
	UINT32 fieldNo = 0;
	oneTimeBits.packer(mode, a, lastPack, changes, fieldNo++);
	clientFlags1.packer(mode, a, lastPack, changes, fieldNo++);
	clientFlags2.packer(mode, a, lastPack, changes, fieldNo++);
	fireMask.packer(mode, a, lastPack, changes, fieldNo++);
	ridingVeh.packer(mode, a, lastPack, changes, fieldNo++);
	ridingSeat.packer(mode, a, lastPack, changes, fieldNo++);
}

Vehicle::Update::Model(TeulTimeMS& parent)
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

Vehicle::Update::Model& operator = (const Model& rhs)
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

void Vehicle::Update::Model::groupPacker(LOGICAL mode, ACTION_OBJECT* const a, const TeulTimeMS& lastPack, UINT32& changes)
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

