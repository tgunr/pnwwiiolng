#include <syscfg.h> // Path is project specific.
// Created: Mon Oct 22 13:19:34 CDT 2007
//
// Provides an ordered queue of queries to be sent to the
// audit database
//
// Copyright (C) PlayNet, Inc 1999-2007 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "wwii/wwiiAuditDB.h"				// Our header file

typedef ConditionQueue<char*> AUDIT_QUEUE ;
static AUDIT_QUEUE auditQueue ;

//////////////////////////////////////////////////////////////////////
// Audit DB thread function

static void*
_auditDBThreadFn(void* p)
{
	DB::thread_t* conn = NULL ;
	threadLog(_FLI, "auditThread active") ;
	char statement[TEXT_POOL_SIZE+1] ;

	while ( 1 )
	{
		try
		{
			statement[0] = 0 ;
			if ( !auditQueue.tryGetnext(statement) )
			{
				sleep(1) ;
				continue ;
			}
			if ( conn == NULL )
				conn = threadsOpenDBConnection(true, "audit") ;
			if ( conn == NULL )
				throw std::logic_error("Unable to open database connection") ;

			DBQuery q(conn, statement) ;
		}
		catch ( std::exception& e )
		{
			threadLog(_FLA, "statusThread: %s", e.what()) ;
			if ( conn )
			{
				mysql_close(conn) ;
				conn = NULL ;
			}
			sleep(5) ;
		}
	}
	return p ;
}

//////////////////////////////////////////////////////////////////////
// Establish the audit DB thread
