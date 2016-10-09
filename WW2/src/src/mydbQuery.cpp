#include <syscfg.h> // Path is project specific.
//
// Created: Jan 12 15:03:35 2004
//
// C++ wrapper class for MySQL queries
//
// Copyright (c) 2004 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Oliver "kfsone" Smith
// Supporting Programmer:  Marty "Vandal" Poulin

#include <stdexcept>

#include "include/mydbQuery.h"

#include <mysql.h>

#include "errmsg.h"
#include "wwii/wwiiString.h"
#include "weap/weapSpec.h"

using namespace std;

//////////////////////////////////////////////////////////////////////
// Definitions for DBRow
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
// Constructor
DBRow::DBRow(UINT32 columns, DB::result_t* result) :
	m_row(nullptr),
	m_columns(columns),
	m_currentColumn(0)
{
	if ( columns == 0 )
		throw std::domain_error("Tried to create 0 column row");
	if ( result == nullptr )
		throw std::domain_error("NULL result pointer");

	m_row = mysql_fetch_row(result);
	if ( m_row == nullptr )
		throw std::logic_error("fetch_row returned null");
}

//////////////////////////////////////////////////////////////////////
// Little helper function
const char*
DBRow::NextString(char* destination, size_t size)
{
	if ( destination == nullptr )
		throw std::invalid_argument("null destination for nextString");

	destination[0] = 0;
	if ( m_currentColumn < m_columns && size > 0 )
	{
		const char* source = NextColumn();
		if ( source != nullptr )
			strlcpy(destination, source, size);
	}

	return destination;
}

//////////////////////////////////////////////////////////////////////
// Definitions for DBQuery
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
// Clear out the current variables. Set "thorough" to false if you
// only want to clean up the basics

void
DBQuery::_finish(bool thorough)
{
	// Clean up the row object
	if ( m_row != nullptr )
	{
		delete m_row;
		m_row = nullptr;
	}

	// Clean up the MySQL result
	if ( m_result )
	{
		mysql_free_result(m_result);
		m_result = nullptr;
	}

	if ( thorough )
	{
		m_executed = false;
		m_error = 0;
		m_rows = m_affectedRows = m_cols = m_lastInsertID = 0;
		m_currentRow = m_nextRow = 0;
	}
}

#ifdef DB_USE_SQLITE
//////////////////////////////////////////////////////////////////////
// Throw an error for an sqlite3 connection

static void
_throwSqlite3Error(DB::handle_t* handle)
{
	std::string error = sqlite3_errmsg(m_conn);
	throw std::logic_error(error);
}
#endif

//////////////////////////////////////////////////////////////////////
// Check for a result set and make a smart decision as to whether
// to call _handleResult or _handleNoResult

void
DBQuery::_processResultSet()
{
	if ( m_row != nullptr )
	{
		delete m_row;
		m_row = nullptr;
	}

	m_result = mysql_store_result(m_conn);
	if ( m_result != nullptr )
	{
		m_rows = (UINT32)mysql_num_rows(m_result);
		m_affectedRows = 0;
		m_cols = (UINT32)mysql_num_fields(m_result);
		return;
	}

	// Should there have been a result?
	if ( mysql_field_count(m_conn) != 0 )
	{
		m_error = mysql_errno(m_conn);
		if ( m_error != 0 )
		{
			std::string err = "MySQL error retrieving results from query: ";
			err += mysql_error(m_conn);
			throw std::logic_error(err);
		}
		else
			throw std::logic_error("Failed to retrieve result set");
	}

	m_rows = m_affectedRows = (UINT32)mysql_affected_rows(m_conn);
	m_lastInsertID = (UINT32)mysql_insert_id(m_conn);
}

//////////////////////////////////////////////////////////////////////
// Perform/wrap the underlying mysql query for a given statement

void
DBQuery::_query(const char* statement, size_t stlen)
{
	// We need to find the first non-space characters in the query
	while ( stlen > 0 && *statement && isspace(*statement) )
	{
		++statement;
		--stlen;
	}

	if ( *statement == 0 || stlen == 0 )
		throw std::logic_error("empty statement");

	UINT32 queryResult = mysql_real_query(m_conn, statement, stlen);
	if ( queryResult != CR_SERVER_GONE_ERROR && queryResult != CR_SERVER_LOST )
		m_executed = true;

	if ( queryResult != 0 )
	{
		// Failed
		m_error = mysql_errno(m_conn);
		std::string _lastQueryError = (const char*)mysql_error(m_conn);

		throw std::logic_error(_lastQueryError);
	}

	_processResultSet();
}

//////////////////////////////////////////////////////////////////////
// Constructor (with a query)

DBQuery::DBQuery(DBHANDLE& conn, const char* query, ...) :
	m_conn(conn),
	m_result(nullptr),
	m_executed(false),
	m_error(0),
	m_affectedRows(0),
	m_cols(0),
	m_lastInsertID(0),
	m_currentRow(0),
	m_nextRow(0),
	m_rows(0),
	m_row(nullptr)
{
	if ( m_conn == nullptr )
		throw std::domain_error("NULL conn");

	if ( query == nullptr )
		throw std::invalid_argument("query == NULL");

	while (*query && isspace(*query))
		query++;

	if ( *query == 0 )
		throw std::invalid_argument("query is empty");

	char statement[4096+1];
	int stlen = -1;

	va_list args;
	va_start(args, query);
	stlen = vsnprintf(statement, sizeof(statement)-1, query, args);
	va_end(args);

	if ( stlen == -1 || stlen >= (int)(sizeof(statement) - 1) )
	{
		statement[80] = 0;
		sprintf(statement + 80, "... SQL Query > %" PRSizeT " bytes long", sizeof(statement)-1);
		throw std::logic_error(statement);
	}

	_query(statement, stlen);
}

//////////////////////////////////////////////////////////////////////
// Constructor (with a prepared query)
DBQuery::DBQuery(DB::handle_t* conn, const char* query) :
	m_conn(conn),
	m_result(nullptr),
	m_executed(false),
	m_error(0),
	m_affectedRows(0),
	m_cols(0),
	m_lastInsertID(0),
	m_currentRow(0),
	m_nextRow(0),
	m_rows(0),
	m_row(nullptr)
{
	if ( m_conn == nullptr )
		throw std::domain_error("null connection");

	size_t len = strlen(query);
	if ( len == 0 )
		throw std::invalid_argument("empty query");

	_query(query, len);
}

//////////////////////////////////////////////////////////////////////
// Constructor (without a query)
DBQuery::DBQuery(DB::handle_t* conn) :
	m_conn(conn),
	m_result(nullptr),
	m_executed(false),
	m_error(0),
	m_affectedRows(0),
	m_cols(0),
	m_lastInsertID(0),
	m_currentRow(0),
	m_nextRow(0),
	m_rows(0),
	m_row(nullptr)
{
	if ( m_conn == nullptr )
		throw std::domain_error("dbh has null connection[0]");
}

//////////////////////////////////////////////////////////////////////
// Simple "ask another" query function

void
DBQuery::Query(size_t stlen, const char* statement)
{
	_finish(true);
	_query(statement, stlen);
}

//////////////////////////////////////////////////////////////////////
// Destructor
DBQuery::~DBQuery()
{
	_finish(false);

	// Drain any unconsumed result sets
	if ( m_conn != nullptr )
	{
		while ( mysql_next_result(m_conn) == 0 )
		{
			m_result = mysql_store_result(m_conn);
			if ( m_result != nullptr )
			{
				mysql_free_result(m_result);
				m_result = nullptr;
			}
		}
	}

	m_conn = nullptr;
}

//////////////////////////////////////////////////////////////////////
// Iterator for rows.
bool
DBQuery::FetchRow()
{
	// Conditions under which we shouldn't be getting called
	if ( !m_executed )
		throw std::logic_error("No query executed to fetch rows from");
	if ( m_error )
		throw std::logic_error("Can't fetch rows, query failed");

	if ( m_row )
	{
		delete m_row;
		m_row = nullptr;
	}

	// Have we reached the end of the data, or is the result
	// empty columns?
	if ( HasRows() == false || m_cols == 0 )
	{
		return false;
	}

	try
	{
		m_row = new DBRow(m_cols, m_result);
		m_nextRow++;
		m_currentRow = m_nextRow - 1;
	}
	catch (std::logic_error& e)
	{
		m_error = mysql_errno(m_conn);
		throw std::logic_error(e.what());
	}

	// Having succeeded, we now need to return a pointer to our
	// row member so that the caller can use it to deref, etc.
	return true;
}

bool
DBQuery::NextResultSet()
{
	_finish(true);

	int status = mysql_next_result(m_conn);
	if ( status > 0 )
		throw std::logic_error("Error fetching next result set");
	if ( status < 0 )
		return false;

	m_executed = true;

	_processResultSet();

	return true;
}

//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

void
DBRow::NextWeaponSpec(WEAPON_SPEC& spec)
{
	spec.wCountry = (UCHAR)NextUInt();
	spec.wCategory = (UCHAR)NextUInt();
	spec.wClass = (UCHAR)NextUInt();
	spec.wType = (UCHAR)NextUInt();
}

