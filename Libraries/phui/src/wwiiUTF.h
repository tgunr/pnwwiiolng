#pragma once
// wwiiUTF.h
//
// Date: Wed Sep  3 20:59:47 CDT 2008
//
// Copyright (C) PlayNet, Inc 1999-2008 - All rights reserved.
// Not for redistribution.
// 
// Provides WWII::ConvertChars.
// 
// Example Usage:
//    char characters[80] = "Hello World" ;
//    wchar_t wideCharacters[80] = "" ;
//
//    // Convert the ASCII string in characters to wchar_ts in wideCharacters
//    bool success = WWII::ConvertChars(characters, wideCharacters, sizeof(wideCharacters)) ;
//    if ( !success ) throw "First conversion failed" ;
//
//    // Now convert the wchar_ts in wideCharacters to printable utf8 in characters.
//    success = WWII::ConvertChars(wideCharacters, characters sizeof(characters)) ;
//    if ( !success ) throw "Return conversion failed" ;
//
// Originating Programmer: Oliver "kfsone" Smith
//
//

// Copyright 2001 Unicode, Inc.
// 
// Disclaimer
// 
// This source code is provided as is by Unicode, Inc. No claims are
// made as to fitness for any particular purpose. No warranties of any
// kind are expressed or implied. The recipient agrees to determine
// applicability of information provided. If this file has been
// purchased on magnetic or optical media from Unicode, Inc., the
// sole remedy for any claim will be exchange of defective media
// within 90 days of receipt.
// 
// Limitations on Rights to Redistribute This Code
// 
// Unicode, Inc. hereby grants the right to freely use the information
// supplied in this file in the creation of products supporting the
// Unicode Standard, and to make copies of this file in any form
// for internal or external distribution as long as this notice
// remains attached.

#include "wchar.h"

namespace WWII				// Playnet namespace
{
	/* Some fundamental constants */
	static const unsigned long UNI_REPLACEMENT_CHAR = 0x0000FFFD ;
	static const unsigned long UNI_MAX_WCHAR = 0x0010FFFF ;

	// Map of the size of UTF8 characters.
	// e.g. char* p = source + WWII::BytesFromCharacter[*source];
	extern const char BytesFromCharacter[256] ;

	// Convert a UTF16/32 wchar_t* string to UTF8 char* characters.
	// sizeofTarget should include the terminating zero byte
	//   Returns false if source contains invalid characters
	//   Returns false if target is not large enough to contain UTF8 encoded source
	extern bool ConvertChars(const wchar_t* source, char* target, size_t sizeofTarget) ;

	// Convert a UTF8 char* string to UTF16/32 wchar_t* characters.
	// sizeofTarget should include the terminating zero wchar_t
	//   Returns false if source contains invalid characters
	//   Returns false if target is not large enough to contain all the characters
	extern bool ConvertChars(const char* source, wchar_t* target, size_t sizeofTarget) ;

	// Check if a UTF8 sequence of characters is legal
	extern bool IsLegalUTF8(const char* source) ;
} ;

/* --------------------------------------------------------------------- */
