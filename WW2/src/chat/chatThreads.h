#pragma once
// Created: Fri Sep 23 11:18:20 CDT 2005
//
// Headers for chatThreads
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

// Initialize threads to a ready state
extern void chatThreadsSetup();

// Any threads that want main server setup to be completed
// before they start their things need to be enabled here
extern void chatThreadsEnable();

// Bring the threads into sync and shut them down
extern void chatThreadsShutdown();
