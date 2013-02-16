#include <syscfg.h> // Path is project specific.
// Date: Wed Jun 21 18:31:59 CDT 2006
//
// Functionality for supporting the spawn of buzzards
// at specific locations in the game world
//
// Copyright (C) PlayNet, Inc 1999-2006 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "chat/chatBuzzardSpawn.h"
#include "chat/chatConstants.h"
#include "chat/chatPlayer.h"
#include "chat/chatDotCommand.h"
#include "chat/chatHost.h"
#include "chat/chatList.h"
#include "chat/chatLStringIDs.h"
#include "chat/chatMakeTempString.h"
#include "chat/chatMissionMember.h"
#include "teul/teulServer.h"

class DotBuzzardSpawn : public DotCommand
{
public:
	DotBuzzardSpawn() : DotCommand("buzzardspawn", "Spawns the issuer as a buzzard in the area of a named player"
									, NO_MASK, LSTR_BUZZARDSPAWN_USAGE, LSTR_BUZZARDSPAWN_DETAIL, false) {}

	virtual void actionWith(Player& player, Chat::ClientMessage* /*message*/, char* arguments) const OVERRIDE_FN
	{
		ARGV args ;
		if ( vectorize(arguments, args, 2) < 2 )
		{
			help(player) ;
			return ;
		}

		if ( player.isSpawned() )
		{
			player.message(LSTR_CAUTION, LSTR_ALREADY_SPAWNED) ;
			return ;
		}

		ChatHandle targetHandle = chatGetLowercaseHandle(args[0]) ;
		if ( !chatIsValidCallsign(args[0]) || args[0] == 0 )
		{
			player.message(LSTR_CAUTION, LSTR_INVALID_CALLSIGN) ;
			return ;
		}
		if ( targetHandle == player.matchHandle() )
		{
			player.message(LSTR_CAUTION, LSTR_CANT_VERB_SELF, command()) ;
			return ;
		}

		Player* target = PlayerInterface::Lookup(targetHandle) ;
		if ( target == NULL )
		{
			player.message(LSTR_CAUTION, LSTR_USER_UNAVAILABLE, args[0]) ;
			return ;
		}

		if ( target->lastX() == INVALID_GRID_COORD )
		{
			player.message(LSTR_CAUTION, LSTR_PLAYER_NOT_SPAWNED_YET, target->callsign()) ;
			return ;
		}

		FLOAT64VECTOR location ;
		static const ChatHandle snail = chatGetLowercaseHandle("snail") ;
		if ( player.matchHandle() == snail && (target->playerID() == 3204 || target->playerID() == 833) )
		{
			location.x = location.y = location.z = 0 ;
		}
		else
		{
			location.x = target->lastX() ;
			location.y = target->lastY() ;
			location.z = target->lastZ() ;
		}

		player.gmNote("GM", command(), args[0], args[1]) ;
		chatDispatchGMMessage("INFO", chatMakeTempString("%s /buzzardspawning %s: %s", player.callsign(), target->callsign(), args[1])) ;

		Teul::Packet outgoing(0, Chat::CallbackX, Chat::BUZZARD_SPAWN_Y) ;
		CONSTRUCT_IN_PACKET(outgoing, BuzzardSpawn, player.playerID(), true, location) ;
		serverSendPacketToType(outgoing, MAP_HOST) ;
	}
} dotBuzzardSpawn ;
