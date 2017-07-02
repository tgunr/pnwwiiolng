#include "syscfg.h" // Path is project specific.
// Date: Tue Oct  4 12:36:49 CDT 2005
//
// Function for converting a time_t into a time interval string
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "str/stringLib.h"

#include "teul/teulTime.h"

//////////////////////////////////////////////////////////////////////
// Display a h/m/s time interval

const char*
chatTimeInterval(unsigned int interval)
{
	static const int STR_SIZE = 64;
	static const int SUB_SIZE = 48;
	static char str[STR_SIZE+1];
	static char sub[SUB_SIZE+1];

	if ( PN_UNLIKELY(interval <= 0) )
		return "moments";

	int days, hours, minutes, seconds;
	days = interval / DAY_S;
	hours = (interval % DAY_S) / HOUR_S;
	minutes = (interval % HOUR_S) / MINUTE_S;
	seconds = (interval % MINUTE_S);

	str[0] = 0;
	sub[0] = 0;

	if ( days > 1 )
	{
		strprintf(str, "%u days", (unsigned int)days);
		return str;
	}
	else if ( days == 1 )
	{
		if ( hours == 0 )
			return "1 day";
		minutes = seconds = 0;
	}

	if ( hours > 0 )
	{
		if ( minutes > 0 || seconds > 0 )
			strprintf(sub, "%02uh%02um%02u", (unsigned int)hours, (unsigned int)minutes, (unsigned int)seconds);
		else if ( hours > 1 )
			strprintf(sub, "%u hours", (unsigned int)hours);
		else
			strsafecpy(sub, "1 hour");
	}
	else if ( minutes > 0 )
	{
		if ( seconds > 0 )
			strprintf(sub, "%02um%02u", (unsigned int)minutes, (unsigned int)seconds);
		else if ( minutes > 1 )
			strprintf(sub, "%u minutes", (unsigned int)minutes);
		else
			strsafecpy(sub, "1 minute");
	}
	else if ( seconds > 0 )
	{
		if ( seconds > 1 )
			strprintf(sub, "%u seconds", (unsigned int)seconds);
		else
			strsafecpy(sub, "1 second");
	}

	// 1 day X hours
	if ( days == 1 && sub[0] != 0 )
	{
		strprintf(str, "1 day %s", sub);
		return str;
	}

	return sub;
}

