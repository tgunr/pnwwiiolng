#pragma once
// Created: Tue Jan 15 11:31:45 CST 2008
//
// Structure sent to the client to activate it for logging in
// to a cluster.
//
// Copyright (C) PlayNet, Inc 1999-2008 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "wwii/wwiiServerTypeEnums.h"		// For SERVER_TYPE enum

struct WWII_ACTIVATE
{
	WWII_ACTIVATE()
		: serverType((SERVER_TYPE)0)
		, port(0)
		, hostIP(0)
		, hostTime(0)
		, teulIndex(-1)
		, playerID(0)
		, otp(0)
	{
		pkey[0] = 0;
	}

	SERVER_TYPE	serverType;
	UINT16		port;
	UINT32		hostIP;
	UINT32		hostTime;
	UINT16		teulIndex;
	playerid_t	playerID;
	UINT32		otp;
	UCHAR		pkey[80];

	SINT32 packer(PackMode mode, ACTION_OBJECT* a)
	{
		a->offset = teulPackUINT32(mode, a->packed, a->offset, (UINT32*)(&serverType), 8);
		a->packer(mode, port);
		a->packer(mode, hostIP);
		a->packer(mode, hostTime);
		a->packer(mode, teulIndex);
		a->packer(mode, playerID);
		a->packer(mode, otp);

		if ( teulIndex != 0 && playerID != 0 )
			a->offset = teulPackVoid(mode, a->packed, a->offset, pkey, sizeof(pkey));

		return a->offset;
	}
};
