#include <syscfg.h> // Path is project specific.
// Date: Wed Oct 25 14:59:29 CDT 2006
//
// Host interface for the brigade control system
//
// Copyright (C) PlayNet, Inc 1999-2006 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "chat/chatBrigadeControl.h"
#include "chat/chatBrigadeControlList.h"
#include "chat/chatBrigadeVote.h"				// For class Vote
#include "chat/chatBroadcastHCPacket.h"
#include "chat/chatBroadcastObject.h"
#include "chat/chatConstants.h"
#include "chat/chatData.h"
#include "chat/chatDB.h"
#include "chat/chatDecisions.h"
#include "chat/chatHost.h"
#include "chat/chatHostMessages.h"
#include "chat/chatList.h"
#include "chat/chatLStringErr.h"
#include "chat/chatName.h"
#include "chat/chatPlayer.h"						// For class and PlayerInterface::Lookup
#include "strt/strtAttackOrder.h"
#include "strt/strtConstants.h"
#include "strt/strtChatTypes.h"
#include "strt/strtCPFlags.h"
#include "teul/teulPacket.h"
#include "wwii/wwiiConfig.h"
#include "wwii/wwiiHCUnits.h"

// Temporary
#include "chat/chatDotCommand.h"

class BrigadeController;

using CONTROLLERS = std::map<UINT32, BrigadeController*>;
static CONTROLLERS controllers;
using ELECTIONS = std::set<UINT32>;
static ELECTIONS elections;

using BRIGADE_UPDATES = std::vector<BrigadeController*>;

//////////////////////////////////////////////////////////////////////
// The brigade controller class itself

class BrigadeController
{
public:
	BrigadeController(HCUnit& fromUnit)
		: m_unit(fromUnit), m_oic(nullptr), m_oicHandle(0LL), m_appointed(0)
		, m_vote(nullptr)
	{
	}

	~BrigadeController();

public:
	// Get will create a controller if you need one
	static BrigadeController& Get(UINT32 unitID);
	// Lookup will only return a controller if one exists
	static BrigadeController* Lookup(UINT32 unitID);

public:
    Player* oic() const { return m_oic; }
    ChatHandle oicHandle() const { return m_oicHandle; }
	const HCUnit& unit() const { return m_unit; }
	const Vote* vote() const { return m_vote; }
	Vote* vote() { return m_vote; }

public:
	VOTE_PERMIT votePermitRequirement() const;
	bool canVote(const Player* player) const;
	bool isOfferedTo(const Player* player) const;

	void voteFor(Vote::Mode mode, BrigadeControl::BRIGADE_ACTION_REQ action, const ChatHandle nominee, const ChatHandle sponsor, UINT32 duration, const FULL_OBJ_ID origin=nullObjID);
	void voteVeto(Vote::Mode mode, const ChatHandle nominee, const ChatHandle sponsor);
	void cancelVote();

	void assignOIC(BrigadeControl::BRIGADE_ACTION_REQ action, Player* player, const ChatHandle sponsor);
	void checkOIC(bool announce);
	void removeOIC(BrigadeControl::BRIGADE_ACTION_REQ action, const ChatHandle sponsor, bool announce, bool checkForReplacement);

	void requestNominate(Player* sponsor, const ChatHandle officer);
	void requestVetoPlayer(Player* sponsor, const ChatHandle officer);
	void requestVetoAction(Player* sponsor, const FULL_OBJ_ID destination);
	void requestVolunteer(Player* player);
	void requestResign(Player* player);
	void requestReplace(Player* player, const ChatHandle officer);
	void requestRemove(Player* player, const ChatHandle officerHandle);
	void requestMove(Player* player, const FULL_OBJ_ID destination, const FULL_OBJ_ID from);
	void requestAttack(Player* player, FULL_OBJ_ID& target);
	void requestDefend(Player* player, FULL_OBJ_ID& target);
	void requestClear(Player* player, FULL_OBJ_ID& target);

	void processVoteExpired();
	bool processElectionExpired();
	bool processOfferExpired();
#ifdef SLOW_REMOVE
	bool processToRemoveExpired();
#endif
	void processReplacingExpired();
	void processToMoveExpired();
	void processToAttackExpired();
	void processToDefendExpired();
	void processToClearExpired();

private:
	void setOIC(Player* player)
	{
		m_oic = player;
		m_oicHandle = (player) ? player->chatHandle() : 0LL;
		m_appointed = 0;
		///TODO:128 Tell the strat host
	}

private:
	HCUnit&		m_unit;			// Pointer to the HC Unit
	Player*     m_oic;             // Current officer in charge
	ChatHandle	m_oicHandle;
	UINT32		m_appointed;		// When they took the seat
	Vote*		m_vote;
};

//////////////////////////////////////////////////////////////////////
// Constructor for brigade control notice response

BrigadeControlNotice::
BrigadeControlNotice(UINT32 notice, UINT32 error, BRIGADE_ACTION_REQ action, VOTE_PERMIT votePermit
					, HCUnit const& unit, const ChatHandle subject, const ChatHandle actor
					, UINT32 duration)
	: BrigadeControl(action, unit.country(), unit.branch(), unit.unitID(), subject, actor)
	, m_notice(notice), m_error(error), m_time(duration), m_votePermit(votePermit)
{
	if ( isTargettedAction() )
		m_origin.FromKey(subject);
}

//////////////////////////////////////////////////////////////////////
// Destructor for a controller

BrigadeController::~BrigadeController()
{
	if ( m_vote )
		cancelVote();
}

//////////////////////////////////////////////////////////////////////
// Get or create a particular brigade controller

BrigadeController&
BrigadeController::Get(UINT32 unitID)
{
	BrigadeController* result = nullptr;
	CONTROLLERS::iterator it = controllers.lower_bound(unitID);
	if ( it != controllers.end() && it->first == unitID )
		result = it->second;
	else
	{
		HCUnit* unit = HCUnit::Lookup(unitID);
		if ( unit == nullptr )
		{
			dbugLog(_FLA, "Invalid HCUnit: %u", unitID);
			throw BCERR_INVALID_REQ;
		}
		result = new BrigadeController(*unit);
		controllers.insert(it, std::make_pair(unitID, result));
	}

	return *result;
}

//////////////////////////////////////////////////////////////////////
// Find a particular brigade controller

BrigadeController*
BrigadeController::Lookup(UINT32 unitID)
{
	CONTROLLERS::iterator it = controllers.find(unitID);
	return ( it != controllers.end() ) ? it->second : nullptr;
}

//////////////////////////////////////////////////////////////////////
// Validation helpers

// Officer must be populated with a legal alue
static void
_requiresOfficer(const ChatHandle officer)
{
	if ( chatIsValidHandle(officer) )
		return;

	dbugLog(_FLW, "officer is required");
	throw BCERR_INVALID_CALLSIGN;
}

// Specified player must be in the specified brigade id and
// not attempting to lead a different brigade
static void
_requirePlayerInBrigade(const Player* player, const HCUnit& unit)
{
	if ( player == nullptr )
		throw BCERR_PLAYER_UNAVAILABLE;
	if ( player->brigadeID() == unit.unitID() )
		return;
	if ( unit.isAbove(player->brigadeID()) )
		return;
	throw BCERR_WRONG_BRIGADE;
}

//////////////////////////////////////////////////////////////////////
// Populate the controllers

static void
_populateControllers()
{
	static bool hasRun = false;
	if ( hasRun )
		return;

	for ( auto it = hcUnits.begin(), end = hcUnits.end(); it != end; ++it )
		BrigadeController::Get(it->second->unitID());

	hasRun = true;
}

//////////////////////////////////////////////////////////////////////
// Action functions

static void
_requestList(Player* player, UCHAR country, UCHAR branch)
{
	if ( !isValidSide(player->side()) )
	{
		PLAYER_LOGA("invalid side %u selected", player->side());
		throw BCERR_INVALID_REQ;
	}

	// You can't select info for the other side
	if ( country && wwiiSide(country) != player->side() )
	{
		PLAYER_LOGA("tried to get list of wrong (%u) side", player->side());
		throw BCERR_INVALID_REQ;
	}

	_populateControllers();

	Teul::Packet outgoing(Teul::PacketId(Chat::CallbackX, Chat::BRIGADE_CONTROL_LIST_Y));

	outgoing.reserve((controllers.size() / 3) * sizeof(BrigadeControlList));
	outgoing.append(country);
	outgoing.append(branch);
	outgoing.append(player->permissionMask());

	for ( auto it = controllers.begin(), end = controllers.end(); it != end; ++it )
	{
		BrigadeController& bc = *(it->second);
		if ( bc.unit().side() != player->side() )
			continue;
		if ( country && bc.unit().country() != country )
			continue;
		if ( branch && bc.unit().branch() != branch )
			continue;

		// Create a new BCL on the packet.
		auto& entry = outgoing.construct<BrigadeControlList>(it->first, bc.canVote(player));
		if ( bc.oic() )
		{
			entry.m_officer = bc.oic()->chatHandle();
			entry.m_officerRank = bc.oic()->effectiveRank();
		}
		else
		{
			if ( bc.isOfferedTo(player) )
				entry.m_canTakeCharge = true;
			else if ( !bc.vote() )
				entry.m_canTakeCharge = player->isActiveCommander();
		}
	}

	outgoing.send(player->teulIndex());
}

//////////////////////////////////////////////////////////////////////
// Send a Notice to all of a unit and its descendants

#ifdef OMG
static void
_noticeToBrigade(UINT32 notice, UINT32 action, UINT32 error, VOTE_PERMIT votePermit
				, const HCUnit& unit, const ChatHandle nominee, const ChatHandle sponsor, UINT32 delay, FULL_OBJ_ID origin=nullObjID
				, Player* alsoPlayer=NULL)
{
	BrigadeControlNotice outgoing(notice, error, action, votePermit, unit, nominee, sponsor, delay);
	if ( outgoing.isTargettedAction() )
		outgoing.m_target.fromKey(nominee);
	else
		outgoing.m_target = nullObjID;
	outgoing.m_origin = origin;

	HCUnitBroadcast::send(&unit, &outgoing, outgoing.s_callback);
	if ( alsoPlayer && alsoPlayer->teulIndex() >= 0 && !alsoPlayer->isActiveCommander() && alsoPlayer->brigadeID() != unit.unitID() )
		outgoing.send(alsoPlayer->teulIndex());
}
#endif

static void
_noticeToOfficers(UINT32 notice, BrigadeControl::BRIGADE_ACTION_REQ action, UINT32 error, VOTE_PERMIT votePermit
				, const HCUnit& unit, const ChatHandle nominee, const ChatHandle sponsor, UINT32 delay=0, const FULL_OBJ_ID origin=nullObjID
				, Player* alsoPlayer=nullptr)
{
	BrigadeControlNotice outgoing(notice, error, action, votePermit, unit, nominee, sponsor, delay);
	if ( outgoing.isTargettedAction() )
		outgoing.m_target.FromKey(nominee);
	else
		outgoing.m_target = nullObjID;
	outgoing.m_origin = origin;

	HCUnitBroadcast::sendToOfficers(&unit, &outgoing, outgoing.s_callback);
	if ( alsoPlayer && !alsoPlayer->isActiveCommander() && alsoPlayer->teulIndex() >= 0 )
		outgoing.send(alsoPlayer->teulIndex());
}

static void
_noticeToSide(UINT32 notice, BrigadeControl::BRIGADE_ACTION_REQ action, const HCUnit& unit, const FULL_OBJ_ID target, const ChatHandle sponsor, UCHAR side, const FULL_OBJ_ID origin=nullObjID)
{
	BrigadeControlNotice outgoing(notice, 0, action, VP_NONE, unit, 0LL, sponsor, 0);
	outgoing.m_target = target;
	outgoing.m_origin = origin;

	chatBroadcastObject(sidePlayers[side].begin(), sidePlayers[side].end(), outgoing.s_callback, outgoing);
}

static void
_noticeToSide(UINT32 notice, BrigadeControl::BRIGADE_ACTION_REQ action, const HCUnit& unit, const ChatHandle nominee, const ChatHandle sponsor)
{
	BrigadeControlNotice outgoing(notice, 0, action, VP_NONE, unit, nominee, sponsor, 0);
	outgoing.m_target = outgoing.m_origin = nullObjID;

	chatBroadcastObject(sidePlayers[unit.side()].begin(), sidePlayers[unit.side()].end(), outgoing.s_callback, outgoing);
}


//////////////////////////////////////////////////////////////////////
// Record a voting event

static void
_voting(const ChatHandle sponsor, UINT32 action, const char* description, UINT32 duration, const ChatHandle nominee, const char* unitName, UINT32 mode)
{
	Chat::Name sponsorCallsign(sponsor);
	char nomineeCallsign[32];
	if ( Vote::IsTargettedAction(mode) )
	{
		FULL_OBJ_ID fullObjID;
		fullObjID.FromKey(nominee);
		strprintf(nomineeCallsign, "%d.%d/%u", fullObjID.cell.ox, fullObjID.cell.oy, fullObjID.id);
	}
	else
		chatGetCallsign(nomineeCallsign, nominee);

	static const char* codes[Vote::MAX_VOTE_MODES] =
	{
		"nomination"
	,	"offer"
	,	"toremove"
	,	"removing"
	,	"replacing"
	,	"tomove"
	,	"toattack"
	,	"toclear"
	,	"todefend"
	,   "changecountry"
	};
	dbugLog(_FLI, "%s %s(in %u) %s %s %s %d"
					, sponsorCallsign.c_str()
					, description
					, duration
					, nomineeCallsign
					, unitName
					, codes[mode]
					, mode
					);
}

//////////////////////////////////////////////////////////////////////
// Check if a player is being offered control of a brigade

bool
BrigadeController::isOfferedTo(const Player* player) const
{
	return ( vote() && vote()->m_voteMode == Vote::OFFER && player->hasHandle(vote()->m_nominee) );
}

//////////////////////////////////////////////////////////////////////
// Determine the vote permit for this brigade

VOTE_PERMIT
BrigadeController::votePermitRequirement() const
{
	if ( !vote() )
		return VP_NONE;
	switch ( vote()->m_voteMode )
	{
	case Vote::NOMINATION:
	case Vote::TOREMOVE:
		return VP_COMMANDER;

	case Vote::TOMOVE:
	case Vote::TOATTACK:
	case Vote::TODEFEND:
	case Vote::TOCLEAR:
		return VP_OFFICER;

	default:
	break;
	}

	return VP_NONE;
}

//////////////////////////////////////////////////////////////////////
// Test whether or not a player can vote for a given brigade

bool
BrigadeController::canVote(const Player* player) const
{
	switch ( votePermitRequirement() )
	{
	case VP_ALL:
		return true;
	case VP_COMMANDER:
		return player->isActiveCommander();
	case VP_OFFICER:
		return player->isActiveOfficer();
	case VP_OIC:
		return (player == oic());
	case VP_NONE:
	default:
		return false;
	}
}

//////////////////////////////////////////////////////////////////////
// Initiate an election or vote "for" the current vote on a nominee

void
BrigadeController::voteFor(Vote::Mode mode, BrigadeControl::BRIGADE_ACTION_REQ action, const ChatHandle nominee, const ChatHandle sponsor, UINT32 duration, const FULL_OBJ_ID origin)
{
	if ( m_vote )
		throw BCERR_ALREADY_VOTING;

	_voting(sponsor, action, "nominating", duration, nominee, unit().callsign(), mode);
	m_vote = new Vote(mode, action, nominee, sponsor, duration, origin);
	if ( m_vote == nullptr )
		throw BCERR_NO_VOTE_TO_VETO;

	elections.insert(unit().unitID());

	// Send notification to HC officers
	_noticeToOfficers(BrigadeControl::_PROPOSAL, m_vote->m_forAction, 0, votePermitRequirement(), unit(), nominee, sponsor, duration, origin);
}

//////////////////////////////////////////////////////////////////////
// Eliminate the vote for this controller

void
BrigadeController::cancelVote()
{
	// Delete the vote
	delete m_vote;
	m_vote = nullptr;

	// Remove this brigade from the list of pending elections
	elections.erase(unit().unitID());
}

//////////////////////////////////////////////////////////////////////
// Vote against an vote on a given nominee

void
BrigadeController::voteVeto(Vote::Mode mode, const ChatHandle nominee, const ChatHandle sponsor)
{
	if ( !vote() )
		throw BCERR_NO_VOTE_TO_VETO;

	if ( vote()->m_voteMode != mode	)
		throw BCERR_VETO_MISMATCH;
	if ( vote()->isTargettedAction() && vote()->m_nominee != nominee )
		throw BCERR_VETO_MISMATCH;
	else if ( !vote()->isTargettedAction() && chatGetLowercaseHandle(vote()->m_nominee) != chatGetLowercaseHandle(nominee) )
		throw BCERR_VETO_MISMATCH;

	_voting(sponsor, vote()->m_forAction, "vetoing", 0, vote()->m_nominee, unit().callsign(), mode);

	// Send notification to officers
	_noticeToOfficers(BrigadeControl::_VETOED, vote()->m_forAction, 0, VP_NONE, unit(), vote()->m_nominee, sponsor, 0, vote()->m_origin);

	cancelVote();
}

//////////////////////////////////////////////////////////////////////
// Appoint a player as OIC of this brigade

void
BrigadeController::assignOIC(BrigadeControl::BRIGADE_ACTION_REQ action, Player* player, const ChatHandle sponsor)
{
	// Assign this player to this brigade
	PLAYER_LOGI("Assigning control of brigade#%u %s", unit().unitID(), unit().callsign());

	setOIC(player);

	// Update the player's status, add them to the HC lists etc
	player->SetLeadingBrigade(unit().unitID());

	// Note: we put the player into the command position first so that when
	// we send this notification out, they're included in the active officer list
	// and thus get the feedback message too
	_noticeToSide(BrigadeControl::_APPOINTED, action, unit(), oicHandle(), sponsor);

	cancelVote();
}

//////////////////////////////////////////////////////////////////////
// Remove the existing OIC

void
BrigadeController::removeOIC(BrigadeControl::BRIGADE_ACTION_REQ action, const ChatHandle sponsor, bool announce, bool checkForReplacement)
{
	if ( oic() )
	{
		dbugLog(_FLI, "t:%d p:%u c:%s Removing OIC status from hcu:%u(%s)"
						, oic()->teulIndex(), oic()->playerID(), oic()->callsign()
						, unit().unitID(), unit().shortName());

		Player* nominee = PlayerInterface::Lookup(oicHandle());

		if ( announce )
			_noticeToSide(BrigadeControl::_REMOVED, action, unit(), oicHandle(), sponsor);

		if ( nominee == nullptr )
		{
			dbugLog(_FLN, "hcu:%u(%s) during removal OIC was not online", unit().unitID(), unit().callsign());
		}
		else if ( nominee->leadingBrigade() == unit().unitID() )
			nominee->SetLeadingBrigade(0);

		// Make sure we don't try to remove someone else
		if ( !oic()->hasHandle(oicHandle()) )
			LogAbort("hcu:%u(%s) during removal OIC did not match the one being removed", unit().unitID(), unit().callsign());
	}

	setOIC(nullptr);

	// Check for a replacement vote and, if it exists, execute it.
	if ( checkForReplacement && vote() && vote()->m_voteMode == Vote::TOREPLACE )
		processReplacingExpired();

	// Check for a removal vote
	if ( vote() && (vote()->m_voteMode == Vote::TOREMOVE || vote()->m_voteMode == Vote::REMOVING) )
		cancelVote();
}

//////////////////////////////////////////////////////////////////////
// Check if the existing OIC of a brigade is currently valid (online)

void
BrigadeController::checkOIC(bool announce)
{
	if ( !oic() )
		return;

	Player* player = PlayerInterface::Lookup(oicHandle());
	if ( player == nullptr || player != oic() )
		removeOIC(BrigadeControl::REQ_REMOVE, oicHandle(), announce, true);
}

//////////////////////////////////////////////////////////////////////
// Nominate someone for OIC of a brigade; other HC officers have
// 60 seconds in which to veto. After 60 seconds, with no vetos,
// the position is offered to the player who then has 90 seconds
// to accept.

void
BrigadeController::requestNominate(Player* sponsor, const ChatHandle officerHandle)
{
	checkOIC(true);

	if ( oic() )
		throw BCERR_HAS_OIC;

	// Is a vote already in progress?
	if ( vote() )
		throw BCERR_ALREADY_VOTING;

	// Only HC officers can nominate
	if ( !sponsor->isActiveCommander() )
		throw BCERR_MUST_BE_HC_STAFF;

	// Are they actually trying to nominate themself?
	if ( sponsor->hasHandle(officerHandle) )
	{
		return requestVolunteer(sponsor);
	}

	// Make sure an officer is specified
	_requiresOfficer(officerHandle);

	// Make sure the officer is online/onside/inbrigade
	Player* nominee = PlayerInterface::Lookup(officerHandle);
	if ( !nominee || !nominee->isVisibleTo(sponsor) )
		throw BCERR_PLAYER_UNAVAILABLE;
	if ( nominee->permissions(ALLIED_COMMAND_STAFF|AXIS_COMMAND_STAFF) )
		throw BCERR_CANT_NOMINATE_HC;
	if ( nominee->permissions(CAN_OIC) == false )
		throw BCERR_NOMINEE_CANT_OIC;
	if (!unit().isBrigade() && !unit().isGarrison())
		throw BCERR_WRONG_BRIGADE;
	_requirePlayerInBrigade(nominee, unit());
	if ( nominee->leadingBrigade() )
		throw BCERR_PLAYER_IS_ALREADY_OIC;

	// Make sure they're not being nominated or removed from another brigade
	for ( auto it = controllers.begin(), end = controllers.end(); it != end; ++it )
	{
		BrigadeController* bc = it->second;
		if ( !bc->vote() || bc->vote()->isTargettedAction() )
			continue;
		if ( nominee->hasHandle(bc->vote()->m_nominee) )
			throw BCERR_PLAYER_IS_ALREADY_OIC;
	}

	voteFor(Vote::NOMINATION, BrigadeControl::REQ_NOMINATE, nominee->chatHandle(), sponsor->chatHandle(), host_config.oic.nominationVoteTime_s);
}

//////////////////////////////////////////////////////////////////////
// Request to take charge of a brigade; you must either be an active
// HC or you must have been nominated without objection.

void
BrigadeController::requestVolunteer(Player* player)
{
	checkOIC(true);

    // Is someone else already in control of the brigade?
	if ( oic() )
	{
		// Is it you?
		if ( player == oic() )
			throw BCERR_YOU_ALREADY_OIC;
		// Is it an earlier instance of you?
		if ( player->hasHandle(oicHandle()) )
		{
			m_oic = player;
			throw BCERR_YOU_ALREADY_OIC;
		}
		throw BCERR_HAS_OIC;
	}

	if ( player->brigadeID() != unit().unitID() && !unit().isAbove(player->brigadeID()) )
		throw BCERR_YOU_WRONG_BRIGADE;
	if ( player->leadingBrigade() )
	{
		if ( player->leadingBrigade() != unit().unitID() )
			throw BCERR_YOU_ALREADY_OIC;
		player->SetLeadingBrigade(0);
	}

	// HC Staff can "take charge" of a brigade (volunteer) anytime they
	// like, so long as it is empty and someone else is not being
	// nominated right now.
	if ( isOfferedTo(player) )
	{
		assignOIC(vote()->m_forAction, player, vote()->m_sponsor);
		return;
	}

	if ( vote() && vote()->m_voteMode == Vote::OFFER )
		throw BCERR_OFFER_PENDING;
	if ( !player->isActiveCommander() )
		throw BCERR_NOT_OFFERED;

	assignOIC(BrigadeControl::REQ_VOLUNTEER, player, player->chatHandle());
}

//////////////////////////////////////////////////////////////////////
// Vote against someone's nomination/removal for OIC of a brigade

void
BrigadeController::requestVetoPlayer(Player* vetoer, const ChatHandle officer)
{
	// Only HC staff can veto
	if ( !vetoer->isActiveCommander() )
		throw BCERR_MUST_BE_HC_STAFF;

	_requiresOfficer(officer);

	if ( vetoer->hasHandle(officer) )
		throw BCERR_CANT_VETO_VS_SELF;

	voteVeto(vote()->m_voteMode, officer, vetoer->chatHandle());
}

//////////////////////////////////////////////////////////////////////
// Vote against someone's action proposal

void
BrigadeController::requestVetoAction(Player* vetoer, const FULL_OBJ_ID destination)
{
	// Only HC staff can veto
	if ( !vetoer->isActiveOfficer() )
		throw BCERR_MUST_BE_OFFICER;

	voteVeto(vote()->m_voteMode, destination.Key(), vetoer->chatHandle());
}

//////////////////////////////////////////////////////////////////////
// Resign from OIC of a brigade -- allows the system to provide
// a graceful advance warning to players
///TODO:OIC: Audit when players resign vs bail and whether their
///TODO:OIC: objectives are completed at the time or not.

void
BrigadeController::requestResign(Player* player)
{
	if ( !player->isActiveOfficer() )
		throw BCERR_MUST_BE_OFFICER;

	// You have to be in this brigade :)
	_requirePlayerInBrigade(player, unit());

	// And you do, kinda, have to be leading the brigade :)
	if ( player->leadingBrigade() != unit().unitID() || !player->hasHandle(oicHandle()) )
		throw BCERR_NOT_BRIGADE_LEADER;

	// If we move to delayed resigns, we can change the last argument to false
	removeOIC(BrigadeControl::REQ_RESIGN, player->chatHandle(), true, true);
}

//////////////////////////////////////////////////////////////////////
// Brigade owner wishes to supplant another player controlling their unit

void
BrigadeController::requestReplace(Player* player, const ChatHandle officer)
{
	checkOIC(true);

	// You have to specify whom it is you are replacing so we can
	// avoid sync problems
	_requiresOfficer(officer);

	// You have to be* in* the brigade
	_requirePlayerInBrigade(player, unit());
	if ( player->leadingBrigade() )
		throw BCERR_YOU_ALREADY_OIC;

	// You have to be the owner of the brigade
	if ( !player->hasHandle(unit().owner()) )
		throw BCERR_NO_PERMISSION;

	if ( vote() )
		throw BCERR_ALREADY_VOTING;

//	_noticeToOfficers(BrigadeControl::_REPLACE_PENDING, BrigadeControl::REQ_REPLACE, 0, VP_NONE
//						, unit(), player->chatHandle(), player->chatHandle(), host_config.oicReplaceDelay_s);
//	voteFor(Vote::TOREPLACE, BrigadeControl::REQ_REPLACE, oicHandle(), player->chatHandle(), host_config.oicReplaceDelay_s);
	removeOIC(BrigadeControl::REQ_REPLACE, player->chatHandle(), false, false);
	assignOIC(BrigadeControl::REQ_REPLACE, player, player->chatHandle());
}

//////////////////////////////////////////////////////////////////////
// HC officer wants to forcibly remove an OIC

void
BrigadeController::requestRemove(Player* sponsor, const ChatHandle officerHandle)
{
	checkOIC(true);

	// Are you actually trying to resign?
	if ( sponsor->hasHandle(officerHandle) )
	{
		if ( unit().unitID() == sponsor->leadingBrigade() )
		{
			requestResign(sponsor);
			return;
		}

		dbugLog(_FLA, "Player trying to remove self from a brigade they are not leading?");
		throw BCERR_NOT_BRIGADE_LEADER;
	}

	// Only HC staff can do this
	if ( !sponsor->isActiveCommander() )
		throw BCERR_MUST_BE_HC_STAFF;

	// Must supply an officer name
	_requiresOfficer(officerHandle);

	// Must be the current OIC
	if ( !oic() || !oic()->hasHandle(officerHandle) )
		throw BCERR_PLAYER_NOT_OIC;

	voteFor(Vote::TOREMOVE, BrigadeControl::REQ_REMOVE, oicHandle(), sponsor->chatHandle(), host_config.oic.removalVoteTime_s);
}

//////////////////////////////////////////////////////////////////////
// Process a brigade movement request

void
BrigadeController::requestMove(Player* player, const FULL_OBJ_ID destination, const FULL_OBJ_ID from)
{
	checkOIC(true);

	bool isHCStaff = player->isActiveCommander();
	// OIC can only move their own brigade or their division
	if ( !isHCStaff && unit().unitID() != player->leadingBrigade() )
	{
		HCUnit* playerUnit = HCUnit::Lookup(player->leadingBrigade());
		if ( !playerUnit )
			throw BCERR_MUST_BE_HC_STAFF;
		if ( playerUnit->parentID() == 0 )
			throw BCERR_MUST_BE_OFFICER;
		if ( !playerUnit->isBelowOrEqual(unit().unitID()) )
			throw BCERR_MUST_BE_OFFICER;
	}

	// Brigade must be at the origin
	if ( destination == from || destination == unit().homeCPID() )
		throw BCERR_NOT_A_MOVE;

	if ( from != unit().homeCPID() )
		throw BCERR_ALREADY_MOVED;

	// If the brigade has an OIC and you aren't them...
	if ( oic() && !player->hasHandle(oicHandle()) )
		throw BCERR_BRIGADE_HAS_OIC;

	if ( vote() )
		throw BCERR_ALREADY_VOTING;

	chatSendBrigadeMove(player->chatHandle(), false, unit().unitID(), destination, false);
}

//////////////////////////////////////////////////////////////////////
// Send an attack order to the strat host.

static bool
_sendAttackOrder(const Player* const player
				, const HCUnit& unit
				, const Property* const property
				, const ORDER_TYPE orderType
				, const bool execute)
{
	Teul::Packet outgoing(Teul::PacketId(Strat::CallbackX, STRT_ATTACK_ORDER_Y));
	outgoing.construct<AttackOrder>(player->chatHandle(), unit.unitID(), property->m_fid, orderType, execute);
	return serverSendPacketToType(outgoing, STRAT_HOST);
}

//////////////////////////////////////////////////////////////////////
// Request an attack

void
BrigadeController::requestAttack(Player* player, FULL_OBJ_ID& target)
{
	checkOIC(true);

	if ( !player->isActiveOfficer() )
		throw BCERR_MUST_BE_OFFICER;

	if ( vote() )
		throw BCERR_ALREADY_VOTING;

	Property* property = chatFindProperty(target);
	if ( property == nullptr )
		throw BCERR_INVALID_REQ;

	// If the player clicked on a facility; upgrade to the CP
	if ( property->IsFacility() )
	{
		if ( property->Parent() == nullptr )
			throw BCERR_INVALID_REQ;
		property = property->Parent();
		target = property->m_fid;
	}

	if ( player->side() == ALLIED && (property->m_flags & CP_ALLIED_OBJECTIVES) )
		throw BCERR_ALREADY_OBJECTIVE;
	else if ( player->side() == AXIS && (property->m_flags & CP_AXIS_OBJECTIVES) )
		throw BCERR_ALREADY_OBJECTIVE;

	if ( !property->IsBridge() )
	{
		if ( property->Side() == player->side() )
			throw BCERR_OWN_TOWN;

		// Is it frontline?
		if ( (property->m_flags & CP_STAT_ENEMY_LINKED) == 0 )
			throw BCERR_NOT_FRONTLINE;
	}

	// If the brigade has an OIC and you aren't them...
	if ( oic() && !player->hasHandle(oicHandle()) )
		throw BCERR_BRIGADE_HAS_OIC;

	// Request attack (true), don't execute (false)
	if ( !_sendAttackOrder(player, unit(), property, ORD_ATTACK, false) )
		throw BCERR_INVALID;
}

//////////////////////////////////////////////////////////////////////
// Request a defense (bridges only)

void
BrigadeController::requestDefend(Player* player, FULL_OBJ_ID& target)
{
	checkOIC(true);

	if ( !player->isActiveOfficer() )
		throw BCERR_MUST_BE_OFFICER;

	if ( vote() )
		throw BCERR_ALREADY_VOTING;

	Property* property = chatFindProperty(target);
	if ( property == nullptr )
		throw BCERR_INVALID_REQ;

	// If the player clicked on a facility; upgrade to the CP
	if ( property->IsFacility() )
	{
		Property* parent = property->Parent();
		if ( parent == nullptr )
		{
			dbugLog(_FLA, "Property %s has NULL parent???", property->Name());
			throw BCERR_INVALID_REQ;
		}

		property = property->Parent();
		target = property->m_fid;
	}

	if ( !property->IsBridge() )
		throw BCERR_INVALID_REQ;

	if ( player->side() == ALLIED && (property->m_flags & CP_ALLIED_OBJECTIVES) )
		throw BCERR_ALREADY_OBJECTIVE;
	else if ( player->side() == AXIS && (property->m_flags & CP_AXIS_OBJECTIVES) )
		throw BCERR_ALREADY_OBJECTIVE;

	// If the brigade has an OIC and you aren't them...
	if ( oic() && !player->hasHandle(oicHandle()) )
		throw BCERR_BRIGADE_HAS_OIC;

	// Request attack (true), don't execute (false)
	if ( !_sendAttackOrder(player, unit(), property, ORD_DEFEND, false) )
		throw BCERR_INVALID;
}

//////////////////////////////////////////////////////////////////////
// Request the withdrawl of an attack

void
BrigadeController::requestClear(Player* player, FULL_OBJ_ID& target)
{
	checkOIC(true);

	if ( !player->isActiveOfficer() )
		throw BCERR_MUST_BE_OFFICER;

	if ( vote() )
		throw BCERR_ALREADY_VOTING;

	Property* property = chatFindProperty(target);
	if ( property == nullptr )
		throw BCERR_INVALID_REQ;

	if ( player->side() == ALLIED && (property->m_flags & CP_ALLIED_OBJECTIVES) == 0 )
		throw BCERR_NOT_OBJECTIVE;
	if ( player->side() == AXIS && (property->m_flags & CP_AXIS_OBJECTIVES) == 0 )
		throw BCERR_NOT_OBJECTIVE;

	if ( !property->IsBridge() )
	{
		if ( player->side() == ALLIED && (property->m_flags & CP_STAT_ALLIED_DEFEND_OBJ)  )
			throw BCERR_CANT_CANX_DEFEND;
		if ( player->side() == AXIS && (property->m_flags & CP_STAT_AXIS_DEFEND_OBJ) )
			throw BCERR_CANT_CANX_DEFEND;
	}

	// If the brigade has an OIC and you aren't them...
	if ( oic() && !player->hasHandle(oicHandle()) )
		throw BCERR_BRIGADE_HAS_OIC;

	// Request clear (false), don't execute yet (false)
	if ( !_sendAttackOrder(player, unit(), property, ORD_CLEAR, false) )
		throw BCERR_INVALID;
}

//////////////////////////////////////////////////////////////////////
// .attack -- place an attack objective
//////////////////////////////////////////////////////////////////////

class DotAttack : public DotCommand
{
public:
	DotAttack() : DotCommand("attack", "Issue an attack objective on targetted CP"
								, ACTIVE_OFFICER, LSTR_ATTACK_USAGE, LSTR_ATTACK_DETAIL
								) { }

	virtual void action(Player& player, Chat::ClientMessage* /*message*/) const override
	{
		if ( player.noSide() )
			throw LSTR_SELECT_SIDE;

		if ( player.uiTarget().Key() == 0 )
			throw LSTR_SEL_TARGET;

		BrigadeControlRequest request;
		request.m_action = BrigadeControl::REQ_ATTACK;
		request.m_brigadeID = player.brigadeID();
		request.m_officer = player.uiTarget().Key();
		request.m_target = player.uiTarget();

		request.processRequest(player.teulIndex());
	}
} dotAttack;

//////////////////////////////////////////////////////////////////////
// .clear -- remove an objective
//////////////////////////////////////////////////////////////////////

class DotClear : public DotCommand
{
public:
	DotClear() : DotCommand("clear", "Withdraw an objective on targetted CP"
								, ACTIVE_OFFICER, LSTR_CLEAR_USAGE, LSTR_CLEAR_DETAIL
								) { }

	virtual void action(Player& player, Chat::ClientMessage* /*message*/) const override
	{
		if ( player.noSide() )
			throw LSTR_SELECT_SIDE;

		if ( player.uiTarget().Key() == 0 )
			throw LSTR_SEL_TARGET;

		BrigadeControlRequest request;
		request.m_action = BrigadeControl::REQ_CLEAR;
		request.m_brigadeID = player.brigadeID();
		request.m_officer = player.uiTarget().Key();
		request.m_target = player.uiTarget();

		request.processRequest(player.teulIndex());
	}
} dotClear;

//////////////////////////////////////////////////////////////////////
// .defend -- place a defense objective
//////////////////////////////////////////////////////////////////////

class DotDefend : public DotCommand
{
public:
	DotDefend() : DotCommand("defend", "Issue a defense objective on targetted CP"
								, ACTIVE_OFFICER, LSTR_DEFEND_USAGE, LSTR_DEFEND_DETAIL
								) { }

	virtual void action(Player& player, Chat::ClientMessage* /*message*/) const override
	{
		if ( player.noSide() )
			throw LSTR_SELECT_SIDE;

		if ( player.uiTarget().Key() == 0 )
			throw LSTR_SEL_TARGET;

		BrigadeControlRequest request;
		request.m_action = BrigadeControl::REQ_DEFEND;
		request.m_brigadeID = player.brigadeID();
		request.m_officer = player.uiTarget().Key();
		request.m_target = player.uiTarget();

		request.processRequest(player.teulIndex());
	}
} dotDefend;

//////////////////////////////////////////////////////////////////////
// .move -- request a brigade movement
//////////////////////////////////////////////////////////////////////

class DotMoveBrigade : public DotCommand
{
public:
	DotMoveBrigade() : DotCommand("movebrigade", "Request movement of your current brigade"
								, ACTIVE_OFFICER, LSTR_MOVEBRIGADE_USAGE, LSTR_MOVEBRIGADE_DETAIL
								) { }

	virtual void action(Player& player, Chat::ClientMessage* /*message*/) const override
	{
		if ( player.noSide() )
			throw LSTR_SELECT_SIDE;

		if ( !player.brigadeID() )
			throw LSTR_NO_BRIGADE;

		if ( player.uiTarget().Key() == 0 )
			throw LSTR_SEL_TARGET;

		HCUnit* unit = HCUnit::Lookup(player.brigadeID());
		HCUnit* unitParent;
		if ( unit ){
			unitParent = unit->parent();
			if ( !unitParent )
				throw LSTR_NO_BRIGADE;
		}else{
			throw LSTR_NO_BRIGADE;
		}

		BrigadeControlRequest request;
		request.m_action = BrigadeControl::REQ_MOVE;
		request.m_brigadeID = unit->unitID();
		request.m_officer = player.uiTarget().Key();
		request.m_target = player.uiTarget();
		request.m_origin = unit->homeCPID();

		request.processRequest(player.teulIndex());
	}
} dotMoveBrigade;

class DotMoveDivision : public DotCommand
{
public:
	DotMoveDivision() : DotCommand("movedivision", "Request movement of your current division"
								, ACTIVE_OFFICER, LSTR_MOVEDIVISION_USAGE, LSTR_MOVEDIVISION_DETAIL
								) { }

	virtual void action(Player& player, Chat::ClientMessage* /*message*/) const override
	{
		if ( player.noSide() )
			throw LSTR_SELECT_SIDE;

		if ( !player.brigadeID() )
			throw LSTR_NO_BRIGADE;

		if ( player.uiTarget().Key() == 0 )
			throw LSTR_SEL_TARGET;

		HCUnit* unit = HCUnit::Lookup(player.brigadeID());
		if ( unit )
			unit = unit->parent();
		if ( !unit )
			throw LSTR_NO_BRIGADE;

		BrigadeControlRequest request;
		request.m_action = BrigadeControl::REQ_MOVE;
		request.m_brigadeID = unit->unitID();
		request.m_officer = player.uiTarget().Key();
		request.m_target = player.uiTarget();
		request.m_origin = unit->homeCPID();

		request.processRequest(player.teulIndex());
	}
} DotMoveDivision;

//////////////////////////////////////////////////////////////////////
// Map a brigade control request into brigade controller actions

void
BrigadeControlRequest::processRequest(SINT32 teulIndex)
{
	static const char* errors[MAX_BC_ERRORS] =
	{
	  "INVALID"
	, "INVALID_REQ"
    , "INVALID_CALLSIGN"
    , "PLAYER_UNAVAILABLE"
    , "WRONG_BRIGADE"
    , "NOT_BRIGADE_LEADER"
    , "ALREADY_VOTING"
    , "NO_VOTE_TO_VETO"
    , "VETO_MISMATCH"
    , "HAS_OIC"
    , "MUST_BE_HC_STAFF"
    , "MUST_BE_OFFICER"
    , "CANT_NOMINATE_HC"
    , "PLAYER_IS_ALREADY_OIC"
    , "YOU_ALREADY_OIC"
    , "OFFER_PENDING"
    , "NOT_OFFERED"
    , "NO_PERMISSION"
    , "PLAYER_NOT_OIC"
    , "NOT_A_MOVE"
    , "ALREADY_MOVED"
    , "BRIGADE_HAS_OIC"
    , "CANT_VETO_VS_SELF"
    , "YOU_WRONG_BRIGADE"
	, "OWN_TOWN"
	, "ALREADY_OBJECTIVE"
	, "NOT_OBJECTIVE"
	, "CANT_CANX_DEFEND"
	, "NOT_FRONTLINE"
	};

	// First up, find out who's requesting
	Player* player = PlayerInterface::Lookup(teulIndex);
	if ( player == nullptr )
	{
		dbugLog(_FLA, "t:%d no persona, disconnecting", teulIndex);
		teulClose(teulIndex);
		return;
	}

	try
	{
		if ( m_action == REQ_LIST )
		{
			_requestList(player, m_country, m_branch);
			return;
		}

		if ( isValidSide(player->side()) == false )
		{
			player->log(_FLA, "Requesting brigade without side selected");
			throw BCERR_INVALID_REQ;
		}

		if ( isObjective() )
		{
			if ( player->leadingBrigade() )
				m_brigadeID = player->leadingBrigade();
			else
				m_brigadeID = player->brigadeID();
		}

		if ( m_brigadeID == 0 )
		{
			player->log(_FLA, "Requesting brigade 0");
			throw BCERR_INVALID_REQ;
		}

		// All the other actions require that you be an
		// active HC Officer

		// Look up the brigade, use a reference because
		// any fail condition that would result in a NULL
		// will throw the neccessary RESPONSE CODE
		BrigadeController& bc = BrigadeController::Get(m_brigadeID);

		if ( bc.unit().side() != player->side() )
		{
			player->log(_FLA, "Requesting enemy brigade");
			throw BCERR_INVALID_REQ;
		}

		// Validation
		///TODO: Check if we should disable this.
		if ( m_country && bc.unit().country() != m_country )
		{
			player->log(_FLA, "Requesting brigade in different country");
			throw BCERR_INVALID_REQ;
		}
		if ( m_branch && bc.unit().branch() != m_branch )
		{
			player->log(_FLA, "Requesting brigade in different branch");
			throw BCERR_INVALID_REQ;
		}

		// Ensure that the handle is lowercase
		switch ( m_action )
		{
		case REQ_NOMINATE:
			// Wants to nominate/vote for someone
			bc.requestNominate(player, m_officer);
		break;

		case REQ_VETO:
			// There needs to be a vote in progress
			if ( !bc.vote() )
				throw BCERR_NO_VOTE_TO_VETO;
			if ( bc.vote()->isTargettedAction() )
				bc.requestVetoAction(player, m_target);
			else
				bc.requestVetoPlayer(player, m_officer);
		break;

		case REQ_VOLUNTEER:
			// Wants to volunteer themself
			bc.requestVolunteer(player);
		break;

		case REQ_RESIGN:
			// Wants to resign the position
			bc.requestResign(player);
		break;

		case REQ_REPLACE:
			// Wants to take over from someone
			bc.requestReplace(player, m_officer);
		break;

		case REQ_REMOVE:
			// Wants to remove an officer
			bc.requestRemove(player, m_officer);
		break;

		case REQ_MOVE:
			// Wants to move the brigade
			bc.requestMove(player, m_target, m_origin);
		break;

		case REQ_ATTACK:
			bc.requestAttack(player, m_target);
		break;

		case REQ_DEFEND:
			bc.requestDefend(player, m_target);
		break;

		case REQ_CLEAR:
			bc.requestClear(player, m_target);
		break;

		case REQ_INVALID:
        default:
			PLAYER_LOGA("invalid bcr action: %u", m_action);
			throw BCERR_INVALID_REQ;
		}

		dbugLog(_FLI, "t:%d p:%u c:%s hcu:(%u) action:%s(%u) completed"
						, teulIndex, player->playerID(), player->callsign()
						, m_brigadeID
						, BrigadeControl::to_string(m_action).c_str(), m_action);
	}
	catch (BRIGADE_CONTROL_ERROR error)
	{
		dbugLog(_FLD, "t:%d p:%u c:%s hcu:%u action:%s(%u) error:%s(%u)"
						, teulIndex, player->playerID(), player->callsign()
						, m_brigadeID
						, BrigadeControl::to_string(m_action).c_str(), m_action
						, errors[error], error);
		BrigadeControlNotice response(*this, BrigadeControl::_ERROR, error, 0);
		response.send(teulIndex);
	}
	catch (std::pair<BRIGADE_CONTROL_ERROR,UINT32>& error)
	{
		dbugLog(_FLD, "t:%d p:%u c:%s s:%u u:%u hcu:%u action:%s(%u) error:%s(%u) window:%u"
						, teulIndex, player->playerID(), player->callsign()
						, player->side(), player->brigadeID()
						, m_brigadeID
						, BrigadeControl::to_string(m_action).c_str(), m_action
						, errors[error.first], error.first
						, error.second);
		BrigadeControlNotice response(*this, BrigadeControl::_ERROR, error.first, error.second);
		response.send(teulIndex);
	}
	catch (...)
	{
		LogAbort("uncaught exception");
	}

	return;
}

//////////////////////////////////////////////////////////////////////
// Process the expiry of an election

bool
BrigadeController::processElectionExpired()
{
	// Check if the player is still online
	Player* nominee = PlayerInterface::Lookup(vote()->m_nominee);
	if ( nominee == nullptr || nominee->brigadeID() != unit().unitID() )
	{
		// Player is not online for us to extend an offer to them
		_noticeToOfficers(BrigadeControl::_APPOINTMENT_FAILED, vote()->m_forAction, 0, VP_NONE
							, unit(), vote()->m_nominee, vote()->m_sponsor);
		return true;
	}

	// Resculed the vote as a position offering, with a timeout
	// for 90s or so from now
	vote()->m_voteMode = Vote::OFFER;
	vote()->m_closeAt = teulTime() + host_config.oic.offerTime_s;

	// Notify the nominee that they have the opportunity
	_noticeToOfficers(BrigadeControl::_OFFERING
						, vote()->m_forAction, 0, VP_NONE, unit(), nominee->chatHandle(), vote()->m_sponsor
						, host_config.oic.offerTime_s, nullObjID, nominee);

	// Don't delete me
	return false;
}

//////////////////////////////////////////////////////////////////////
// Process an nomination-offer expiring

bool
BrigadeController::processOfferExpired()
{
	Player* nominee = PlayerInterface::Lookup(vote()->m_nominee);
	if ( nominee == nullptr )
	{
		// Officer has gone offline without accepting the appointment
		_noticeToOfficers(BrigadeControl::_APPOINTMENT_FAILED
							, vote()->m_forAction, 0, VP_NONE, unit(), vote()->m_nominee, vote()->m_sponsor);
		return true;
	}

	_noticeToOfficers(BrigadeControl::_OFFER_LAPSED
						, vote()->m_forAction, 0, VP_NONE, unit(), nominee->chatHandle(), vote()->m_sponsor, 0, nullObjID, nominee);

	return true;
}

#ifdef SLOW_REMOVE
//////////////////////////////////////////////////////////////////////
// Process a remove request expiry

bool
BrigadeController::processToRemoveExpired()
{
	checkOIC(true);

	if ( !oic() )
		return true;

	vote()->m_voteMode = Vote::REMOVING;
	vote()->m_closeAt = teulTime() + Vote::REMOVAL_WINDOW;

	_noticeToBrigade(BrigadeControl::_REMOVAL_PENDING, vote()->m_forAction, 0, VP_NONE, unit(), vote()->m_nominee, vote()->m_sponsor, Vote::REMOVAL_WINDOW);

	return false;
}
#endif

//////////////////////////////////////////////////////////////////////
// Process a replacement request

void
BrigadeController::processReplacingExpired()
{
	checkOIC(false);

	Player* player = PlayerInterface::Lookup(vote()->m_sponsor);
	if ( player == nullptr || player->brigadeID() != unit().unitID() || player->leadingBrigade() )
	{
		// The player wanting to take charge has gone offline, so leave the OIC inplace
		_noticeToOfficers(BrigadeControl::_APPOINTMENT_FAILED, BrigadeControl::REQ_REPLACE, 0, VP_NONE, unit(), vote()->m_nominee, vote()->m_sponsor);
		return;
	}

	// OIC may have gone away, in which case volunteer me
	if ( !oic() )
	{
		assignOIC(BrigadeControl::REQ_VOLUNTEER, player, player->chatHandle());
		return;
	}

	removeOIC(BrigadeControl::REQ_REPLACE, player->chatHandle(), false, false);
	assignOIC(vote()->m_forAction, player, player->chatHandle());
}

//////////////////////////////////////////////////////////////////////
// Process a movement request expiring

void
BrigadeController::processToMoveExpired()
{
	if ( unit().homeCPID() != vote()->m_origin )
	{
		dbugLog(_FLW, "hcu:%u(%s) move has wrong origin [brigade moved?]", unit().unitID(), unit().callsign());
		_noticeToOfficers(BrigadeControl::_ERROR, BrigadeControl::REQ_MOVE, BCERR_ALREADY_MOVED, VP_NONE
							, unit(), vote()->m_nominee, vote()->m_sponsor
							, 0, nullObjID, PlayerInterface::Lookup(vote()->m_sponsor));
		return;
	}

	FULL_OBJ_ID destination;
	destination.FromKey(vote()->m_nominee);
	chatSendBrigadeMove(vote()->m_sponsor, false, unit().unitID(), destination, true);
}

//////////////////////////////////////////////////////////////////////
// Process an attack request

void
BrigadeController::processToAttackExpired()
{
	FULL_OBJ_ID target;
	target.FromKey(vote()->m_nominee);

	Property* property = chatFindProperty(target);
	if ( !property )
		return;

	Player* player = PlayerInterface::Lookup(vote()->m_sponsor);
	if ( player == nullptr )
		return;

	// Attack (true), execute (true)
	_sendAttackOrder(player, unit(), property, ORD_ATTACK, true);
}

//////////////////////////////////////////////////////////////////////
// Process an attack request

void
BrigadeController::processToDefendExpired()
{
	FULL_OBJ_ID target;
	target.FromKey(vote()->m_nominee);

	Property* property = chatFindProperty(target);
	if ( !property )
		return;

	Player* player = PlayerInterface::Lookup(vote()->m_sponsor);
	if ( player == nullptr )
		return;

	// Attack (true), execute (true)
	_sendAttackOrder(player, unit(), property, ORD_DEFEND, true);
}

//////////////////////////////////////////////////////////////////////
// Process a cancel attack request

void
BrigadeController::processToClearExpired()
{
	FULL_OBJ_ID target;
	target.FromKey(vote()->m_nominee);

	Property* property = chatFindProperty(target);
	if ( !property )
		return;

	Player* player = PlayerInterface::Lookup(vote()->m_sponsor);
	if ( player == nullptr )
		return;

	// Cancel (false), execute (true)
	_sendAttackOrder(player, unit(), property, ORD_CLEAR, true);
}

//////////////////////////////////////////////////////////////////////
// Process a vote

void
BrigadeController::processVoteExpired()
{
	if ( !vote() )
	{
		dbugLog(_FLW, "hcu:%u(%s) had an entry in elections but no vote", unit().unitID(), unit().callsign());
		return;
	}
	if ( !vote()->expired() )
		return;

	dbugLog(_FLI, "hcu:%u(%s) election type %u lapsed, executing", unit().unitID(), unit().callsign(), vote()->m_voteMode);

	bool endVote = true;
	switch (vote()->m_voteMode)
	{
	case Vote::NOMINATION:
		endVote = processElectionExpired();
	break;
	case Vote::OFFER:
		endVote = processOfferExpired();
	break;
	case Vote::TOREMOVE:
//		endVote = processToRemoveExpired();
//	break;
	case Vote::REMOVING:
		removeOIC(BrigadeControl::REQ_REMOVE, vote()->m_sponsor, true, false);
	break;
	case Vote::TOREPLACE:
		processReplacingExpired();
	break;
	case Vote::TOMOVE:
		processToMoveExpired();
	break;
	case Vote::MAX_VOTE_MODES:
	break;
	case Vote::TOATTACK:
		processToAttackExpired();
	break;
	case Vote::TODEFEND:
		processToDefendExpired();
	break;
	case Vote::TOCLEAR:
		processToClearExpired();
	break;
	}

	if ( endVote && m_vote )
	{
		/// CAUTION: Don't call cancelVote() here because
		/// you will confuse the election poll routine
		delete m_vote;
		m_vote = nullptr;
	}
}

//////////////////////////////////////////////////////////////////////
// Patrol the brigade elections

bool
chatPatrolBrigadeElections()
{
	if ( elections.empty() )
		return true;
	ELECTIONS::iterator it = elections.begin();
	while ( it != elections.end() )
	{
		BrigadeController& bc = BrigadeController::Get(*it);
		++it;
		bc.processVoteExpired();
		if ( bc.vote() == nullptr )
			bc.cancelVote();
	}
	return true;
}

//////////////////////////////////////////////////////////////////////
// Dot command to list brigade controllers

class DotListOICs : public DotCommand
{
public:
	DotListOICs() : DotCommand("listoics", "List current brigade/OIC assignments"
								, NO_MASK, LSTR_LISTOIC_USAGE, LSTR_LISTOIC_DETAIL
								) { }

	virtual void action(Player& player, Chat::ClientMessage* /*message*/) const override
	{
		if ( !isValidSide(player.side()) )
			throw Chat::LStringErr(LSTR_SELECT_SIDE);

		bool assignments = false;
		std::string results = "";
		std::string entry = "";
		for ( auto it = controllers.begin(), end = controllers.end(); it != end; ++it )
		{
			BrigadeController* controller = it->second;
			if ( controller->unit().side() != player.side() )
				continue;
			if ( controller->oic() == nullptr && controller->vote() == nullptr )
				continue;

			if ( results.length() )
				results += ", ";

			entry = controller->unit().callsign();
			if ( controller->vote() )
			{
				switch ( controller->vote()->m_voteMode )
				{
				case Vote::NOMINATION:
					entry += "+?";
				break;
				case Vote::OFFER:
					entry += "+>";
				break;
				case Vote::TOREMOVE:
					entry += "-?";
				break;
				case Vote::REMOVING:
					entry += "<-";
				break;
				case Vote::TOREPLACE:
					entry += "<-<";
				break;
				case Vote::TOMOVE:
					entry += "moving";
				break;
				case Vote::TOATTACK:
					entry += "attack";
				break;
				case Vote::TOCLEAR:
					entry += "clear";
				break;
				case Vote::TODEFEND:
					entry += "defend";
				default:
					entry += "<err>";
				break;
				}
				if ( !Vote::IsTargettedAction(controller->vote()->m_voteMode) )
					entry += Chat::Name(controller->vote()->m_nominee).c_str();
			}
			else if ( controller->oic() )
				controller->oic()->addName(entry, ":");

			if ( results.length() + entry.length()  >= MAX_MESSAGE_LENGTH )
			{
				player.message(LSTR_NOTE, LSTR_GENERIC, results.c_str());
				results = "";
			}

			results += entry;
			assignments = true;
		}

		if ( assignments == false )
			throw Chat::LStringErr(LSTR_NO_OICS);

		player.message(LSTR_NOTE, LSTR_GENERIC, results.c_str());
	}
} dotListOICs;

//////////////////////////////////////////////////////////////////////
// Place a movement proposal for a brigade, inform HC officers and giving them
// opportunity to block the movement.

void
chatBrigadeMoveProposal(UINT32 brigadeID, const FULL_OBJ_ID destination, ChatHandle byHandle)
{
	assert( brigadeID != 0 );
	if ( byHandle == 0LL )
		byHandle = systemHandleID;

	FULL_OBJ_ID origin = nullObjID;
	// Lookup the brigade controller for this brigade, creating it if neccessary
	BrigadeController& bc = BrigadeController::Get(brigadeID);
	try
	{
		origin = bc.unit().homeCPID();

		if ( bc.vote() )
			throw BCERR_ALREADY_VOTING;

		if ( byHandle != systemHandleID && bc.oic() && !bc.oic()->hasHandle(byHandle) )
			throw BCERR_BRIGADE_HAS_OIC;

		UINT32 delay = host_config.oic.moveVoteTime_s;
		// Are you the brigade oic?
		if ( !(bc.oic() && bc.oic()->hasHandle(byHandle)) )
			delay += delay / 2;

		bc.voteFor(Vote::TOMOVE, BrigadeControl::REQ_MOVE, destination.Key(), byHandle, delay, origin);
	}
	catch (BRIGADE_CONTROL_ERROR error)
	{
		Player* player = PlayerInterface::Lookup(byHandle);
		if ( player != nullptr && player->teulIndex() >= 0 )
		{
			BrigadeControlNotice outgoing(BrigadeControl::_ERROR, error, BrigadeControl::REQ_MOVE, VP_NONE, bc.unit(), destination.Key(), byHandle, 0);
			outgoing.m_origin = origin;
			outgoing.send(player->teulIndex());
		}
	}
}

//////////////////////////////////////////////////////////////////////
// Process an attack order execution completed

void static
_orderExecuted(AttackOrder& order, const ChatHandle handle, BrigadeController& bc, BrigadeControl::BRIGADE_ACTION_REQ action, UINT32 vote)
{
	Property* property = chatFindProperty(order.m_target);
	// If a city is being attacked, we tell everyone.
	if ( !property )
		LogAbort("garbage data in order");

	_noticeToSide(BrigadeControl::_OBJECTIVE, action, bc.unit(), order.m_target, handle, bc.unit().side());

	// If its a city objective, we need to tell the other side
	if ( property->IsBridge() || action != BrigadeControl::REQ_ATTACK )
		return;

	WWII_SIDE otherSide = property->Side();
	const HCUnit* defender = nullptr;
	UINT32 defScore = 0;
	for ( auto it = hcUnits.begin(), end = hcUnits.end(); defScore < 7 && it != end; ++it )
	{
		HCUnit* cur = it->second;
		if ( cur->side() != otherSide )
			continue;
		UINT32 score = 0;
		if ( cur->homeCPID() == property->m_fid )
			score += 4;
		if ( cur->branch() == BRANCH_LAND )
			score += 2;
		if ( cur->country() == property->m_owner )
			score += 1;

		if ( score > defScore )
			defender = cur;
	}

	if ( defender == nullptr )
		defender = &bc.unit();

	_noticeToSide(BrigadeControl::_OBJECTIVE, BrigadeControl::REQ_DEFEND, *defender, order.m_target, handle, otherSide);
}

//////////////////////////////////////////////////////////////////////
// Attack proposal

void
chatBrigadeAttackOrderResponse(AttackOrder& order)
{
	Player* player = PlayerInterface::Lookup(order.m_requester);
	bool isConclusion = ( order.m_execute && order.m_result == SCMD_SUCCESS );
	if ( player == nullptr && !isConclusion )
	{
		dbugLog(_FLD, "player (%" PR64BitUT ") went away during attack order request", order.m_requester);
		return;
	}

	Vote::Mode vote = Vote::TOATTACK;
	BrigadeControl::BRIGADE_ACTION_REQ action = BrigadeControl::REQ_INVALID;
	const char* cmd = "<undef>";
	switch ( order.m_orderType )
	{
	case ORD_ATTACK:
		vote = Vote::TOATTACK;
		action = BrigadeControl::REQ_ATTACK;
		cmd = "attack";
	break;
	case ORD_DEFEND:
		vote = Vote::TODEFEND;
		action = BrigadeControl::REQ_DEFEND;
		cmd = "defend";
	break;
	case ORD_CLEAR:
		vote = Vote::TOCLEAR;
		action = BrigadeControl::REQ_CLEAR;
		cmd = "clear";
	break;
	}

	UINT32 brigadeID = order.m_brigadeID;
	try
	{
		if ( brigadeID == 0 && player )
		{
			if ( player->leadingBrigade() )
				brigadeID = player->leadingBrigade();
			else
				brigadeID = player->brigadeID();
		}
		if ( brigadeID == 0 )
			throw BCERR_INVALID_REQ;

		BrigadeController& bc = BrigadeController::Get(brigadeID);

		if ( order.m_result != SCMD_SUCCESS && order.m_result != SCMD_ALLOWED )
		{
			if ( player )
			{
				Property* property = chatFindProperty(order.m_target);
				chatSendSCMDResultMessage(player, order.m_result, cmd, property->Name(), order.m_delay);
			}
			return;
		}

		ChatHandle handle = (player) ? player->chatHandle() : order.m_requester;

		if ( !order.m_execute )
		{
			if ( bc.vote() )
				throw BCERR_ALREADY_VOTING;

			// Non-HCs take 2x longer to get an attack in place
			// Non-OIC hcs take 1.5x longer to get an attack in place
			UINT32 delay = host_config.oic.moveVoteTime_s;
			if ( player && !player->isActiveCommander() )
				delay = delay * 2;
			else if ( player && !player->leadingBrigade() )
				delay = delay + delay / 2;
			bc.voteFor(vote, action, order.m_target.Key(), handle, delay);
			return;
		}
		_orderExecuted(order, handle, bc, action, vote);
	}
	catch (BRIGADE_CONTROL_ERROR& error)
	{
		if ( !player )
		{
			dbugLog(_FLI, "attack order response generated error %u", error);
			return;
		}

		BrigadeControlNotice response;
		response.m_error = error;
		response.m_target = order.m_target;
		response.m_sponsor = player->chatHandle();
		response.send(player->teulIndex());
	}
}

//////////////////////////////////////////////////////////////////////
// Remove a player from their OIC position in a specific brigade

void
chatRemoveOICStatus(UINT32 brigadeID, const ChatHandle handle)
{
	// Sanity check
	if ( brigadeID == 0 )
		return;

	// Check that they* are* actually who we think is running this brigade
	BrigadeController* bc = BrigadeController::Lookup(brigadeID);
	if ( bc == nullptr )
		return;

	// Is this player actually the OIC of the brigade?
	if ( bc->oic() && bc->oic()->hasHandle(handle) )
	{
		if ( bc->vote() && bc->vote()->m_voteMode == Vote::TOMOVE )
			bc->cancelVote();
		bc->removeOIC(BrigadeControl::REQ_REMOVE, bc->oicHandle(), true, true);
	}

	if ( !bc->vote() )
		return;

	ChatHandle lowerHandle = chatGetLowercaseHandle(handle);

	if ( (bc->vote()->m_voteMode == Vote::OFFER || bc->vote()->m_voteMode == Vote::NOMINATION )
		&& chatGetLowercaseHandle(bc->vote()->m_nominee) == lowerHandle )
	{
		bc->processElectionExpired();
		bc->cancelVote();
		return;
	}

	if ( chatGetLowercaseHandle(bc->vote()->m_sponsor) == lowerHandle )
		bc->cancelVote();
}

//////////////////////////////////////////////////////////////////////
// Announce brigade movement

void
chatAnnounceBrigadeMove(const HCUnit& unit, const FULL_OBJ_ID origin, const FULL_OBJ_ID destination, ChatHandle byHandle)
{
	if ( byHandle == systemHandleID )
		byHandle = 0LL;

	_noticeToSide(BrigadeControl::_UNIT_MOVED, BrigadeControl::REQ_MOVE, unit, destination, byHandle, unit.side(), origin);

	// Well, maybe now we need to drop the vote.
	BrigadeController& bc = BrigadeController::Get(unit.unitID());
	if ( bc.vote() && bc.vote()->isTargettedAction() )
		bc.cancelVote();
}
