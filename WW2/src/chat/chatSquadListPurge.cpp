#include <syscfg.h> // Path is project specific.
// Date: Mon May  7 11:33:17 CDT 2007
//
// Look for and purge recruits who have not been promoted
// to member within the squad's "rookie_days" variable.
//
// Copyright (C) PlayNet, Inc 1999-2007 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "dbug/dbug.h"							// For host "dbugLog" access

#include "chat/chatHandle.h"
#include "chat/chatLStringIDs.h"
#include "chat/chatPlayer.h"
#include "chat/chatSquadList.h"
#include "chat/chatSquadListPurge.h"				// Our header file
#include "chat/chatSquadListTypes.h"
#include "include/mydbQuery.h"
#include "teul/teulBlipRepeater.h"
#include "wwii/wwiiConfig.h"
#include "wwii/wwiiDBThreads.h"
#include "wwii/wwiiThreads.h"
#include "wwii/wwiiThreadText.h"

// Find the expired recruits
static const char* QRY_EXPIRED_RECRUITS =
	"SELECT p.squadid, p.playerid, p.callsign, s.rookie_days"
		" FROM wwii_player p INNER JOIN wwii_squads s USING (squadid)"
		" WHERE p.squad_rank = 1"
			" AND p.squad_joined < DATE_SUB(NOW(), INTERVAL s.rookie_days DAY)"
		" ORDER BY 1,2"
		" LIMIT 16"
	;

// De-squad the expired recruits
static const char* QRY_REMOVE_RECRUITS =
	"UPDATE wwii_player"
	  " SET squadid=0"
			", squad_rank=0"
			", squad_joined=NOW()"
			", squad_time_played=0"
	" WHERE playerID in (%u"
;

class ExpiredRecruitQuery : public DBThreadQuery
{
public:
	ExpiredRecruitQuery()
		: DBThreadQuery(DBThread::Default, "ExpiredRecruitQuery", QRY_EXPIRED_RECRUITS, DBThreadQuery::_CALLBACK)
		, m_results()
		{}

private:
	struct Result
	{
		UINT32		m_squadID;
		UINT32		m_playerID;
		ChatHandle	m_handle;
		char		m_days[16];
	};
	std::vector<Result> m_results;

public:
	virtual bool Process(const size_t numRows) OVERRIDE_FN
	{
		if ( numRows <= 0 )
			return true;

		Result result;
		while ( Query()->FetchRow() )
		{
			DBRow& row = Query()->Row();

			result.m_squadID = row.NextUInt();
			result.m_playerID = row.NextUInt();
			result.m_handle = chatGetHandle(row.NextColumn());
			strncpy(result.m_days, row.NextColumn(), sizeof(result.m_days) - 1);
			result.m_days[15] = 0;
			m_results.push_back(result);
		}

		return true;
	}

public:
	virtual void Success() OVERRIDE_FN
	{
		if ( m_results.empty() == true )
			return;

		const size_t numRows = m_results.size();
		dbugLog(_FLD, "Got list of %" PRSizeT " expired recruits", numRows);

		// See who was listed.
		UINT32 prevSquadID = 0;
		SquadList* squadList = nullptr;
		SquadMemberEntry* entry = nullptr;

		char* text = nullptr;
		char* tp = nullptr;

		for ( size_t i = 0; i < numRows; ++i )
		{
			const Result& result = m_results[i];

			// Read the values
			// Avoid looking up every squad every cycle
			if ( result.m_squadID != prevSquadID )
			{
				squadList = chatFindSquadList(result.m_squadID);
				prevSquadID = result.m_squadID;
			}

			// If we have the squad loaded, player might be online, handle nicely.
			if ( squadList != nullptr )
			{
				entry = squadList->find(result.m_handle);
				if ( entry )
				{
					if ( entry->player() )
						entry->player()->message(LSTR_SQUAD, LSTR_SQUAD_REMOVED_LAPSED, result.m_days, squadList->squad().shortName());
					squadList->memberLeave(true, entry, LSTR_SQUAD_RECRUIT_LAPSED, result.m_days);
					///TODO:127: Should we continue?
					continue;
				}
			}

			// Nobody online, so we have to do this in the background
			if ( text == nullptr )
			{
				text = threadsAllocText();
				tp = text + snprintf(text, TEXT_POOL_SIZE, QRY_REMOVE_RECRUITS, result.m_playerID);
			}
			else
				tp += sprintf(tp, ",%u", result.m_playerID);

			// Check for reaching the end of the buffer
			if ( tp >= text + TEXT_POOL_SIZE - 20 )
			{
				strcpy(tp, ")");
				DBThread::Do(DBThread::Default, text);
				text = nullptr;
			}
		}

		if ( text != nullptr )
		{
			strcpy(tp, ")");
			DBThread::Do(DBThread::Default, text);
		}
	}
};

void
chatPurgeExpiredSquadRecruits()
{
	// Check that the configuration enables this feature
	if ( !host_config.squad.recruitExpiry )
		return;

	// Check if its time for the next run
	static Teul::BlipRepeater nextRun(5 * 60 * 1000);
	if ( !nextRun.Ready() )
		return;

	// Queue up the query in the background
	(new ExpiredRecruitQuery())->Queue();
}
