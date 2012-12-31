#include <syscfg.h> // Path is project specific.
// Date: Fri Nov 11 13:17:18 CST 2005
//
// Beta commands
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include <stdlib.h>
#include <stdexcept>

#include "versionNo.h"

#ifdef BETA_RELEASE
#include "dbug/dbug.h"

#include "chat/chatPlayer.h"
#include "chat/chatDotCommand.h"
#include "chat/chatList.h"
#include "chat/chatLStringIDs.h"
#include "chat/chatHost.h"
#include "teul/teulTime.h"
#include "wwii/wwiiDBThreads.h"

class DotNukePersonas : public DotCommand
{
public:
	DotNukePersonas() : DotCommand("nukepersonas", "Ask the host to delete all your personas"
									, NO_MASK, LSTR_NUKEPERSONAS_USAGE, LSTR_NUKEPERSONAS_DETAIL, true) {}

	virtual void action(Player& player, Chat::ClientMessage* /*message*/) const OVERRIDE_FN
	{
		DBThread::Do(DBThread::Default, "DELETE FROM wwii_persona WHERE playerid = %u", player.playerID()) ;
		player.message(LSTR_SYSTEM, LSTR_PERSONAS_NUKED) ;
	}
} dotNukePersonas ;

//////////////////////////////////////////////////////////////////////

class DotResetStrat : public DotCommand
{
public:
	DotResetStrat() : DotCommand("resetstrat", "Forces a reset of the strat server"
									, NO_MASK, LSTR_RESETSTRAT_USAGE, LSTR_RESETSTRAT_DETAIL, true) {}

	virtual void action(Player& player, Chat::ClientMessage* /*message*/) const OVERRIDE_FN
	{
		static UINT32 lastRequest = 0 ;
		static UINT32 lastRequester = 0 ;
		static UINT32 lastIssued = 0 ;

		static const UINT32 REQUEST_INTERVAL = 30 ;

		if ( teulTime() - lastIssued < 300 )
		{
			player.message(LSTR_CAUTION, LSTR_RESETSTRAT_5MINUTE_LIMIT) ;
			return ;
		}

		if ( !player.isPlaynet() && playerIndex.size() > 1 )
		{
			if ( teulTime() - lastRequest > REQUEST_INTERVAL )
			{
				lastRequest = teulTime() ;
				lastRequester = player.playerID() ;

				player.message(LSTR_NONE, LSTR_RESETSTRAT_ACK, command()) ;
				chatSystemMessage(LSTR_RESETSTRAT_SYS, player.callsign()) ;

				return ;
			}
			else if ( player.playerID() == lastRequester )
			{
				player.message(LSTR_CAUTION, LSTR_RESETSTRAT_SELF_APPROVAL) ;
				return ;
			}

			chatSystemMessage(LSTR_RESETSTRAT_APPROVED, player.callsign()) ;
		}

		player.sendStratCommand("restart", "", false) ;
		lastIssued = teulTime() ;
	}
} dotResetStrat ;

#endif //BETA_RELEASE

