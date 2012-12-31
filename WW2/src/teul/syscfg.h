#ifndef PN_PROJECT_SYSCFG_H
#define PN_PROJECT_SYSCFG_H 1
// Can't use a pragma guard because there are multiple syscfgs.
// This is the teulKit client syscfg.h
// Copyright 1999 Playnet Inc, All rights reserved
// NOTE : There is NO corresponding source file, this is purely an abstract
// header file.
// Originating Programmer : Stephen W. Evans

#ifndef _DEBUG
	#define NO_DEBUG_WRITE
	#define NO_DEBUG_LOG
//	#define FORCE_DEBUG
#else
	#define NO_DEBUG_WRITE
	//#define FORCE_DEBUG
#endif

#define TEUL_CLIENT	0
#define TEUL_SERVER	1

#if !defined(platBuild)
# define platBuild TEUL_CLIENT
#endif

#include "include/platforms.h"

#include "teul/teulBasic.h"
#include "teul/teulClosingEnums.h"
#include "teul/teulEndpointDefs.h"
#include "teul/teulEndpoint.h"
#include "teul/teulPackEnums.h"
#include "teul/teulPackers.h"
#include "teul/teulPacket.h"
#include "weap/weapSpec.h"

#define Write_Debug(...)

namespace Teul
{
#if !defined(MULTI_CLIENT)
	// Number of users to support per client (cluster is the host term)
	static const size_t MaxClusterUsers = 8 ;
	// How many endpoints teulKit reserve, also limits the
	// number of concurrent connections.
	static const size_t	MaxConnections = MaxClusterUsers * 3 + 4 ;
#else
	// Number of users to support per client instance (cluster is the host term)
	static const size_t MaxClusterUsers = 2000 ;
	// How many endpoints teulKit reserve, also limits the
	// number of concurrent connections.
	static const size_t	MaxConnections = MaxClusterUsers * 3 + (MaxClusterUsers / 4) ;
#endif
}

#endif
