#include <syscfg.h> // Path is project specific.
// We need to register some callbacks to allow the exchange of
// events.
// Only the MMHost needs to receive callbacks.

#include "map/mapPlayer.h"
#include "mm/mmEnums.h"				// For CAPACITY_REQUEST_STATE
#include "mm/mmMain.h"				// For mapGetStratIndex()
#include "score/scoreAssist.h"
#include "score/scoreCapture.h"
#include "score/scoreConstants.h"
#include "score/scoreDeath.h"
#include "score/scoreDespawn.h"
#include "score/scoreGuard.h"
#include "score/scoreHit.h"
#include "score/scoreKill.h"
#include "score/scorePacket.h"
#include "score/scorePPO.h"
#include "score/scoreResupply.h"
#include "score/scoreSortie.h"
#include "score/scoreUMSKill.h"
#include "score/scoreWeaponReturn.h"
#include "strt/strtConstants.h"
#include "strt/strtWeaponsReturn.h"
#include "wwii/wwiiCallbacks.h"
#include "wwii/wwiiConfig.h"
#include "wwii/wwiiExitCodes.h"
#include "wwii/wwiiRTBCodes.h"

extern SINT32 missionSendMMCapacityRequest(Player* player, CAPACITY_REQUEST_STATE tag);

//////////////////////////////////////////////////////////////////////
//

// Quick wrapper for adding cases.
#define PROCESS(_Type)	\
	case Scoring :: Events :: _Type : \
	{	\
		Scoring :: _Type& typedEvt = *static_cast<Scoring :: _Type *>(evt); \
		success = typedEvt.process(lhsPlayer); \
	}	\
	break;

static SINT32
_recvEvent(ACTION_OBJECT* const a)
{
	Scoring::EventBase* evt = (Scoring::EventBase*)(a->packed);
	auto& lhs = evt->lhs();
	dbugLog(_FLI, "p:%u c:%s v:%u in:%llu Scoring::Event::recvEvent:: type %u. Processing: "
					, lhs.playerID()
					, Chat::Name(lhs.handle()).c_str()
					, lhs.sortieID()
					, lhs.instID()
					, evt->type()
					);

	Player* const lhsPlayer = PlayerInterface::Lookup(lhs.playerID());
	if ( lhsPlayer == nullptr )
	{
		dbugLog(_FLM, "p:%u c:%s v:%u in:%llu Player not online for scoring event"
					, lhs.playerID()
					, Chat::Name(evt->lhs().handle()).c_str()
					, lhs.sortieID()
					, lhs.instID()
					);
		return 0;
	}

	bool success = false;
	switch ( evt->type() )
	{
	case Scoring::Events::Invalid:
		dbugLog(_FLA, " That's an INVALID event!");
	break;
		PROCESS( Assist )
		PROCESS( Capture )
		PROCESS( Death )
		PROCESS( Despawn )
		PROCESS( Guard )
		PROCESS( Hit )
		PROCESS( Kill )
		PROCESS( PPO )
		PROCESS( Resupply )
		PROCESS( UMSKill )
		PROCESS( WeaponReturn )
	default:
		dbugLog(_FLA, "Unhandled event type");
	break;
//	default:
//		dbugLog(_FLA, "Unknown scoring event: %u in inst %llu", evt->type(), evt->instanceID());
//	break;
	}

	if ( success == false )
		return -EINVAL;

	return a->offset;
}

//////////////////////////////////////////////////////////////////////
// Find matching sortie.

Scoring::Sortie*
Scoring::FindSortieForPlayer(const Player* const player, const sortieid_t sortieID)
{
	// Avoid a lookup if the sortie is current.
	if ( player->sortie() && player->sortie()->sortieID() == sortieID )
		return const_cast<Scoring::Sortie*>(player->sortie());
	return Scoring::Sortie::Lookup(sortieID);
}

//////////////////////////////////////////////////////////////////////
// Process functions.

bool Scoring::Assist::process(Player* const /*lhsPlayer*/)
{
	dbugLog(_FLI, " assist");		///TODO:134:REMOVE

	///TODO:SCORING+: Similar to handling a kill, except we give
	/// points to other people too.

	return true;
}

//////////////////////////////////////////////////////////////////////
// Credit a player for a capture.

bool Scoring::Capture::process(Player* const lhsPlayer)
{
	dbugLog(_FLI, " capture");	///TODO:134:REMOVE

	Scoring::Sortie* const sortie = FindSortieForPlayer(lhsPlayer, lhs().sortieID());
	if ( sortie == nullptr )
		return true;

	///TODO:SCORING+: We need to move the fancy stuff to Scoring::Sortie rather than
	/// player->missionPacket.
	++lhsPlayer->missionPacket().scoring().captures;

	dbugLog(_FLD, "t:%d p:%u c:%s captures = %u", lhsPlayer->teulIndex(), lhsPlayer->playerID(), lhsPlayer->callsign(), lhsPlayer->missionPacket().scoring().captures.get());

	return true;
}

//////////////////////////////////////////////////////////////////////
// Process a vehicle despawn event.

static const CODE_RTB rtbCodes[] = { RTB_BAIL_ENEMY, RTB_SUCCESS, RTB_DIED, RTB_BAIL_ENEMY, RTB_BAIL_FRIENDLY };
extern const char* const rtbNames[] = { "rtb", "res", "mia", "kia" };

bool Scoring::Despawn::process(Player* const lhsPlayer)
{
	const CODE_RTB rtbCode = (exitCode() <= PEXIT_OVER_FRIENDLY_TERRITORY) ? rtbCodes[exitCode()] : rtbCodes[0];

	dbugLog(_FLI, " despawn, ec:%u rtb:%u", exitCode(), rtbCode);

	Scoring::Sortie* const sortie = FindSortieForPlayer(lhsPlayer, lhs().sortieID());
	if ( sortie == nullptr )
	{
		dbugLog(_FLI, " no sortie"); ///TODO:134:REMOVE
		return true;
	}

	sortie->setRTB(rtbCode, rhs());
	sortie->despawned();

	if ( (lhsPlayer->requestedCapacity() == false) & (lhsPlayer->receivedCapacity() == false) )
	{
		missionSendMMCapacityRequest(lhsPlayer, CAPACITY_AFTER_MISSION);
	}

	// Did we take a hit within the last 1 minute 15 seconds?
	const bool wasRecentlyHit = (sortie->lastHitTime() >=  teulClock64 - (75 * 1000));

	WeaponsReturn weaponsReturn(sortie->sortieID(), sortie->despawnedTo(), (UINT32)exitCode(), isVehicleDead(), wasRecentlyHit, false, isResupply());
	weaponsReturn.m_position = position();

	Teul::Packet outgoing(Teul::PacketId(Strat::CallbackX, STRT_SYNC_WEAPONS_RETURN_Y), weaponsReturn);
	const SINT32 retval = outgoing.send(mapGetStratIndex());
	if ( retval < 0 )
	{
		dbugLog(_FLA, "v:%u couldn't send weaponsreturn retval:%d", sortie->sortieID(), retval);
	}

	return true;
}

//////////////////////////////////////////////////////////////////////
// Credit a player for guarding a capturable.

bool Scoring::Guard::process(Player* const lhsPlayer)
{
	dbugLog(_FLI, " guard"); ///TODO:134:REMOVE

	const UINT32 pointsPer_s = (UINT32)host_config.score.guardPointPer_m * 60;
	if ( pointsPer_s == 0 )
		return true;

	Scoring::Sortie* const sortie = FindSortieForPlayer(lhsPlayer, lhs().sortieID());
	if ( sortie == nullptr )
		return true;

	const UINT32 guardPoints = seconds() / pointsPer_s;
	if ( guardPoints <= 0 )
		return true;

	///TODO:SCORING+: Move to sortie stuff.
	lhsPlayer->missionPacket().scoring().guardingPoints += guardPoints;

	dbugLog(_FLI, " guardpoints = %u", lhsPlayer->missionPacket().scoring().guardingPoints.get());

	return true;
}

//////////////////////////////////////////////////////////////////////
// Credit a player for hitting another vehicle.

bool Scoring::Hit::process(Player* const lhsPlayer)
{
	dbugLog(_FLI, " hit"); ///TODO:134:REMOVE

	Scoring::Sortie* const sortie = FindSortieForPlayer(lhsPlayer, lhs().sortieID());
	if ( sortie == nullptr )
		return true;

	Player* const rhsPlayer = PlayerInterface::Lookup(rhs().playerID());
	if ( rhsPlayer != nullptr )
	{
		++rhsPlayer->missionPacket().scoring().hits;

		Scoring::Sortie* const shooterSortie = FindSortieForPlayer(rhsPlayer, rhs().sortieID());
		if ( shooterSortie != nullptr )
		{
			shooterSortie->addHitTo(lhs(), joules());
		}
	}

	///TODO:SCORING+: Track friendly hits.

	if ( sortie )
		sortie->addHitBy(rhs(), joules());

	return true;
}

//////////////////////////////////////////////////////////////////////
// Credit a vehicle (all players) for getting a kill.

bool Scoring::Kill::process(Player* const lhsPlayer)
{
	dbugLog(_FLI, " kill"); ///TODO:134:REMOVE

	Scoring::Sortie* const killerSortie = FindSortieForPlayer(lhsPlayer, lhs().sortieID());
	Scoring::Sortie* const deadSortie = Sortie::Lookup(rhs().sortieID());
	if ( killerSortie == nullptr )
		dbugLog(_FLA, "p:%u c:%s v:%u unable to find this killer sortie", lhsPlayer->playerID(), lhsPlayer->callsign(), lhs().sortieID());
	if ( deadSortie == nullptr )
		dbugLog(_FLA, "p:%u c:%s v:%u unable to find this victim sortie", rhs().playerID(), Chat::Name(rhs().handle()).c_str(), rhs().sortieID());
	if ( killerSortie == nullptr || deadSortie == nullptr )
		return false;

	///TODO:SCORING: Process the kill.

	return true;
}

//////////////////////////////////////////////////////////////////////
// Credit a player for killing a PPO.

bool Scoring::PPO::process(Player* const /*lhsPlayer*/)
{
	dbugLog(_FLI, " ppokill"); ///TODO:134:REMOVE

	///TODO:SCORING+: Score for ppo kills?

	return true;
}

//////////////////////////////////////////////////////////////////////
// Credit a player for giving another player ammo.

bool Scoring::Resupply::process(Player* const lhsPlayer)
{
	dbugLog(_FLI, " resupply"); ///TODO:134:REMOVE

	++ lhsPlayer->missionPacket().scoring().resupplies;
	++ lhsPlayer->missionPacket().scoring().resuppliesScored;

	return true;
}

//////////////////////////////////////////////////////////////////////
// Credit a player for killing a UMS.

bool Scoring::UMSKill::process(Player* const lhsPlayer)
{
	dbugLog(_FLI, " umskill"); ///TODO:134:REMOVE

	++ lhsPlayer->missionPacket().scoring().umsKills;

	return true;
}

//////////////////////////////////////////////////////////////////////
// Handle a vehicle being returned to supply in some state and
// give the player bonus points for returning it, etc.

bool Scoring::WeaponReturn::process(Player* const /*lhsPlayer*/)
{
	dbugLog(_FLI, " weaponreturn"); ///TODO:134:REMOVE

	///TODO:SCORING: Handle weapon's return (augment player's score)

	return true;
}

namespace Scoring
{
SINT32
InstallCallbacks()
{
	BEGIN_CALLBACK_REGISTRATION(Scoring::CallbackX)
		REGISTER_RECEIVER(Scoring::EVENT_Y, _recvEvent);
	END_CALLBACK_REGISTRATION
}
}
