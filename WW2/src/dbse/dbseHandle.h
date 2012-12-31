#pragma once
// Created: Fri Mar 20 13:18:58 CST 2009
// Copyright (C) PlayNet, Inc 2009-2011 - All rights reserved.
// Not for redistribution.
//
// Describes the DBHANDLE class, the wrapper around
// the MYSQL connection used by the dbse daemon.
// Overloaded operator allows it to be used like a
// MYSQL* pointer.

typedef struct st_mysql_res MYSQL_RES ;
typedef struct st_mysql MYSQL ;

namespace DB
{
	// DB::handle_t is a database connection handle,
	// DB::result_t is a a query or result set handle,
	// DB::row_t is the handle required to read columns

	typedef MYSQL handle_t ;
	typedef MYSQL_RES result_t ;
	typedef char**	row_t ;
}

//The database connection information
class DBHANDLE
{
	bool m_initialized ;
	DB::handle_t* m_conn;

public:
	DBHANDLE() : m_initialized(false), m_conn(NULL) {}
	~DBHANDLE() { disconnect() ; }

public:
	bool connected() const { return m_initialized ; }

public:
	//! Connect to a given database, returns true if successful.
	bool connect(const struct DBCONNECTION& connectionInfo) ;

	//! Close connection to the given database.
	void disconnect() ;

public:
	operator const DB::handle_t* () const { return m_initialized ? m_conn : NULL ; }
	operator DB::handle_t* () { return m_initialized ? m_conn : NULL ; }
} ;

