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
#include "teul/teulTimeMS.h"

namespace Server { enum class PlayerStatus; }

class PlayerStatusNode
{
public:
	PlayerStatusNode() {}
	~PlayerStatusNode() {}

	Auth::Dispatch& dispatch() { return data.dispatch; }
	const Auth::Dispatch& dispatch() const { return data.dispatch; }

	void takeAuthorityForThisPlayer();

public:
	struct ServerSummary
	{
		UINT64			id;		// mac address of eth0 + pid
		UINT32			address;
		SERVER_TYPE		type;
	};

	playerid_t		id;			// playerid
	SINT32			teulIndex;
	UINT32			host;
	bool			inAuth;
	UINT32			logins;
	Server::PlayerStatus			status;
	Server::Transfer::XferReason	xferState;
	ServerSummary	m_curServer;

	Server::PersonaXferData	data;

	Teul::TimeMS	timeStamp;
	void*			localInfo;

	UINT32			serviceID;
};

