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

//////////////////////////////////////////////////////////////////////
// Base virtual class, not intended to be used directly
// Specify _IMMEDIATE if the success() or failure() functions are
// thread safe and can be processed as soon as the query has been
// atempted.
// Specify _CALLBACK to have the success() or failure() functions
// invoked via a callback in the main thread.

class DBThreadQuery
{
public:
	enum { _IMMEDIATE = true, _CALLBACK = false } ;
	enum { MaxStatementLen = 4096 } ;

	DBThreadQuery(const DBThread::DBNo db_, const char* const name_, const char* const statement_, bool immediate)
		: m_db(db_), m_name(name_), m_immediate(immediate), m_query(NULL)
		{
			strcpy(m_statement, statement_) ;
		}

	DBThreadQuery(const DBThread::DBNo db_, const char* const name_, bool immediate, const char* const fmt_, ...)
		: m_db(db_), m_name(name_), m_immediate(immediate), m_query(NULL)
		{
			va_list args ;
			va_start(args, fmt_) ;
			vsetStatement(args, fmt_) ;
			va_end(args) ;
		}
	DBThreadQuery(const DBThread::DBNo db_, const char* const name_, bool immediate, va_list& args, const char* const fmt_)
		: m_db(db_), m_name(name_), m_immediate(immediate), m_query(NULL)
		{
			vsetStatement(args, fmt_) ;
		}

	void vsetStatement(va_list& args, const char* const fmt_)
	{
		int bytes = vsnprintf(m_statement, MaxStatementLen, fmt_, args) ;
		m_statement[bytes] = 0 ;
	}

	virtual ~DBThreadQuery() ;

public:
	const char* Statement() const { return m_statement ; }
	const char* Name() const { return m_name ; }
	DBQuery* Query() const { return m_query ; }

	bool Execute(UINT32 threadNo, MYSQL* conn, DBThread::DBNo*) ;
	void Queue() ;

	//! Called if query succeeds.
	virtual void Success() {}
	//! Called if query fails.
	virtual void Failure() {}
	//! Called before success/failure;
	//! after this, the query is deleted.
	virtual bool Process(const size_t /*numRows*/) { return false ; }

protected:
	DBThread::DBNo		m_db ;
	const char* const	m_name ;
	bool				m_immediate ;
	DBQuery*			m_query ;
	char				m_statement[MaxStatementLen + 1] ;
} ;

