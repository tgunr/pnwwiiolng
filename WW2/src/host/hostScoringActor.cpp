#include <syscfg.h>	// Path is project specific.

#include "host/hostPlayer.h"			// For Player
#include "host/hostVehicle.h"			// For HostVehicle
#include "score/scoreActor.h"			// For Scoring::Actor
#include "wwii/wwiiOwnershipEnums.h"	// For WWII_SIDE

//////////////////////////////////////////////////////////////////////
// Construct an actor from a HostVehicle.

Scoring::Actor::Actor(const HostVehicle* vehicle)
	: m_instID(vehicle->instID())
	, m_weapon(vehicle->spec())
	, m_sortieID(vehicle->sortieID())
	, m_playerID(0)						// Don't assume we have a player - yet.
	, m_handle(vehicle->chatHandle())
	, m_inCaptureBldg(vehicle->isInCaptureBuilding() != WWII_NO_SIDE)
	, m_balance(Scoring::Balance::Even)
{
	if ( vehicle->players().empty() ) {
		dbugLog(_FLW, "v:%u has no players attached", vehicle->index);
		return;
	}

	// Use the first available player as a guide.
	const Player* const player = vehicle->players().front();
	if ( player == nullptr ) {
		dbugLog(_FLA, "v:%u Main player is null");
		return;
	}

	m_playerID = player->playerID();
}

Scoring::Actor::Actor(const HostVehicle* const vehicle, const Player* const player)
	: m_instID(vehicle->instID())
	, m_weapon(vehicle->spec())
	, m_sortieID(vehicle->sortieID())
	, m_playerID(player->playerID())
	, m_handle(player->chatHandle())
	, m_inCaptureBldg(vehicle->isInCaptureBuilding() != WWII_NO_SIDE)
	, m_balance(Scoring::Balance::Even)
{
}
