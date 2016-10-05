#include <syscfg.h> // Path is project specific.
// Created: Wed Mar  4 08:56:28 CST 2009
//
// Pay-to-play charging system
//
// Copyright (C) PlayNet, Inc 1999-2009 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "versionNo.h"					// For version dependent code

#include "chat/chatLStringIDs.h"
#include "host/hostPlayers.h"			// for g_players
#include "map/mapPlayer.h"
#include "wwii/wwiiThreadTextQuery.h"	// For ThreadTextQuery

//////////////////////////////////////////////////////////////////////
// Go through the player list and charge minute balances for all
// players who have passed persona selection.

void
mapChargeMinutes()
{
	static TeulTimeMS nextRun = 0ULL;
	if ( teulClock64 < nextRun )
		return;

	// If there are no players, there is no work to do.
	if ( g_players.empty() )
	{
		// Don't do anything again for a while.
		nextRun = teulClock64 + 5000;
		return;
	}

	// Look at every player and see if they are ready to charge.
	size_t charged = 0;
	TeulTimeMS startTime = teulClock64;

	// Get our prepared statement.
	try
	{
		ThreadTextQuery balanceQry(DBThread::Auth
									, "UPDATE `auth`.`auth_minute_balance"
											" SET balance=balance-1,modified=NOW()"
											" WHERE customerid IN ("
										, ") AND balance>0");

		char playerIDstr[64];

		for ( auto it = g_players.begin(), end = g_players.end(); it != end; ++it )
		{
			Player* const player = (Player*)(*it);
			if ( player == nullptr || player->charge() == false )
				continue;

			++charged;
			strprintf(playerIDstr, "%u", player->playerID());
			balanceQry.addEntry(playerIDstr);
		}

		balanceQry.commit(0);
	}
	catch ( std::exception& e )
	{
		dbugLog(_FLA, "mmb: minute balance processing failed: %s", e.what());
	}

	teulSyncClock();
	TeulTimeMS endTime = teulClock64;
	TeulTimeMS takenTime = endTime - startTime;
	if ( takenTime > 10 )
		dbugLog(_FLM, "mmb: %s took %llu milliseconds to charge %" PRSizeT " of %" PRSizeT " customers", __FUNCTION__, takenTime, charged, g_players.size());

	// Schedule the next run for a second from now
	nextRun = teulClock64 + 1000;
}

//////////////////////////////////////////////////////////////////////
// If the player is on metered access, determine if their charging
// cycle is up.

bool
Player::charge()
{
	if ( meteredAccess() == false )
		return false;

	// Only metered access players get charged
	// and your next charging time has to be up
	if ( (m_nextCharge == 0) | (m_nextCharge >= teulClockTime) | (m_sessionSide == WWII_NO_SIDE) )
		return false;
	if ( inState(PERS_EXITING) | (isSpawned() == false) )
		return false;

#if !defined(DEV_BUILD)
	dbugLog(_FLD, "t:%d p:%u c:%s metered:%c cust:%u tct:%u nextCharge:%u mb:%u"
					, teulIndex(), playerID(), callsign()
					, (meteredAccess() ? 'y' : 'n')
					, customerID()
					, teulClockTime, m_nextCharge
					, m_minuteBalance
					);
#endif

	// Deduct a minute from minute balance
	if ( m_minuteBalance > 0 )
	{
		--m_minuteBalance;
	}

	// Schedule next charge for one minute.
	m_nextCharge = ((m_nextCharge > 0 ) ? m_nextCharge : teulClockTime) + MINUTE_S;

	return true;
}

