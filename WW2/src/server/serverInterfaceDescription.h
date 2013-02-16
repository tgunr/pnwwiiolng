#pragma once
// Created:
// Copyright (C) PlayNet Inc 2011 -- All rights reserved.
// Originating Programmer: Oliver "kfsone" Smith

#include "wwii/wwiiServerTypeEnums.h"

namespace Server
{
	struct InterfaceDescription
	{
		UINT64	mac;		// ethernet mac
//		UINT64	id;			// mac + pid
		UINT32	numIP;

		enum { MaxHostIPs = 4 };
		struct
		{
			UINT32	ip;		// address in host byte order
			UINT32	naddr;	// address in network byte order
		} addr[MaxHostIPs];
		UINT32	pid;		// process id
	} ;

	extern InterfaceDescription myInterface ;
}

