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

// Disable dbugWrite in all host processes
#define NO_DEBUG_WRITE	1

// Legacy definition of platBuild
#if !defined(platBuild)
# if defined(AS_SERVER)
#  define platBuild TEUL_SERVER
# elif defined(AS_CLIENT)
#  define platBuild TEUL_CLIENT
# else
#  error ("No AS_SERVER or AS_CLIENT definition")
# endif
#endif

// Tell FIOD that we need extra help
#define LIMITED_FIO	1

#include "hostproj/versionNo.h"
#include "versionNo.h"

#if defined(HAVE_SYS_CDEFS_H)
#include <sys/cdefs.h>
#endif

#include "include/platforms.h"

// Maximum number of active local connections allowed
namespace Teul
{
	static const size_t MaxConnections = 6000 ;
	static const size_t MaxClusterUsers = 5000 ;
}

#include "include/precompile_includes.h"
#include <stdlib.h>
#include <limits.h>
#include <signal.h>
//#include <atomic>

#include "inst/instDescriptor.h"
#include "wwii/wwiiPlayer.h"
#include "server/serverIndexes.h"
#include "wwii/wwiiIRC.h"
#include "server/serverPeers.h"
#include "wwii/wwiiServerTypeEnums.h"
#include "teul/teulConfigVar.h"

#endif // PN_PROJECT_SYSCFG_H
