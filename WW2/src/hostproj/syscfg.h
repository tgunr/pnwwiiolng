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

#include "hostproj/config.h"

// Tell FIOD that we need extra help
#define LIMITED_FIO	1

#include "hostproj/hostBuildOptions.h"
#include "include/platforms.h"	// includes precompile_includes.h

#if !defined(AS_SERVER) || defined(AS_CLIENT)
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
#include "wwii/wwiiIRC.h"
#include "server/serverPeers.h"
#include "wwii/wwiiServerTypeEnums.h"
#include "teul/teulConfigVar.h"

#endif // PN_PROJECT_SYSCFG_H
