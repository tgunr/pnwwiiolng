#include <syscfg.h> // Path is project specific.
// Created: Tue Jan 31 17:30:06 CST 2006
//
// DB Thread Handling
//
// Copyright (C) PlayNet, Inc 1999-2006 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "versionNo.h"

#include "include/mydbQuery.h"
#include <mysql.h>
#include "teul/teulTime.h"
#include "wwii/wwiiDBThreads.h"
#include "wwii/wwiiThreadTextQuery.h"

//////////////////////////////////////////////////////////////////////
// Wrappers for success/failure

static void
_callSuccess(void* const data)
{
	std::unique_ptr<DBThreadQuery> q(static_cast<DBThreadQuery*>(data));
	q->Success();
}

static void
_callFailure(void* data)
{
	std::unique_ptr<DBThreadQuery> q(static_cast<DBThreadQuery*>(data));
	q->Failure();
}

//////////////////////////////////////////////////////////////////////
// Previous versions allowed you to specify a database by putting
// !schema! at the start of the query.

const char*
_skipExclamationInStatement(const char* src_)
{
	const char* p = src_;

	if ( *p != '!' )
		return src_;

	threadLog(_FLW, "QUERY WITH !: %s", src_);

	++p;
	while ( *p )
	{
		if ( *(p++) == '!' )
			break;
	}

	return p;
}

//////////////////////////////////////////////////////////////////////
// Non-background thread query, processing will occur immediately

bool
DBThreadQuery::Execute(UINT32 threadNo, DB::handle_t* conn, DBThread::DBNo* lastDb)
{
	static const char* const dbName[DBThread::NumDBNos] = { "audit", "auth", "chat", "game", "player", "squad", "sys", "wwii", "wwiionline" };

	// To avoid various potential race conditions on who has ownership of the
	// statement string, we need to make our own copy of the statement.
	const char* const statement = _skipExclamationInStatement(Statement());

#ifdef DEV_BUILD
	if ( g_logDebug )
		threadLog(_FLD, "dbq:%u db:[%s] execute %s: q:[%s]", threadNo, dbName[m_db], Name(), statement);
#endif

	bool successful = false;

	try
	{
		// Wake the connection up if its gone to sleep or gone away
		mysql_ping(conn);

		if ( m_db != *lastDb )
		{
			if ( mysql_select_db(conn, dbName[m_db]) != 0 )
				throw std::invalid_argument("Error selecting database");
			*lastDb = m_db;
		}

		m_query.reset(new DBQuery(conn, statement));

		if ( m_immediate )
			Success();
		else
		{
			// Consume the query here and now, so we can
			// hand the result back to the parent thread.
			const bool processed = Process(Query()->Rows());
			m_query.reset(nullptr);

			if ( processed == true )
				threadCallback(_callSuccess, this);
			else
				threadCallback(_callFailure, this);
		}

		successful = true;
	}
	catch ( std::logic_error& e )
	{
		threadLog(_FLW, "dbq:%u query %s q:[%s] failed: %s", threadNo, Name(), Statement(), e.what());

		if ( m_immediate )
			Failure();
		else
		{
			m_query.reset(nullptr);
			threadCallback(_callFailure, this);
		}
	}

	if ( m_immediate )
	{
		delete this;
	}

	return successful;
}

//////////////////////////////////////////////////////////////////////
// If there is pending data and maxAge milliseconds have elapsed since
// the last commit (or there has been no previous commit)

bool
ThreadTextQuery::commit(const TeulTimeMS maxAge)
{
	if ( (m_length == 0) | ((maxAge) & (m_commitFrom) & (m_commitFrom + maxAge > teulClock64)) )
		return false;			// Buffer needs to mature for a while longer.

	if ( m_length + m_sufLen + 1 >= TEXT_POOL_SIZE )
		throw std::domain_error("Too much data in ThreadTextQuery buffer to commit");

	// Need to add the suffix.
	strncpy(m_statement + m_length, m_suffix, sizeof(m_statement) - m_length);

#ifdef BETA_RELEASE
	dbugLog(_FLD, "Committing: %s", m_statement);
#endif

	// Dispatch to the threading environment.
	(new DBThreadQuery(m_db, "TTQ::commit", m_statement, DBThreadQuery::_IMMEDIATE))->Queue();

	m_length = 0LL;

	m_commitFrom = teulClock64;

	return true;
}

