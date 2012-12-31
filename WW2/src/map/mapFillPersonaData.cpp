#include <syscfg.h> // Path is project specific.
// Created: Wed Aug 26 13:42:46 CDT 2009
//
// Interface for querying persona data for despawning players.
// Queries are batched in windows.
//
// Copyright (C) PlayNet, Inc 1999-2009 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "versionNo.h"						// For version dependent code

#include "map/mapFillPersonaData.h"			// Our header file
#include "wwii/wwiiDBThreads.h"
#include "map/mapPlayer.h"
#include "include/mydbQuery.h"

//////////////////////////////////////////////////////////////////////
// Add a personaID to the query list

class FillPersonaDataQuery : public DBThreadQuery
{
public:
	FillPersonaDataQuery(const Player* player)
		: DBThreadQuery(DBThread::Default, "FillPersonaDataQuery", DBThreadQuery::_CALLBACK, 1
							, "CALL auth.p_persona_refresh(%u, %u)"
								, player->personaID()
								, Server::myServerInfo.stats.arenaID
								)
		, m_playerID(player->playerID())
		, m_teulIndex(player->teulIndex())
		, m_queuedAt(teulClock64)
		, m_result()
		{}

private:
	struct Result
	{
		RANK_NO			m_newRank ;
		UINT32			m_pointsEarned ;
		PermissionMask	m_newMask ;
		std::string		m_newGroups ;
		UINT32			m_metered ;
	} ;

protected:
	// We take the playerID and teulIndex so that we can
	// make sure the player is still where we expect
	// them to be when we get the results back.
	playerid_t	m_playerID ;
	SINT32		m_teulIndex ;
	TeulTimeMS	m_queuedAt ;
	Result		m_result ;

	// Lookup and check the details of a player.
	private: Player* GetPlayer() const
	{
		Player* player = PlayerInterface::Lookup(m_playerID) ;
		if ( player == NULL )
		{
			dbugLog(_FLI, "t:%d p:%u fpdq: player not connected", m_teulIndex, m_playerID) ;
			return NULL ;
		}
		if ( player->teulIndex() != m_teulIndex )
		{
			dbugLog(_FLW, "t:%d p:%u c:%s fpdq: teulIndex changed (%d), assuming relogged and discarding result"
							, m_teulIndex, m_playerID, player->callsign(), player->teulIndex()
							) ;
			return NULL ;
		}
		return player ;
	}

	protected: virtual bool Process(const size_t numRows)
	{
		if ( numRows <= 0 || Query()->FetchRow() == 0 )
			return false ;

		const DBRow& row = Query()->Row() ;

		m_result.m_newRank = static_cast<RANK_NO>(row.NextUInt()) ;
		m_result.m_pointsEarned = row.NextUInt() ;
		m_newMask = row.NextUInt64() ;
		m_newGroups = row.NextString() ;
		if ( m_newGroups.empty() )		// No groups, no subscription.
			newMask &= ~SUBSCRIPTION_ACTIVE ;
		else							// Subscription is active.
			newMask |= SUBSCRIPTION_ACTIVE ;
		m_metered = row.NextUInt() ;

		return true ;
	}

	protected: virtual void Success()
	{
		Player* player = GetPlayer() ;
		if ( player == NULL )
			return ;

		player->SetRank(m_result.m_newRank) ;
		player->m_rankPointsEarned = m_result.m_pointsEarned ;
		player->SetMeteredAccess(m_result.m_metered) ;

		// Apply the mask now, just before setting groups
		player->m_playerMask = m_result.m_newMask ;

		// Apply the group list
		player->SetGroups(m_result.m_newGroups.c_str()) ;

		player->log(_FLI, "fpdq: pers:%u rank:%u gotrank:%d points:%u rankperm:%llx mb:%u took %llums"
						, player->personaID()
						, player->effectiveRank(), player->realRank(), player->points()
						, player->useRank().permissions()
						, player->minuteBalance()
						, teulClock64 - m_queuedAt
						) ;

		player->completeMissionResults() ;
	}

	protected: virtual void Failure()
	{
		Player* player = GetPlayer() ;
		if ( player == NULL )
			return ;

		///TODO: Better behavior.
		// Ok, this is bad for you, so we're going to disco you.
		player->log(_FLA, "fpdq: Failed reading persona data - disconnecting") ;
		player->Release(player->teulIndex(), RELEASE_EXIT) ;
	}
} ;

//////////////////////////////////////////////////////////////////////
// Put together a query

void
mapFillPersonaData(const Player* player)
{
	player->log(_FLN, "fpdq: queuing persona data request") ;
	(new FillPersonaDataQuery(player))->Queue() ;
}

