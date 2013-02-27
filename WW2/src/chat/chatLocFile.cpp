#include <syscfg.h> // Path is project specific.
// Date: Mon Jul 30 15:54:30 CDT 2007
//
// Read the host.loc file
//
// Copyright (C) PlayNet, Inc 1999-2007 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "dbug/dbug.h"							// For host "dbugLog" access

#include <map>
#include <string>

#include "chat/chatLocFile.h"
#include "chat/chatLocalizedMessage.h"
#include "sdm/sdmConfigVars.h"

HOSTLOC g_hostMessages ;

bool
chatReadHostLocFile()
{
	std::string hostLocFile = cv_dataDir.GetString();
	hostLocFile += PATH_SEPARATOR;
	hostLocFile += "host.loc";
	FILE* fp = fopen(hostLocFile.c_str(), "rb") ;
	if ( fp == NULL )
	{
		fprintf(stderr, "Unable to read %s\n", hostLocFile.c_str()) ;
		return false ;
	}
	static const size_t bufSize = 512 ;
	char buffer[bufSize+1] ;
	while (fgets(buffer, bufSize, fp))
	{
		char* p = buffer ;
		while ( isdigit(*p) )
			++p ;
		if ( p != buffer && *p == ',' )
		{
			*(p++) = 0 ;
			int index = atoi(buffer) ;
			if ( index < 0 )
				dbugLog(_FLW, "host.loc invalid index: %s", buffer) ;
			else if ( g_hostMessages.find(index) != g_hostMessages.end() )
				dbugLog(_FLW, "duplicate host.loc index: %s", buffer) ;
			else
			{
				char* end = p ;
				while (*end && *end != '\r' && *end != '\n')
					++end ;
				*end = 0 ;
				g_hostMessages[index] = p ;
			}
		}
	}
	fclose(fp) ;

	return true ;
}

const char*
chatLocString(int i)
{
	HOSTLOC::iterator it = g_hostMessages.find(i) ;
	if ( it != g_hostMessages.end() )
		return it->second.c_str() ;
	static char buf[32] ;
	sprintf(buf, "?#%d?", i) ;
	return buf ;
}

const char*
chatLString(int prefix, int message, const char* ptr0, const char* ptr1, const char* ptr2, const char* ptr3, const char* ptr4, const char* ptr5)
{
	Chat::LocalizedMessage lmsg((LSTRING_ID)prefix, (LSTRING_ID)message, ptr0, ptr1, ptr2, ptr3, ptr4, ptr5) ;
	return chatLString(lmsg) ;
}

const char*
chatLString(const Chat::LocalizedMessage& lmsg)
{
	static std::string output ;

	output = "" ;

	if ( lmsg.m_prefix >= 0 )
	{
		output += chatLocString(lmsg.m_prefix) ;
		output += ": " ;
	}

	if ( lmsg.m_message >= LSTR_TEXT_START && lmsg.m_message <= LSTR_TEXT_END )
	{
		output += lmsg.m_ptr[0] ;
		return output.c_str() ;
	}
	else if ( lmsg.m_message >= 0 )
	{
		output += chatLocString(lmsg.m_message) ;
	}
	else
	{
		output += "<no message?>" ;
	}

	char format[5] ;
	size_t pos ;
	for ( UINT32 i = 0 ; i < Chat::LocalizedMessage::_NUM_ARGUMENTS ; ++i )
	{
		sprintf(format, "{%1u}", i + 1) ;
		while ( 1 )
		{
			pos = output.find(format) ;
			if ( pos == std::string::npos )
				break ;
			output.replace(pos, 3, (lmsg.m_ptr[i][0] ? lmsg.m_ptr[i] : "<null>")) ;
		}
	}

	return output.c_str() ;
}


