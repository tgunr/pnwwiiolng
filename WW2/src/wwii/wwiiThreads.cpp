#include <syscfg.h> // Path is project specific.
// Created: Mon Dec 12 14:41:57 CST 2005
//
// Functions for adding threads to a host
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "versionNo.h"
#include "dbug/dbug.h"
#include <assert.h>

#include <string>
#include <mysql.h>
#include <signal.h>
#include <mutex>

#include "dbse/dbseConnection.h"
#include "dbse/dbseWrapper.h"
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
class DBQueueThread : public PNThread
{
private:
	DB::handle_t*		m_conn;
	uint32_t			m_id;

public:
	DBQueueThread() : PNThread(), m_conn(nullptr), m_id(0) {}
	~DBQueueThread();

	void start(uint32_t id_) {
		m_id = id_ + 1;
		PNThread::start();
	}

	virtual void run();
};

static const UINT32 NUM_HELPER_THREADS = 2;

typedef std::vector<THREAD_LOG*> LOG_QUEUE;
static LOG_QUEUE logQueue;
static size_t logQueueMax, logQueuePushes;
static std::mutex logQueueMutex;

static ConditionQueue<DBThreadQuery*> dbQueue;
DBQueueThread dbThread[NUM_HELPER_THREADS];

typedef std::pair<THREAD_CALLBACK*, void*> CALLBACK_INFO;
typedef std::vector<CALLBACK_INFO*> CALLBACK_QUEUE;
static CALLBACK_QUEUE callbackQueue;
static size_t cbQueueMax, cbQueuePushes;
static std::mutex callbackQueueMutex;

extern DBCONNECTION dbConnection;

void
DBQueueThread::run()
{
	unsigned int threadId = m_id;

	m_conn = threadsOpenDBConnection(true);
	if ( m_conn == nullptr )
	{
		threadLog(_FLA, "dbc:%u Unable to obtain database connection", threadId);
		return;
	}

	threadLog(_FLI, "dbc:%u Connected to database", threadId);

	threadIgnoreSIGPIPE();

	DBThread::DBNo dbNo = DBThread::Default;

	for ( ; ; )
	{
		DBThreadQuery* query = dbQueue.getNext();
		if ( query == nullptr )
			continue;
		if ( query->Execute(threadId, m_conn, &dbNo) )
			continue;

		threadLog(_FLW, "dbc:%u reconnecting to database", threadId);

		if ( m_conn != nullptr )
			mysql_close(m_conn);
		m_conn = nullptr;

		do
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(5));
			m_conn = threadsOpenDBConnection(true);
			if ( m_conn == nullptr )
				threadLog(_FLA, "dbc:%u unable to reconnect to database", threadId);
		} while ( m_conn == nullptr );

		threadLog(_FLW, "dbc:%u reconnected to database", threadId);
	}
}

DBQueueThread::~DBQueueThread()
{
	if ( m_conn != nullptr )
	{
		mysql_close(m_conn);
		m_conn = nullptr;
	}
}

//////////////////////////////////////////////////////////////////////
// Read the database environment from a given parameter

bool
threadsReadDBEnvironment()
{
	const char* db = cv_wwiiDBAddress.GetString();

	// Under Windows, we'll assume you mean "this machine".
#if platOS == OS_WIN32
	if ( db == nullptr || *db == 0 )
		db = "127.0.0.1";
#else
	if ( !db || *db == 0 )
		LogAbort("Unable to read %s database address from", cv_wwiiDBAddress.GetName(), cv_wwiiDBAddress.GetEnvVarName() ? cv_wwiiDBAddress.GetEnvVarName() : "environment");
#endif

	strcpy(dbConnection.host, db);
	strcpy(dbConnection.dbName, cv_wwiiDBDefaultSchema.GetString());
	strcpy(dbConnection.username, "wwiiol");
	strcpy(dbConnection.password, "freebird");
	strcpy(dbConnection.port, "");

	return true;
}

//////////////////////////////////////////////////////////////////////
// Thread-safe db connection opener

DB::handle_t*
threadsOpenDBConnection(bool enableAutoCommit)
{
	DB::handle_t* newConn = dbseGetConnection(dbConnection, false);

	if ( newConn != nullptr )
	{
		mysql_select_db(newConn, dbConnection.dbName);

		mysql_autocommit( newConn, (enableAutoCommit) ? 1 : 0 );
	}

	return newConn;
}

//////////////////////////////////////////////////////////////////////S
// Close database connection

void
threadsCloseDBConnection(MYSQL* conn)
{
	if ( conn == nullptr )
		return;

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
	// Try and clear out any threadLog messages from startup
	threadsLogging();
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

	std::this_thread::yield();
	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	// Clear out any pending threadLogs, but don't take too long at it
	threadsLogging();
}

//////////////////////////////////////////////////////////////////////
// Write logging messages from the threads so that the IO is
// all in one place and the OS can schedule for it more easily.
// LOSSY: If the queue starts to get too full, old entries will
// be dropped and summarized with an overflow warning.

bool
threadsLogging()
{
	// The logging is not critical, if we miss it this time round
	// we'll pick it up soon enough.
	// empty() is atomic.  Worst case scenario is that we don't log
	// this cycle or we proceed and find that the queue is empty.
	// Since we are the only consumer, the latter is unlikely.
	if ( logQueue.empty() )
		return false;

	std::unique_lock<std::mutex> lock(logQueueMutex, std::try_to_lock);
	if ( lock.owns_lock() == false )
		return false;

	if ( logQueue.size() > logQueueMax )
		logQueueMax = logQueue.size();

	for ( auto& it : logQueue )
	{
		const THREAD_LOG* entry = it;
		if ( *entry->m_entry )
			_dbugLog(entry->m_file, entry->m_line, entry->m_level, "%s", entry->m_entry);
		delete entry;
	}

	logQueue.clear();

	return true;
}

//////////////////////////////////////////////////////////////////////
// Drain logging and then check for any waiting callbacks, call the
// first one. Returns false if there was no work done otherwise true.
// This allows threads to issue callbacks for execution in the main
// (or any other) thread.

void
threadsCallNextCallback()
{
	if ( callbackQueue.empty() )
		return;

	std::unique_lock<std::mutex> lock(callbackQueueMutex, std::try_to_lock);
	if ( lock.owns_lock() == false )
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
// Threaded variation of dbugLog, queues the output for logging in
// whichever thread owns the logfile access.

void
threadLog(const char* file, UINT32 line, UINT32 level, const char* fmt, ...)
{
	if ( g_logDebug == false && level == Debug::ERR_DBUG )
		return;

	THREAD_LOG* entry = new THREAD_LOG(file, line, level);

	va_list args;
	va_start(args, fmt);
	vsnprintf(entry->m_entry, LOG_LENGTH, fmt, args);
	va_end(args);

	std::lock_guard<std::mutex> lock(logQueueMutex);
	logQueue.push_back(entry);
	++logQueuePushes;
}

//////////////////////////////////////////////////////////////////////
// Push a DBQuery based object onto the dbQueue

void
DBThreadQuery::Queue()
{
	// The dbQueue version of push_back uses mutex exclusion
	dbQueue.push_back(this);
}

//////////////////////////////////////////////////////////////////////
// Perform a query on an unformatted string.

void
DBThread::Do(const DBThread::DBNo db_, char* queryText)
{
	(new DBThreadQuery(db_, "DBThread::Do(q)", queryText, DBThreadQuery::_IMMEDIATE))->Queue();
}

//////////////////////////////////////////////////////////////////////
// Threaded variation of a database query

void
DBThread::Do(const DBThread::DBNo db_, const char* queryFormat, ...)
{
	va_list args;
	va_start(args, queryFormat);
	(new DBThreadQuery(db_, "DBThread::Do(q)", DBThreadQuery::_IMMEDIATE, args, queryFormat))->Queue();
	va_end(args);
}

//////////////////////////////////////////////////////////////////////
// Add a callback to be invoked inside the main thread

void
threadCallback(THREAD_CALLBACK* fn, void* param)
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

	threadLog(_FLI, "Stats thread active [thread #%u]", std::this_thread::get_id());

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
				dbCurSize = dbQueue.m_queue.size();
				dbMaxSize = dbQueue.m_maxSize;
				dbUsed = dbQueue.m_hits - dbLastUsed;
				dbLastUsed += dbUsed;
			});

			{
				std::lock_guard<std::mutex> lock(callbackQueueMutex);
				cbCurSize = callbackQueue.size();
				cbMaxSize = cbQueueMax;
				cbUsed = cbQueuePushes - cbLastUsed;
				cbLastUsed += cbUsed;
			}

			threadLog(_FLD, "THREADS: dbq(ttl:%llu,used:%llu,max:%llu,cur:%llu), cbq(ttl:%llu,used:%llu,max:%llu,cur:%llu)"
							, dbLastUsed, dbUsed, dbMaxSize, dbCurSize
							, cbLastUsed, cbUsed, cbMaxSize, cbCurSize);

		}
		catch ( std::exception& e )
		{
			threadLog(_FLD, "statsthread: %s", e.what());
		}
	}
}

