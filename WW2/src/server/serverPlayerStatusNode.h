#pragma once
// Created: Sat Oct 15 14:53:29 CDT 2011
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer:  Oliver "kfsone" Smith

#include "auth/authDispatch.h"
#include "server/serverPersonaXferData.h"
#include "server/serverServerSummaryEnum.h"
#include "teul/teulTimeMS.h"

namespace Server { enum class PlayerStatus; }

class PlayerStatusNode
{
public:
	PlayerStatusNode() {}
	~PlayerStatusNode() {}

	Auth::Dispatch& dispatch() { return data.dispatch ; }
	const Auth::Dispatch& dispatch() const { return data.dispatch ; }

	void takeAuthorityForThisPlayer() ;

public:
	playerid_t		id;			// playerid
	SINT32			teulIndex;
	UINT32			host;
	LOGICAL			inAuth;
	UINT32			logins;
	Server::PlayerStatus			status;
	Server::Transfer::XferReason	xferState;
	SERVER_SUMMARY	curServer;

	Server::PersonaXferData	data ;

	Teul::TimeMS	timeStamp;
	void*			localInfo;
	void*			sessionInfo;

	UINT32			serviceID ;
} ;

