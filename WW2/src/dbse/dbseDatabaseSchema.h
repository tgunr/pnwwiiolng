#pragma once

class DBHANDLE;

namespace DB
{
using Revisions = std::vector< std::vector<const char*>>;

//! @class DB::Schema
//! @brief Auto-manages database schema changes.
//! Revisions are stored in-order a DB::Revisions vector.
//! The current revision of the schema is tracked in a table
//! called schema_info. When new revisions are compiled
//! in to a server binary, this table will be updated as they
//! are applied.

class Schema
{
public:
	//! Constructor with name of the database (schema) to operate in.
	//! @param[in] dbh_ Database handle to work on.
	//! @param[in] database_ Name of the database containing schema_info table.
	Schema(DBHANDLE& dbh_, const char* const database_);

	//! Perform any updates that this database has not yet seen.
	//! @param[in] history_ Ordered list of revisions to the database.
	void Update(const DB::Revisions& history_);

	// Since visual studio doesn't yet support vector initializers,
	// I had to flatten the schema list into a null-seperated list of
	// revisions.
	template<size_t size>
	void Update(const char* (&revisionArray)[size])
	{
		DB::Revisions revs;
		std::vector<const char*> curRevision;
		for ( size_t i = 0; i < size; ++i ) {
			if ( revisionArray[i] == nullptr ) {
				if ( !curRevision.empty() )
					revs.push_back(curRevision);
				curRevision.clear();
			} else {
				curRevision.push_back(revisionArray[i]);
			}
		}
		if ( !curRevision.empty() )
			revs.push_back(curRevision);
		Update(revs);
	}

	Schema& operator=(const Schema& rhs) = delete;	// Do not use!

private:
#if 0
	class DBHANDLE& m_dbh;
	const char* const m_database;
	unsigned int m_schemaVersion;
#endif
};

}

