#include <syscfg.h> // Path is project specific.
// Date: Thu Sep 22 11:57:18 CDT 2005
//
// Functions for managing squad creation
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include <algorithm>

#include "chat/chatSquadControl.h"
#include "wwii/wwiiSquads.h"
#include "chat/chatPlayer.h"
#include "wwii/wwiiHash.h"
#include "chat/chatList.h"
#include "chat/chatSquadInvite.h"
#include "chat/chatSaveChannels.h"
#include "chat/chatSquadList.h"
#include "chat/chatSquadListTypes.h"

static const UINT32 CREATE_REQUEST_EXPIRY_S = 6 * 30;	// 3 minutes

struct SQUAD_REQUEST
{
	UINT32					expires;
	SquadControl			control;
	bool					approved;	// Did we get a CREATE_CHECK approval back?
	std::set<ChatHandle>	agreed;	// Which players have agreed
};

using SQUAD_REQUESTS = std::unordered_map<ChatHandle,SQUAD_REQUEST>;

extern UINT32 chatSendSquadCreateRequest(SquadControl& control, Player* player);

SQUAD_REQUESTS requestIndex;

static void
_expired(Player* player)
{
	if ( player == nullptr || player->teulIndex() < 0 || player->squadID() ) return;

	static SquadControlResult reply(SQR_CREATE_EXPIRED, SquadControl::_CREATE, 0);
	reply.send(player->teulIndex());
}

static void
_sendInvites(const ChatHandle squadHandle, const std::string& squadName, const ChatHandle co, const ChatHandle xo, const PLAYER_INDEX& invites)
{
	dbugLog(_FLI, "sending %s squad invites", squadName.c_str());
	for ( PLAYER_INDEX::const_iterator it = invites.begin(); it != invites.end(); ++it )
	{
		RANK_NO rank = (it->first == xo) ? Squad::XO : Squad::MEMBER;
		Player* invitee = it->second;
		dbugLog(_FLI, "c:%s sending invite to %s", invitee->callsign(), squadName.c_str());
		invitee->sendSquadCreateInvite(squadHandle, squadName, co, rank);
	}
}

static void
_checkMembers(Player* player, SquadControl& control, bool sendInvites)
{
	std::vector<ChatHandle>& players = control.m_players;
	PLAYER_INDEX approved;

	approved.clear();
	control.m_playerIDs.clear();

	SquadControlResult result(SquadControl::_INVALID, SquadControl::_CREATE, 0LL);

	for ( size_t i = 0, size = players.size(); i < size; ++i )
	{
		result.m_handle = players[i];
		try
		{
			ChatHandle handleID = chatGetLowercaseHandle(players[i]);
			if ( handleID == player->matchHandle() )
				throw SQR_CANT_DO_TO_SELF;
			// Skip duplicates
			if ( approved.find(handleID) != approved.end() )
				continue;
			Player* invitee = PlayerInterface::Lookup(handleID);
			approved[handleID] = invitee;
			if ( invitee == nullptr
				|| !invitee->isVisibleTo(player)
				|| invitee->ignoring(player)
				)
				throw SQR_PLAYER_NOT_ONLINE;
			if ( invitee->squadID() )
				throw SQR_PLAYER_IN_OTHER_SQUAD;
			if ( invitee->squadInvite() && invitee->squadInvite()->m_from != player->matchHandle() )
				throw SQR_PLAYER_CONSIDERING_OTHER;

			control.m_playerIDs.push_back(invitee->playerID());
		}
		catch (SQUAD_CONTROL_RESULT& e)
		{
			result.m_result = e;
			result.send(player->teulIndex());
		}
	}

	if ( approved.size() < SquadControl::CREATE_PLAYERS_REQUIRED )
		throw SQR_CREATE_TOO_FEW_MEMBERS;

	if ( control.m_playerIDs.size() < SquadControl::CREATE_PLAYERS_REQUIRED )
		throw SQR_CREATE_MEMBER_ERROR;

	// Can't proceed without the XO
	ChatHandle& xo = players[0];
	if ( approved[xo] == nullptr )
		throw SQR_CREATE_MEMBER_ERROR;

	if ( sendInvites )
		_sendInvites(control.m_handle, control.m_text, player->matchHandle(), xo, approved);
}

void
chatProcessSquadCreateRequest(SquadControl& control, Player* player)
{
	if ( !player || player->teulIndex() < 0 )
		throw SQR_OPERATION_FAILED;

	if ( requestIndex.find(player->matchHandle()) != requestIndex.end() )
		throw SQR_CREATE_IN_PROGRESS;

	// Check the name isn't already being applied for
	ChatHandle lowerHandle = chatGetLowercaseHandle(control.m_handle);
	SQUAD_REQUESTS::iterator it;
	for ( it = requestIndex.begin(); it != requestIndex.end(); ++it )
	{
		if ( chatGetLowercaseHandle(it->second.control.m_handle) == lowerHandle )
			throw SQR_SQUAD_EXISTS;
	}

	_checkMembers(player, control, false);

	SQUAD_REQUEST& request = requestIndex[player->matchHandle()];
	request.expires = teulTime() + CREATE_REQUEST_EXPIRY_S;
	request.control = control;
	request.approved = false;

	dbugLog(_FLD, "t:%d p:%u c:%s ok to proceed with squad creation", player->teulIndex(), player->playerID(), player->callsign());
}

static SQUAD_CONTROL_RESULT
_processCreateCheck(const ChatHandle /*originator*/, SQUAD_REQUEST& request, Player* player)
{
	_checkMembers(player, request.control, true);

	request.expires = teulTime() + CREATE_REQUEST_EXPIRY_S + 1;
	request.approved = true;

	return SQR_DONE;
}

static SQUAD_CONTROL_RESULT
_processCreateComplete(const ChatHandle /*originator*/, SQUAD_REQUEST& request, Player* player)
{
	bool updatedPlayer = false;
	for ( UINT32 i = 0; i < request.control.m_players.size(); ++i )
	{
		Player* member = PlayerInterface::Lookup(request.control.m_players[i]);
		if ( member != nullptr )
		{
			if ( member == player )
				updatedPlayer = true;
			member->SetSquadInvite(nullptr);
			member->tuneChannelType(SQUAD_CHANNEL);
			chatSaveChannels(*member);
		}
	}
	if ( !updatedPlayer && player && player->teulIndex() >= 0 && player->initialized() )
	{
		player->SetSquadInvite(nullptr);
		player->tuneChannelType(SQUAD_CHANNEL);
		chatSaveChannels(*player);
	}

	if ( player )
	{
		SquadList* squadList = chatFindSquadList(player->squadID());
		if ( squadList )
			squadList->created();
	}

	return SQR_CREATED;
}

SQUAD_CONTROL_RESULT
chatProcessSquadCreateReply(bool created, const ChatHandle originator, Player* player)
{
	// Find the original request for the member's list
	SQUAD_REQUESTS::iterator it = requestIndex.find(originator);
	if ( !created && it == requestIndex.end() )
		throw SQR_CREATE_EXPIRED;

	try
	{
		if ( !created )
			return _processCreateCheck(originator, it->second, player);
		else
		{
			_processCreateComplete(originator, it->second, player);
			requestIndex.erase(it);
			throw SQR_CREATED;
		}
	}
	catch (SQUAD_CONTROL_RESULT /*result*/)
	{
		requestIndex.erase(originator);
		throw;
	}
}

//////////////////////////////////////////////////////////////////////
// Deal with someone declining a squad creation request

void
chatSquadDeclineCreateInvite(Player* decliner, Player* creator)
{
	if ( decliner == nullptr || creator == nullptr )
	{
		dbugLog(_FLA, "%s: NULL pointers", __FUNCTION__);
		return;
	}

	if ( creator->teulIndex() >= 0 )
	{
		SquadControlResult notice(SQR_DECLINED, SquadControl::_CREATE, decliner->chatHandle());
		notice.send(creator->teulIndex());
	}

	// Look up the request
	SQUAD_REQUESTS::iterator it = requestIndex.find(creator->matchHandle());
	if ( it != requestIndex.end() )
	{
		SQUAD_REQUEST& request = it->second;
		Player* member;
		for ( UINT32 i = 0; i < request.control.m_players.size(); ++i )
		{
			member = PlayerInterface::Lookup(request.control.m_players[i]);
			if ( member != nullptr )
				member->SetSquadInvite(nullptr);
		}
	}

	// We could remove the request, but then you could spam,
	// so we won't
	//requestIndex.erase(creator->matchHandle());
}

SQUAD_CONTROL_RESULT
chatSquadAcceptCreateInvite(Player* acceptor, Player* creator)
{
	// Use a try block so that any failures result in you
	// losing your squad invitation. However success doesn't
	// cancel your invitation - so that you can't be
	// invited to another squad after helping create the
	// first one.
	// Pointer check
	if ( acceptor == nullptr || creator == nullptr )
	{
		dbugLog(_FLA, "NULL pointers");
		throw SQR_NO_INVITE;
	}

	// Does the create request still exist?
	SQUAD_REQUESTS::iterator it = requestIndex.find(creator->matchHandle());
	if ( it == requestIndex.end() )
	{
		dbugLog(_FLA, "creator has no request pending");
		throw SQR_NO_INVITE;
	}

	SQUAD_REQUEST& request = it->second;
	SquadControl& control = request.control;

	// Am I in the list of people being requested?
	// or has the guy started a new request to which I'm not an invitee
	// in which case letting me vote would be bad.
	if ( find(control.m_players.begin(), control.m_players.end(), acceptor->matchHandle()) == control.m_players.end() )
	{
		dbugLog(_FLA, "acceptor is not in the players list");
		throw SQR_NO_INVITE;
	}

	// Have I already agreed?
	if ( find(request.agreed.begin(), request.agreed.end(), acceptor->matchHandle()) != request.agreed.end() )
	{
		dbugLog(_FLA, "%s has already accepted invite to %s's squad", acceptor->callsign(), creator->callsign());
		throw SQR_NO_INVITE;
	}

	if ( request.agreed.size() >= SquadControl::CREATE_PLAYERS_REQUIRED )
	{
		dbugLog(_FLA, "squad already met member requirements");
		throw SQR_NO_INVITE;
	}

	// So add me
	request.agreed.insert(acceptor->matchHandle());

	// The CO needs to know which user just accepted
	if ( creator->teulIndex() >= 0 )
	{
		SquadControlResult notice(SQR_ACCEPTED, SquadControl::_CREATE, acceptor->chatHandle());
		notice.send(creator->teulIndex());
	}

	if ( request.agreed.size() >= SquadControl::CREATE_PLAYERS_REQUIRED )
		chatSendSquadCreateRequest(control, creator);

	return SQR_CREATE_WAITING_OTHERS;
}

bool
chatExpireSquadCreates()
{
	SQUAD_REQUESTS::iterator it, expired;

	for ( it = requestIndex.begin(); it != requestIndex.end(); )
	{
		Player* creator = PlayerInterface::Lookup(it->first);
		if ( it->second.expires > teulTime() && creator != nullptr && creator->teulIndex() >= 0 )
			++it;
		else
		{
			dbugLog(_FLD, "c:%s expiring squad request (%u:%u)", Chat::Name(it->first).c_str(), it->second.expires, teulTime());
			if ( it->second.approved )
				_expired(creator);
			expired = it++;
			requestIndex.erase(expired);
		}
	}

	return true;
}

