#include <syscfg.h> // Path is project specific.
// Created: Mon Dec 12 14:41:57 CST 2005
//
// Functions for adding threads to a host
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include <mysql.h>
#include <signal.h>
#include <mutex>
#include <sstream>

#include "dbse/dbseWrapper.h"
#include "include/mydbQuery.h"
#include "teul/teulConfigVar.h"
#include "wwii/wwiidbConfigVars.h"
#include "wwii/wwiiDBThreads.h"
#include "wwii/wwiiThreadText.h"
#include "wwii/wwiiThreadTypes.h"

// We can't dbugLog from threads directly, so we store
// up their log messages into a queue for logging when
// the host has time.
static const UINT32 LOG_LENGTH = 2048;
struct THREAD_LOG
{
	THREAD_LOG(const char* file, UINT32& line, UINT32& level)
		: m_file(file), m_line(line), m_level(level) { m_entry[0] = m_entry[LOG_LENGTH] = 0; }
	const char*		m_file;
	UINT32			m_line;
	UINT32			m_level;
	char			m_entry[LOG_LENGTH+1];
};

// Storing information about a DB helper thread
class DBQueueThread : public DBThread::Thread
{
private:
	uint32_t			m_id = 0;

	using DBThread::Thread::start;

public:
	virtual ~DBQueueThread();

	void start(uint32_t id_) {
		m_id = id_ + 1;
		start();
	}

	DB::handle_t* getConn() const { return m_conn; }

	virtual void dbrun() override;
};

static const UINT32 NUM_HELPER_THREADS = 2;

static ConditionQueue<DBThreadQuery> dbQueue;
DBQueueThread dbThread[NUM_HELPER_THREADS];

using CALLBACK_INFO = std::pair<THREAD_CALLBACK, void*>;
using CALLBACK_QUEUE = std::vector<CALLBACK_INFO*>;
static CALLBACK_QUEUE callbackQueue;
static size_t cbQueueMax, cbQueuePushes;
static std::mutex callbackQueueMutex;

//////////////////////////////////////////////////////////////////////
// Wrapper for a DBThread::Thread's run function

void DBThread::Thread::run()
{
	m_conn = threadsOpenDBConnection(true);
	if ( m_conn == nullptr )
	{
		dbugLog(_FLA, "unable to connect to mysql database");
		return;
	}

	threadIgnoreSIGPIPE();

	dbrun();

	threadsCloseDBConnection(m_conn);
	mysql_thread_end();
}

void
DBQueueThread::dbrun()
{
	const unsigned int threadId = m_id;

	dbugLog(_FLI, "dbc:%u Connected to database", threadId);

	DBThread::DBNo dbNo = DBThread::Default;

	while ( !isCancelled() )
	{
		DBThreadQuery* query = dbQueue.getNext();
		if ( query == nullptr )
			break;
		if ( query->Execute(threadId, m_conn, &dbNo) )
			continue;

		threadsCloseDBConnection(m_conn);
		m_conn = nullptr;

		while (!isCancelled() && m_conn == nullptr) {
			std::this_thread::sleep_for(std::chrono::milliseconds(5));
			dbugLog(_FLW, "dbc:%u reconnecting to database", threadId);
			m_conn = threadsOpenDBConnection(true);
			if ( m_conn == nullptr )
				dbugLog(_FLA, "dbc:%u unable to reconnect to database", threadId);
		}

		dbugLog(_FLW, "dbc:%u reconnected to database", threadId);
	}
}

DBQueueThread::~DBQueueThread()
{
	dbQueue.destroy();
}

//////////////////////////////////////////////////////////////////////
// Thread-safe db connection opener

DB::handle_t*
threadsOpenDBConnection(bool enableAutoCommit)
{
	DB::handle_t* newConn = dbseGetConnection(false);

	if ( newConn != nullptr )
	{
		mysql_autocommit(newConn, (enableAutoCommit) ? 1 : 0);
	}

	return newConn;
}

//////////////////////////////////////////////////////////////////////S
// Close database connection

void
threadsCloseDBConnection(DB::handle_t* conn)
{
	if ( conn != nullptr )
		mysql_close(conn);
}

//////////////////////////////////////////////////////////////////////
// Set per-thread ignoring of sig pipe
// MySQL can generate SIGPIPE when a connection goes away;
// since we're using mysql_ping to try and keep the connections
// alive silently, we really don't want the application to crash
// just because it idled out.

void
threadIgnoreSIGPIPE()
{
#if platOS != OS_WIN32
	// Mask the signal we want to ignore
	sigset_t signal_mask;
	sigemptyset(&signal_mask);
	sigaddset(&signal_mask, SIGPIPE);

	pthread_sigmask(SIG_BLOCK, &signal_mask, nullptr);
#else
#endif
}

//////////////////////////////////////////////////////////////////////
// Initialize basic threads

void
threadsSetup()
{
	// Create a number of worker threads, each of which is a
	// database thread contesting for work
	for ( UINT32 i = 0; i < NUM_HELPER_THREADS; ++i )
	{
		dbThread[i].start(i);
	}
}

//////////////////////////////////////////////////////////////////////
// Any work that needs doing to set threads rolling

void
threadsEnable()
{
}

//////////////////////////////////////////////////////////////////////
// Tell the threads to shut down and give them chance to do so

void
threadsShutdown()
{
	for ( UINT32 i = 0; i < NUM_HELPER_THREADS; ++i )
	{
		dbThread[i].cancel();
	}
	dbQueue.destroy();

	std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

//////////////////////////////////////////////////////////////////////
// Drain logging and then check for any waiting callbacks, call the
// first one. Returns false if there was no work done otherwise true.
// This allows threads to issue callbacks for execution in the main
// (or any other) thread.

void
threadsCallNextCallback()
{
	std::unique_lock<std::mutex> lock(callbackQueueMutex, std::try_to_lock);
	if ( lock.owns_lock() == false )
		return;

	if ( callbackQueue.empty() )
		return;

	if ( callbackQueue.size() > cbQueueMax )
		cbQueueMax = callbackQueue.size();

	for ( auto& it : callbackQueue )
	{
		try
		{
			it->first(it->second);
		}
		catch ( std::exception& e )
		{
			dbugLog(_FLA, "UNHANDLED EXCEPTION THROWN BY THREAD CALLBACK: %s", e.what());
		}
		delete it;
	}

	callbackQueue.clear();
}

//////////////////////////////////////////////////////////////////////
// Push a DBQuery based object onto the dbQueue

void
DBThreadQuery::Queue()
{
	// The dbQueue version of push_back uses mutex exclusion
	dbQueue.push_back(this);
}

void
DBThread::EscapeString(DB::handle_t* conn, std::string& text)
{
	auto buff = std::make_unique<char[]>(text.length() * 2 + 1);
    mysql_real_escape_string(conn, buff.get(), text.c_str(), text.length());
    text = buff.get();
}

void
DBThread::EscapeString(std::string& text)
{
	auto buff = std::make_unique<char[]>(text.length() * 2 + 1);
    dbQueue.doLocked([&] {
		///TODO: FIX: Conflicting access to the connection pointer
        mysql_real_escape_string(dbThread[0].getConn(), buff.get(), text.c_str(), text.length());
    });
    text = buff.get();
}

//////////////////////////////////////////////////////////////////////
// Add a callback to be invoked inside the main thread

void
threadCallback(THREAD_CALLBACK fn, void* param)
{
	std::lock_guard<std::mutex> lock(callbackQueueMutex);
	callbackQueue.push_back(new CALLBACK_INFO(fn, param));
	++cbQueuePushes;
}

//////////////////////////////////////////////////////////////////////
// Thread function for keeping stats on the threading functions :)
// Takes one argument, a host name which is used as a prefix for the
// table to record to.

void
StatsThread::run()
{
	UINT64 dbCurSize, dbMaxSize, dbUsed;
	UINT64 dbLastUsed = 0L;
	UINT64 cbCurSize, cbMaxSize, cbUsed;
	UINT64 cbLastUsed = 0L;

	dbugLog(_FLI, "Stats thread active");

	UINT32 count = 0;
	for ( ; ; )
	{
		// sleep for 15,000 milliseconds (15 seconds).
		for (uint32_t i = 0; i < 15 * 10; ++i) {
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			if(isCancelled())
				return;
		}

		if ( ++count < 4 )
			continue;
		count = 0;

		try
		{
			dbQueue.doLocked([&] {
				dbCurSize = dbQueue.size_locked();
				dbMaxSize = dbQueue.maxSize_locked();
				dbUsed = dbQueue.hits_locked() - dbLastUsed;
				dbLastUsed += dbUsed;
			});

			{
				std::lock_guard<std::mutex> lock(callbackQueueMutex);
				cbCurSize = callbackQueue.size();
				cbMaxSize = cbQueueMax;
				cbUsed = cbQueuePushes - cbLastUsed;
				cbLastUsed += cbUsed;
			}

			dbugLog(_FLD, "THREADS: dbq(ttl:%" PR64BitUT ",used:%" PR64BitUT ",max:%" PR64BitUT ",cur:%" PR64BitUT "), cbq(ttl:%" PR64BitUT ",used:%" PR64BitUT ",max:%" PR64BitUT ",cur:%" PR64BitUT ")"
							, dbLastUsed, dbUsed, dbMaxSize, dbCurSize
							, cbLastUsed, cbUsed, cbMaxSize, cbCurSize);

		}
		catch ( std::exception& e )
		{
			dbugLog(_FLD, "statsthread: %s", e.what());
		}
	}
}

