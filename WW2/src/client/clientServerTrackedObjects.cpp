#include "syscfg.h" // Path is project specific.
//
// Date: Fri Mar 24 16:11:48 CST 2006
//
// Client handling over server tracked objects
//
// Copyright (C) PlayNet, Inc 1999-2006 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith
//

#include <map>

#include "teul/teulPackers.h"					// For the packer definitions etc
#include "wwii/wwiiServerTrackedObject.h"		// The base STO type
#include "client/clientServerTrackedObjects.h"		// Our header
#include "client/clientSTO.h"						// Client's extened STO types
#include "fx/stoeffects.h"
#include "chat/chatSTOJoules.h"					// How to describe damage applied
#include "client/GameClient.h"
#include "teul/teulKit.h"						// For teulSend
#include "pers/persmain.h"

static STO_JOULES _stoJoulesPacket;

//////////////////////////////////////////////////////////////////////
// Send any outgoing sto damage/repair joules

void
clientSendPendingSTOJoules()
{
	// Check for any work
	if(_stoJoulesPacket.empty())
		return;

	_stoJoulesPacket.send(PlayerClient()->ChatConnection());

	_stoJoulesPacket.clear();
}

//////////////////////////////////////////////////////////////////////
// Describe how to do damage to various STO's

bool VehicleSTO::applyJoules(const SINT64& joules, class Player* player)
{
	if(joules == 0LL) return false;

	STO_JOULES::array_type::iterator it;
	for(it = _stoJoulesPacket.data().begin(); it != _stoJoulesPacket.data().end(); ++it)
	{
		if(it->m_uid != uid())
			continue;

		SINT64 totalJoules = it->m_joules + joules;

		// In all healthy cases the absolute joules amount should have increased
		// Exceptions would be:
		// - wrap of SINT64 by addition of very large or many values
		// - change from +ve joules to -ve joules (damage vs repair)
		if(joules > 0LL)
		{
			if(totalJoules > it->m_joules && totalJoules > joules)
			{
				it->m_joules = totalJoules;
				return true;
			}
		}
		else
		{
			if(totalJoules < it->m_joules && totalJoules < joules)
			{
				it->m_joules = totalJoules;
				return true;
			}
		}

		// Otherwise we need another entry
	}

	_stoJoulesPacket.add(STOJoules(uid(), joules));

	return true;
}

void TerrainSTO::enter()
{
	const SINT64 joules = -1;
	applyJoules(joules, nullptr);
}

void TerrainSTO::leave()
{
	const SINT64 joules = 0;
	applyJoules(joules, nullptr);
}

bool TerrainSTO::applyJoules(const SINT64& joules, class Player* player)
{
	STO_JOULES::array_type::iterator it;
	for(it = _stoJoulesPacket.data().begin(); it != _stoJoulesPacket.data().end(); ++it)
	{
		if(it->m_uid != uid())
			continue;

		if ( joules == 0 )
			return true;

		SINT64 totalJoules = it->m_joules + joules;

		// In all healthy cases the absolute joules amount should have increased
		// Exceptions would be:
		// - wrap of SINT64 by addition of very large or many values
		// - change from +ve joules to -ve joules (damage vs repair)
		if(joules > 0LL)
		{
			if(totalJoules > it->m_joules && totalJoules >= joules)
			{
				it->m_joules = totalJoules;
				return true;
			}
		}
		else
		{
			if(totalJoules < it->m_joules && totalJoules <= joules)
			{
				it->m_joules = totalJoules;
				return true;
			}
		}

		// Otherwise we need another entry
	}

	_stoJoulesPacket.add(STOJoules(uid(), joules));

	return true;
}

//////////////////////////////////////////////////////////////////////
// Unpack a UID from an action object and delete the STO it ids

static void _removeSTO(ACTION_OBJECT* const a)
{
	UINT16 type = 0;
	UINT32 uid = 0;

	// Extract the type
	a->offset = wwiiPackServerTrackedObjectType(TEUL_UNPACK, a->packed, a->offset, &type, 0);

	// Now get the actual number
	a->offset = teulPackUINT32(TEUL_UNPACK, a->packed, a->offset, &uid, 32);

	if(uid > 0)
	{
		// otherwise fall through and remove it
		wwiiRemoveSTO(uid);
	}
}

//////////////////////////////////////////////////////////////////////
// Process an individual STO from an ACTION_OBJECT stream

bool clientReceiveServerTrackedObject(UINT16 type, ACTION_OBJECT* a)
{
	ServerTrackedObject* sto = nullptr;

	// The type tells us which class to expand to
	switch ( type )
	{
	case ServerTrackedObject::_REMOVAL:
		_removeSTO(a);
		return true;
	break;

	case ServerTrackedObject::_UMS:
	{
		sto = new MobileSpawnSTO(a);
	}
	break;

	case ServerTrackedObject::_PPO:
	{
		sto = new PlayerPlacedSTO(a);
	}
	break;

	case ServerTrackedObject::_BAILOUT:
	{
		sto = new BailoutSTO(a);

		vehVehicle* pv = playerGetVehicle();

		if(sto && pv)
		{
			if(sto->owner() == pv->Chat_Handle) // don't use this sto if we created it
			{
				delete sto;
				sto = nullptr;
			}
			else if(pv->ModelData->MultiCrewPos > 1) // if we're a crew
			{
				ChatHandle& commander = PlayerClient()->MultiCrewCommander();

				// if this is my pilot
				if(commander && commander == sto->owner())
					vehmainBailout(static_cast<BailoutSTO*>(sto));
			}
		}
	}
	break;

	case ServerTrackedObject::_VEHICLE:
	{
		// this STO will get added at next spawn
		// prevents double vehicle rendering when despawning to a UMS
		vehVehicle* pv = playerGetVehicle();

		if(pv && pv->VehicleDeadTime == 0)
			sto = new VehicleSTO(a);
	}
	break;

	case ServerTrackedObject::_HULK:
		sto = new HulkSTO(a);
	break;

	case ServerTrackedObject::_EFFECT:
		sto = new SmokeSTO(a);
	break;

	case ServerTrackedObject::_ORDNANCE:
		sto = new OrdnanceSTO(a);
	break;

	default:
		WriteLog(LOG_COMM, LOG_WARN, "Unknown STO type %hu", type);
	break;
	}

	if ( PN_UNLIKELY(sto == nullptr) )
		return false;

	switch ( ServerTrackedObject::addToIndex(sto) )
	{
	case ServerTrackedObject::_ADD_FAIL:
		delete sto;
		return false;
	break;
	case ServerTrackedObject::_ADD_SUCCESS:
	break;
	case ServerTrackedObject::_ADD_DUPLICATE:
		delete sto;
	break;
	}

	return true;
}

