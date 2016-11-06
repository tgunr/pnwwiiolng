#pragma once
// Created: Mon Dec 12 14:41:57 CST 2005
// Copyright (C) PlayNet, Inc 2005-2011 - All rights reserved.
// Not for redistribution.
//
// WWII Server-side thread functionality
//
// Originating Programmer: Oliver "kfsone" Smith

#include "include/pnThread.h"
#include "dbse/dbseHandle.h"

using MYSQL = struct st_mysql;
class DBQuery;

using THREAD_CALLBACK = void(*)(void*);

//////////////////////////////////////////////////////////////////////
// Threads-system calls ("threads")

// Initialize threads to a ready state
extern void threadsSetup();

// Any threads that want main server setup to be completed
// before they start their things need to be enabled here
extern void threadsEnable();

// Bring the threads into sync and shut them down
extern void threadsShutdown();

// Process the next callback on the queue
extern void threadsCallNextCallback();

// Open a new database connection in a thread-safe way
extern DB::handle_t* threadsOpenDBConnection(bool enableAutoCommit);

// Close a connection.
extern void threadsCloseDBConnection(DB::handle_t* conn);

//////////////////////////////////////////////////////////////////////
// Client calls ("thread")

// Threaded implementation of dbugLog (non-immediate)
#define threadLog(...) dbugLog(__VA_ARGS__)
//extern void threadLog(const char* file, unsigned int line, unsigned int level, FORMAT_ARG(0) const char* fmt, ...) FORMAT_FN(4, 5);

#ifdef BETA_RELEASE
#define betaThreadLog(...) threadLog(__VA_ARGS__)
#else
#define betaThreadLog(...)
#endif

// Add a callback to the thread queue
extern void threadCallback(THREAD_CALLBACK fn, void*);

// Escape a string for mysql/js usage
extern void wwiiEscapeString(std::string& text);

// Set per-thread ignoring of sig pipe
extern void threadIgnoreSIGPIPE();

// Thread for monitoring database and worker queue performance.
class StatsThread : public PNThread
{
public:
	virtual void run() override;
};
