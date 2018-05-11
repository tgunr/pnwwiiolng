#include "syscfg.h" // Path is project specific.
//
// Date: Wed Oct 11 12:36:21 CDT 2006
//
// Client-specific Brigade Control functionality
//
// Copyright (C) PlayNet, Inc 1999-2006 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith
//


#include "chat/chatBrigadeControlList.h"
#include "client/clientBrigadeControlDescriptors.h"
#include "wwii/wwiiOwnershipEnums.h"
#include "mm/mmEnums.h"
#include "chat/chatHandle.h"
#include "client/GameClient.h"
#include "map/mapPersonaSummary.h"

//////////////////////////////////////////////////////////////////////
// Return false if the passed integer does not appear to be a valid
// teulIndex value

static inline bool _invalidTeulIndex(SINT32 teulIndex)
{
	return ( teulIndex < 0 );
}

//////////////////////////////////////////////////////////////////////
// Return false if the brigade ID passed does not refer to an actual
// HC unit

static inline bool _invalidBrigade(HCUnitID brigadeID)
{
	return IsInvalid(brigadeID);
}

//////////////////////////////////////////////////////////////////////
// Return false if the country/branch combination is not valid

static inline bool _invalidCountryBranch(UCHAR country, UCHAR branch)
{
	if ( country >= MAX_COUNTRIES || branch >= MAX_BRANCHES )
		return true;
	// If branch is zero, country must be too.
	return ( branch == 0 && country != 0 );
}

//	------------------------------	BrigadeControl (base class)

// See chatBrigadeControl.cpp for ::packer


//	------------------------------	BrigadeControlRequest

//////////////////////////////////////////////////////////////////////
// HOST/CLIENT: Interface for a client to request a list of brigades.
// for a full list (e.g. for GMs) you can specify
// country of WWII_NO_COUNTRY and BRANCH_NOT_USED
// All functions return false if the request is either not valid or
// if the request was not sent to the host for caching reasons.

bool BrigadeControlRequest::requestList(SINT32 teulIndex, UCHAR country, UCHAR branch)
{
	if ( PN_UNLIKELY(_invalidTeulIndex(teulIndex) || _invalidCountryBranch(country, branch)) )
		return false;

	m_action = REQ_LIST;
	m_country = country;
	m_branch = branch;

	return send(teulIndex);
}

//////////////////////////////////////////////////////////////////////
// H/C: Client volunteering for OIC

bool BrigadeControlRequest::volunteerOIC(SINT32 teulIndex, HCUnitID brigade)
{
	if ( PN_UNLIKELY(_invalidTeulIndex(teulIndex) || _invalidBrigade(brigade)) )
		return false;

	m_action = REQ_VOLUNTEER;
	m_brigadeID = brigade;

	return send(teulIndex);
}

//////////////////////////////////////////////////////////////////////
// H/C: Client nominating an OIC
// Brigade must be supplied to eliminate race conditions

bool BrigadeControlRequest::nominateOIC(SINT32 teulIndex, HCUnitID brigade, const ChatHandle nomineeHandle)
{
	if ( PN_UNLIKELY(_invalidTeulIndex(teulIndex) || _invalidBrigade(brigade) || !chatIsValidHandle(nomineeHandle)) )
		return false;

	m_action = REQ_NOMINATE;
	m_brigadeID = brigade;
	m_officer = nomineeHandle;

	return send(teulIndex);
}

//////////////////////////////////////////////////////////////////////
// H/C: Client vetoing an OIC

bool BrigadeControlRequest::vetoOIC(SINT32 teulIndex, HCUnitID brigade, const ChatHandle onHandle)
{
	if ( PN_UNLIKELY(_invalidTeulIndex(teulIndex) || _invalidBrigade(brigade) || !chatIsValidHandle(onHandle)) )
		return false;

	m_action = REQ_VETO;
	m_brigadeID = brigade;
	m_officer = onHandle;

	return send(teulIndex);
}

//////////////////////////////////////////////////////////////////////
// H/C: Client resigning OIC position of a specified brigade
// The brigade has to be specified to eliminate race conditions.

bool BrigadeControlRequest::resignOIC(SINT32 teulIndex, HCUnitID brigade)
{
	if ( PN_UNLIKELY(_invalidTeulIndex(teulIndex) || _invalidBrigade(brigade)) )
		return false;

	m_action = REQ_RESIGN;
	m_brigadeID = brigade;

	return send(teulIndex);
}

//////////////////////////////////////////////////////////////////////
// H/C: Client requesting to replace existing OIC (must be HC)
// Handle is provided for race condition elimination

bool BrigadeControlRequest::replaceOIC(SINT32 teulIndex, HCUnitID brigade, const ChatHandle oldHandle)
{
	if ( PN_UNLIKELY(_invalidTeulIndex(teulIndex) || _invalidBrigade(brigade) || !chatIsValidHandle(oldHandle)) )
		return false;

	m_action = REQ_REPLACE;
	m_brigadeID = brigade;
	m_officer = oldHandle;

	return send(teulIndex);
}

//////////////////////////////////////////////////////////////////////
// H/C: Client requesting to remove existing OIC (must be HC)
// Handle is provided for race condition elimination

bool BrigadeControlRequest::removeOIC(SINT32 teulIndex, HCUnitID brigade, const ChatHandle oicHandle)
{
	if ( PN_UNLIKELY(_invalidTeulIndex(teulIndex) || _invalidBrigade(brigade) || !chatIsValidHandle(oicHandle)) )
		return false;

	m_action = REQ_REMOVE;
	m_brigadeID = brigade;
	m_officer = oicHandle;

	return send(teulIndex);
}

//////////////////////////////////////////////////////////////////////
// H/C: Propose a brigade move; must specify the location you think you are
// moving it from.

bool BrigadeControlRequest::requestBrigadeMove(SINT32 teulIndex, HCUnitID brigade, const FULL_OBJ_ID destination, const FULL_OBJ_ID origin)
{
	if ( PN_UNLIKELY(_invalidTeulIndex(teulIndex) || _invalidBrigade(brigade)) )
		return false;

	// You must specify where you want to move it to and where you want to move it from
	if ( PN_UNLIKELY(!destination.id || !origin.id) )
		return false;

	m_action = REQ_MOVE;
	m_brigadeID = brigade;
	m_officer = destination.Key();
	m_target = destination;
	m_origin = origin;

	return send(teulIndex);
}

//////////////////////////////////////////////////////////////////////
// H/C: Propose a brigade move; must specify the location you think you are
// moving it from.

bool BrigadeControlRequest::vetoBrigadeMove(SINT32 teulIndex, HCUnitID brigade, const FULL_OBJ_ID destination, const FULL_OBJ_ID origin)
{
	if ( PN_UNLIKELY(_invalidTeulIndex(teulIndex) || _invalidBrigade(brigade)) )
		return false;

	// You must specify where you want to move it to and where you want to move it from
	if ( PN_UNLIKELY(!destination.id || !origin.id) )
		return false;

	m_action = REQ_VETO;
	m_brigadeID = brigade;
	m_officer = destination.Key();
	m_target = destination;
	m_origin = origin;

	return send(teulIndex);
}

//////////////////////////////////////////////////////////////////////
// H/C: Veto an objective (attack/defend/clear)

bool BrigadeControlRequest::vetoObjective(SINT32 teulIndex, HCUnitID brigade, const FULL_OBJ_ID target)
{
	if ( PN_UNLIKELY(_invalidTeulIndex(teulIndex) || _invalidBrigade(brigade)) )
		return false;

	// You must specify where you want to move it to and where you want to move it from
	if ( PN_UNLIKELY(!target.id) )
		return false;

	m_action = REQ_VETO;
	m_brigadeID = brigade;
	m_officer = target.Key();
	m_target = target;
	m_origin = nullObjID;

	return send(teulIndex);
}

bool BrigadeControlRequest::requestAttack(SINT32 teulIndex, const FULL_OBJ_ID destination)
{
	if ( PN_UNLIKELY(_invalidTeulIndex(teulIndex)) )
		return false;

	// You must specify where you want to move it to and where you want to move it from
	if ( PN_UNLIKELY(!destination.id) )
		return false;

	m_action = REQ_ATTACK;
	m_brigadeID = HCUnitID::null;
	m_officer = destination.Key();
	m_target = destination;

	return send(teulIndex);
}

bool BrigadeControlRequest::requestDefend(SINT32 teulIndex, const FULL_OBJ_ID destination)
{
	if ( PN_UNLIKELY(_invalidTeulIndex(teulIndex)) )
		return false;

	// You must specify where you want to move it to and where you want to move it from
	if ( PN_UNLIKELY(!destination.id) )
		return false;

	m_action = REQ_DEFEND;
	m_brigadeID = HCUnitID::null;
	m_officer = destination.Key();
	m_target = destination;

	return send(teulIndex);
}

bool BrigadeControlRequest::requestClear(SINT32 teulIndex, const FULL_OBJ_ID destination)
{
	if ( PN_UNLIKELY(_invalidTeulIndex(teulIndex)) )
		return false;

	// You must specify where you want to move it to and where you want to move it from
	if ( PN_UNLIKELY(!destination.id) )
		return false;

	m_action = REQ_CLEAR;
	m_brigadeID = HCUnitID::null;
	m_officer = destination.Key();
	m_target = destination;

	return send(teulIndex);
}

//	------------------------------	BrigadeControlNotice

//////////////////////////////////////////////////////////////////////
// Process a general host announcement

void BrigadeControlNotice::processNotice()
{
	// This is either a notice of an event or a response to a request
	// that we initiated - be careful in matching up because you may
	// get a notice on an event other than the one you are requesting
	// e.g. you request "nominate gophur" and you get a* notice*
	// that gophur was removed from another brigade before you get
	// your response to your request...

	PlayerClient()->ProcessBrigadeControlNotice(this);
}

//////////////////////////////////////////////////////////////////////
// Translate a brigade control notice into human readable form

BrigadeControlDescriptor BrigadeControlNotice::getDescription() const
{
	bool me = (m_officer == PlayerClient()->Personas().m_callsign) ? true: false;

	switch(m_notice)
	{
		case _ERROR:
		{
			switch(m_error)
			{
				case BCERR_INVALID_CALLSIGN:		return BCD_ControlError(this, "error.badcallsign");		// Candidate is unavailable
				case BCERR_PLAYER_UNAVAILABLE:		return BCD_ControlError(this, "error.badplayer");		// Candidate must join the brigade first
				case BCERR_WRONG_BRIGADE:			return BCD_ControlError(this, "error.notmember");		// Candidate must join the brigade first
				case BCERR_NOT_BRIGADE_LEADER:		return BCD_ControlError(this, "error.notleader");		// Can't do that unless you are the brigades OIC
				case BCERR_ALREADY_VOTING:			return BCD_ControlError(this, "error.voting");			// Brigade already has a vote in progress
				case BCERR_NO_VOTE_TO_VETO:			return BCD_ControlError(this, "error.notvoting");		// Brigade does not have a vote to veto
				case BCERR_VETO_MISMATCH:			return BCD_ControlError(this, "error.badveto");			// Brigade not voting on action you tried to veto
				case BCERR_HAS_OIC:					return BCD_ControlError(this, "error.hasoic");			// Brigade already has an OIC
				case BCERR_MUST_BE_HC_STAFF:		return BCD_ControlError(this, "error.nothc");			// Must be HC staff to do that
				case BCERR_NO_PERMISSION:			return BCD_ControlError(this, "error.noperms");			// Must be HC staff to do that
				case BCERR_MUST_BE_OFFICER:			return BCD_ControlError(this, "error.notofficer");		// Must be HC staff or OIC to do that
				case BCERR_CANT_NOMINATE_HC:		return BCD_ControlError(this, "error.cantnominate");	// Can't nominate HC staff to OIC
				case BCERR_PLAYER_IS_ALREADY_OIC:	return BCD_ControlError(this, "error.isoic");			// Player is already OIC of a brigade
				case BCERR_YOU_ALREADY_OIC:			return BCD_ControlError(this, "error.isoic.me");		// You are already OIC of a brigade
				case BCERR_OFFER_PENDING:			return BCD_ControlError(this, "error.hasoffer");		// Brigade has been offered to another player
				case BCERR_NOT_OFFERED:				return BCD_ControlError(this, "error.nooffer");			// Brigade is not being offered to you/offer expired
				case BCERR_PLAYER_NOT_OIC:			return BCD_ControlError(this, "error.notoic");			// Player is not OIC of the brigade
				case BCERR_NOT_A_MOVE:				return BCD_ControlError(this, "error.badtarget");		// Brigade is already there
				case BCERR_ALREADY_MOVED:			return BCD_ControlError(this, "error.badorigin");		// Brigade is not at the origin you specified
				case BCERR_BRIGADE_HAS_OIC:			return BCD_ControlError(this, "error.hasleader");		// Can't do that while the brigade has an OIC
				case BCERR_CANT_VETO_VS_SELF:		return BCD_ControlError(this, "error.selfveto");		// Can't veto an action on yourself
				case BCERR_YOU_WRONG_BRIGADE:		return BCD_ControlError(this, "error.wrongbrigade");	// You are in the wrong brigade
				case BCERR_OWN_TOWN:				return BCD_ControlError(this, "error.owntown");				// Target invalid because its friendly
				case BCERR_ALREADY_OBJECTIVE:		return BCD_ControlError(this, "error.alreadyobjective");	// Target already has an objective
				case BCERR_NOT_OBJECTIVE:			return BCD_ControlError(this, "error.notobjective");		// Not an objective
				case BCERR_CANT_CANX_DEFEND:		return BCD_ControlError(this, "error.cantcancel");			// Can't cancel defend, tit
				case BCERR_NOT_FRONTLINE:			return BCD_ControlError(this, "error.notfrontline");		// Can't attack non-frontline towns
				case BCERR_NOMINEE_CANT_OIC:			return BCD_ControlError(this, "error.notpermitted");		// We can't nominate this player
//				case BCERR_HAS_OTHER_OFFER:			return BCD_ControlError(this, "error.hasotheroffer");	// Nominee already has an offer for another brigade
			}
		}
		break;
		case _PROPOSAL:
		{
			switch(m_action)
			{
				case REQ_NOMINATE:			return BCD_SponsorOfficerBrigade(this, "proposal.nominate", me);	// Nominate a player for OIC of a brigade
				case REQ_REPLACE:			return BCD_OfficerBrigade(this, "proposal.replace", me);		// Request to take over from an existing OIC
				case REQ_REMOVE:			return BCD_SponsorOfficerBrigade(this, "proposal.remove", me);		// Want to remove an OIC
				case REQ_MOVE:				return BCD_SponsorBrigadeOriginTarget(this, "proposal.move", me);		// Move a brigade

				case REQ_ATTACK:
				case REQ_DEFEND:
				case REQ_CLEAR:
					return BCD_Objective(this, "proposal.objective", false);
			}
		}
		break;
		case _VETOED:
		{
			switch(m_action)
			{
				case REQ_NOMINATE:			return BCD_OfficerSponsorBrigade(this, "vetoed.nominate", me);		// Nominate a player for OIC of a brigade
				case REQ_REMOVE:			return BCD_SponsorOfficerBrigade(this, "vetoed.remove", me);		// Want to remove an OIC
				case REQ_MOVE:				return BCD_SponsorBrigadeOriginTarget(this, "vetoed.move", me);			// Move a brigade

				case REQ_ATTACK:
				case REQ_DEFEND:
				case REQ_CLEAR:
					return BCD_Objective(this, "vetoed.objective", false);
			}
		}
		break;
		case _OBJECTIVE:			return BCD_Objective(this, "objective", false);
		case _OFFERING:				return BCD_OfficerBrigade(this, "offering", me);
		case _APPOINTED:			return BCD_OfficerBrigade(this, "appointed", me);
		case _REMOVAL_PENDING:		return BCD_OfficerBrigade(this, "removal.pending", me);
		case _APPOINTMENT_FAILED:	return BCD_OfficerBrigade(this, "appointment.failed", me);
		case _REMOVED:				return BCD_OfficerBrigade(this, "removed", me);
		case _REPLACE_PENDING:		return BCD_SponsorOfficerBrigade(this, "replace.pending", me);
		case _OFFER_LAPSED:			return BCD_OfficerBrigade(this, "offer.lapsed", me);
		case _UNIT_MOVED:	// brigade was moved
		{
			if(m_sponsor)
				return BCD_SponsorBrigadeOriginTarget(this, "unit.moved", false);
			else
				return BCD_BrigadeOriginTarget(this, "unit.moved.system", false);
		}
	}

	return BrigadeControlDescriptor("invalid");
}

//	------------------------------	BrigadeControlList

