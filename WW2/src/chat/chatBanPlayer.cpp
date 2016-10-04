#include <syscfg.h> // Path is project specific.
// Date: Fri Oct 14 14:09:44 CDT 2005
//
// Functionality for banning/unbanning players
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "versionNo.h"

#include <mysql.h>

#include "chat/chatPlayer.h"
#include "chat/chatHost.h"
#include "chat/chatLStringIDs.h"
#include "chat/chatName.h"
#include "wwii/wwiiThreads.h"
#include "wwii/wwiiThreadText.h"
#include "wwii/wwiiThreadTypes.h"
#include "chat/chatTimeInterval.h"
#include "include/mydbQuery.h"

struct BAN
{
	BAN(const ChatHandle handle, UINT32 duration, const char* message, UINT32 issuer, const ChatHandle issuerHandle)
		: m_handle(handle), m_duration(duration), m_message(message), m_issuer(issuer), m_issuerHandle(issuerHandle)
		, m_banNo(0), m_playerID(0)
		{}
	~BAN() {}

	ChatHandle	m_handle;
	UINT32		m_duration;
	std::string	m_message;
	UINT32		m_issuer;
	ChatHandle	m_issuerHandle;
	UINT32		m_banNo;
	playerid_t	m_playerID;
};
typedef ConditionQueue<BAN> BAN_QUEUE;
BAN_QUEUE banQueue;

//////////////////////////////////////////////////////////////////////
// Ban a player -- if player is not null then the player is notified
// of the banning but is not kicked.

void
chatBanPlayer(const ChatHandle handle, UINT32 duration, const char* message, UINT32 issuer, const ChatHandle issuerHandle)
{
	banQueue.push_back(new BAN(handle, duration, message, issuer, issuerHandle));
}

void
chatForceKickPlayer(const ChatHandle handle, const char* message, UINT32 issuer, const ChatHandle issuerHandle)
{
	banQueue.push_back(new BAN(handle, 0, message, issuer, issuerHandle));
}

//////////////////////////////////////////////////////////////////////

void
_banFail(void* data)
{
	BAN* ban = (BAN*)data;

	// Tell the banner it failed
	Player* player = PlayerInterface::Lookup(ban->m_issuerHandle);
	if ( player != nullptr )
	{
		Chat::Name bannedName(ban->m_handle);
		player->message(LSTR_CAUTION, LSTR_ERROR_VERB_SUBJECT, (ban->m_duration > 0) ? "ban" : "kick", bannedName.c_str());
		player->message(LSTR_CAUTION, LSTR_GENERIC, ban->m_message.c_str());
	}

	delete ban;
}

//////////////////////////////////////////////////////////////////////

void
_banSuccess(void* data)
{
	BAN* const ban = (BAN*)data;
	Player* const issuer = PlayerInterface::Lookup(ban->m_issuerHandle);
	if ( issuer != nullptr )
	{
		// Tell the issuer they were successful
		Chat::Name banned(ban->m_handle);
		char msg[MAX_MESSAGE_LENGTH+1];
		if ( ban->m_duration == 0 )
			strprintf(msg, "%s has force-kicked %s: %s", issuer->callsign(), banned.c_str(), ban->m_message.c_str());
		else
			strprintf(msg, "%s has issued a %s ban(#%u) on %s: %s"
						, issuer->callsign()
						, chatTimeInterval(ban->m_duration)
						, ban->m_banNo
						, banned.c_str()
						, ban->m_message.c_str()
			);
		chatDispatchGMMessage("INFO", msg);
	}

	if ( ban->m_duration == 0 )
	{
		betaLog(_FLI, "p:%u issuing smite on playerID", ban->m_playerID);
		chatSmitePlayer(ban->m_playerID);
	}
	else
	{
		// Tell the banee they're banned
		Player* const player = PlayerInterface::Lookup(ban->m_handle);
		if ( player != nullptr )
			player->message(LSTR_WARNING, LSTR_ACCOUNT_SUSPENDED_NOTICE, chatTimeInterval(ban->m_duration), ban->m_message.c_str());
	}

	delete ban;
}

//////////////////////////////////////////////////////////////////////
// Function for processing an entry from the ban queue

static void
_processBan(DB::handle_t* conn, BAN& ban)
{
	char* p = nullptr;
	Chat::Name name(ban.m_handle);
	Chat::Name issuer(ban.m_issuerHandle);
	char    *message = nullptr;

	try
	{
		mysql_ping(conn);

		threadLog(_FLN, "p:%u c:%s %s(%u) %s", ban.m_issuer, issuer.c_str(), (ban.m_duration > 0) ? "ban":"kick", ban.m_duration, name.c_str());

		message = new char[strlen(ban.m_message.c_str())*2 + 1];
		mysql_real_escape_string(conn, message, ban.m_message.c_str(), ban.m_message.length());
		
		// p_ban_player ( playerID, banFor, banBy, reason ) returns either
		// 0, <reason>
		// 1, <#bans>
		static const char QRY_CALL_BAN_PLAYER[] = "CALL wwii.p_ban_player ('%s', %u, %u, '%s')";

		p = threadsPrintf(QRY_CALL_BAN_PLAYER, name.c_str(), ban.m_duration, ban.m_issuer, message);
		DBQuery q(conn, p);

		delete message;
		message = nullptr;
	
		playerid_t playerID = 0;
		if ( q.HasRows() && q.FetchRow() )
			playerID = q.Row().NextUInt();

		if ( playerID == 0 )
			throw std::domain_error(q.Row().NextString());

		// If its positive, we have playerid, notes, warnings, bans
		ban.m_playerID = playerID;
		ban.m_banNo = q.Row().NextUInt();

		threadCallback(_banSuccess, &ban);

		threadLog(_FLN, "p:%u subj:%s %llu vs %llu issued %us ban", ban.m_issuer, name.c_str(), ban.m_issuerHandle, ban.m_handle, ban.m_duration);
	}
	catch (std::exception& e)
	{
		if (message != nullptr)
			delete message;

		ban.m_message = e.what();
		threadCallback(_banFail, &ban);
		threadLog(_FLW, "p:%u subj:%s failed: %s", ban.m_issuer, name.c_str(), e.what());
	}

	if ( p )
		threadsFreeText(p);
}

//////////////////////////////////////////////////////////////////////
// Thread for processing bans

void chatBanQueueThread()
{
	static DB::handle_t* conn = threadsOpenDBConnection(false);
	if ( conn == nullptr )
	{
		threadLog(_FLA, "bqt: Unable to open database connection");
		return;
	}

	threadLog(_FLI, "Ban Queue Thread active");

	threadIgnoreSIGPIPE();

	for ( ; ; )
	{
		auto ban = banQueue.getNext();
		if (ban == nullptr)
			break;
		_processBan(conn, *ban);
	}

	threadsCloseDBConnection(conn);

	return;
}
