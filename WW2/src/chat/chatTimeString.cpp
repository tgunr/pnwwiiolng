#include <syscfg.h> // Path is project specific.
// Date: Fri Aug 11 16:54:44 CDT 2006
//
// Returns a pointer to a string of a given time
//
// Copyright (C) PlayNet, Inc 1999-2006 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include <time.h>

const char*
chatTimeString(time_t timestamp)
{
	static const UINT32 bufSize = 30;
	static char timeStr[bufSize+1];

	struct tm tm; (void)tm;
	strftime(timeStr, bufSize, "%a %b %d %T %Z", localtime_r(&timestamp, &tm));

	return timeStr;
}
