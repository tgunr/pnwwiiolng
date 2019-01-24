#pragma once
// Created: Thu May 18 11:20:59 2000
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// The header file for the Database Wrapper stuff
//
// Originating Programmer: Shane "Elmo" Wickson

#include <mysql.h>

namespace DB
{
	// DB::handle_t is a database connection handle,
	// DB::result_t is a a query or result set handle,
	// DB::row_t is the handle required to read columns

	using handle_t = MYSQL;
	using result_t = MYSQL_RES;
	using row_t = char**;
}

//The database connection information
class DBHANDLE
{
	bool m_initialized;
	DB::handle_t* m_conn;

public:
	DBHANDLE() : m_initialized(false), m_conn(nullptr) {}
	~DBHANDLE() { disconnect(); }

public:
	bool connected() const { return m_initialized; }

public:
	//! Connect to a given database, returns true if successful.
	bool connect();

	//! Close connection to the given database.
	void disconnect();
	
	//! Reconnect to the database unconditionally
	bool reconnect();

public:
	operator const DB::handle_t* () const { return m_initialized ? m_conn : nullptr; }
	operator DB::handle_t* () { return m_initialized ? m_conn : nullptr; }
	operator bool() const noexcept { return m_initialized && m_conn;  }
};

extern DBHANDLE dbh;

//Connects to all the databases that are passed to it.
//The first parameter is an array of pointers to connection structs, and the
//	second parameter is the number of structures to use out of that array

DB::handle_t* dbseGetConnection();

const char* escapeString(const char* src, char* dest, size_t destSize);
