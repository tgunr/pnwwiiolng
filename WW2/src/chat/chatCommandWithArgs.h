#pragma once
// Created: Fri Jun 26 14:23:56 CDT 2009
//
// Provides a class that encapsulates pointers to
// the command and argument portions of a command line.
// Performs wide-character aware whitespace pruning,
// removal and translation to sincel space characters.
// Command is translated to lowercase.
//
// Copyright (c) 2009 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Oliver "kfsone" Smith

#include <wctype.h>
#include "wwii/wwiiUTF.h"
#include "chat/chatDefines.h"

namespace Chat
{

class CommandWithArgs
{

	// Accessors
	public:	char* Command() { return m_input ; }
	public:	char* Args() { return m_args ; }

	public: CommandWithArgs(char* input)
		: m_input(input)
		, m_args(nullptr)
	{
		assert( m_input != nullptr ) ;
		// Translate the input into wide characters.
		wchar_t wide[MAX_MESSAGE_LENGTH] ;
		WWII::ConvertChars(m_input, wide, sizeof(wide)) ;

		//// Extract the first word (command)

		// Traverse leading whitespace characters.
		wchar_t* wp = wide ;			// 'wp' points to start of command
		while ( *wp && iswspace(*wp) )
			++wp ;

		// Force lowercase of everything up to the next whitespace
		wchar_t* wsrc = wp ;
		while ( *wsrc && !iswspace(*wsrc) )
		{
			*wsrc = towlower(*wsrc) ;
			++wsrc ;
		}
		if ( *wsrc )
		{
			*(wsrc++) = 0 ;
		}

		// Translate back to UTF8 in the "command" space.
		WWII::ConvertChars(wp, m_input, MAX_MESSAGE_LENGTH) ;
		size_t cmdLen = strlen(m_input) ;

		// Skip whitespace between the command and any arguments.
		while ( *wsrc && iswspace(*wsrc) )
			++wsrc ;

		//// Cleanup any arguments

		// Force whitespace combining as spaces and trailing whitespace elimination.
		wp = wide ;						// 'wp' is now acting as a 'dest' pointer.
		*wp = 0 ;
		while ( *wsrc )
		{
			if ( iswspace(*wsrc) )
			{
				do
				{
					++wsrc ;
				} while ( *wsrc && iswspace(*wsrc) ) ;
				if ( *wsrc )
					*(wp++) = ' ' ;
			}
			else
				*(wp++) = *(wsrc++) ;
		}
		*wp = 0 ;

		m_args = m_input + cmdLen + 1 ;
		if ( *wide == 0 )
			*m_args = 0 ;
		else
			WWII::ConvertChars(wide, m_args, MAX_MESSAGE_LENGTH - cmdLen) ;
	}

private:
	char*	m_input ;
	char*	m_args ;
} ;

}

