#include <syscfg.h> // Path is project specific.
// Created: Wed Jan 13 12:57:57 CST 2010
//
// Host functions for Chat::LocalizedMessages
//
// Copyright (C) PlayNet, Inc 1999-2010 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith
//

// For sending messages to players...
#include "chat/chatConstants.h"
#include "chat/chatLocalizedMessage.h"
#include "wwii/wwiiCallbacks.h"
#include "wwii/wwiiPlayer.h"
#include "RakNetTypes.h"
#include "teul/teulBasic.h"

//	------------------------------	Declarations

//////////////////////////////////////////////////////////////////////
// Send a message to a player thru teul

static SINT32
_sendLocalisedMessage(ACTION_OBJECT* const a)
{
	Chat::LocalizedMessage* message = (Chat::LocalizedMessage*)(a->localRawData);
	return message->packer(TEUL_PACK, a);
}

//////////////////////////////////////////////////////////////////////
// Register the chat callbacks.

SINT32
wwiiRegisterPlayerMessageCallback()
{
	BEGIN_CALLBACK_REGISTRATION(Chat::CallbackX)
	{
		REGISTER_SENDER(Chat::LOCALIZED_MESSAGE_Y, _sendLocalisedMessage, Chat::_SEND_LOCALIZED_MESSAGE);
	}
	END_CALLBACK_REGISTRATION(Chat::CallbackX)
}

//////////////////////////////////////////////////////////////////////
// Send a localized message to a particular wwiiPlayer

bool
wwiiPlayer::message(const Chat::LocalizedMessage& lmsg) const
{
	return (teulSend(teulIndex(), Chat::_SEND_LOCALIZED_MESSAGE, const_cast<Chat::LocalizedMessage*>(&lmsg)) >= 0);
}

bool
wwiiPlayer::message(int prefix, int messageID, const char* const p0, const char* const p1, const char* const p2, const char* const p3, const char* const p4, const char* const p5) const
{
	Chat::LocalizedMessage lmsg(prefix, messageID, p0, p1, p2, p3, p4, p5);
	return (teulSend(teulIndex(), Chat::_SEND_LOCALIZED_MESSAGE, &lmsg) >= 0);
}

bool
wwiiPlayer::messagef(int prefix, int messageID, const char* fmt, ...)
{
	if ( fmt == nullptr )
		return false;

	va_list args;
	char text[MAX_MESSAGE_LENGTH+1];
	va_start(args, fmt);
	vsnprintf(text, MAX_MESSAGE_LENGTH, fmt, args);
	va_end(args);

	return message(prefix, messageID, text, "");
}

