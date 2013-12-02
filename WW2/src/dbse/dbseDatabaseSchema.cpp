#include <syscfg.h> // Path is project specific.
#include "dbug/dbug.h"
#include "dbse/dbseDatabaseSchema.h"
#include "dbse/dbseWrapper.h"
#include "include/mydbQuery.h"

namespace DB
{

static const char* const SchemaTable = "schema_info";
static const char* const SchemaVersion = "version_no";

Schema::Schema(DBHANDLE& dbh_, const char* const database_)
	: m_dbh(dbh_)
	, m_database(database_)
	, m_schemaVersion(0)
{
}

static unsigned int
_getDatabaseSchema(DBHANDLE& dbh, const char* const database)
{
	try
	{
		// Fetch the highest revision count.
		DBQuery q(dbh, "SELECT MAX(`%s`) FROM `%s`.`%s` LIMIT 1"
						, SchemaVersion
						, database
						, SchemaTable
						);

		// Allow for an unpopulated table.
		if ( q.HasRows() == true && q.FetchRow() == true )
		{
			// Return 1-based result for size compatability and
			// to distinguish from 0 (none).
			return q.Row().NextUInt() + 1;
		}
	}
	catch ( std::exception& e )
	{
		dbugLog(_FLI, "dbschema: unreadable/uninitialized: %s", e.what());
	}

	return 0;
}


void
Schema::Update(const DB::Revisions& history)
{
	const unsigned int latestRevision = history.size();
	const unsigned int dbRevision = _getDatabaseSchema(m_dbh, m_database);

	if ( dbRevision == latestRevision )
	{
		dbugLog(_FLI, "dbschema: database is upto date at revision %u", dbRevision);
		return;
	}

	if ( dbRevision > latestRevision )
	{
		dbugLog(_FLM, "dbschema: database has more recent schema (%u) than host (%u)", dbRevision, latestRevision);
		return;
	}

	dbugLog(_FLN, "dbschema: database revision %u, updating to revision %u", dbRevision, latestRevision);

	unsigned int updates = 0;
	unsigned int currentRevision = dbRevision;

	try
	{
		for (; currentRevision < latestRevision; ++currentRevision )
		{
			if ( history[currentRevision].empty() || history[currentRevision][0] == NULL )
				continue;

			dbugLog(_FLI, "dbschema: applying revision #%u", currentRevision);
			unsigned int entry = 0;
			for ( auto it = history[currentRevision].begin(); it != history[currentRevision].end(); ++it, ++entry )
			{
				if ( *it == NULL )
					continue;
				dbugLog(_FLD, "dbschema:%u/%u: %s", currentRevision, entry, *it);
				DBQuery q(m_dbh, "%s", *it);
			}

			DBQuery(m_dbh, "INSERT IGNORE INTO `%s`.`%s` (`%s`) VALUES (%u)"
							, m_database, SchemaTable, SchemaVersion, currentRevision);

			++updates;
		}
	}
	catch ( std::exception& e )
	{
		char message[4096];
		snprintf(message, 4095, "FATAL ERROR: Unable to apply `%s` database schema revision %u: %s.", m_database, currentRevision, e.what());
//		wwiiIRCMessage("-sys,#servers", message);
		LogAbort("dbschema: %s", message);
		abort();
	}

	dbugLog(_FLN, "dbschema: `%s` updated from %u to revision %u (%u updates)"
					, m_database, dbRevision, latestRevision, updates);
}

} // namespace

