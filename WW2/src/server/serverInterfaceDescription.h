#pragma once
// Created:
// Copyright (C) PlayNet Inc 2011 -- All rights reserved.
// Originating Programmer: Oliver "kfsone" Smith

#include "wwii/wwiiServerTypeEnums.h"

namespace Server
{
	struct InterfaceDescription
	{
		UINT32	pid;		// process id
		UINT32	numIP;
		enum { MaxHostIPs = 32 };
		struct
		{
			UINT32	ip;		// address in host byte order
			UINT32	naddr;	// address in network byte order
		} addr[MaxHostIPs];
	};

	extern InterfaceDescription myInterface;
}

