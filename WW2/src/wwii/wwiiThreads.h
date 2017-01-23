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

using THREAD_CALLBACK = std::function<void(void*)>;

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

// Add a callback to the thread queue
extern void threadCallback(THREAD_CALLBACK fn, void*);

// Add a callback to the thread queue and wait for it to be executed
template <typename R>
static inline R threadCallbackSync(const std::function<R(void)> &fn) {
    static_assert(std::is_move_constructible<R>::value, "R must be move-constructible");
    std::mutex waitMutex;
    std::condition_variable waitCondition;
    bool waitSignal = false;
    std::aligned_storage<sizeof(R)> result;
    
    threadCallback([&fn, &result, &waitMutex, &waitSignal, &waitCondition] (void *) {
        new (&result) R(std::move(fn()));
        {
            std::lock_guard<std::mutex> lock(waitMutex);
            waitSignal = true;
            waitCondition.notify_all();
        }
    }, nullptr);
    {
        std::unique_lock<std::mutex> lock(waitMutex);
        waitCondition.wait(lock, [&waitSignal] { return waitSignal == true; });
    }
    return *reinterpret_cast<R *>(&result);
}

template <>
inline void threadCallbackSync(const std::function<void(void)> &fn) {
    (void) threadCallbackSync<bool>([&fn] { fn(); return false; });
}

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
