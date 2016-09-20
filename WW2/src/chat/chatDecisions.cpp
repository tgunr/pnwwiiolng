#include <syscfg.h> // Path is project specific.
// Date: Thu Jun 03 10:00:00 2004
//
// Functions for handling chat/strat decisions and
// dispatching the appropriate orders to strat
//
// Copyright (c) 2004 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Oliver "kfsone" Smith
// Supporting Programmer:  Oliver "kfsone" Smith

#include "chat/chatPlayer.h"
#include "chat/chatDB.h"
#include "chat/chatDecisions.h"
#include "chat/chatDecisionTypes.h"			// Types/typedefs/consts etc
#include "chat/chatDotCommand.h"
#include "chat/chatList.h"
#include "chat/chatHost.h"
#include "chat/chatIcons.h"
#include "chat/chatHostMessages.h"
#include "chat/chatTimeInterval.h"
#include "chat/chatHostBrigadeControl.h"
#include "chat/chatHoldFallVote.h"
#include "wwii/wwiiConfig.h"

//////////////////////////////////////////////////////////////////////
// Strat Decision class

//// ---> Local variables

static const UINT32 DECISION_POLL_INTERVAL_S = 30;

// History of overruns and executions
DECISION_QUEUE _pendQueue;
DECISION_QUEUE _histQueue;

static const char* sc_commands[SC_NUM_COMMANDS] =
{
	"<ERR>"
,	"OVERRUN"
,	"HOLD"
,	"FALLBACK"
,	"ATTACK"
,	"DEFEND"
,	"CLEAR"
};

TEUL_CALLBACK* HoldFallVote::s_callback;

//////////////////////////////////////////////////////////////////////
// Helper class for storing and sending the result of an scmd decision

class ScmdResponse
{
public:
	LSTRING_ID	m_type;
	LSTRING_ID	m_message;
	const char*	m_arg1;
	const char*	m_arg2;

	ScmdResponse() : m_type(LSTR_NONE), m_message(LSTR_NONE), m_arg1(nullptr), m_arg2(nullptr)
	{ }

	void set(LSTRING_ID type, LSTRING_ID message, const char* arg1 = nullptr, const char* arg2 = nullptr)
	{
		m_type = type;
		m_message = message;
		m_arg1 = arg1;
		m_arg2 = arg2;
	}

	void caution(LSTRING_ID msg, const char* arg1 = nullptr, const char* arg2 = nullptr)
	{	set(LSTR_CAUTION, msg, arg1, arg2);	}

	void note(LSTRING_ID msg, const char* arg1 = nullptr, const char* arg2 = nullptr)
	{	set(LSTR_NOTE, msg, arg1, arg2);	}

	void generic(LSTRING_ID msg, const char* arg1 = nullptr, const char* arg2 = nullptr)
	{	set(LSTR_NONE, msg, arg1, arg2);	}

	void warn(const char* arg1, const char* arg2 = nullptr)
	{	set(LSTR_WARNING, LSTR_GENERIC, arg1, arg2); }
	void warn(LSTRING_ID msg, const char* arg1 = nullptr, const char* arg2 = nullptr)
	{	set(LSTR_WARNING, msg, arg1, arg2); }

	void send(Player* player)
	{	player->message(m_type, m_message, m_arg1, m_arg2);	}
};

//////////////////////////////////////////////////////////////////////
// TEMPORARY: Generate a text string for an SCMD - this should be
// replaced with localized strings

static void
_printSCMDResult(ScmdResponse& sr
				, SCMD_RESULT result
				, const char* cmd
				, const char* cp
				, UINT32 data
				)
{
	switch (result)
	{
	case SCMD_NO_EFFECT:
		///FIXME: We may want to report these, undefined behavior for now
		sr.warn("Internal host error", "pscrNE");
	break;

	case SCMD_SUCCESS:
		sr.note(LSTR_CMD_SUCCESS, cmd, cp);
	break;

	case SCMD_ALLOWED:
		sr.note(LSTR_CMD_ALLOWED, cmd, cp);
	break;

	case SCMD_NOT_OWNER:
		sr.caution(LSTR_ENEMY_OWNS, cp);
	break;

	case SCMD_WRONG_SIDE:
		sr.caution(LSTR_ENEMY_OWNS, cp);
	break;

	case SCMD_MUST_OAC:
		sr.caution(LSTR_MUST_OAC, cp, cmd);
	break;

	case SCMD_NO_ENEMY:
		sr.caution(LSTR_NO_ATTACK, cp);
	break;

	case SCMD_NOT_OVERRUN:
		sr.caution(LSTR_NO_OVERRUN, cp);
	break;

	case SCMD_NO_ABS:
		sr.caution(LSTR_DONT_OWN_ABS, cp);
	break;

	case SCMD_NO_LINKS:
		sr.caution(LSTR_ABS_NOT_LINKED, cp);
	break;

	case SCMD_NO_CONTENTION:
		sr.caution(LSTR_NO_CONTENTION, cp);
	break;

	case SCMD_REPEAT:
		sr.note(LSTR_ALREADY_REPORTING, cp, cmd);
	break;

	case SCMD_OTHER_ORDERS:
		sr.caution(LSTR_OTHER_ORDERS, cp);
	break;

	case SCMD_ORDER_LIMIT:
		sr.caution(LSTR_OUT_OF_OBJECTIVES);
	break;

	case SCMD_REQUEST_OUTSTANDING:
		sr.caution(LSTR_PREVIOUS_OBJ_LIVES, cp);
	break;

	case SCMD_NO_OBJECTIVE:
		sr.caution(LSTR_NO_OBJECTIVE, cp);
	break;

	case SCMD_OBJECTIVE_FRONTLINE:
		sr.caution(LSTR_NOT_FRONT_TOWN, cp);
    break;

	case SCMD_FRIENDLY_TOWN:
		sr.caution(LSTR_FRIENDLY_TOWN, cp);
	break;

	case SCMD_CITY_AUTODEFENSE:
		sr.caution(LSTR_DEFEND_TOWN);
	break;

	case SCMD_YOUNG_OBJECTIVE:
		sr.caution(LSTR_CLEAR_TOO_SOON, cp, chatTimeInterval(data));
	break;

	case SCMD_STUCK_UNITS:
		sr.caution(LSTR_STUCK_UNITS, cp, cmd);
	break;

	case SCMD_SOFTCAP_DENIED:
		sr.caution(LSTR_CANT_START_SOFTCAP_NOW);
	break;

	case SCMD_NO_BDE_LINK:
		sr.caution(LSTR_SCMD_NO_BDE_LINK);
	break;

	case SCMD_NO_DIV_LINK:
		sr.caution(LSTR_SCMD_NO_DIV_LINK);
	break;

	case SCMD_ERROR:
		sr.warn(LSTR_CMD_FAILED, cmd, cp);
	break;

	default:
		dbugLog(_FLA, "%s produced unhandled SCMD_RESULT: %u", cmd, result);
		sr.warn("Internal host error: pscrNE");
	}
}

//////////////////////////////////////////////////////////////////////
// Find a decision on the overrun queue by it's location

static bool
_findOverrunAt(const Property* property, WWII_SIDE side, DECISION_QUEUE::iterator& it)
{
	for ( it = _pendQueue.begin(); it != _pendQueue.end(); ++it )
	{
		if ( (*it)->m_property == property && (*it)->m_side == side )
			break;
	}

	// Return true if we found a decision
	return ( it != _pendQueue.end() );
}

//////////////////////////////////////////////////////////////////////
// Add an overrun to the decision queue

static StratDecision*
_addOverrun(WWII_SIDE side, const Property* property)
{
	// Firstly, find if there is already a decision for this property that
	// we just haven't timed out yet
	DECISION_QUEUE::iterator it;
	if ( _findOverrunAt(property, side, it) )
		_pendQueue.erase(it);

	StratDecision* decision = new StratDecision(side, property);
	if ( decision == nullptr )
		throw std::runtime_error("Out of memory allocating strat decision");	///Terminal

	_pendQueue.push_back(decision);

	return decision;
}

//////////////////////////////////////////////////////////////////////
// Go through a decisions queue and expire old entries

static void
_drainDecisions(const char* const queueName, DECISION_QUEUE& queue, UINT32 now, UINT32 cutoff_min)
{
	if ( queue.empty() )
		return;

	// Translate into seconds from minutes
	const UINT32 cutoff = now - (cutoff_min * 60);

	dbugLog(_FLD, "%s decision queue. items:%" PRSizeT ", life:%um, next:%ds", queueName, queue.size(), cutoff_min, queue.front()->m_time - cutoff);

	while ( !queue.empty() )
	{
		StratDecision* decision = queue.front();
		ABORT_IF( decision == nullptr );
		if ( decision->m_time > cutoff )
			break;
		queue.erase(queue.begin());

		dbugLog(_FLD, "Expired decision for %u:%s"
						, decision->m_side
						, decision->m_property->Name()
						);

		delete decision;
	}
}

//////////////////////////////////////////////////////////////////////
// Determine if there is room for an order and, if not, when
// the next opening will be.

UINT32
_timeToNextOpening(WWII_SIDE side)
{
	// No entries, its immediate
	if ( _histQueue.empty() )
		return 0;

	UINT32 cutoff = teulTime() - (host_config.objective.holdFallWindow_m * 60);
	UINT32 timeLeft = 0;
	UINT32 count = 0;

	// Purge any pending events
	_drainDecisions(_sideNameAdj[side], _histQueue, teulTime(), host_config.objective.holdFallWindow_m);

	for ( auto it = _histQueue.begin(), end = _histQueue.end(); it != end; ++it )
	{
		if ( (*it)->m_side == side )
		{
			if ( timeLeft == 0 )
				timeLeft = (*it)->m_time - cutoff;

			++count;
		}
	}

	// If we haven't used all available slots, we can just roll with it
	if ( count < host_config.objective.holdFallLimit )
		return 0;

	// Otherwise report the seconds until the next free decision.
	return timeLeft;
}

//////////////////////////////////////////////////////////////////////
// Determine if we have an opening for a new hold/fall order for
// a particular side: return true if we do, otherwise notify the
// player when the next opening will be.

static bool
_haveOpening(Player* player, WWII_SIDE side)
{
	UINT32 delay = _timeToNextOpening(side);

	if ( delay <= 0 )
		return true;

	player->message(LSTR_CAUTION, LSTR_CANT_ISSUE_HF_ORDERS_FOR, chatTimeInterval(delay));

	return false;
}

//////////////////////////////////////////////////////////////////////
// Returns 'true' if specified player already appears in the voter list

static bool
_voted(Player* player, const StratDecision* decision)
{
	register ChatHandle handle = player->matchHandle();

	for ( UINT32 i = 0; i < decision->m_voters.size(); ++i )
	{
		if ( decision->m_voters[i] == handle )
		{
			player->message(LSTR_CAUTION, LSTR_ALREADY_VOTED);
			return true;
		}
	}

	return false;
}

//////////////////////////////////////////////////////////////////////
// An issued strat-command returned "allowed"

static void
_overrunSuccess(Player* player, const Property* property, WWII_SIDE side)
{
	if ( player == nullptr || property == nullptr )
	{
		dbugLog(_FLA, "*** %s called with NULL player=%p, property=%p", __FUNCTION__, player, property);
		return;
	}

	player->message(LSTR_NOTE, LSTR_RAISING_OVERRUN);

	chatHCMessage(side, LSTR_OVERRUN_REPORTED, property->Name(), property->Name());

	// Make a note of it for recollection
	dbugLog(_FLI, "Adding decision %u:%s to queue", side, property->Name());

	_addOverrun(side, property);
}

//////////////////////////////////////////////////////////////////////
// When a decision has been completed, we transfer it from the
// _pendQueue to the _histQueue which is a history of orders issued

static void
_transferDecision(const Property* property, WWII_SIDE side)
{
	DECISION_QUEUE::iterator it;
	if ( !_findOverrunAt(property, side, it) )
		return;

	_histQueue.push_back(*it);

	_pendQueue.erase(it);
}

//////////////////////////////////////////////////////////////////////
// Handle success reported on a hold at all costs

static void
_holdSuccess(Player* player, const Property* property, WWII_SIDE side)
{
	player->message(LSTR_NOTE, LSTR_GO_HOLD, property->Name());

	chatSideMessage(side, LSTR_HOLD_AT_ALL_COSTS, property->Name());

	_transferDecision(property, side);
}

//////////////////////////////////////////////////////////////////////
// Handle success on a fallback

static void
_fallSuccess(Player* player, const Property* property, WWII_SIDE side)
{
#ifdef BETA_RELEASE
	UINT32 startTime = (UINT32)(host_config.capture.start_m * (UINT32)(60 * 1.5));
	player->message(LSTR_BETA, LSTR_FALLBACK_SURRENDER_IN, property->Name(), chatTimeInterval(startTime));
#endif

	chatSideMessage(side, LSTR_FALLBACK_START, property->Name());

	_transferDecision(property, side);
}

//////////////////////////////////////////////////////////////////////
// Handle success on any scmd

static void
_SCMDsuccess(const StratCommand* scmd, const char* command, Player* player, const Property* property)
{
	dbugLog(_FLI, "'%s' by %s[%d] was successful", command, player->callsign(), player->playerID());

	switch (scmd->m_command)
	{
	case SC_OVERRUN:
		_overrunSuccess(player, property, scmd->m_side);
	break;

	case SC_HOLD:
		_holdSuccess(player, property, scmd->m_side);
	break;

	case SC_FALLBACK:
		_fallSuccess(player, property, scmd->m_side);
	break;

	case SC_ATTACK:
		LogAbort("SC_ATTACK/DEFEND/CLEAR no-longer supported");
	break;

	default:
		dbugLog(_FLW, "Unrecognized scmd reporting success: %d", scmd->m_command);
	break;
	}
}

//////////////////////////////////////////////////////////////////////
// Score a particular vote field and update the "winning" field

void
_addVote(Player* player, StratDecision* decision, UINT32& scoreField, const char* order)
{
	player->recordEvent("CMD", order, decision->m_property->Name());

	// Add to the history and scoring
	decision->m_voters.push_back(player->matchHandle());

	++scoreField;
	if ( scoreField > decision->m_score )
		decision->m_score = scoreField;

	UINT32 conceedTime = teulTime() + 90;
	if ( conceedTime > decision->m_conceed )
		decision->m_conceed = conceedTime;

	///TRANSMIT
	HoldFallVote hfv(decision, HoldFallVote::_VOTED); // "[{id}] {callsign} requests '{order}' (H:%u/F:%u/D:%u)"
	hfv.m_requester = player->chatHandle();
	// sent in destructor
}

//////////////////////////////////////////////////////////////////////
// Check the common pre-requisites for placing a vote, and if they are
// all met, adjust the history and score to suite.
// If it finds a reason not to allow the vote, it will inform the user
// and then return false.
// Otherwise returns true.

static bool
_vote(Player* player, const Property* property, WWII_SIDE side, SINT32 score)
{
	DECISION_QUEUE::iterator it;
	StratDecision* decision;

	// Look for an overrun, if none is present, we'll need to create
	// our own decision entry and use that.
	if ( _findOverrunAt(property, side, it) )
		decision = *it;
	else
	{
		if ( !_haveOpening(player, side) )
			return false;

		decision = _addOverrun(side, property);

		// If we get here, we ran out of memory
		assert(decision != nullptr);
	}

	// See if they managed to sneak in a vote while we were away
	if ( _voted(player, decision) )
		return false;

	// If a decision has already been made but is being processed by the
	// strat host...
	if ( decision->m_chosen > SC_OVERRUN )
	{
		dbugLog(_FLI, "vote received for %s while waiting on strat response", property->Name());
		return true;
	}

	if ( score > 0 )
		_addVote(player, decision, decision->m_holds, "hold");
	else if ( score < 0 )
		_addVote(player, decision, decision->m_falls, "fallback");
	else
		dbugLog(_FLA, "_vote passed zero score? wtf");

	return true;
}

//////////////////////////////////////////////////////////////////////
// A hold command has been allowed

static void
_holdAllowed(Player* player, const Property* property, WWII_SIDE side)
{
	if ( !_vote(player, property, side, +1) )
		return;
}

//////////////////////////////////////////////////////////////////////
// A fallback command has been allowed

static void
_fallAllowed(Player* player, const Property* property, WWII_SIDE side)
{
	if ( !_vote(player, property, side, -1) )
		return;
}

//////////////////////////////////////////////////////////////////////
// An issued strat-command returned "allowed"

static void
_SCMDallowed(const StratCommand* scmd, const char* command, Player* player, const Property* property)
{
	dbugLog(_FLI, "'%s' by %s[%d] was allowed", command, player->callsign(), player->playerID());
	switch (scmd->m_command)
	{
	case SC_OVERRUN:
		player->message(LSTR_NOTE, LSTR_AWAITING_CONFIRMATION);
	break;

	case SC_HOLD:
		_holdAllowed(player, property, scmd->m_side);
	break;

	case SC_FALLBACK:
		_fallAllowed(player, property, scmd->m_side);
	break;

	case SC_ATTACK:
	case SC_DEFEND:
	case SC_CLEAR:
		LogAbort("SC_ATTACK/DEFEND/CLEAR no-longer supported");
	break;

	default:
		dbugLog(_FLW, "_allowed doesn't recognize this command");
	break;
	}
}

//////////////////////////////////////////////////////////////////////
// If conditions change, its possible for the final strat command of
// a vote to fail. At that point, we need to remove the vote from
// the queue.

static void
_commandFailed(const Property* property, WWII_SIDE side)
{
	DECISION_QUEUE::iterator it;
	const StratDecision* decision;
	if ( _findOverrunAt(property, side, it) == false )
		return;

	decision = *it;

	// Ignore undecided commands.
	if ( decision->m_chosen == SC_NO_COMMAND )
		return;

	const char* command = (decision->m_chosen == SC_HOLD) ? "HOLD" : "FALLBACK";
	dbugLog(_FLI, "decision: side:%u %s@%s order failed on execution, cancelling"
					, decision->m_side
					, ((decision->m_chosen == SC_HOLD) ? "hold" : "fallback")
					, decision->m_property->Name()
					);

	chatHCMessage(decision->m_side, LSTR_HOLDFALL_ORDER_FAILED, command);

	_pendQueue.erase(it);
}

//////////////////////////////////////////////////////////////////////
// Function for processing an incoming strat command packet (i.e. a result)

void
StratCommand::process(void)
{
	Player* player = PlayerInterface::Lookup(m_issuer);
	Property* property = nullptr;
	const char* cpName = nullptr;
	const char* command = nullptr;

	if ( player == nullptr )
		throw std::logic_error("Unable to find persona on this teulIndex");

	property = chatFindProperty(m_cp);
	cpName = chatPropertyName(property, m_cp);

	dbugLog(_FLI, "p:%u StratCommandResult: cmd=%u, cp=%s, side=%u, result=%d"
					, m_issuer
					, m_command
					, cpName
					, m_side
					, m_result
					);

	if ( player->playerID() != m_issuer )
		throw std::logic_error("TeulIndex does not correspond to issuing player");

	if ( property == nullptr )
		throw std::logic_error("Unable to locate a Property at those co-ordinates");

	if ( m_command < SC_NUM_COMMANDS )
		command = sc_commands[m_command];
	else
		command = sc_commands[SC_NO_COMMAND];

	switch ( m_result )
	{
		case SCMD_ALLOWED:
			_SCMDallowed(this, command, player, property);
		break;
		case SCMD_SUCCESS:
			_SCMDsuccess(this, command, player, property);
		break;
		default:
			// This may warrant cancellation of the overrun.
			if ( m_command != SC_NO_COMMAND && m_command != SC_OVERRUN )
				_commandFailed(property, m_side);

			///LOCALIZE
			ScmdResponse sr;
			_printSCMDResult(sr, m_result, command, cpName, m_data);
			sr.send(player);

			// Unlock the feature, since they didn't get to use it.
			player->featureLock(command, 0);
		break;
	}
}

//////////////////////////////////////////////////////////////////////
// Send an SCMD result code to a player

void
chatSendSCMDResultMessage(Player* player, UINT32 result, const char* cmd, const char* cpName, UINT32 delay)
{
	if ( player == nullptr || result == SCMD_ALLOWED || result == SCMD_SUCCESS )
		return;
	ScmdResponse sr;
	_printSCMDResult(sr, result, cmd, cpName, delay);
	sr.send(player);
}

//////////////////////////////////////////////////////////////////////
// Test if a player is allowed to make a vote on this decisions

static bool
_validVoter(Player* player, StratDecision* decision)
{
	if ( _voted(player, decision) )
		return false;

	if ( player->inFacility() && memcmp(&player->originCpID(), &decision->m_property->m_fid, sizeof(FULL_OBJ_ID)) == 0 )
		return true;

	player->message(LSTR_CAUTION, LSTR_MUST_ISSUE_FROM, decision->m_property->Name(), decision->m_property->Name());

	return false;
}

//////////////////////////////////////////////////////////////////////
// Try to find a decision by ID, but if we can't, tell the player why.

static StratDecision*
_lookupDecision(const char* id, Player* player)
{
	if ( player == nullptr || id == nullptr )
	{
		dbugLog(_FLW, "%s called with player=%p, id=%p", __FUNCTION__, player, id);
		return nullptr;
	}

	StratDecision* decision = nullptr;
	for ( auto it = _pendQueue.begin(), end = _pendQueue.end(); it != end; ++it )
	{
		if ( player->side() == (*it)->m_side && strcasecmp((*it)->m_property->Name(), id) == 0 )
		{
			decision = *it;
			break;
		}
	}

	// If we didn't find a decision, you can't vote
	if ( decision == nullptr )
	{
		player->message(LSTR_WARNING, LSTR_NO_SUCH_ORDER, id);
		return nullptr;
	}

	// If a decision has already been made, you're too late
	if ( decision->m_chosen == SC_HOLD || decision->m_chosen == SC_FALLBACK )
	{
		player->message(LSTR_WARNING, LSTR_ORDER_IN_PROGRESS
				, id, (decision->m_chosen == SC_HOLD) ? "hold" : "fallback");
		return nullptr;
	}

	if ( !_validVoter(player, decision) )
		return nullptr;

	return decision;
}

//////////////////////////////////////////////////////////////////////
// Vote "Hold" on the decision specified by the given ID

bool
chatVoteHold(Player* player, const char* id)
{
	StratDecision* decision = _lookupDecision(id, player);
	if ( decision == nullptr )
		return false;

	// If nobody has voted and no openings are open for a new vote, decline.
	if ( decision->m_voters.size() == 0 )
	{
		if ( !_haveOpening(player, decision->m_side) )
			return false;

		// Hold and fallback work differently, but I'm not sure why
		// hold sends a stratcommand in this case while fallback doesn't.
		// Guess I missed my chance to document this first time round, sigh.
		player->sendStratCommand("hold", "", true);
		return true;
	}

	player->contextualStratCommand("hold", "", player->country(), decision->m_side, decision->m_property);
	return true;
}

//////////////////////////////////////////////////////////////////////
// Vote "Fallback" on the decision specified by the given ID

bool
chatVoteFallback(Player* player, const char* id)
{
	StratDecision* decision = _lookupDecision(id, player);
	if ( decision == nullptr )
		return false;

	// If nobody has voted and no openings are open for a new vote, decline.
	if ( decision->m_voters.size() == 0 && !_haveOpening(player, decision->m_side) )
		return false;

	player->contextualStratCommand("fallback", "", player->country(), decision->m_side, decision->m_property);
	return true;
}

//////////////////////////////////////////////////////////////////////

bool
chatStartHold(Player* player, const FULL_OBJ_ID& target)
{
	const Property* property = chatFindProperty(target);
	if ( property == nullptr )
	{
		player->message(LSTR_SYSTEM, LSTR_SELECT_FACILITY, "hold", nullptr);
		return false;
	}

	DECISION_QUEUE::iterator it;
	if ( _findOverrunAt(property, player->side(), it) )
	{
		// Voting on a pre-existing fallback
		return chatVoteHold(player, (*it)->m_property->Name());
	}

	if ( !_haveOpening(player, player->side()) )
		return false;

	player->contextualStratCommand("hold", "", player->country(), player->side(), property);
	return true;
}

//////////////////////////////////////////////////////////////////////

bool
chatStartFallback(Player* player, const FULL_OBJ_ID& target)
{
	const Property* property = chatFindProperty(target);
	if ( property == nullptr )
	{
		player->message(LSTR_SYSTEM, LSTR_SELECT_FACILITY, "fallback", nullptr);
		return false;
	}

	DECISION_QUEUE::iterator it;
	if ( _findOverrunAt(property, player->side(), it) )
	{
		// Voting on a pre-existing fallback
		return chatVoteFallback(player, (*it)->m_property->Name());
	}

	if ( !_haveOpening(player, player->side()) )
		return false;

	player->contextualStratCommand("fallback", "", player->country(), player->side(), property);
	return true;
}

//////////////////////////////////////////////////////////////////////
//

bool
chatVetoDecision(Player* player, const char* id)
{
	StratDecision* decision = _lookupDecision(id, player);
	if ( decision == nullptr )
		return false;

	// Add to the history and veto and extend voting time
	_addVote(player, decision, decision->m_vetos, "delay");

	return true;
}

//////////////////////////////////////////////////////////////////////
// predict the eta on a decision

UINT32
StratDecision::eta() const
{
	// A clear lead of 3+ votes requires only 2 minutes
	if ( m_score > 2 && m_score == m_voters.size() )
		return std::max(m_time + 120, m_conceed);

	// A lone commander requires 3 minutes
	if ( m_voters.size() == 1 )
		return std::max(m_time + 180, m_conceed);

	// Otherwise, use the conceed time
	return m_conceed;
}

//////////////////////////////////////////////////////////////////////
// Expire things from the decision queues

bool
StratDecision::won() const
{
	// If a choice has been made or no votes places, no win
	if ( m_chosen != SC_OVERRUN || m_voters.size() == 0 || m_score == 0 )
		return false;

	if ( m_vetos >= m_score )
		return false;

	if ( m_holds == m_falls )
		return false;

	if ( eta() > teulTime() )
		return false;

	return true;
}

//////////////////////////////////////////////////////////////////////
// Constructor for a hold/fall vote object.

HoldFallVote::HoldFallVote(const class StratDecision* decision, Status status)
	: m_requester(0LL)
	, m_fullObjID(decision->m_property->Fid())
	, m_status(status)
	, m_remaining(0)
	, m_side(decision->m_side)
	, m_score(decision->m_score)
	, m_holds(decision->m_holds)
	, m_falls(decision->m_falls)
	, m_vetos(decision->m_vetos)
{
}

//////////////////////////////////////////////////////////////////////
// Determine if a reminder needs to be issued for a particular command

void
StratDecision::reminder()
{
	if ( teulTime() <= m_time )
	{
		dbugLog(_FLW, "overrun %s %u seconds overdue for processing", m_property->Name(), m_time - teulTime());
		return;
	}

	int beats = (teulTime() - m_time) / DECISION_POLL_INTERVAL_S;

	// Only bleat on even beats (every 60s)
	if ( beats < 2 || beats & 1 )
		return;

	if ( m_voters.size() == 0 )
	{
		// Only report about non-votes after 2, 4 and 7 minutes
		if ( beats != 4 && beats != 8 && beats != 14 )
			return;
		HoldFallVote hfv(this, HoldFallVote::_WAITING);	// "[%s] reporting overrun, awaiting orders"
		// Sent by destructor.
		return;
	}

	// If we have only one vote and it's not a veto...
	if ( m_voters.size() == 1 && m_vetos != 1 )
	{
		UINT32 remaining = (UINT32)ceil((eta() - teulTime()) / DECISION_POLL_INTERVAL_S);

		// Don't try to print negative numbers, or spam numbers which are
		// about to occur now.
		if ( remaining <= 0 )
			return;

		// Round to the nearest
		// m_holds > 0 ? "hold" : "fall";
		HoldFallVote hfv(this, HoldFallVote::_STANDS); 		// "[%s] Order stands as '%s'. %s to execution."
		hfv.m_remaining = remaining * DECISION_POLL_INTERVAL_S;
		// Sent by destructor.
		return;
	}

	// If holds/falls are tied
	if ( m_holds == m_falls && m_holds >= m_vetos && teulTime() > m_conceed )
	{
		m_conceed += 60;
		HoldFallVote hfv(this, HoldFallVote::_TIED); // "[%s] Orders are tied."
		// Sent by destructor
		return;
	}
}

//////////////////////////////////////////////////////////////////////

bool
chatCheckDecisionQueues()
{
	_drainDecisions("pending", _pendQueue, teulTime(), OVERRUN_TIMEOUT_MIN);
	_drainDecisions("history", _histQueue, teulTime(), host_config.objective.holdFallWindow_m);

	// Finish by seeing if any decisions need conceeding
	DECISION_QUEUE::iterator it = _pendQueue.begin();
	while ( it != _pendQueue.end() )
	{
		StratDecision* decision = *it;
		++it;

		if ( decision->won() == false )
		{
			decision->reminder();
			continue;
		}

		// Find a player from the voters to hold accountable
		VOTES& voters = decision->m_voters;
		Player* player = nullptr;
		for ( auto voter = voters.begin(), end = voters.end(); voter != end; ++voter )
		{
			player = PlayerInterface::Lookup(*voter);
			if ( player != nullptr )
				break;
		}

		if ( player == nullptr )
		{
			dbugLog(_FLI, "Decision %s has no players present, ignoring", decision->m_property->Name());
			continue;
		}

		dbugLog(_FLI, "Decision %s conceeding: Voters:%" PRSizeT ", Vetos:%d, Holds:%d, Falls:%d, Scoring:%d (%s)"
						, decision->m_property->Name()
						, voters.size()
						, decision->m_vetos
						, decision->m_holds
						, decision->m_falls
						, decision->m_score
						, (decision->m_score == decision->m_holds) ? "HOLD" : "FALL"
						);

		if ( decision->m_score == decision->m_holds )
		{
			player->contextualStratCommand("hold", "!", WWII_NO_COUNTRY, decision->m_side, decision->m_property);
			decision->m_chosen = SC_HOLD;
		}
		else if ( decision->m_score == decision->m_falls )
		{
			player->contextualStratCommand("fallback", "!", WWII_NO_COUNTRY, decision->m_side, decision->m_property);
			decision->m_chosen = SC_FALLBACK;
		}
		else
		{
			dbugLog(_FLA, "Somehow won decision %s that was neither hold nor fall", decision->m_property->Name());
		}
	}

	return true;
}

//////////////////////////////////////////////////////////////////////

static const PropCP*
_checkObjectiveTarget(const FULL_OBJ_ID& fid, Player* player)
{
	if ( fid.id == 0 )
	{
		player->message(LSTR_CAUTION, LSTR_SEL_TARGET);
		return nullptr;
	}

	const Property* property = chatFindProperty(fid);

	if ( property != nullptr && property->m_fid.objClass == CLASS_FACILITY )
		property = property->Parent();

	if ( property == nullptr )
	{
		dbugLog(_FLW, "player %s[%d/%d] requested objective " PRF_FULL_OBJ_ID " is not a valid target?"
					, player->callsign()
					, player->playerID()
					, player->teulIndex()
					, PRA_FULL_OBJ_ID(fid)
					);
		return nullptr;
	}

	if ( property->m_fid.objClass != CLASS_CP )
	{
		player->message(LSTR_CAUTION, LSTR_TARGET_CP);
		return nullptr;
	}

	return static_cast<PropCP*>(const_cast<Property*>(property));
}

//////////////////////////////////////////////////////////////////////

bool
chatVoteAttack(Player* player, const FULL_OBJ_ID& id)
{
	const PropCP* target = _checkObjectiveTarget(id, player);
	if ( !target )
		return false;

	dbugLog(_FLI, "%s[%u/%d] requesting %s attack for %s (" PRF_FULL_OBJ_ID ")"
					, player->callsign()
					, player->playerID()
					, player->teulIndex()
					, _sideNameAdj[player->side()]
					, target->Name()
					, PRA_FULL_OBJ_ID(id)
					);

	chatSendStratCommand(player->playerID(), SC_ATTACK, target->m_fid, player->side());

	return true;
}

//////////////////////////////////////////////////////////////////////

bool
chatVoteClear(Player* player, const FULL_OBJ_ID& id)
{
	const PropCP* target = _checkObjectiveTarget(id, player);
	if ( !target )
		return false;

	dbugLog(_FLI, "%s[%d/%d] requesting %s clear for %s (" PRF_FULL_OBJ_ID ")"
					, player->callsign()
					, player->playerID()
					, player->teulIndex()
					, _sideNameAdj[player->side()]
					, target->Name()
					, PRA_FULL_OBJ_ID(id)
					);

	chatSendStratCommand(player->playerID(), SC_CLEAR, target->m_fid, player->side());

	return true;
}

//////////////////////////////////////////////////////////////////////

bool
chatVoteDefend(Player* player, const FULL_OBJ_ID& id)
{
	const PropCP* target = _checkObjectiveTarget(id, player);
	if ( !target )
		return false;

	if ( target->m_fid.objType != CP_BRIDGE )
	{
		player->message(LSTR_CAUTION, LSTR_DEFEND_TOWN);
		return false;
	}

	dbugLog(_FLI, "%s[%d/%d] requesting %s defend for %s (" PRF_FULL_OBJ_ID ")"
					, player->callsign()
					, player->playerID()
					, player->teulIndex()
					, _sideNameAdj[player->side()]
					, target->Name()
					, PRA_FULL_OBJ_ID(id)
					);

	chatSendStratCommand(player->playerID(), SC_DEFEND, target->m_fid, player->side());

	return true;
}
