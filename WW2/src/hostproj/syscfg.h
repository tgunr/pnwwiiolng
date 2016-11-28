#ifndef PN_PROJECT_SYSCFG_H
#define PN_PROJECT_SYSCFG_H 1
// Created: Sun Nov 19 12:50:55 CST 2006
//
//! Host-project specific defines, includes, etc.
//! This file is force-included by the compiler.
//
// Copyright (C) PlayNet, Inc 1999-2006 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "config.h"
#include "hostproj/hostBuildOptions.h"
#include "include/platforms.h"	// includes precompile_includes.h


#define NOGDI
#define WIN32_LEAN_AND_MEAN

#if platBuild != BUILD_SERVER
#error "Why are you building the host project without AS_SERVER?"
#endif

// Maximum number of active local connections allowed
namespace Teul
{
	static const size_t MaxConnections = 6000;
	static const size_t MaxClusterUsers = 5000;
}

#include <signal.h>

#include "inst/instDescriptor.h"
#include "wwii/wwiiPlayer.h"
#include "server/serverIndexes.h"
#include "server/serverPeers.h"
#include "wwii/wwiiServerTypeEnums.h"
#include "teul/teulConfigVar.h"

#endif // PN_PROJECT_SYSCFG_H
