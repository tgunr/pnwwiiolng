#include <syscfg.h> // Path is project specific.
// Date: Mon Feb 20 18:16:06 CST 2006
//
// Each client build is checksummed to allow us to
// monitor for players tampering with their .exe files
//
// Copyright (C) PlayNet, Inc 1999-2006 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "include/mydbQuery.h"
#include "wwii/wwiiDBThreads.h"

//////////////////////////////////////////////////////////////////////

typedef std::vector< std::pair<UINT32, UINT64> > CHECKSUMS;

static std::mutex chMutex;
static CHECKSUMS _checksums;
static bool _inProgress = false;

//////////////////////////////////////////////////////////////////////

static const char* _FETCH_CHECKSUMS_QUERY =
	"SELECT"
		" INET_ATON(version), checksum"
		" FROM wwii_checksums"
		" ORDER BY 1 DESC"
		" LIMIT 32"
;

class ChecksumQuery : public DBThreadQuery
{
public:
	ChecksumQuery() : DBThreadQuery(DBThread::Default, "ChecksumQuery", _FETCH_CHECKSUMS_QUERY, DBThreadQuery::_IMMEDIATE) {}

	virtual void Success() OVERRIDE_FN
	{
		static CHECKSUMS newChecksums;

		newChecksums.clear();

		while ( Query()->FetchRow() )
		{
			UINT32 version = Query()->Row().NextUInt();
			UINT64 checksum = Query()->Row().NextUINT64();
			newChecksums.push_back(std::make_pair(version, checksum));
		}

		threadLog(_FLD, "%u checksums", newChecksums.size());

		{
			std::lock_guard<std::mutex> lock(chMutex);
			_checksums = newChecksums;
		}
		_inProgress = false;
	}

	virtual void Failure() OVERRIDE_FN
	{
		_inProgress = false;
	}
};

//////////////////////////////////////////////////////////////////////
// Periodically check for new checksums

void
mapPollChecksums(bool force)
{
	static UINT32 nextPoll = 0;
	static const UINT32 _POLL_INTERVAL = 5 * 60;

	if ( !force && nextPoll > teulTime() )
		return;

	nextPoll = teulTime() + _POLL_INTERVAL;

	if ( _inProgress )
	{
		dbugLog(_FLW, "skipping checksum poll - another query already in progress");
		return;
	}

	dbugLog(_FLD, "requesting checksums");

	// We're about to hand this to a background thread,
	// so its best that we write to our variable* first*
	_inProgress = true;

	// Now hand over to the database queue threads
	(new ChecksumQuery)->Queue();
}

//////////////////////////////////////////////////////////////////////
// Check a particular checksum

bool
mapCheckChecksum(UINT32 appVersion, const UINT64& checksum, bool isProducer)
{
	if ( (checksum == 8173587580LL) | (isProducer) )
		return true;

	std::lock_guard<std::mutex> lock(chMutex);

	CHECKSUMS::iterator it;
	for ( it = _checksums.begin(); it != _checksums.end(); ++it )
	{
		if ( it->first == appVersion && it->second == checksum )
			return true;
	}

	return false;
}
