#pragma once
// Created: Wed Sep  3 20:59:47 CDT 2008
//
// Copyright (C) PlayNet, Inc 1999-2008 - All rights reserved.
// Not for redistribution.
//! @file wwii/wwiiUTF.h
//! @brief Provides WWII::ConvertChars

/*!
 *   Example Usage:
 *    @code
 *    char characters[80] = "Hello World" ;
 *    wchar_t wideCharacters[80] = "" ;
 *
 *    // Convert the ASCII string in characters to wchar_ts in wideCharacters
 *    bool success = WWII::ConvertChars(characters, wideCharacters, sizeof(wideCharacters)) ;
 *    if ( !success ) throw "First conversion failed" ;
 *
 *    // Now convert the wchar_ts in wideCharacters to printable utf8 in characters.
 *    success = WWII::ConvertChars(wideCharacters, characters sizeof(characters)) ;
 *    if ( !success ) throw "Return conversion failed" ;
 *	  @endcode
 */
// Originating Programmer: Oliver "kfsone" Smith

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

//! Shortcut-macro for conversion a buffer of a known-size.
#define WWIIConvert(from,to) WWII::ConvertChars(from, to, sizeof(to))

//! Playnet Namespace
namespace WWII
{
	/* Some fundamental constants */
	static const unsigned long UNI_REPLACEMENT_CHAR = 0x0000FFFD ;
	static const unsigned long UNI_MAX_WCHAR = 0x0010FFFF ;

	//! @internal
	//! Map of the size of UTF8 characters.
	//! e.g. char* p = source + WWII::BytesFromCharacter[*source];
	extern const char BytesFromCharacter[256] ;

	//! Convert a UTF16/32 wchar_t* string to UTF8 char* characters.
	//! @param[in]	source		Pointer to source string.
	//! @param[out]	target		Pointer to destination buffer.
	//! @param[in]	sizeofTarget_inBytes	Size of 'target' in bytes (not wchars) including terminating zero.
	//! @return false if source contains invalid characters or target is not large enough to contain UTF8 encoded version of source.
	//! @warning sizeOfTarget is in BYTES and must include the terminating zero.
	extern bool ConvertChars(const wchar_t* source, char* target, size_t sizeofTarget_inBytes) ;

	//! Convert a UTF8 char* string to UTF16/32 wchar_t* characters.
	//! @param[in]	source		Pointer to source string.
	//! @param[out]	target		Pointer to destination buffer.
	//! @param[in]	sizeofTarget_inBytes	Size of 'target' in bytes (not wchars) including terminating zero.
	//! @return false if source contains invalid characters or target is not large enough to contain wchart_t encoded version of source.
	extern bool ConvertChars(const char* source, wchar_t* target, size_t sizeofTarget_inBytes) ;

	//! Check if a UTF8 sequence of characters is legal.
	//! @param[in] source		Pointer to characters.
	//! @return true/false whether source points to a valid UTF8 character.
	extern bool IsLegalUTF8(const char* source) ;

	//! Advances "source" beyond any leading whitespace.
	//! @param	source			References a pointer to the source character string. The specified pointer is modified.
	//! @return false if we reach an invalid character/sequence.
	extern bool SkipWhitespace(const char*& source) ;
}
