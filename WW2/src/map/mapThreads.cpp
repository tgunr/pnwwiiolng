#include <syscfg.h> // Path is project specific.
//
// Date: Tue Feb  7 17:59:15 CST 2006
//
// Chat thread functionality and processes
//
// Copyright (C) PlayNet, Inc 1999-2006 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith
//

#include "wwii/wwiiThreads.h"
#include "mm/mmdbPersonaSelections.h"

static PersonaSelectionThread _personaThread;
static StatsThread _statsThread;


void
mapThreadsSetup()
{
	threadsSetup();

	_statsThread.start();
	_personaThread.start();
}

void
mapThreadsEnable()
{
	threadsEnable();
}

void
mapThreadsShutdown()
{
	_personaThread.cancel();
	_personaThread.join();

	_statsThread.cancel();
	_statsThread.join();

	threadsShutdown();
}
