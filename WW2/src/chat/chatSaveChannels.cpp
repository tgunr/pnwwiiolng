#include <syscfg.h> // Path is project specific.
// Created: Mon Feb 13 11:22:53 CST 2006
//
// Save player's channels and re-load them
//
// Copyright (C) PlayNet, Inc 1999-2006 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "chat/chatDebugConditions.h"

#include "wwii/wwiiDBThreads.h"
#include "wwii/wwiiThreadText.h"
#include "chat/chatPlayer.h"
#include "include/mydbQuery.h"
#include "chat/chatDefaultChannels.h"
#include "mem/mem.h"

struct Tunes
{
	UINT32		types[MAX_CLIENT_CHANNELS];
	UINT64		subs[MAX_CLIENT_CHANNELS];
};

class PlayerChannels : public DBThreadQuery
{
public:
	PlayerChannels(UINT32 playerID)
		: DBThreadQuery(DBThread::Default, "PlayerChannels", DBThreadQuery::_CALLBACK, "SELECT slotid, type, subid FROM chat_tunes WHERE playerid = %u", playerID)
		, m_results()
		, m_playerID(playerID)
		{}

private:
	struct Result
	{
		UINT32 m_slot, m_type;
		UINT64 m_sub;
	};
	std::vector<Result> m_results;

public:
	virtual bool Process(const size_t numRows) override
	{
		if ( numRows <= 0 )
			return false;

		Result result;
		while ( Query()->FetchRow() )
		{
			result.m_slot = Query()->Row().NextUInt();
			result.m_type = Query()->Row().NextUInt();
			result.m_sub = Query()->Row().NextUINT64();
			m_results.push_back(result);
		}

		return true;
	}

public:
	virtual void Success() override;
	virtual void Failure() override;

private:
	UINT32 m_playerID;
};

//////////////////////////////////////////////////////////////////////

void
PlayerChannels::Failure()
{
	Player* player = PlayerInterface::Lookup(m_playerID);
	if ( player == nullptr )
		return;

	player->tuneInitialChannels();
}

//////////////////////////////////////////////////////////////////////

void
PlayerChannels::Success()
{
	// First check if the player is still around
	Player* player = PlayerInterface::Lookup(m_playerID);
	if ( player == nullptr )
	{
		dbugLog(_FLI, "p:%u player has gone away, discarding channel tunes", m_playerID);
		return;
	}

	if ( player->haveChannels() || player->m_tunes )
	{
		dbugLog(_FLD, "t:%d p:%u c:%s already has channels, so ignoring database fetch", player->teulIndex(), player->playerID(), player->callsign());
		return;
	}

	if ( m_results.empty() == true )
	{
		dbugLog(_FLD, "t:%d p:%u c:%s player has no tunes, using defaults", player->teulIndex(), player->playerID(), player->callsign());
		return;
	}

	player->m_tunes = (Tunes*)memCalloc(1, sizeof(Tunes));

	// If there are no rows, then we need to use defaults
	const size_t numRows = m_results.size();
	for ( size_t i = 0; i < numRows; ++i )
	{
		const UINT32& slot = m_results[i].m_slot;
		player->m_tunes->types[slot] = m_results[i].m_type;
		player->m_tunes->subs[slot] = m_results[i].m_sub;
		if ( g_debugRadios )
			dbugLog(_FLD, "t:%d p:%u c:%s saved tune %u to %u/%llu"
							, player->teulIndex(), player->playerID(), player->callsign()
							, slot, player->m_tunes->types[slot], player->m_tunes->subs[slot]
							);
	}
}

//////////////////////////////////////////////////////////////////////
// Fetch the saved channels for a particular player

void
chatRestoreChannels(const Player* player)
{
	if ( player->haveChannels() || player->m_tunes != nullptr )
		return;

	// Queue the request
	(new PlayerChannels(player->playerID()))->Queue();
}

//////////////////////////////////////////////////////////////////////

void
chatSaveChannels(Player& player)
{
	if ( !player.haveChannels() )
		return;

	char save[TEXT_POOL_SIZE + 1];
	char* sp = save + sprintf(save, "REPLACE chat_tunes (playerid, slotid, type, subid) VALUES");
	int saved = 0;

	for ( UINT32 i = 0; i < MAX_CLIENT_CHANNELS; ++i )
	{
		const Chat::ChannelSubscription& cnode = player.getChannelSub(i);
		UINT32 type = 0;
		UINT64 subID = 0;

		switch ( cnode.type() )
		{
		// Channels we don't save
		case NO_CHANNEL:
		case MAX_CHANNEL_TYPES:
		case PRIVATE_CHANNEL:
		case ADMIN_CHANNEL:
		break;

		// Channels where we just save type
		case BRIGADE_CHANNEL:
		case MISSION_CHANNEL:
		case SQUAD_CHANNEL:
		case GCI_CHANNEL:
		case CP_CHANNEL:
		case ORIGIN_CP_CHANNEL:
		case OBJECTIVE_CP_CHANNEL:
		case SIDE_GENERAL_CHANNEL:
		case SIDE_HELP_CHANNEL:
		case SIDE_OPS_CHANNEL:
			type = cnode.type();
		break;

		// Channels where we save the id
		case NUMBERED_CHANNEL:
			type = cnode.type();
			if ( cnode.channel() )
				subID = cnode.channel()->channelID.SubID();
		break;
		}

		// Add separator or space
		*(sp++) = (saved++) ? ',' : ' ';

		sp += sprintf(sp, "(%u,%u,%u,%llu)", player.playerID(), i, type, subID);
	}
	if ( saved == 0 )
	{
		return;
	}

	// We don't care if this works or not; so we can just
	// queue up a raw threadstatement.
	(new DBThreadQuery(DBThread::Default, "SaveChannels", save, DBThreadQuery::_IMMEDIATE))->Queue();
}

//////////////////////////////////////////////////////////////////////
// Use the default or previously saved channels for this player

void
Player::tuneInitialChannels(bool forceReset)
{
	if ( forceReset )
	{
		m_haveChannels = false;
		if ( m_tunes )
		{
			memFree(m_tunes);
			m_tunes = nullptr;
		}
	}

	if ( !isValidSide(side()) || haveChannels() )
		return;

	Tunes* tunes = m_tunes;

	if ( m_tunes == nullptr )
	{
		static bool _haveInitialized = false;
		static Tunes defaultTunes;

		tunes = &defaultTunes;

		if ( !_haveInitialized )
		{
			// Set all radios to their defaults.
			for ( UINT32 i = 0; i < MAX_CLIENT_CHANNELS; ++i )
			{
				tunes->types[i] = Chat::defaultChannels[i];
			}
			_haveInitialized = true;
		}
	}

	// Tune the radios as specified
	for ( UINT32 i = 0; i < MAX_CLIENT_CHANNELS; ++i )
	{
		detuneRadio(i);
		CHANNEL_TYPE type = (CHANNEL_TYPE)tunes->types[i];
		if ( type >= MAX_CHANNEL_TYPES )
			continue;

		switch ( type )
		{
		case NO_CHANNEL:
			continue;

		case NUMBERED_CHANNEL:
			tuneRadioToNumberedChannel(i, (UINT32)tunes->subs[i]);
		break;

		default:
			tuneRadio(i, type, 0);
		}
	}

	// If the player has a Tunes structure callocd, free it
	if ( m_tunes )
	{
		memFree(m_tunes);
		m_tunes = nullptr;
	}

	m_haveChannels = true;
}

