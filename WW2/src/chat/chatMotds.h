#pragma once
// Created: Thu Oct 13 17:47:51 CDT 2005
//
// CRS/Side MOTDs
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include <string>
#include "teul/teulPackers.h"

namespace MOTD
{
	enum { _CRS, _ALLIED, _AXIS, _MAX_TYPES };
}

struct MOTD_REQUEST
{
	MOTD_REQUEST(UINT32 Type=0) : type(Type) {}
	MOTD_REQUEST(ACTION_OBJECT* const a) { a->offset = packer(TEUL_UNPACK, a); }

	SINT32 packer(LOGICAL mode, ACTION_OBJECT* a);

	UINT32	type;
};

struct MOTD_REPLY
{
	static const UINT32 _MAX_LENGTH = 800;

	MOTD_REPLY(UINT32 Type=0, UINT32 Added=0, const ChatHandle& Poster=0LL, const std::string& Message="")
		: type(Type), added(Added), poster(Poster), message(Message) {}
	MOTD_REPLY(ACTION_OBJECT* const a) { a->offset = packer(TEUL_UNPACK, a); }

	SINT32 packer(LOGICAL mode, ACTION_OBJECT* a);

	UINT32		type;
	UINT32		added;
	ChatHandle	poster;
	std::string	message;
};
