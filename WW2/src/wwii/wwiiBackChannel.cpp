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
#include "ui/uiChat.h"
#include "pi/piogl.h"
#include "ogl/oglcolors.h"
#include "ui/uiMain.h"

#include "dbug/dbug.h"

extern bool libCatSecurityEnabled;

//////////////////////////////////////////////////////////////////////
// Connect a back channel -- one per player client

bool wwiiBackChannelConnect(const char* serverIP, UINT16 port, SINT32 teulIndex, UINT32 playerId, UINT32 otp, char * packedPublicKey)
{
	wwiiBackChannelDisconnect(teulIndex) ;

	if ( !g_wantNetcode )
	{
		if (playerId != 0)
			uiChatPrintLineColor(colorGREENALPHA, uiLocString("connect.netcode3.enabled.server").c_str());

		return false ;
	}

	// make sure there is a Raknet component to the activate information
	if (playerId != 0 &&
		(!libCatSecurityEnabled || reinterpret_cast<int *>(packedPublicKey)))
	{
		ENDPOINT *ep = teulGetEndpoint(teulIndex);
		if (ep == nullptr)
		{
			dbugLog(_FLW, "Endpoint not found for backchannel connect for teul index %d", teulIndex);
			return false;
		}
		
		if (ep->m_pairdUdpConn == nullptr)
			ep->m_pairdUdpConn = new RakNetChannel(false, port, ep);
		ep->m_pairdUdpConn->init();
		int resp = ep->m_pairdUdpConn->setPublicKey(packedPublicKey, teulIndex);
		switch (resp)
		{
		case RN_SK_CONNECTED:
			uiChatPrintLineColor(colorGREENALPHA, uiLocString("connect.netcode3.enabled").c_str());
			break;
		case RN_SK_FAILED:
			uiChatPrintLineColor(colorYELLOWALPHA, uiLocString("connect.netcode3.failed").c_str());
			break;
		}
		return (resp == RN_SK_CONNECTED);
	}
	else
	{
		uiChatPrintLineColor(colorYELLOWALPHA, uiLocString("connect.netcode3.disabled.server").c_str());
	}
	
	// No, Raknet must be disabled by the server
	return false;
}

//////////////////////////////////////////////////////////////////////
// Disconnect a player client

void wwiiBackChannelDisconnect(SINT32 teulIndex)
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
	}
}

