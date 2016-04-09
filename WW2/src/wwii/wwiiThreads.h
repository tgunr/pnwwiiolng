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

typedef struct st_mysql MYSQL;
class DBQuery;

typedef void (THREAD_CALLBACK)(void*);

//////////////////////////////////////////////////////////////////////
// Threads-system calls ("threads")

// Initialize threads to a ready state
extern void threadsSetup();

// Any threads that want main server setup to be completed
// before they start their things need to be enabled here
extern void threadsEnable();

// Bring the threads into sync and shut them down
extern void threadsShutdown();

// Threads can't write to dbugLog directly, so periodically
// the main process has to log for them
extern bool threadsLogging();

// Process the next callback on the queue
extern void threadsCallNextCallback();

// Prepare the dbConnection structure from the environment
extern bool threadsReadDBEnvironment();

// Open a new database connection in a thread-safe way
extern DB::handle_t* threadsOpenDBConnection(bool enableAutoCommit);

// Close a connection.
extern void threadsCloseDBConnection(DB::handle_t* conn);

//////////////////////////////////////////////////////////////////////
// Client calls ("thread")

// Threaded implementation of dbugLog (non-immediate)
extern void threadLog(const char* file, unsigned int line, unsigned int level, const char* fmt, ...);

#ifdef BETA_RELEASE
#define betaThreadLog(...) threadLog(__VA_ARGS__)
#else
#define betaThreadLog(...)
#endif

// Add a callback to the thread queue
extern void threadCallback(THREAD_CALLBACK* fn, void*);

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

namespace DBThread
{
	typedef enum { Audit, Auth, Chat, Game, Player, Squad, Sys, Wwii, WwiiOnline, NumDBNos, Default = WwiiOnline } DBNo;
	// Build a query in a textAlloc and format it for me.
	extern void Do(const DBNo db_, const char* queryFormat, ...);
	// Pre-formatted textAlloc query
	extern void Do(const DBNo db_, char* queryTextAlloc);
}
