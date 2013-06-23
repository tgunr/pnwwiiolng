#include <syscfg.h> // Path is project specific.
// Created: Tue Sep  6 15:57:52 CDT 2005
//
// Chat Thread functionality and thread processes
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "wwii/wwiiThreads.h"
#include "chat/chatBanPlayer.h"

//////////////////////////////////////////////////////////////////////
// Threads
//////////////////////////////////////////////////////////////////////

struct BanThread : public PNThread
{
	virtual void run() { chatBanQueueThread(NULL);	}
};
static BanThread _banThread ;
static StatsThread _statsThread ;

//////////////////////////////////////////////////////////////////////
// Initialize threads

void
chatThreadsSetup()
{
	threadsSetup() ;
	_banThread.start();
	_statsThread.start();
}

//////////////////////////////////////////////////////////////////////
// Some threads don't want to start doing their thing until the host
// has done initializing. Unblock those threads here.

void
chatThreadsEnable()
{
	threadsEnable() ;
}

//////////////////////////////////////////////////////////////////////
// Tell the threads to shut down and give them chance to do so

void
chatThreadsShutdown()
{
	_banThread.cancel();
	_statsThread.cancel();
	threadsShutdown() ;
}

