#pragma once
// Created: Tue Jul 25 16:51:50 2000
//
// Types for the chat system
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson

//Ok here's the deal: clients only send a message to their radio channels.
//	The host knows which channels the client is tuned to, so it sends the
//	message out on those channels.  Before it sends the message out to the
//	end user, it changes the radioChannels bitmask to match that of the user
//	that its sending the message to.
//
//	I'm sure that makes no sense, so read the design doc

// Forward declarations

struct BANDWIDTH_CHANGE
{
	playerid_t	playerID ;
	UINT32		mode ;
} ;

struct BANDWIDTH_NOTICE
{
	playerid_t	playerID ;
	UINT32		mode ;
	char		response[80+1] ;
	bool		display ;
} ;

#ifndef FRONT_END
#include "chat/chatServerTypes.h"
#endif
