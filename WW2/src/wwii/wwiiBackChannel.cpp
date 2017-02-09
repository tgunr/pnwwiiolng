#include <syscfg.h> // Path is project specific.
// Created: Fri Sep 29 18:09:45 CST 2006
//
// Management of the 'backchannel' connection via raknet
//
// Copyright (C) PlayNet, Inc 1999-2017 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "wwii/wwiiBackChannel.h"
#include "cfg/cfgPrefs.h"

#include "teul/teulEndpoint.h"
#include "teul/raknetConn.h"
#include "teul/teulBasic.h"

#include "dbug/dbug.h"

//////////////////////////////////////////////////////////////////////
// Connect a back channel -- one per player client

bool wwiiBackChannelConnect(const char* serverIP, UINT16 port, UINT32 teulIndex, UINT32 playerId, UINT32 otp, char * packedPublicKey)
{
	wwiiBackChannelDisconnect(teulIndex) ;

	if ( cfgPrefsGetBool("app.usenetcode", false) == true )
	{
		return false ;
	}

	ENDPOINT *ep = teulGetEndpoint(teulIndex);
	if (ep == nullptr)
	{
		dbugLog(_FLW, "Endpoint not found for backchannel connect for teul index %d", teulIndex);
		return false;
	}
	
	ep->m_pairdUdpConn = new RakNetChannel(false, port, ep);
	ep->m_pairdUdpConn->init();
	return ep->m_pairdUdpConn->setPublicKey(packedPublicKey, teulIndex);
}

//////////////////////////////////////////////////////////////////////
// Disconnect a player client

void wwiiBackChannelDisconnect(UINT32 teulIndex)
{

	ENDPOINT *ep = teulGetEndpoint(teulIndex);
	if (ep == nullptr)
	{
		dbugLog(_FLW, "Endpoint not found for backchannel disconnect for teul index %d", teulIndex);
		return;
	}

	if (ep->m_pairdUdpConn != nullptr)
	{
		ep->m_pairdUdpConn->release();
		ep->m_pairdUdpConn = nullptr;
	}
}

