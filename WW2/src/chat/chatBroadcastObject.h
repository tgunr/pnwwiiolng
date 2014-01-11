#pragma once
// Created: Fri Sep 23 20:51:39 CDT 2005
//
// Template function for broadcasting objects to all clients in
// a sequence or associative container of teulindexes or objects
// with teulIndex() members.
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "teul/teulTransmission.h"
#include "chat/chatLocalizedMessage.h"

namespace Chat { extern TEUL_CALLBACK* _SEND_LOCALIZED_MESSAGE ; }

template<typename _Iterator, typename _Object, typename _Filter>
void
chatBroadcastObject(const _Iterator& begin, const _Iterator& end, TEUL_CALLBACK* const callback, _Object& message, const _Filter& filterFn)
{
	if ( begin == end ) return ;

	static TeulTransmission broadcast(callback) ;

	broadcast.populate(&message) ;
	broadcast.send(begin, end, filterFn) ;

	return ;
}

template<typename _Iterator, typename _Object>
void
chatBroadcastObject(const _Iterator& begin, const _Iterator& end, TEUL_CALLBACK* const callback, _Object& message)
{
	if ( begin == end ) return ;

	static TeulTransmission broadcast(callback) ;

	broadcast.populate(&message) ;
	broadcast.send(begin, end) ;

	return ;
}

#define chatBroadcastLocMessage(begin, end, ...) chatBroadcastObject(begin, end, Chat::_SEND_LOCALIZED_MESSAGE, __VA_ARGS__)
#define chatBroadcastHostMessage(begin, end, ...) chatBroadcastObject(begin, end, CHAT_SEND_MESSAGE, __VA_ARGS__)

template<typename _Iterator>
void
chatDistributeLocMessage(const _Iterator& begin, const _Iterator& end, int prefix, int msg, const char* const t1=nullptr, const char* const t2=nullptr, const char* const t3=nullptr, const char* const t4=nullptr, const char* const t5=nullptr, const char* const t6=nullptr)
{
	Chat::LocalizedMessage lmsg(prefix, msg, t1, t2, t3, t4, t5, t6) ;
	chatBroadcastLocMessage(begin, end, lmsg) ;
}
