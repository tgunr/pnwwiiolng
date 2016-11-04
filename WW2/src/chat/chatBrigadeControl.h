#pragma once
// Created: Mon Oct  9 14:30:13 CDT 2006
//
// Copyright (C) PlayNet, Inc 1999-2006 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith
//
// Classes used to request and manipulate brigade lists and OICs
//
// The brigade OIC system happens on the chat host, while the
// authority of the brigade list is currently on the MMHost.

// Some of these requests require other players to approve them,
// when this happens the host will send the relevant messages
// to the client to be processed in BrigadeControlNotice.process()

// BrigadeControlRequest is provided as a courtesy layer, it
// extends BrigadeControl only by providing request functions.

// bcr->requestList(ChatConnection, country or 0, branch or 0)
//	Request an updated list of brigade info for country/branch
//		SEE ALSO: BrigadeControlList for the response format
// bcr->volunteerOIC(ChatConnection, brigadeID)
//  If you are volunteering to be a brigade OIC, use this
//  call. Use the same call to* approve* a request
// bcr->nominateOIC(ChatConnection, brigade, handle)
//  Call to nominate someone else as OIC for a brigade
// bcr->vetoOIC(ChatConnection, brigade, handleID)
//	Block someone from becoming OIC
// bcr->resignOIC(ChatConnection, brigadeID)
//	Volunteer to give up your OIC position
// bcr->replaceOIC(ChatConnection, brigadeID)
//	Brigade/HC staff wishing to take the place of an existing
//	OIC
// bcr->evictOIC(ChatConnection, brigadeID, handleID)
//	Call to have an HC officer removed

// I'll add the move/etc commands once we get the basics
// working.

#include "teul/teulBasic.h"
#include "teul/teulPackers.h"
#include "wwii/wwiiFullObjID.h"

enum BRIGADE_CONTROL_ERROR
{
	BCERR_INVALID = 0

,	BCERR_INVALID_REQ				// Invalid side/country/branch in request
,	BCERR_INVALID_CALLSIGN			// Callsign for officer is invalid
,	BCERR_PLAYER_UNAVAILABLE		// Candidate is unavailable
,	BCERR_WRONG_BRIGADE				// Candidate must join the brigade first
,	BCERR_NOT_BRIGADE_LEADER		// Can't do that unless you are the brigades OIC
,	BCERR_ALREADY_VOTING			// Brigade already has a vote in progress
,	BCERR_NO_VOTE_TO_VETO			// Brigade does not have a vote to veto
,	BCERR_VETO_MISMATCH				// Brigade not voting on action you tried to veto
,	BCERR_HAS_OIC					// Brigade already has an OIC
,	BCERR_MUST_BE_HC_STAFF			// Must be HC staff to do that
,	BCERR_MUST_BE_OFFICER			// Must be HC staff or OIC to do that
,	BCERR_CANT_NOMINATE_HC			// Can't nominate HC staff to OIC
,	BCERR_PLAYER_IS_ALREADY_OIC		// Player is already OIC of a brigade
,	BCERR_YOU_ALREADY_OIC			// You are already OIC of a brigade
,	BCERR_OFFER_PENDING				// Brigade has been offered to another player
,	BCERR_NOT_OFFERED				// Brigade is not being offered to you/offer expired
,	BCERR_NO_PERMISSION				// You don't have permission to do that
,	BCERR_PLAYER_NOT_OIC			// Player is not OIC of the brigade
,	BCERR_NOT_A_MOVE				// Brigade is already there
,	BCERR_ALREADY_MOVED				// Brigade is not at the origin you specified
,	BCERR_BRIGADE_HAS_OIC			// Can't do that while the brigade has an OIC
,	BCERR_CANT_VETO_VS_SELF			// Can't veto an action on yourself
,	BCERR_YOU_WRONG_BRIGADE			// You aren't in that brigade
,	BCERR_OWN_TOWN					// Target invalid because its friendly
									// (e.g. for attack)
,	BCERR_ALREADY_OBJECTIVE			// Target already has an objective
,	BCERR_NOT_OBJECTIVE				// Not an objective
,	BCERR_CANT_CANX_DEFEND			// Can't cancel defend, tit
,	BCERR_NOT_FRONTLINE				// Can't attack non-frontline towns
,	BCERR_NOMINEE_CANT_OIC			// The player you tried to nominate doesn't have OIC permission
,	MAX_BC_ERRORS
};

//////////////////////////////////////////////////////////////////////
// Base class which describes a particular brigade control function

class BrigadeControl
{
public:
	//// NOTICE CODES: (used by BrigadeControlNotice)
	// These messages double as both feedback to individual requests,
	// and announcements to players: e.g. the appointment messages
	// are sent to all players instead of sending a separate chat message.
	enum RESPONSES
	{
		_ERROR					// An error notice

		// Most requests are submitted to the HC staff for review before
		// being executed. The HC can prevent an action by vetoing it.
		// To parse PROPOSAL and VETOED you must start by looking at
		// m_action, and then at other fields as befits the REQ code in it.
	,	_PROPOSAL				// Notice sent to indicate that a votable
								// item has been posted, e.g. a nomination
								// for OIC, a brigade move, etc.
								// See m_action for the REQ_* being
								// proposed.
								// m_sponsor: player issuing proposal
								//	or "SYSTEM"
								// m_nominee: [may be] player affected
								// m_target: [may be] destination/target of action
								// m_time: seconds until action occurs
	,	_VETOED					// Someone has vetoed the proposal described
								// See m_action for the REQ_* being blocked.

		// These are personal notifications

		// These messages double as feedback to you AND notice to all players
		// so make sure you check the handle of the player...

	,	_OFFERING				// You are being offered a position
	,	_UNIT_MOVED				// Notice that a unit has been moved
	,	_APPOINTED				// Player has been promoted to OIC of a brigade
	,	_REMOVAL_PENDING		// Player is being removed from OIC position
								// NOTE: If sponsor is you, it's a resignation
	,	_APPOINTMENT_FAILED		// Appointment failed because player offline/changed brigade/already OIC
	,	_REMOVED				// Player has been removed/stepped down from OIC
	,	_REPLACE_PENDING		// OIC is replacing you
	,	_OFFER_LAPSED			// Offer to a player timed out
	,	_OBJECTIVE				// Attack/Clear/Defend see action code
	};

public:
	//// ACTION CODES:
	enum
	{
		REQ_INVALID				// Uninitialized
	,	REQ_LIST				// Request a list
	,	REQ_NOMINATE			// Nominate a player for OIC of a brigade
	,	REQ_VETO				// Veto a nomination
	,	REQ_VOLUNTEER			// Volunteer for an OIC position
	,	REQ_RESIGN				// Request to step down
	,	REQ_REPLACE				// Request to take over from an existing OIC
								// (If you request position while there is an
								//  active OIC and you are not brigade owner,
								//  your request will be _DENIED)
	,	REQ_REMOVE				// Want to remove an OIC
	,	REQ_MOVE				// Move a brigade
	,	REQ_ATTACK				// Attack a target
	,	REQ_CLEAR				// Clear an attack or defend objective
	,	REQ_DEFEND				// Defend a target

	,	MAX_BC_REQ_CODES		// Maximum brigade control request codes
	};

public:
	BrigadeControl()
		: m_action(REQ_INVALID), m_country(0), m_branch(0), m_brigadeID(0)
		, m_officer(0), m_sponsor(0)
		, m_target(nullObjID), m_origin(nullObjID)
		{}
	BrigadeControl(UINT32 action, UCHAR country, UCHAR branch, UINT32 brigadeID, const ChatHandle officer, const ChatHandle sponsor, const FULL_OBJ_ID origin=nullObjID)
		: m_action(action), m_country(country), m_branch(branch), m_brigadeID(brigadeID), m_officer(officer), m_sponsor(sponsor), m_origin(origin)
		{}
    
    BrigadeControl(ACTION_OBJECT* const a) { packer(TEUL_UNPACK, a); }

	bool isTargettedAction() const { return (m_action == REQ_MOVE || m_action == REQ_ATTACK || m_action == REQ_CLEAR || m_action == REQ_DEFEND); }
	bool isObjective() const { return (m_action == REQ_ATTACK || m_action == REQ_CLEAR || m_action == REQ_DEFEND); }

public:
	UINT32		m_action;		// What action you was requested
	UCHAR		m_country;		// Country affected
	UCHAR		m_branch;		// Branch affected
	UINT32		m_brigadeID;	// Brigade No
	ChatHandle	m_officer;		// Callsign of officer
	ChatHandle	m_sponsor;		// Callsign of sponsor (electee/vetoer) where applicable
	FULL_OBJ_ID	m_target;		// Move destination/attack target
	FULL_OBJ_ID	m_origin;		// Where its moving from

public:
	SINT32 packer(PackMode mode, ACTION_OBJECT* a);

public:
	static TEUL_CALLBACK* s_callback;

public:
	bool send(SINT32 teulIndex) { return (teulSend(teulIndex, s_callback, this) >= 0); }
};

//////////////////////////////////////////////////////////////////////
// BrigadeControlRequest class for issuing requests

class BrigadeControlRequest : public BrigadeControl
{
public:
	BrigadeControlRequest() : BrigadeControl() {}
	BrigadeControlRequest(ACTION_OBJECT* const a) : BrigadeControl(a) {}
	~BrigadeControlRequest() {}

public:
	//// HOST: Process an incoming request
	void processRequest(SINT32 teulIndex);

public:
	// HOST/CLIENT: Interface for a client to request a list of brigades.
	// for a full list (e.g. for GMs) you can specify
	// country of WWII_NO_COUNTRY and BRANCH_NOT_USED
	// All functions return false if the request is either not valid or
	// if the request was not sent to the host for caching reasons.
	bool requestList(SINT32 teulIndex, UCHAR country, UCHAR branch);
	// H/C: client volunteering for OIC
	bool volunteerOIC(SINT32 teulIndex, UINT32 brigade);
	// H/C: client nominating an OIC
	bool nominateOIC(SINT32 teulIndex, UINT32 brigade, const ChatHandle nomineeHandle);
	// H/C: client vetoing an OIC
	bool vetoOIC(SINT32 teulIndex, UINT32 brigade, const ChatHandle onHandle);
	// H/C: client resigning OIC position of a specified brigade
	bool resignOIC(SINT32 teulIndex, UINT32 brigade);
	// H/C: client requesting to replace existing OIC (must be HC)
	bool replaceOIC(SINT32 teulIndex, UINT32 brigade, const ChatHandle oldHandle);
	// H/C: client wants to remove an existing OIC
	bool removeOIC(SINT32 teulIndex, UINT32 brigade, const ChatHandle oicHandle);
	// H/C: Propose a brigade move; must specify the location you think you are
	// moving it from.
	bool requestBrigadeMove(SINT32 teulIndex, UINT32 brigade, const FULL_OBJ_ID destination, const FULL_OBJ_ID origin);
	// H/C: Propose to attack a city with this brigade
	bool requestAttack(SINT32 teulIndex, const FULL_OBJ_ID destination);
	// H/C: Propose the cancellation of an attack; not worried about the origin for this one
	bool requestDefend(SINT32 teulIndex, const FULL_OBJ_ID destination);
	// H/C: Propose the cancellation of an attack; not worried about the origin for this one
	bool requestClear(SINT32 teulIndex, const FULL_OBJ_ID destination);
	// H/C: Veto a brigade move
	bool vetoBrigadeMove(SINT32 teulIndex, UINT32 brigade, const FULL_OBJ_ID destination, const FULL_OBJ_ID origin);
	// H/C: Veto an objective order
	bool vetoObjective(SINT32 teulIndex, UINT32 brigade, const FULL_OBJ_ID target);
};

//////////////////////////////////////////////////////////////////////
// BrigadeControlNotice doubles as both the response mechanism for
// brigade control requests and for announcements of brigade control
// changes.
//
// Per e.g. when a player is nominated to control of a brigade,
// all players in that brigade and all HC officers will receive
// an _APPOINTED notification.

struct BrigadeControlDescriptor;

// Voting permits
// None = you can't veto this
// All = If you see this you can veto it
// Officer = You must be commander or OIC to veto
// Commander = You must be a high commander to veto
// OIC = You must be an OIC to veto
enum VOTE_PERMIT { VP_NONE, VP_ALL, VP_OFFICER, VP_COMMANDER, VP_OIC };

class BrigadeControlNotice : public BrigadeControl
{
public:
	BrigadeControlNotice(UINT32 notice=_ERROR)
		: BrigadeControl(), m_notice(notice), m_error(0), m_time(0), m_votePermit(VP_NONE), m_received(0) {}
	BrigadeControlNotice(UINT32 notice, UINT32 error, UINT32 action, VOTE_PERMIT votePermit, class HCUnit const& unit, const ChatHandle subject, const ChatHandle actor, UINT32 duration=0);
	BrigadeControlNotice(const BrigadeControl& rhs, UINT32 notice, UINT32 error, UINT32 time)
		: BrigadeControl(rhs), m_notice(notice), m_error(error), m_time(time), m_votePermit(VP_NONE), m_received(0) {}
	BrigadeControlNotice(ACTION_OBJECT* const a) { packer(TEUL_UNPACK, a); }

public:
	//// CLIENT: These functions are essentially entry points
	//// for data coming back from the host
	void processResponse();
	void processNotice();
	BrigadeControlDescriptor getDescription() const;

	// OICbrigade = id of the brigade you are OICing or 0
	// amCommander = true if you are a high command staffer
	bool isVetoable(UINT32 oicBrigade, bool amCommander) const
	{
		switch ( m_votePermit )
		{
		case VP_ALL: return true;
		break;
		case VP_COMMANDER: return amCommander;
		break;
		case VP_OFFICER: return (oicBrigade > 0 || amCommander);
		break;
		case VP_OIC: return (oicBrigade > 0 && oicBrigade == m_brigadeID);
		break;
		default:
		break;
		}
		return false;
	}

public:
	UINT32		m_notice;		// Message code
	UINT32		m_error;		// If m_notice is _ERROR, this is populate with an LStringID
	UINT32		m_time;			// How-long (in sec) before a delayed effect takes place (e.g. nomination)
	UINT32		m_votePermit;	// See VOTE_PERMIT enums
	SINT32		m_received;		// point in time to base m_time off of

public:
	SINT32 packer(PackMode mode, ACTION_OBJECT* a);
};

