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
#include <errmsg.h>

#include "include/mydbQuery.h"
#include "teul/teulConfigVar.h"
#include "include/configVars.h"
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

static const UINT32 NUM_HELPER_THREADS = 4;

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
    threadIgnoreSIGPIPE();

	m_conn = threadsOpenDBConnection(true);
	if ( m_conn == nullptr )
	{
		dbugLog(_FLA, "unable to connect to mysql database");
		return;
	}

	dbrun();

	threadsCloseDBConnection(m_conn);
	mysql_thread_end();
}

using namespace std::chrono_literals;
void
DBQueueThread::dbrun()
{
	dbugLog(_FLI, "dbc:%u Connected to database", m_id);

	DBThread::DBNo dbNo = DBThread::Default;

	while ( !isCancelled() )
	{
        DBThreadQuery *query = nullptr;

        if (!dbQueue.getNextWithin(5s, &query)) {
            // No queries to make after 5s. Try to keep the DB connection alive.
            if (mysql_ping(m_conn) != 0) {
                dbugLog(_FLA, "dbc:%u server connection failure - %d(%s)", m_id, mysql_errno(m_conn), mysql_error(m_conn));
				threadsDBReconnect(&m_conn, m_id, this);
            }
        } else if (query == nullptr) {
            // No queries left, queue was destroyed.
			break;
        } else {
            // Got a query. Try to execute it.
            if (!query->Execute(m_id, m_conn, &dbNo)) {
				// attempt reconnect only if failure was due to bad connection
            	if (isDBConnectionGone(m_conn)) {
	                dbugLog(_FLA, "dbc:%u server connection failure - %d(%s)", m_id, mysql_errno(m_conn), mysql_error(m_conn));
					threadsDBReconnect(&m_conn, m_id, this);
	            }
            }
        }
	}
}

DBQueueThread::~DBQueueThread()
{
	dbQueue.destroy();
}

bool
isDBConnectionGone(DB::handle_t *conn)
{
	int mysql_err_no = mysql_errno(conn);
	switch(mysql_err_no)
	{
		case CR_SERVER_GONE_ERROR:
		case CR_SERVER_HANDSHAKE_ERR:
		case CR_SERVER_LOST:
		case CR_SERVER_LOST_EXTENDED:
			return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////
// Thread-safe db re-connect
void
threadsDBReconnect(DBHANDLE& dbHandle)
{
	dbugLog(_FLW, "reconnecting DB on Main thread");

	do {
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
		dbugLog(_FLW, "Reconnecting to database, main thread");
		if (!dbHandle.reconnect()) {
			dbugLog(_FLA, "Unable to reconnect to database on main thread, will retry");
		}
	} while (!dbHandle.connected());
	dbugLog(_FLW, "Reconnected to database on main thread");
}

//////////////////////////////////////////////////////////////////////
// Thread-safe db re-connect
void
threadsDBReconnect(DB::handle_t ** conn, UINT32 threadRef, PNThread *pThread)
{
	assert(conn!=nullptr);

	dbugLog(_FLW, "reconnecting DB dbc:%u", threadRef);

	threadsCloseDBConnection(*conn);
	*conn = nullptr;

	while ((!pThread || !pThread->isCancelled()) && *conn == nullptr) {
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
		dbugLog(_FLW, "dbc:%u reconnecting to database", threadRef);
		*conn = threadsOpenDBConnection(true);
		if (*conn == nullptr) {
			dbugLog(_FLA, "dbc:%u unable to reconnect to database, will retry", threadRef);
		}
	}
	dbugLog(_FLW, "dbc:%u reconnected to database", threadRef);
}

//////////////////////////////////////////////////////////////////////
// Thread-safe db connection opener

DB::handle_t*
threadsOpenDBConnection(bool enableAutoCommit)
{
	DB::handle_t* newConn = dbseGetConnection();

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
        //dbThread[i].join(); // We can't actually join these threads, as there's no way to break the condition queue's wait.
	}
	dbQueue.destroy();
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
#ifdef NDEBUG
		try
#endif
		{
			it->first(it->second);
		}
#ifdef NDEBUG
		catch ( std::exception& e )
		{
			dbugLog(_FLA, "UNHANDLED EXCEPTION THROWN BY THREAD CALLBACK: %s", e.what());
		}
#endif
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

