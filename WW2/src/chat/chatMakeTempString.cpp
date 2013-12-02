#include <syscfg.h> // Path is project specific.
// Date: Wed Oct  5 14:21:32 CDT 2005
//
// Build a temporary string, basically a wrapper for sprintf that
// helps us avoid having to continually allocate new buffers
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include <stdarg.h>

#include "chat/chatDefines.h"

static char buffer[4][MAX_MESSAGE_LENGTH+1];

//////////////////////////////////////////////////////////////////////
// Build a temporary string, basically a wrapper for sprintf that
// helps us avoid having to continually allocate new buffers

const char*
chatMakeTempString(const char* fmt, ...)
{
	va_list args;

	static int bufNo = 0;
	if ( bufNo > 3 )
		bufNo = 0;

	buffer[bufNo][0] = 0;

	if ( fmt != NULL )
	{
		va_start(args, fmt);
		vsnprintf(buffer[bufNo], MAX_MESSAGE_LENGTH, fmt, args);
		va_end(args);
	}

	return buffer[bufNo++];
}
