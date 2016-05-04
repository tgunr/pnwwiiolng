#include <syscfg.h> // Path is project specific.
// Created: Tue Nov  8 10:30:34 CST 2005
//
// Functions for managing a player's update framing mode
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "host/hostFraming.h"
#include "host/host.h"
#include "host/hostPlayer.h"
#include "host/hostVehicle.h"
#include "teul/teulEndpoint.h"
#include "teul/teulEnums.h"
#include "wwii/wwiiConfig.h"

//////////////////////////////////////////////////////////////////////
// Check through all the players attached to a vehicle and increment
// the MTU as high as packet sizes will allow

static UINT16
_getMode(HostVehicle* vehicle)
{
	if ( PN_UNLIKELY(vehicle->players().empty()) )
	{
		return host_config.update.defaultUM;
	}

	// Determine if all expected players have attached to the vehicle
	PLAYER_LIST::iterator it = vehicle->players().begin();
	const PLAYER_LIST::const_iterator end = vehicle->players().end();
	UINT16 minMode = (*it)->getPreferredFramingMode();
	while ( it != end )
	{
		Player* player = (*it);
		++it;

		if ( PN_UNLIKELY(player == nullptr) )
			continue;

		if ( player->teulIndex() < 0 || !player->active )
			continue;

		minMode = std::min(minMode, player->getPreferredFramingMode());
	}

	return minMode;
}

//////////////////////////////////////////////////////////////////////
// Get the MTU size of a particular packet size

static SINT32
_getFramingMTU(UINT16 mode)
{
	switch ( framings[mode].packetSize )
	{
	case UpdateFraming::_MAX:
		return Teul::TCPMaxPayload;
	break;
	case UpdateFraming::_LARGE:
		return Teul::TCPMaxPayload;
	break;
	default: break;
	}
	return Teul::TCPStdPayload;
}

//////////////////////////////////////////////////////////////////////
// Select the best framing mode for the vehicle based on the players
// attached to it

void
HostVehicle::selectBestFramingMode()
{
	framingMode = _getMode(this);

	mtu = _getFramingMTU(framingMode);

	UINT32 interval = framings[framingMode].updateInterval;

	betaLog(_FLI, "c:%s v:%u selecting framing mode %u [%u] dU:%u", vehicleData.callsign, index, framingMode, mtu, interval);

	if ( isActive() )
		updateScheduleVehicle(this, interval);
}

//////////////////////////////////////////////////////////////////////
// Receive a player-specific change of framing mode (or initial
// selection)

void
Player::setFramingMode(UINT16 newMode)
{
	newMode = std::min(newMode, UINT16(UpdateFraming::_MODES));

	m_framingMode = newMode;

	if ( vehicle != nullptr )
	{
		vehicle->selectBestFramingMode();
		if ( g_logDebug )
			dbugLog(_FLD, "t:%d p:%u c:%s v:%u best framing mode %u", teulIndex(), playerID(), callsign(), vehicle->index, vehicle->framingMode);
	}
}

//////////////////////////////////////////////////////////////////////
// Return the MTU of either the player's vehicle or their preferred MTU

UINT16
Player::getCurrentFramingMode()
{
	if ( vehicle )
		return vehicle->framingMode;
	return m_framingMode;
}

