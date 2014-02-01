#include <syscfg.h> // Path is project specific.
// Date: Wed Aug  8 13:41:10 CDT 2007
//
// Message sending RPC functions for chat support
//
// Copyright (C) PlayNet, Inc 1999-2007 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "chat/chatBroadcast.h"					// for Chat::Broadcast
#include "chat/chatConstants.h"
#include "chat/chatDirectMessage.h"				// For Chat::DirectMessage
#include "chat/chatSupportRPCMessages.h"		// Our include file
#include "host/hostCommand.h"
#include "server/serverIndexes.h"
#include "teul/teulPacket.h"
#include "teul/teulTickSample.h"

//////////////////////////////////////////////////////////////////////
// Sends a targeted SYSTEM message

bool
Chat::Broadcast::send()
{
	Teul::Packet outgoing(0, Teul::PacketId(Chat::CallbackX, Chat::BROADCAST_MESSAGE_Y));
	outgoing.append(*this);
	return serverSendPacketToType(outgoing, CHAT_HOST);
}

bool
chatSendBroadcast(Chat::BCAST_TYPE type, const instanceid_t& instanceID, UINT64 targetID, LSTRING_ID prefix, LSTRING_ID message
						, const char* ptr0, const char* ptr1, const char* ptr2, const char* ptr3, const char* ptr4, const char* ptr5)
{
	Teul::Packet outgoing(sizeof(Chat::Broadcast), Teul::PacketId(Chat::CallbackX, Chat::BROADCAST_MESSAGE_Y));
	CONSTRUCT_IN_PACKET(outgoing, Chat::Broadcast, Chat::_EVERYONE, instanceID, targetID, prefix, message, ptr0, ptr1, ptr2, ptr3, ptr4, ptr5);
	return serverSendPacketToType(outgoing, CHAT_HOST);
}

bool
chatSendBroadcastAll(LSTRING_ID prefix, LSTRING_ID messageID
					, const char* const ptr0, const char* const ptr1, const char* const ptr2, const char* const ptr3, const char* const ptr4, const char* const ptr5)
{
	Chat::Broadcast broadcast(Chat::_EVERYONE, 0, 0, prefix, messageID, ptr0, ptr1, ptr2, ptr3, ptr4, ptr5);
	return broadcast.send();
}

//////////////////////////////////////////////////////////////////////
// Send a message that is broadcast to all players serverwide as SYSTEM:

bool
chatSendAdminMessage(const char* fmt, ...)
{
	if ( fmt == nullptr )
		return false;

	va_list args;
	char message[MAX_MESSAGE_LENGTH+2];
	va_start(args, fmt);
	vsnprintf(message, MAX_MESSAGE_LENGTH, fmt, args);
	va_end(args);

	Chat::Broadcast outgoing(Chat::_ADMINISTRATIVE, 0, 0, LSTR_NONE, LSTR_GM_MSG, "HOST", message);
	return outgoing.send();
}

//////////////////////////////////////////////////////////////////////
// Send a message directly to a single player

bool
chatSendLocalizedMessage(const playerid_t playerID, LSTRING_ID prefix, LSTRING_ID message
					, const char* const ptr0, const char* const ptr1, const char* const ptr2, const char* const ptr3, const char* const ptr4, const char* const ptr5)
{
	// PlayerIDs of 0 or 1 are invalid ...
	if ( PN_UNLIKELY(playerID <= 1) )
		return FALSE;

	Teul::Packet outgoing(sizeof(Chat::DirectMessage), Teul::PacketId(Chat::CallbackX, Chat::LOCALIZED_MESSAGE_Y));
	CONSTRUCT_IN_PACKET(outgoing, Chat::DirectMessage, playerID, prefix, message, ptr0, ptr1, ptr2, ptr3, ptr4, ptr5);
	return serverSendPacketToType(outgoing, CHAT_HOST);
}

//////////////////////////////////////////////////////////////////////
// Formats a text string into an LString to send to a player

bool
chatSendDirectMessage(const playerid_t playerID, LSTRING_ID prefix, LSTRING_ID message, const char* fmt, ...)
{
	// PlayerIDs of 0 or 1 are invalid ...
	if ( playerID <= 1 )
	{
		dbugLog(_FLA, "%s called with playerID of %u!", __FUNCTION__, playerID);
		return false;
	}

	if ( fmt == nullptr )
		return false;

	va_list args;
	char text[MAX_MESSAGE_LENGTH+1];
	va_start(args, fmt);
	vsnprintf(text, MAX_MESSAGE_LENGTH, fmt, args);
	va_end(args);

	return chatSendLocalizedMessage(playerID, prefix, message, text);
}

void
chatSendTickSample(const UINT32 source, const playerid_t playerID, const void* data)
{
	const Teul::TickSample* srcSample = (const Teul::TickSample*)data;
	Teul::Packet outgoing(0, Teul::PacketId(Chat::CallbackX, Chat::TICK_SAMPLE_Y));
	auto* sample = CONSTRUCT_IN_PACKET(outgoing, Teul::TickSample, srcSample->host_ms, srcSample->client_ms);
	sample->source = source;
	sample->playerID = playerID;
	serverSendPacketToType(outgoing, CHAT_HOST);
}

