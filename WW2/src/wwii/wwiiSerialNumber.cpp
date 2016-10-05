#include <syscfg.h> // Path is project specific.
// Created: Thu Mar 16 13:31:56 CST 2006
//
// Class that defines a database-retained incremental resource
// counter like serial numbers or mission ids
//
// Copyright (C) PlayNet, Inc 1999-2006 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "wwii/wwiiSerialNumber.h"				// Our header file
#include "wwii/wwiiDBThreads.h"
#include "include/mydbQuery.h"

extern DBHANDLE dbh;

SerialNumber::serialno_t
SerialNumber::getNext()
{
	bool isFirst = false;

    if ( m_counter == 0 )
    {
        try
        {
			isFirst = true;

            DBQuery q(dbh, "SELECT last_serial FROM wwii_serial_numbers WHERE resource = '%s'", m_name);
            if ( q.Rows() && q.FetchRow() )
            {
                m_counter = (serialno_t)q.Row().NextUINT64() + m_interval;
				dbugLog(_FLI, "SerialNumber(%s) starting from %llu", m_name, (UINT64)m_counter);
            }
			else
			{
				dbugLog(_FLI, "SerialNumber(%s) starting from default (%llu)", m_name, (UINT64)m_minimum);
			}
        }
        catch (std::exception& e)
        {
            LogAbort("Unable to get initial %s number: %s", m_name, e.what());
        }
    }

    if ( isFirst || (m_counter % m_interval) == 0 )
    {
        DBThread::Do(DBThread::Default
						, "REPLACE INTO wwii_serial_numbers (resource, last_serial) VALUES ('%s', %llu)"
                            , m_name, m_counter
							);
    }
	++m_counter;
	if ( m_maximum > 0 && ( m_counter >= m_maximum || m_counter <= m_minimum ) )
		m_counter = m_minimum;

    return m_counter;
}
