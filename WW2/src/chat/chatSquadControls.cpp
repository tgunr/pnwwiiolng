#include <syscfg.h> // Path is project specific.
//
// Date: Tue Sep 20 20:02:14 CDT 2005
//
// Handler functions for squad controls
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith
//

#include "versionNo.h"

#include <assert.h>

#include "chat/chatdbClientMessages.h"
#include "chat/chatList.h"
#include "chat/chatMissionInviteMsg.h"
#include "chat/chatPlayer.h"
#include "chat/chatPreferenceEnum.h"
#include "chat/chatSquadControl.h"
#include "chat/chatSquadInvite.h"
#include "chat/chatSquadList.h"
#include "chat/chatSquadListTypes.h"
#include "chat/chatSquadMemberUpdates.h"
#include "chat/chatSquadCreate.h"
#include "chat/chatSaveChannels.h"
#include "teul/teulKit.h"
#include "wwii/wwiiSquads.h"
#include "wwii/wwiiString.h"
#include "wwii/wwiiThreads.h"

static char _originator[MAX_HANDLE_SIZE], _subject[MAX_HANDLE_SIZE];

//////////////////////////////////////////////////////////////////////
// Write an audit entry for a squad action

static void
_audit(UINT32 squadID, UINT32 action
		, const char* actor, const char* subject
		, UINT32 newval=0, UINT32 prev=0
		, UINT32 coID=0
		, std::string text="")
{
	if ( !text.empty() )
		wwiiEscapeString(text);

	DBThread::Do(DBThread::Default
		, "INSERT LOW_PRIORITY INTO wwii_squad_audit"
				" (squadID, action, occured, originator, target, newval, prev, coID, textmessage)"
			" VALUES (%u, %u, now(), '%s', '%s', %u, %u, %u, '%s')"
					, squadID, action, actor, subject, newval, prev, coID, text.c_str()
	);
}

//////////////////////////////////////////////////////////////////////
// Add a player to a squad
// Kinda expensive - three database queries get queued up!

static void
_addSquadMember(SquadList* squadList, RANK_NO atRank, Player* addee, const ChatHandle& byHandle, bool recruited)
{
	UINT32 squadID = squadList->squad().squadID();

	DBThread::Do(DBThread::Default
		, "UPDATE wwii_player p, wwii_squads s"
			" SET p.squadid=s.squadid, p.squad_rank=%u, p.squad_joined=NOW(), p.squad_time_played=0"
			", s.last_joined=NOW(), s.last_joiner=p.callsign"
			" WHERE playerid=%u AND s.squadid=%u"
			, atRank, addee->playerID(), squadID
			);

	char adder[MAX_HANDLE_LEN+1];
	chatGetCallsign(adder, byHandle);
	_audit(squadID, SquadMemberUpdate::_ADD, adder, addee->callsign(), atRank, 0, 0, (recruited) ? "recruit" : "invite");

	squadList->memberJoin(addee, addee->matchHandle(), atRank, teulTime(), (recruited) ? adder : NULL, true);
}

//////////////////////////////////////////////////////////////////////
// Remove a player from a squad

static void
_removeSquadMember(bool removeFromDB, SquadList* list, SquadMemberEntry* entry, const ChatHandle& byHandle
					, int lstringMsg, const char* param)
{
	Player* player = entry->player();
	const ChatHandle& handle = entry->handle();

	if ( player )
	{
		player->unrecruit();
		player->stopRecruiting();
	}

	Chat::Name callsign(handle);
	if (player)
		callsign = player->callsign();

	if ( removeFromDB )
	{
		Chat::Name remover(byHandle);
		UINT32 squadID = list->squad().squadID();

		if ( player )
		{
			DBThread::Do(DBThread::Default
				, "UPDATE wwii_player"
					" SET squadid=0, squad_rank=0, squad_joined=NOW(), squad_time_played=0"
					" WHERE playerid=%u AND squadid=%u"
					, player->playerID(), squadID
					);
		}
		else
		{
			DBThread::Do(DBThread::Default
				, "UPDATE wwii_player"
					" SET squadid=0, squad_rank=0, squad_joined=NOW(), squad_time_played=0"
					" WHERE callsign='%s' AND squadid=%u"
					, callsign.c_str(), squadID
					);
		}

		_audit(squadID, SquadMemberUpdate::_REMOVE, remover.c_str(), callsign.c_str(), 0, entry->squadRank(), 0, (param ? param : ""));
	}

	if ( !list )
		throw SQR_SQUAD_UNAVAILABLE;

	list->removeMember(handle);
	list->sendMessage(LSTR_SQUAD, lstringMsg, callsign.c_str(), param);
}

void
SquadList::memberJoin(Player* player, const ChatHandle& handle, RANK_NO atRank, UINT32 joined, const char* recruitedBy, bool announceToSquad)
{
	if ( announceToSquad )
	{
		if ( recruitedBy )
			sendMessage(LSTR_SQUAD, LSTR_SQUAD_RECRUITED_BY_RECRUITER, player ? player->callsign() : Chat::Name(handle).c_str(), recruitedBy);
		else
			sendMessage(LSTR_SQUAD, LSTR_SQUAD_ADDITION, player ? player->callsign() : Chat::Name(handle).c_str(), squad().shortName());
	}

	updateMember(handle, atRank, player, joined);

	if ( player )
	{
		player->squelchSquadAnnounce();
		player->tuneChannelType(SQUAD_CHANNEL);
		chatSaveChannels(*player);

		player->SetSquadInvite(NULL);
		player->sendSquadInfo(this);
	}
}

static void
_trimWhitespace(std::string& text)
{
	while ( text.length() > 1 && (UCHAR)text[0] <= (UCHAR)' ' )
		text.erase(0, 1);
	while ( text.length() > 1 && (UCHAR)text[text.length()-1] <= (UCHAR)' ' )
		text.erase(text.length()-1, 1);
}

//////////////////////////////////////////////////////////////////////
// Check that a squad short name is OK

static void
_checkSquadName(std::string& name)
{
	_trimWhitespace(name);

	// Does it make a legal handle?
	ChatHandle handle = chatGetLowercaseHandle(name.c_str());
	if ( handle == 0LL )
		throw SQR_INVALID_SQUAD_SHORT;

	// Did we use the whole thing?
	char unpacked[MAX_HANDLE_SIZE];
	chatGetCallsign(unpacked, handle);
	if ( strcasecmp(name.c_str(), unpacked) != 0 )
		throw SQR_INVALID_SQUAD_SHORT;
	size_t len = strlen(unpacked);
	if ( len < 2 || len > Squad::SHORT_NAME_LEN )
		throw SQR_INVALID_SQUAD_SHORT;

	const char* p = name.c_str();
	while ( isdigit(*p) )
		++p;
	if ( *p == 0 )
		throw SQR_INVALID_SQUAD_SHORT;
}

//////////////////////////////////////////////////////////////////////
// Check and sanitize a homepage URL

static void
_checkHomepageURL(std::string& url)
{
   if ( url.length() == 0 )
        return;

	std::string protocol = "http://";
	size_t dot = url.find(".");
	size_t n = url.find(":/");

	if ( n >= url.length() )
		n = url.find(";/");

	if ( n >= url.length() )
	{
		n = url.find_first_of(":;");
		if ( n > dot )
			n = url.length();
	}

	if ( n < url.length() )
	{
		url[n++] = ':';
		protocol = url.substr(0, n);
		for ( UINT32 i = 0; i < n; ++i )
			protocol[i] = tolower(protocol[i]);
		if ( protocol.compare("file:") == 0 )
			throw SQR_TEXT_INVALID;
		if ( protocol.compare("http:") == 0 )
		{
			while ( url[n] == '/' )
				++n;
			protocol = "http://";
		}
		else if ( protocol.compare("https:") == 0 )
		{
			while ( url[n] == '/' )
				++n;
			protocol = "https://";
		}
	}
	else
		n = 0;

    if ( n > 0 )
        url.erase(0, n);
    url = protocol + url;
}

//////////////////////////////////////////////////////////////////////
// Validate a squads full name during create

static void
_checkSquadFullName(std::string& name)
{
	_trimWhitespace(name);

	if ( name.length() < 4 )
		throw SQR_INVALID_SQUAD_FULL;

	if ( name.length() > Squad::NAME_LENGTH )
		throw SQR_TEXT_TOO_LONG;
}

//////////////////////////////////////////////////////////////////////
// Check that a player* isn't* in a squad

static inline void
_checkNoSquad(const Player* player)
{
	if ( player->squadID() )
		throw SQR_ALREADY_IN_SQUAD;
}

//////////////////////////////////////////////////////////////////////
// Check that a player* is* in a squad and optionally has a given rank

static inline void
_checkInSquad(const Player* player, RANK_NO minRank=0)
{
	if ( !player->squadID() )
		throw SQR_NOT_IN_A_SQUAD;
	if ( minRank && player->squadRank() < minRank )
		throw SQR_RANK_TOO_LOW;
}

//////////////////////////////////////////////////////////////////////
// Check for an online player
// -- if source is non-null, that source can see the cilent,
// -- if requireSquad is zero, checks they are not in a squad,
// -- if requireSquad is non-zero, checks they are in that squad
// If requireSquad fails, mySquad is used to differ between
// "this squad" vs "any squad"

static Player*
_checkForPlayer(const ChatHandle& handle, const Player* source, UINT32 requireSquad, UINT32 mySquad)
{
	if ( handle < 64 )
		throw SQR_NO_SUCH_PLAYER;

	if ( source && handle == source->matchHandle() )
		throw SQR_CANT_DO_TO_SELF;

	Player* player = PlayerInterface::Lookup(handle);
	if ( player == NULL )
		throw SQR_PLAYER_NOT_ONLINE;
	if ( source && !player->isVisibleTo(source) )
		throw SQR_PLAYER_NOT_ONLINE;

	if ( player == source )
		throw SQR_CANT_DO_TO_SELF;

	if ( requireSquad == 0 && player->squadID() != 0 )
	{
		if ( mySquad != 0 && player->squadID() == mySquad )
			throw SQR_PLAYER_IN_SQUAD;
		throw SQR_PLAYER_IN_OTHER_SQUAD;
	}
	else if ( requireSquad != 0 && !player->inSquad(requireSquad) )
	{
		if ( !player->squadID() )
			throw SQR_PLAYER_NOT_IN_SQUAD;
		throw SQR_PLAYER_IN_OTHER_SQUAD;
	}

	return player;
}

static SquadMemberEntry*
_checkSquadMember(Player* player, SquadList* list, const ChatHandle& handle, RANK_NO minRank=Squad::STAFFER)
{
	if ( player->matchHandle() == handle )
		throw SQR_CANT_DO_TO_SELF;

	_checkInSquad(player, minRank);

	if ( list == NULL )
		throw SQR_SQUAD_UNAVAILABLE;

	SquadMemberEntry* entry = list->find(handle);
	if ( entry == NULL )
	{
		dbugLog(_FLI, "%llu [%s] is not in squad list", handle, Chat::Name(handle).c_str());
		throw SQR_PLAYER_NOT_IN_SQUAD;
	}

	if ( entry->squadRank() > player->squadRank() )
		throw SQR_PLAYER_OUTRANKS;

	return entry;
}

static void
_checkEmptyPosition(RANK_NO rank, SquadList* squadList)
{
	assert(squadList != NULL);

	if ( rank != Squad::CO ) return;

	const ChatHandle& handle = squadList->squad().coHandle();
	if ( handle == 0LL ) return;

	SquadMemberEntry* member = squadList->find(handle);
	if ( member == NULL ) return;
	if ( member->squadRank() != rank ) return;

	throw SQR_POSITION_OCCUPIED;
}

//////////////////////////////////////////////////////////////////////
// Decline current squad invite

static bool
_decline(Player* player)
{
	SquadInvite* invite = player->squadInvite();
	if ( !invite || invite->expired() )
		return false;

	Player* sender = PlayerInterface::Lookup(invite->m_from);
	if ( sender != NULL )
	{
		if ( invite->isCreation() )
			chatSquadDeclineCreateInvite(player, sender);
		else
			sender->message(LSTR_NOTE, LSTR_DECLINED_INVITE, player->callsign());
	}

	player->SetSquadInvite(NULL);

	return true;
}

//////////////////////////////////////////////////////////////////////
// Send an update to the dbd and return SQR_IN_PROGRESS

static UINT32
_update(UINT32 cmd, UINT32 action, UINT32 squadID, const ChatHandle& handle, UCHAR val, UCHAR prev, const ChatHandle& originator, const std::string& text="", UINT32 coID=0, const UINT32* members=NULL)
{
	SquadMemberUpdate smu;
	smu.m_action = action;
	smu.m_cmd = cmd;
	smu.m_squadID = squadID;
	smu.m_handle = handle;
	smu.m_rank = val;
	smu.m_prev = prev;
	smu.m_originator = originator;
	smu.m_coID = coID;
	strlcpy(smu.m_text, text.c_str(), sizeof(smu.m_text));
	if ( members )
	{
		for ( UINT32 i = 0; i < SquadControl::CREATE_PLAYERS_REQUIRED; ++i )
			smu.m_members[i] = members[i];
	}
	smu.m_result = 0;

	chatdbSendSquadMemberUpdate(smu);

	return SQR_IN_PROGRESS;
}

//////////////////////////////////////////////////////////////////////
// Process a create request

static UINT32
_processCreate(SquadControl& request, Player* player, SquadList* /*squadList*/)
{
	// Trash any invite we have
	_decline(player);

	_checkNoSquad(player);

	std::string squadname = _subject;
	_checkSquadName(squadname);
	_checkSquadFullName(request.m_text);

	// Check the name isn't already being applied for
	if ( chatCheckForSquadName(request.m_handle) )
		throw SQR_SQUAD_EXISTS;
	if ( chatFindSquadListByName(Chat::Name(request.m_handle).c_str()) )
		throw SQR_SQUAD_EXISTS;

	if ( request.m_players.empty() )
	{
		betaLog(_FLA, "m_players is empty");
		throw SQR_CREATE_TOO_FEW_MEMBERS;
	}
	if ( request.m_players.size() < SquadControl::CREATE_PLAYERS_REQUIRED )
	{
		betaLog(_FLA, "m_players contains %" PRSizeT " instead of %" PRSizeT " members", request.m_players.size(), SquadControl::CREATE_PLAYERS_REQUIRED);
		throw SQR_CREATE_TOO_FEW_MEMBERS;
	}

	chatProcessSquadCreateRequest(request, player);

	return _update(request.m_action, SquadMemberUpdate::_CREATE_CHECK, 0, request.m_handle, 0, 0, player->matchHandle(), request.m_text);
}

UINT32
chatSendSquadCreateRequest(SquadControl& request, Player* player)
{
	return _update(request.m_action, SquadMemberUpdate::_CREATE, 0, request.m_handle, 0, 0, player->matchHandle(), request.m_text, player->playerID(), &request.m_playerIDs[0]);
}

void
chatSendCOAppointment(UINT32 squadID, const ChatHandle& handle)
{
	_update(SquadControl::_INTERNAL, SquadMemberUpdate::_APPOINT, squadID, handle, Squad::CO, 0, 0LL);
}

//////////////////////////////////////////////////////////////////////
// Disband a squad

static UINT32
_disbandSquad(UINT32 squadID, SquadList* squadList, Player* player)
{
	// Dispatch database instructions
	DBThread::Do(DBThread::Default
					, "UPDATE LOW_PRIORITY wwii_player"
						" SET squadid=0, squad_rank=0, squad_joined=NOW(), squad_time_played=0"
						" WHERE squadid=%u"
						, squadID);
	DBThread::Do(DBThread::Default
					, "UPDATE wwii_squads"
						" SET disbanded=1 WHERE squadid=%u"
						, squadID);
	_audit(squadID, SquadMemberUpdate::_DISBAND, player->callsign(), player->callsign(), 0, 0, 0, "disband");

	// If we have a squadList pointer, then the squad is loaded
	// and we need to process the individual squad removals
	if ( !squadList )
		return SQR_DONE;

	assert( squadID == squadList->squad().squadID() );

	SQUAD_MEMBERS::iterator it;
	Player* member;

	for ( it = squadList->begin(); it != squadList->end(); ++it )
	{
		member = it->second.player();
		if ( member == NULL )
			continue;

		member->unrecruit();
		member->stopRecruiting();

		member->SetSquad(0, Squad::NOT_IN_SQUAD, 0L);
		member->sendSquadInfo(NULL);
		member->updateRadios(true, false, false, false, false, false);

		if ( member->teulIndex() >= 0 )
			member->message(LSTR_CAUTION, LSTR_SQUAD_DISBANDED
							, player->callsign(), squadList->squad().shortName());
	}

	chatRemoveSquadList(squadID);
	chatRemoveSquadName(squadList->squad().shortHandle());

	return SQR_DONE;
}

//////////////////////////////////////////////////////////////////////
// Process a disband request

static UINT32
_processDisband(SquadControl& /*request*/, Player* player, SquadList* squadList)
{
	_checkInSquad(player, Squad::CO);
	return _disbandSquad(squadList->squad().squadID(), squadList, player);
}

//////////////////////////////////////////////////////////////////////
// Process a player requesting to leave a squad

static UINT32
_processLeave(SquadControl& /*request*/, Player* player, SquadList* squadList)
{
	_checkInSquad(player);

	if ( squadList->members().size() <= 2 )
	{
		return _disbandSquad(squadList->squad().squadID(), squadList, player);
	}

	if ( player->squadRank() == Squad::CO )
		throw SQR_MUST_RESIGN;

	SquadMemberEntry* entry = squadList->find(player->matchHandle());
	if ( entry == NULL )
		throw SQR_SQUAD_UNAVAILABLE;

	_removeSquadMember(true, squadList, entry, entry->handle(), LSTR_SQUAD_LEAVER, "leave");

	throw SQR_LEFT;
}

//////////////////////////////////////////////////////////////////////
// Process a recruit request

static UINT32
_processRecruit(SquadControl& request, Player* player, SquadList* /*squadList*/)
{
	if ( request.m_handle == player->matchHandle() )
		throw SQR_CANT_DO_TO_SELF;

	_checkInSquad(player, Squad::STAFFER);

	Player* target = _checkForPlayer(request.m_handle, player, 0, player->squadID());
	if ( target->ignoring(player) )
		throw SQR_PLAYER_NOT_ONLINE;

	SquadInvite* invite = target->squadInvite();
	if ( invite && !invite->expired() )
	{
		if ( player->inSquad(invite->m_squadID) )
			throw SQR_PLAYER_CONSIDERING;
		throw SQR_PLAYER_CONSIDERING_OTHER;
	}

	target->sendSquadInvite(player->squadID(), player->matchHandle());

	return SQR_DONE;
}

//////////////////////////////////////////////////////////////////////
// Process a recruit-accept request

static UINT32
_processAccept(SquadControl& /*request*/, Player* player, SquadList*& squadList)
{
	if ( player->squadID() )
		throw SQR_ALREADY_IN_SQUAD;

	SquadInvite* invite = player->squadInvite();
	if ( !invite || invite->expired() )
		throw SQR_NO_INVITE;

	ChatHandle inviter = player->matchHandle();
	Player* sender = PlayerInterface::Lookup(invite->m_from);

	if ( invite->isCreation() )
	{
		player->SetSquadInvite(NULL);
		return chatSquadAcceptCreateInvite(player, sender);
	}

	squadList = chatFindSquadList(invite->m_squadID);
	if ( squadList == NULL )
		throw SQR_NO_INVITE;

	if ( sender != NULL )
	{
		sender->message(LSTR_NOTE, LSTR_ACCEPTED_INVITE, player->callsign());
		inviter = sender->chatHandle();
	}

	_addSquadMember(squadList, Squad::RECRUIT, player, inviter, false);

	return SQR_ACCEPTED;
}

//////////////////////////////////////////////////////////////////////
// Process a recruit-decline request

static UINT32
_processDecline(SquadControl& /*request*/, Player* player, SquadList* /*squadList*/)
{
	if ( player->squadID() )
		throw SQR_NO_INVITE;

	if ( !_decline(player) )
		throw SQR_NO_INVITE;

	return SQR_DONE;
}

//////////////////////////////////////////////////////////////////////
// Process a kick request

static UINT32
_processKick(SquadControl& request, Player* player, SquadList* squadList)
{
	dbugLog(_FLI, "t:%d p:%u c:%s sq:%d attempting to remove %llu[%s]", player->teulIndex(), player->playerID(), player->callsign(), player->squadID(), request.m_handle, _subject);
	SquadMemberEntry* entry = _checkSquadMember(player, squadList, request.m_handle);

	Player* target = PlayerInterface::Lookup(request.m_handle);
	if ( target )
	{
		SquadControlResult notice(SQR_KICKED, SquadControl::_KICK, player->chatHandle());
		notice.send(target->teulIndex());
	}

	_removeSquadMember(true, squadList, entry, player->chatHandle(), LSTR_SQUAD_KICK, player->callsign());

	throw SQR_HAVE_KICKED;
}

static UINT32
_processPromote(SquadControl& request, Player* player, SquadList* squadList)
{
	SquadMemberEntry* entry = _checkSquadMember(player, squadList, request.m_handle);
	RANK_NO newRank = entry->squadRank() + 1;

	// Can't promote someone above CO or your own rank
	if ( newRank > Squad::CO || newRank > player->squadRank() )
		throw SQR_PROMOTE_LIMIT;
	// And only a GM can promote someone* to* CO
	if ( player->squadRank() <= Squad::CO && newRank > Squad::XO )
		throw SQR_PROMOTE_LIMIT;

	_checkEmptyPosition(newRank, squadList);

	return _update(request.m_action, SquadMemberUpdate::_PROMOTE, player->squadID(), request.m_handle, newRank, entry->squadRank(), player->matchHandle());
}

static UINT32
_processDemote(SquadControl& request, Player* player, SquadList* squadList)
{
	SquadMemberEntry* entry = _checkSquadMember(player, squadList, request.m_handle);
	RANK_NO newRank = entry->squadRank() - 1;

	if ( newRank < Squad::MEMBER || newRank >= Squad::CO )
		throw SQR_DEMOTE_LIMIT;

	_checkEmptyPosition(newRank, squadList);

	return _update(request.m_action, SquadMemberUpdate::_DEMOTE, player->squadID(), request.m_handle, newRank, entry->squadRank(), player->matchHandle());
}

static bool
_checkSquadNight(std::string& text)
{
	UINT32 night = 0 , hour = 0;
	SINT32 timeZone = 0;

	int fields = sscanf(text.c_str(), "%u,%u,%d", &night, &hour, &timeZone);

	if ( fields != 3 )
		return false;
	if ( night > 6 || hour > 23 || timeZone < -23 || timeZone > 23 )
		return false;

	char reprint[Squad::MAX_NIGHT_LENGTH+1];
	snprintf(reprint, Squad::MAX_NIGHT_LENGTH, "%u,%u,%d", night, hour, timeZone);
	reprint[Squad::MAX_NIGHT_LENGTH] = 0;
	text = reprint;

	return true;
}

static bool
_checkSlogan(std::string& text)
{
	while ( text[0] == ' ' )
		text.erase(0);
/*
	while ( text.length() > 1 && text[text.length()-1] == ' ' )
		text.erase(text.length()-1);
*/
	return true;
}

static UINT32
_processText(SquadControl& request, Player* player, SquadList* squadList)
{
	_checkInSquad(player, Squad::STAFFER);

	const char* column = NULL;
	// m_handle enumerates which text field is being affected.
	switch ( request.m_key )
	{
			// Squad full name
	case SquadControl::TEXT_NAME:
		_checkSquadFullName(request.m_text);
		squadList->setFullName(request.m_text);
		column = "name";
	break;
			// Squad description
	case SquadControl::TEXT_DESCRIPTION:
		if ( request.m_text.length() < 3 ) throw SQR_TEXT_TOO_SHORT;
		if ( request.m_text.length() > Squad::DESCRIPTION_LENGTH ) throw SQR_TEXT_TOO_LONG;
		squadList->setDescription(request.m_text);
		column = "description";
	break;
			// Squad homepage URL
	case SquadControl::TEXT_HOMEPAGE:
		_checkHomepageURL(request.m_text);
		if ( request.m_text.length() > Squad::HOMEPAGE_LENGTH ) throw SQR_TEXT_TOO_LONG;
		squadList->setHomepage(request.m_text);
		column = "homepage";
	break;
			// Squad's message of the day
	case SquadControl::TEXT_MOTD:
		if ( request.m_text.length() > Squad::MOTD_LENGTH ) throw SQR_TEXT_TOO_LONG;
		squadList->setMOTD(request.m_text);
		column = "motd";
	break;
			// Squad-night field
	case SquadControl::TEXT_NIGHT:
		if ( request.m_text.length() < Squad::MIN_NIGHT_LENGTH ) throw SQR_TEXT_TOO_SHORT;
		if ( request.m_text.length() > Squad::MAX_NIGHT_LENGTH ) throw SQR_TEXT_TOO_LONG;
		if ( !_checkSquadNight(request.m_text) ) throw SQR_TEXT_INVALID;
		squadList->setSquadNight(request.m_text);
		column = "squad_night";
	break;

	case SquadControl::TEXT_SLOGAN:
		if ( request.m_text.length() && request.m_text.length() < 16 ) throw SQR_TEXT_TOO_SHORT;
		if ( request.m_text.length() > Squad::SLOGAN_LENGTH ) throw SQR_TEXT_TOO_LONG;
		if ( !_checkSlogan(request.m_text) ) throw SQR_TEXT_INVALID;
		squadList->setSlogan(request.m_text);
		column = "slogan";
	break;
	default:
		throw SQR_OPERATION_FAILED;
	break;
	}

	wwiiEscapeString(request.m_text);

	DBThread::Do(DBThread::Default
					, "UPDATE wwii_squads SET %s = '%s' WHERE squadid = %u"
						, column, request.m_text.c_str(), squadList->squad().squadID());

	// You ought to see the changes straight away
	player->sendSquadInfo(squadList);

	return SQR_DONE;
}

static UINT32
_processResign(SquadControl& request, Player* player, SquadList* squadList)
{
	SquadMemberEntry* entry = _checkSquadMember(player, squadList, request.m_handle, Squad::CO);
	if ( entry == NULL ) throw SQR_SQUAD_UNAVAILABLE;
	if ( player->squadRank() == Squad::GM ) throw SQR_OPERATION_FAILED;

	return _update(request.m_action, SquadMemberUpdate::_APPOINT, player->squadID(), request.m_handle, player->squadRank(), player->squadRank(), player->matchHandle());
}

static UINT32
_processInteger(SquadControl& request, Player* player, SquadList* squadList)
{
	_checkInSquad(player, Squad::STAFFER);

	// Name of the database column to affect
	const char* column = NULL;
	// Validate the value
	switch ( request.m_key )
	{
	case SquadControl::INT_LANGUAGE:
		_checkInSquad(player, Squad::XO);
		squadList->setLanguage(request.m_value);
		column = "pri_language";
	break;
	case SquadControl::INT_RECRUITING:
		request.m_value = (request.m_value) ? true : false;
		squadList->setRecruiting(request.m_value, player);
		column = "recruiting";
	break;
	case SquadControl::INT_VOTING:
		_checkInSquad(player, Squad::XO);
		request.m_value = (request.m_value) ? true : false;
		squadList->setVoting(request.m_value, player);
		column = "voting";
	break;
	default:
		dbugLog(_FLD, "t:%d p:%u c:%s invalid INT instruction k/v:%u,%u"
						, player->teulIndex(), player->playerID(), player->callsign()
						, request.m_key, request.m_value);
		throw SQR_OPERATION_FAILED;
	break;
	}

	if ( column == NULL || *column == 0 )
	{
		dbugLog(_FLW, "t:%d p:%u c:%s invalid squad integer %u"
						, player->teulIndex(), player->playerID(), player->callsign()
						, request.m_key);
		throw SQR_OPERATION_FAILED;
	}

	DBThread::Do(DBThread::Default
					, "UPDATE wwii_squads SET %s = %u WHERE squadid = %u"
						, column, request.m_value, squadList->squad().squadID());

	// You ought to see the changes straight away
	player->sendSquadInfo(squadList);

	return SQR_DONE;
}

// Issued by a recruit candidate requesting this squad from the squadlist
static UINT32
_processJoin(SquadControl& request, Player* player, SquadList*& squadList)
{
	// You're already in a squad
	if ( player->squadID() )
		throw SQR_ALREADY_IN_SQUAD;

	Player* recruiter = PlayerInterface::Lookup(request.m_handle);
	if ( recruiter == NULL )
		throw SQR_RECRUITER_GONE;
	squadList = recruiter->isAvailableRecruiter(player);
	if ( squadList == NULL )
		throw SQR_RECRUITER_GONE;

	_decline(player);

	MissionInvite invite(
		recruiter->missionID()
		, recruiter->country()
		, recruiter->branch()
		, recruiter->brigadeID()
		, recruiter->chatHandle()
		, squadList->squad().shortHandle()
		, true
		);
	MISSION_INVITE_MSG outgoing(invite);
	if ( outgoing.send(player->teulIndex()) < 0 )
		throw SQR_OPERATION_FAILED;

	player->recruited(recruiter);

	_addSquadMember(squadList, Squad::RECRUIT, player, recruiter->chatHandle(), true);

	return SQR_ACCEPTED;
}

static const char*
_control(UINT32 action)
{
	switch ( action )
	{
	case SquadControl::_CREATE: return "CREATE";
	case SquadControl::_DISBAND: return "DISBAND";
	case SquadControl::_RECRUIT: return "RECRUIT";
	case SquadControl::_ACCEPT: return "ACCEPT";
	case SquadControl::_DECLINE: return "DECLINE";
	case SquadControl::_KICK: return "KICK";
	case SquadControl::_LEAVE: return "LEAVE";
	case SquadControl::_PROMOTE: return "PROMOTE";
	case SquadControl::_DEMOTE: return "DEMOTE";
	case SquadControl::_TEXT: return "TEXT";
	case SquadControl::_INTEGER: return "INT";
	case SquadControl::_JOIN: return "JOIN";
	case SquadControl::_RESIGN: return "RESIGN";
	default: return "??UNKNOWN??";
	}
}

SINT32
SquadControl::receive(SINT32 teulIndex)
{
	Player* player = PlayerInterface::Lookup(teulIndex);
	if ( player == NULL )
	{
		dbugLog(_FLA, "t:%d not a valid player to receive squad controls from", teulIndex);
		return -EINVAL;
	}

	SquadList* squadList = NULL;
	SquadControlResult reply(_INVALID, m_action, m_handle);

	switch ( m_action )
	{
	case _DISBAND:
	case _ACCEPT:
	case _DECLINE:
	case _LEAVE:
	case _TEXT:
	case _INTEGER:
		_subject[0] = 0;
		/* Ignore m_handle */
	break;
	default:
		// Force handle to lowercase.
		m_handle = chatGetLowercaseHandle(m_handle);
		chatGetCallsign(_subject, m_handle);
	}

	chatGetCallsign(_originator, player->matchHandle());

	try
	{
		if ( player->squadID() )
		{
			squadList = chatFindSquadList(player->squadID());
			if ( squadList == NULL )
			{
				player->log(_FLA, "sq:%u No squad list?", player->squadID());
				throw SQR_SQUAD_UNAVAILABLE;
			}
		}

		player->log(_FLI, "squadControl: %s %s[%u] %s[%llx]"
							, _originator
							, _control(m_action), m_action
							, _subject, m_handle);

		switch ( m_action )
		{
		case SquadControl::_CREATE:		reply.m_result = _processCreate(*this, player, squadList); break;
		case SquadControl::_DISBAND:	reply.m_result = _processDisband(*this, player, squadList); break;
		case SquadControl::_RECRUIT:	reply.m_result = _processRecruit(*this, player, squadList); break;
		case SquadControl::_ACCEPT:		reply.m_result = _processAccept(*this, player, squadList); break;
		case SquadControl::_DECLINE:	reply.m_result = _processDecline(*this, player, squadList); break;
		case SquadControl::_KICK:		reply.m_result = _processKick(*this, player, squadList); break;
		case SquadControl::_LEAVE:		reply.m_result = _processLeave(*this, player, squadList); break;
		case SquadControl::_PROMOTE:	reply.m_result = _processPromote(*this, player, squadList); break;
		case SquadControl::_DEMOTE:		reply.m_result = _processDemote(*this, player, squadList); break;
		case SquadControl::_TEXT:		reply.m_result = _processText(*this, player, squadList); break;
		case SquadControl::_INTEGER:	reply.m_result = _processInteger(*this, player, squadList); break;
		case SquadControl::_JOIN:		reply.m_result = _processJoin(*this, player, squadList); break;
		case SquadControl::_RESIGN:		reply.m_result = _processResign(*this, player, squadList); break;
		default:
			player->log(_FLA, "invalid action %d", m_action);
			throw SQR_INVALID;
		break;
		}
	}
	catch (SQUAD_CONTROL_RESULT result)
	{
		reply.m_result = result;
	}

	// If the host is waiting on further progress
	switch ( reply.m_result )
	{
	case SQR_IN_PROGRESS:
		return 0;
	break;
	case SQR_ACCEPTED:
		reply.m_handle = (squadList) ? squadList->squad().shortHandle() : 0LL;
	break;
	case SQR_LEFT:
		reply.m_handle = (squadList) ? squadList->squad().shortHandle() : chatGetHandle("squad");
	break;
	default:
	break;
	}

	return reply.send(teulIndex);
}

static void
_processCreateReply(const SquadMemberUpdate& update, Player* orig, Player* /*target*/, SquadList* /*list*/)
{
	bool created = (update.m_action == SquadMemberUpdate::_CREATE);
	throw chatProcessSquadCreateReply(created, update.m_originator, orig);
}

static void
_processKickReply(const SquadMemberUpdate& update, Player* orig, Player* /*target*/, SquadList* list)
{
	const ChatHandle& origHandle = orig ? orig->chatHandle() : systemHandleID;
	SquadMemberEntry* entry = list->find(update.m_handle);
	if ( entry == NULL ) throw SQR_NOT_IN_A_SQUAD;

	_removeSquadMember(false, list, entry, origHandle, LSTR_SQUAD_KICK, _originator);

	throw SQR_HAVE_KICKED;
}

void
SquadList::memberLeave(bool removeFromDB, SquadMemberEntry* entry, int reasonLstringID, const char* extraText)
{
	_removeSquadMember(removeFromDB, this, entry, entry->handle(), reasonLstringID, extraText);
}

static void
_processLeaveReply(const SquadMemberUpdate& update, Player* /*orig*/, Player* /*targ*/, SquadList* list)
{
	if ( !list ) throw SQR_SQUAD_UNAVAILABLE;

	SquadMemberEntry* entry = list->find(update.m_handle);
	if ( !entry ) throw SQR_NOT_IN_A_SQUAD;

	list->memberLeave(false, entry, LSTR_SQUAD_LEAVER, "left");

	throw SQR_LEFT;
}

static void
_processPromoteReply(const SquadMemberUpdate& update, Player* /*orig*/, Player* targ, SquadList* list)
{
	if ( targ )
		targ->unrecruit();

	if ( !list ) throw SQR_SQUAD_UNAVAILABLE;

	list->updateMember(update.m_handle, update.m_rank, targ);

	LSTRING_ID msg = LSTR_NONE;
	switch (update.m_rank)
	{
	case Squad::MEMBER: msg = LSTR_PROMO_TO_MEMBER; break;
	case Squad::STAFFER: msg = LSTR_PROMO_TO_STAFFER; break;
	case Squad::XO: msg = LSTR_PROMO_TO_XO; break;
	case Squad::CO: msg = LSTR_PROMO_TO_CO; break;
	default: break;
	}

	if ( msg != LSTR_NONE )
		list->sendMessage(LSTR_SQUAD, msg, _originator, _subject);

	if ( targ != NULL )
		targ->unrecruit();

	list->setUpdated();
}

static void
_processDemoteReply(const SquadMemberUpdate& update, Player* /*orig*/, Player* targ, SquadList* list)
{
	if ( targ && update.m_rank < Squad::STAFFER )
		targ->stopRecruiting();

	if ( !list ) throw SQR_SQUAD_UNAVAILABLE;

	list->updateMember(update.m_handle, update.m_rank, targ);

	LSTRING_ID msg = LSTR_NONE;
	switch (update.m_rank)
	{
	case Squad::MEMBER: msg = LSTR_DEMO_TO_MEMBER; break;
	case Squad::STAFFER: msg = LSTR_DEMO_TO_STAFFER; break;
	case Squad::XO: msg = LSTR_DEMO_TO_XO; break;
	default: break;
	}

	if ( msg != LSTR_NONE )
		list->sendMessage(LSTR_SQUAD, msg, _originator, _subject);

	list->setUpdated();
}

static void
_processResignReply(const SquadMemberUpdate& update, Player* orig, Player* targ, SquadList* list)
{
	if ( !list )
		throw SQR_SQUAD_UNAVAILABLE;

	if ( update.m_rank == Squad::CO )
		list->sendMessage(LSTR_SQUAD, LSTR_CO_RESIGNED, _originator, _subject);
	else if ( update.m_rank == Squad::XO )
		list->sendMessage(LSTR_SQUAD, LSTR_XO_RESIGNED, _originator, _subject);

	// Update the originator first
	list->updateMember(update.m_originator, Squad::XO, orig);

	// Follow with an update of the target
	list->updateMember(update.m_handle, update.m_rank, targ);

	list->setUpdated();
}

//////////////////////////////////////////////////////////////////////
// Receive an incoming SquadMemberUpdate response

SINT32
SquadMemberUpdate::receive(SINT32 teulIndex)
{
	SquadControlResult reply(m_result, m_cmd, m_handle);

	chatGetCallsign(_subject, m_handle);
	chatGetCallsign(_originator, m_originator);
	betaLog(_FLD, "t:%d receiving squad member update act:%u cmd:%u sq:%u h:%s r:%u orig:%s text:%s res:%u [%s]"
					, teulIndex, m_action, m_cmd, m_squadID, _subject, m_rank, _originator, m_text, m_result, reply.printResult());

	Player* orig = PlayerInterface::Lookup(m_originator);
	Player* targ = (m_cmd == _CREATE) ? NULL : PlayerInterface::Lookup(m_handle);
	SquadList* list = chatFindSquadList(m_squadID);

	try
	{
		if ( m_result != SQR_DONE ) throw (SQUAD_CONTROL_RESULT)m_result;

		switch ( m_cmd )
		{
		case SquadControl::_CREATE:		_processCreateReply(*this, orig, targ, list); break;
		case SquadControl::_KICK:		_processKickReply(*this, orig, targ, list); break;
		case SquadControl::_LEAVE:		_processLeaveReply(*this, orig, targ, list); break;
		case SquadControl::_PROMOTE:	_processPromoteReply(*this, orig, targ, list); break;
		case SquadControl::_DEMOTE:		_processDemoteReply(*this, orig, targ, list); break;
		case SquadControl::_RESIGN:		_processResignReply(*this, orig, targ, list); break;

		case SquadControl::_INTERNAL:	break;

		default:
			dbugLog(_FLA, "t:%d invalid cmd %d", teulIndex, m_cmd);
			throw SQR_INVALID;
		break;
		}
	}
	catch (SQUAD_CONTROL_RESULT result)
	{
		reply.m_result = result;
	}
	catch (UINT32 result)
	{
		reply.m_result = result;
	}
	catch (std::exception& e)
	{
		dbugLog(_FLA, "squad control failed: %s", e.what());
		reply.m_result = SQR_OPERATION_FAILED;
	}

	Player* replyTo;
	switch ( reply.m_result )
	{
	case SQR_ACCEPTED:
		reply.m_handle = list->squad().shortHandle();
		replyTo = targ;
	break;
	case SQR_LEFT:
		reply.m_handle = list->squad().shortHandle();
		replyTo = orig;
	break;
	default:
		replyTo = orig;
	break;
	}

	if ( replyTo != NULL && replyTo->teulIndex() >= 0 )
		return reply.send(replyTo->teulIndex());

	return 0;
}

//////////////////////////////////////////////////////////////////////
// Change the CO of a squad -- wrapped to avoid needing to expose
// the innards of SquadList

void
chatSquadMakeCO(Player* player, SquadList* list)
{
	list->appointNewCO(player->matchHandle(), player);
}
