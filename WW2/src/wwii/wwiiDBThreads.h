#pragma once
// Created: Tue Jan 31 17:19:13 CST 2006
//
// Database Threads
//
// Copyright (C) PlayNet, Inc 1999-2006 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "wwii/wwiiThreads.h"
#include "include/mydbQuery.h"

#define MYSQL_ESC_QUERY_VAR_MAX_LEN   512

namespace DBThread
{
	typedef enum { Audit, Auth, Strat, Toe, Wwii, WwiiOnline, NumDBNos, Default = WwiiOnline } DBNo;
}

//////////////////////////////////////////////////////////////////////
// Base virtual class, not intended to be used directly
// Specify _IMMEDIATE if the success() or failure() functions are
// thread safe and can be processed as soon as the query has been
// attempted.
// Specify _CALLBACK to have the success() or failure() functions
// invoked via a callback in the main thread.

class DBThreadQuery
{
public:
	static constexpr bool _IMMEDIATE = true, _CALLBACK = false;
	static constexpr size_t MaxStatementLen = 4096;

	DBThreadQuery(const DBThread::DBNo db_, const char* const name_, const char* const statement_, bool immediate)
		: m_db(db_), m_name(name_), m_immediate(immediate), m_query(nullptr)
		{
			strsafecpy(m_statement, statement_);
		}

	template<typename... Args>
	DBThreadQuery(const DBThread::DBNo db_, const char* const name_, bool immediate_, const char* const fmt_, Args... args_)
		: m_db(db_), m_name(name_), m_immediate(immediate_), m_query(nullptr)
		{
			strprintf(m_statement, fmt_, std::forward<Args>(args_)...);
			m_statement[MaxStatementLen] = '\0';
		}

	virtual ~DBThreadQuery() {}

public:
	const char* Statement() const { return m_statement; }
	const char* Name() const { return m_name; }
	DBQuery* Query() const { return m_query.get(); }

	bool Execute(UINT32 threadNo, MYSQL* conn, DBThread::DBNo*);
	void Queue();

	//! Called if query succeeds.
	virtual void Success() {}
	//! Called if query fails.
	virtual void Failure() {}
	//! Called before success/failure;
	//! after this, the query is deleted.
	virtual bool Process(const size_t /*numRows*/) { return false; }

	template<typename... Args>
	void Do(const DBThread::DBNo db_, const char* fmt, Args&&... args)
	{
		(new DBThreadQuery(db_, "DBThreadQuery::Do(q)", DBThreadQuery::_IMMEDIATE, fmt, std::forward<Args>(args)...))->Queue();
	}

protected:
	DBThread::DBNo		m_db;
	const char* const	m_name;
	bool				m_immediate;
	std::unique_ptr<DBQuery>
						m_query;
	char				m_statement[MaxStatementLen + 1];
};

namespace DBThread
{
	//! Creates a legal SQL string for use in an SQL statement based on the
	//! character set / quoting context of the specified db connection.
	extern void EscapeString(DB::handle_t* conn, std::string& text);
	//! Creates a legal SQL string for use in an SQL statement using the
	//! default, dbh, database connection.
	//! NOTE: Not re-entrant, not thread safe, use only from main thread.
	static inline void EscapeString(std::string& text) { EscapeString(dbh, text); }

	struct Thread : public PNThread
	{
		virtual ~Thread() {}

		virtual void run() override final;
		virtual void dbrun() = 0;

		DB::handle_t* m_conn = nullptr;
	};

	// Build a query in a textAlloc and format it for me.
	template<typename... Args>
	void Do(const DBNo db_, const char* queryFormat, Args&&... args)
	{
		(new DBThreadQuery(db_, "DBThread::Do(q)", DBThreadQuery::_IMMEDIATE, queryFormat, std::forward<Args>(args)...))->Queue();
	}
	// Pre-formatted textAlloc query
	static inline void Do(const DBThread::DBNo db_, const char* queryText)
	{
		(new DBThreadQuery(db_, "DBThread::Do(q)", queryText, DBThreadQuery::_IMMEDIATE))->Queue();
	}
}
