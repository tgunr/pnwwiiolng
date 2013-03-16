#pragma once
// Copyright (C) PlayNet Inc 2010
// Originating Programmer: Oliver 'kfs1' Smith <osmith@playnet.com>
//
// Abstraction layer for querying databases; supports databases
// defined in dbse/dbseHandle.h (currently MySQL, SQLite 3)
//
// Components:
//
//  DBQuery :- Class for encapsulating database queries and
//             the result sets they return.
//  DBRow   :- Class for encapsulating a single row from a
//             result set.
//
// CAUTION:
//  DBQuery does NOT provide asynchronous operations.

/*

DBQuery & DBRow API

1. Errors.

	All DBQuery functions handle non-trivial errors by throwing an exception
	derived from std::exception (#include <stdexcept>).

2. Executing SQL.

	You can execute an SQL statement either by using the main constructor:

		DBQuery query(DB::handle_t databaseHandle, const char* query);

			Attempts to construct a complete query object and execute the
			supplied SQL statement on the specified database. databaseHandle
			must be a valid and open database handle.

		Errors:
			Any runtime errors, such as invalid SQL, will result in
			an exception derived from std::exception.

	or you can create a place-holder DBQuery object for performing later
	queries without having to instantiate multiple instances of the object.

		DBQuery query(DB::handle_t databaseHandle);

			Constructs an empty query object with a database handle reference,
			which can be used with member function 'Query()' to execute SQL
			later on.

	To execute a new statement on an existing DBQuery object:

		query.Query(int stmtLength, const char* statement);
		or
		query.Query(const std::string& statement);

	CAVEAT: Result sets are stored in memory, and any locks held by the query
	may persist, until the query object destructor is called, so "static"
	DBQuery objects are strongly discouraged.

3. Result Details.

	Once a statement has been executed, the following accessor members provide
	details of the result set returned, if any:

		int query.Error()
			Returns a database-specific error if an error occurred.

		bool query.HasRows()
			'true' or 'false' as to whether any rows were returned.

		unsigned int query.Rows()	*MySQL Only*
			Returns the number of rows in the current result set.
			(Unsupported databases return MAX_INT)

		unsigned int query.Cols()
			Returns the number of columns-per-row in the current result set.

		unsigned int query.CurrentRow()
			Returns the current row index within the result set.

	If the query was a data-modifying statement (INSERT, UPDATE, etc)

		unsigned int query.AffectedRows()
			Returns the number of rows affected by the statement.

		unsigned int LastInsertID()
			If the statement generated a new automatic row ID, returns
			the id. Zero if no id was generated or the database does not
			support automatic automatic ids.

4. Retrieving rows of data.

	Rows returned by SQL queries are abstracted through the "DBRow" class and
	are retrieved row-at-a-time

		bool query.FetchRow()
			Advances to the next row in the result set and returns true.
			Returns false when the last row has been retrieved.

		DBRow& Row()
			Returns a reference to the last row retrieved by FetchRow().

5. Retrieving values (columns) from DBRow.

	The DBRow class provides an interface for translating SQL results into
	native data types. DBRow provides two methods for accessing columns,
	the first provides you raw access to the raw members via operator[],
	the second is to use the self-cursoring Next*() functions to retrieve
	specific value types.

		const char* row.NextColumn()
			Returns a pointer to the character array representation of
			the next column. If the column was NULL, returns a NULL
			pointer.

		const char* row.NextString()
			Returns a pointer to the character array representation of
			the next column. If the column was NULL, returns a pointer
			to an empty string ("").

		const char* row.NextString(char* dest, size_t destMaxLen)
			Copies the character array representation of the next
			column into "dest", upto destMaxLen bytes total.
			*Does not ensure null-byte placement on truncation*

		row.NextSINT32()
		row.NextUINT32()
		row.NextSINT64()
		row.NextUINT64()
		row.NextSINT16()
		row.NextUINT16()
		row.NextFLOAT32()
		row.NextFLOAT64()
			Interprets the next column as the specified type and
			returns the value.

		row.NextWeaponSpec(WEAPON_SPEC& dest)
			Assumes the next column is a WEAPON_SPEC value and loads
			it into 'dest'.

6. Examples

	#include <stdexcept>
	#include "dbse/dbseQuery.h"

	try
	{
		DB::handle_t dbh = _yourConnectFunction();

		// Select the current database time. If the SQL is
		// invalid or the database handle is invalid or there
		// is a problem with the query, an exception will be
		// thrown.
		DBQuery query(dbh, "SELECT NOW()");

		// We're pretty sure this should return a row.
		if ( query.HasRows() == false )
			throw std::logic_error("Database returned nothing");

		// We must retrieve the row
		if ( query.FetchRow() == false )
			throw std::logic_error("Unable to retrieve date");

		printf("Database time is: %s\n", query.Row().NextString();

		// Assuming "t_foo" is a table containing several rows,
		//  1, "one"; 2, "two", 3, "three", 4, "four"
		const char* statement = "SELECT id, label FROM t_foo";
		query.Query(strlen(statement), statement);

		if ( Query.HasRows() == false )
			throw std::logic_error("Nothing in t_foo");

		while ( query.FetchRow() == true )
		{
			const DBRow& row = query.Row();

			UINT32 id = row.NextUInt();
			const char* label = row.NextString();
			printf("Native: id %u, label %s\n", id, label);

			printf("char*: id %s, label %s\n", row[0], row[1]);
		}
	}
	catch (std::exception& e)
	{
		printf("ERROR: %s\n", e.what());
	}
*/

#include "dbse/dbseHandle.h"

// If you want to execute a really simple query and you don't
// want the result set, don't care how long it takes, and
// don't care whether it works or not, use DBSimpleQuery
#define DBSimpleQuery(dbh, ...)	{ try { DBQuery __dbsq(dbh, __VA_ARGS__); } catch (std::exception& e) { dbugLog(_FLA, "%s: %s", __FUNCTION__, e.what()); } }

//////////////////////////////////////////////////////////////////////
// Definition of the 'row' type, which is basically a wrapper for
// dereferencing a row

class DBRow
{
public:
	// Overloaded array operator to deref from m_row
	const char*	operator[](UINT32 index) const
	{
		if ( index >= m_columns )
			throw std::invalid_argument("index out of bounds");

		return m_row[index];
	};

	// Can return null
	const char* NextColumn()
	{
		if ( m_currentColumn >= m_columns )
			return NULL;
		return m_row[m_currentColumn++];
	}

	// Always returns a valid character array pointer
	const char* NextString()
	{
		const char* p = NextColumn();
		return ( p ) ? p : "";
	}

	// NOTE: Does not ensure null-byte placement
	const char* NextString(char* destination, size_t maxLen);

	SINT32 NextSInt()
	{
		if ( m_currentColumn >= m_columns ) return 0;
		const char* source = m_row[m_currentColumn++];			// Null record protection
		return ( source != NULL ) ? (SINT32)strtol(source, NULL, 10) : 0;
	}

	UINT32 NextUInt()
	{
		if ( m_currentColumn >= m_columns ) return 0;
		const char* source = m_row[m_currentColumn++];			// Null record protection
		return ( source != NULL ) ? (UINT32)strtoul(source, NULL, 10) : 0;
	}

	UINT64 NextUINT64()
	{
		if ( m_currentColumn >= m_columns ) return 0;
		const char* source = m_row[m_currentColumn++];
		return ( source != NULL ) ? strtoull(source, NULL, 10) : 0;
	}

	SINT64 NextSINT64()
	{
		if ( m_currentColumn >= m_columns ) return 0;
		const char* source = m_row[m_currentColumn++];
		return ( source != NULL ) ? strtoll(source, NULL, 10) : 0;
	}

	FLOAT32 NextFLOAT32()
	{
		if ( m_currentColumn >= m_columns ) return 0.0;
		const char* source = m_row[m_currentColumn++];
		return ( source != NULL ) ? (FLOAT32)strtof(source, NULL) : 0.0f;
	}

	FLOAT64 NextFLOAT64()
	{
		if ( m_currentColumn >= m_columns ) return 0.0;
		const char* source = m_row[m_currentColumn++];
		return ( source != NULL ) ? (FLOAT64)strtod(source, NULL) : 0.0;
	}

	void NextWeaponSpec(struct WEAPON_SPEC& spec);

protected:
	// Constructor
	DBRow(UINT32 columns, DB::result_t* result);
	friend class DBQuery;					// But DBQuery can access our constructor.

private:
	DB::row_t		m_row;					// The data
	UINT32			m_columns;				// Number of columns
	UINT32			m_currentColumn;		// Current column
};

//////////////////////////////////////////////////////////////////////
// Definition of a query, which is basically a wrapper for performing
// a query, with cleanup. This also provides you with fetchRow()
// to iterate over rows that the result returned -- if any.

class DBQuery
{
public:
	// Constructor (with query)
	DBQuery(DBHANDLE&, const char* query, ...);
	// Constructor with prepared query
	DBQuery(DB::handle_t*, const char* query);
	// Constructor (without query)
	DBQuery(DB::handle_t*);
	// Destructor
	~DBQuery();

	// Perform another query
	void Query(size_t stlen, const char* statement);
	void Query(const std::string& qry) { Query(qry.length(), qry.c_str()); }

	// Access to result fields
	int				Error() const		{ return m_error; }

	public: bool HasRows() const { return m_rows > 0 && m_nextRow < m_rows; }

	bool			FetchRow();

	UINT32			AffectedRows() const { return m_affectedRows; }
	UINT32			Cols() const		{ return m_cols; }
	UINT32			CurrentRow() const	{ return m_currentRow; }
	UINT32			LastInsertID() const { return m_lastInsertID; }

	DBRow&	Row() const
	{
		if ( m_row != NULL ) return const_cast<DBRow&>(*m_row);
		throw std::logic_error("No row to return");
	}

	bool NextResultSet();
	UINT32			Rows() const		{ return m_rows; }

private:
	DB::handle_t*	m_conn;				// Database connection handle
	DB::result_t*	m_result;				// The query result or statement

	bool			m_executed : 1;		// FALSE until we try the command
	int				m_error;				// Error No of any error

	UINT32			m_affectedRows;		// NUmber of rows modified
	UINT32			m_cols;				// Number of columns
	UINT32			m_lastInsertID;		// Last insert ID if we inserted

	UINT32			m_currentRow;			// 0-based cursor
	UINT32			m_nextRow;				// 1-based cursor

	UINT32			m_rows;				// Number of rows returned

private:
	DBRow*			m_row;

	// Private member functions
	void			_finish(bool thorough);
	void			_processResultSet();
	void			_query(const char* statement, size_t stlen);

};
