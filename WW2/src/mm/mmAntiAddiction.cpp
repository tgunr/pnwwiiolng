#include <syscfg.h> // Path is project specific.
// Date: Tue May 27 01:06:16 CDT 2008
//
// HTSC:PRC Anti-Addiction System Implementations
//
// Copyright (C) PlayNet, Inc 1999-2008 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "chat/chatLStringIDs.h"
#include "chat/chatSupportRPCMessages.h"	// For chatSendDirectMessage
#include "host/hostPlayers.h"
#include "map/mapPlayer.h"
#include "wwii/wwiiAntiAddiction.h"			// Chinese anti-addiction code
#include "wwii/wwiiConfig.h"
#include "wwii/wwiiThreads.h"				// For threadDBSTatement

#undef _IN_DEVELOPMENT

#ifdef _IN_DEVELOPMENT
#define DEV_LOG(args...) dbugLog(args)
#else
#define DEV_LOG(...)
#endif

struct AAStatus
{
	static const UINT32 _SAVE_INTERVAL = 120;	// Save once every 2 minutes
	AAStatus(UINT32 priorPlayedTime)
		: m_priorPlayedTime(priorPlayedTime)
		, m_state(0)
		, m_lastNotice(0)
		, m_nextSaveTime(teulTime() + _SAVE_INTERVAL)
		, m_playedHours(0)
		{}
	UINT32		m_priorPlayedTime;
	UINT32		m_state;
	UINT32		m_lastNotice;
	UINT32		m_nextSaveTime;
	UINT16		m_playedHours;
};

static PLAYERS g_minors;

//////////////////////////////////////////////////////////////////////
// Save the status of a player

static void _saveMinorUsage(Player* player)
{
	// Save their accrued time
	const UINT32 played = player->m_antiAddiction->m_priorPlayedTime;
	if ( played > 0 || (player->timeIn > 0 && teulTime() > player->timeIn) )
	{
		UINT32 accumulated = (teulTime() - player->timeIn) + played;
		DBThread::Do(DBThread::Default
			, "REPLACE INTO wwii.minor_usage (`playerid`,`seconds`,`expires`)"
				"VALUES(%u,%u,DATE_ADD(NOW(), INTERVAL %u MINUTE))"
				, player->playerID()
				, accumulated
				, AntiAddiction::SLEEP_HOURS * host_config.arena.minutesPerAAHour
		);

		player->m_antiAddiction->m_nextSaveTime = teulTime() + AAStatus::_SAVE_INTERVAL;

		dbugLog(_FLD, "t:%d p:%u c:%s saving accumulated played time %u seconds"
						, player->teulIndex(), player->playerID(), player->callsign()
						, accumulated);
	}
}

//////////////////////////////////////////////////////////////////////
// Add a minor to the list

void
mmAddMinor(Player* player, UINT32 priorPlayedTime)
{
	assert( player != nullptr );
	assert( player->m_antiAddiction == nullptr );

	player->m_antiAddiction = new AAStatus(priorPlayedTime);
	assert( player->m_antiAddiction != nullptr );

	player->m_implementAntiAddiction = false;

	g_minors.insert(player);

	dbugLog(_FLD, "t:%d p:%u c:%s added to minor's list now %" PRSizeT ""
					, player->teulIndex(), player->playerID(), player->callsign(), g_minors.size()
			);
}

//////////////////////////////////////////////////////////////////////
// Remove a minor from the list

void
mmRemoveMinor(Player* player)
{
	assert( player != nullptr );

	assert( player->m_antiAddiction != nullptr );

	_saveMinorUsage(player);

	delete player->m_antiAddiction;
	player->m_antiAddiction = nullptr;

	g_minors.erase(player);

	dbugLog(_FLD, "t:%d p:%u c:%s removed from minor's list now %" PRSizeT ""
					, player->teulIndex(), player->playerID(), player->callsign(), g_minors.size()
			);
}

//////////////////////////////////////////////////////////////////////
// Handle a player who is over the maximum time

static bool
_penaltyTime(Player* player, AAStatus* status, bool newHour)
{
	static const UINT32 _WARNING_INTERVAL = (UINT32)(60 * host_config.arena.minutesPerAAHour * AntiAddiction::TIRED_WARNING_INTERVAL);
	if ( status->m_state != AntiAddiction::_PENALTY_TIME || newHour )
	{
		DEV_LOG(_FLI, " first notice");
		// First notice
		chatSendLocalizedMessage(player->playerID(), LSTR_WARNING, LSTR_ANTI_ADDICTION_PENALTY);
		status->m_state = AntiAddiction::_PENALTY_TIME;
		status->m_lastNotice = teulTime();
		return true;
	}
	else if ( teulTime() > status->m_lastNotice + _WARNING_INTERVAL )
	{
		DEV_LOG(_FLI, " repeat notice");
		// Repeat notice
		chatSendLocalizedMessage(player->playerID(), LSTR_WARNING, LSTR_ANTI_ADDICTION_PENALTY);
		status->m_lastNotice = teulTime();
		return true;
	}
	else
	{
		DEV_LOG(_FLI, " already warned");
	}

	return false;
}

//////////////////////////////////////////////////////////////////////
// Handle a player who is in the tired time state

static bool
_tiredTime(Player* player, AAStatus* status, bool newHour)
{
	int messageID = LSTR_ANTI_ADDICTION_HOUR1 + (status->m_playedHours - 1);
	if ( status->m_state != AntiAddiction::_TIRED_TIME  || newHour )
	{
		DEV_LOG(_FLI, " notifying");
		// First notice
		chatSendLocalizedMessage(player->playerID(), LSTR_CAUTION, (LSTRING_ID)messageID);
		status->m_state = AntiAddiction::_TIRED_TIME;
		status->m_lastNotice = teulTime();
		return true;
	}
	else
	{
		DEV_LOG(_FLI, " no notice required");
	}

	return false;
}

//////////////////////////////////////////////////////////////////////
// Check up on all the minors

void
mmCheckAddictionTimers()
{
	DEV_LOG(_FLI, __FUNCTION__);

	// Clear out any old usages.
	static TeulTimeMS nextDelete = 0ULL;
	if ( teulClock64 > nextDelete )
	{
		DBThread::Do(DBThread::Default, "DELETE FROM wwii.minor_usage WHERE expires <= NOW()");
		nextDelete = teulClock64 + (60 * 1000);
	}

	if ( host_config.arena.minutesPerAAHour == 0 )
		return;

	PLAYERS::iterator it = g_minors.begin();
	while ( it != g_minors.end() )
	{
		Player* player = static_cast<Player*>(*it);
		AAStatus* status = player->m_antiAddiction;

		assert( status != nullptr );

		++it;

		DEV_LOG(_FLI, "t:%d p:%u c:%s minor check: ", player->teulIndex(), player->playerID(), player->callsign());

		if ( player->timeIn == 0 )
		{
			DEV_LOG(_FLI, "not logged in yet (%u <= %u)", player->timeIn, teulTime());
			continue;
		}

		UINT32 playedHours = (status->m_priorPlayedTime + teulTime() - player->timeIn) / (60 * host_config.arena.minutesPerAAHour);

		bool newHour = (playedHours > status->m_playedHours);
		status->m_playedHours = playedHours;

		bool stateChanged = false;
		if ( playedHours >= AntiAddiction::PENALTY_HOUR )
		{
			DEV_LOG(_FLI, "penalty time");
			stateChanged = _penaltyTime(player, status, newHour);
		}
		else if ( playedHours >= AntiAddiction::TIRED_HOUR )
		{
			DEV_LOG(_FLI, "tired time");
			stateChanged = _tiredTime(player, status, newHour);
		}
		else
		{
			if ( newHour )
			{
				int noticeID = LSTR_ANTI_ADDICTION_HOUR1 + (playedHours - 1);
				chatSendLocalizedMessage(player->playerID(), LSTR_NOTE, (LSTRING_ID)noticeID);
			}

			DEV_LOG(_FLI, "regular time");
		}

		status->m_playedHours = playedHours;

		if ( stateChanged || teulTime() >= status->m_nextSaveTime )
		{
			DEV_LOG(_FLI, " saving");
			_saveMinorUsage(player);
		}
	}
}

//////////////////////////////////////////////////////////////////////
// Returns true if the specified player is valid and in penalty time

FLOAT32
mmAntiAddictionScoringMultiplier(const Player* player)
{
	assert( nullptr != player );

	// Setting minutes-per-hour to zero disables anti-addiction systems.
	if ( host_config.arena.minutesPerAAHour == 0 )
		return 1.0;

	const AAStatus* const status = player->m_antiAddiction;
	if ( nullptr == status || status->m_state <= AntiAddiction::_STANDARD_TIME )
		return 1.0;
	if ( status->m_state == AntiAddiction::_TIRED_TIME )
		return 0.5;
	return 0;
}

