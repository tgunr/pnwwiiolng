#include <syscfg.h> // Path is project specific.
// Created: Wed Sep  3 20:59:47 CDT 2008
//
/*
 * Derived from Convert.c:
 * Copyright 2001 Unicode, Inc.
 *
 * Disclaimer
 *
 * This source code is provided as is by Unicode, Inc. No claims are
 * made as to fitness for any particular purpose. No warranties of any
 * kind are expressed or implied. The recipient agrees to determine
 * applicability of information provided. If this file has been
 * purchased on magnetic or optical media from Unicode, Inc., the
 * sole remedy for any claim will be exchange of defective media
 * within 90 days of receipt.
 *
 * Limitations on Rights to Redistribute This Code
 *
 * Unicode, Inc. hereby grants the right to freely use the information
 * supplied in this file in the creation of products supporting the
 * Unicode Standard, and to make copies of this file in any form
 * for internal or external distribution as long as this notice
 * remains attached.
 */
//	Conversions between UTF32, UTF-16, and UTF-8. Source code file.
//	Author: Mark E. Davis, 1994.
//	Rev History: Rick McGowan, fixes & updates May 2001.
//	Sept 2001: fixed const & error conditions per
//		mods suggested by S. Parent & A. Lillich.
//
 // See the header file "wwiiUTF.h" for complete documentation.

// Copyright (C) PlayNet, Inc 1999-2008 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "wwii/wwiiUTF.h"				// Our header file

typedef unsigned long UTF32;
typedef unsigned char UTF8;

static const int halfShift	= 10; /* used for shifting by 10 bits */

static const UTF32 halfBase	= 0x0010000UL;
static const UTF32 halfMask	= 0x3FFUL;

#define UNI_SUR_HIGH_START	(UTF32)0xD800
#define UNI_SUR_HIGH_END	(UTF32)0xDBFF
#define UNI_SUR_LOW_START	(UTF32)0xDC00
#define UNI_SUR_LOW_END		(UTF32)0xDFFF
static const unsigned long UNI_MAX_BMP = 0x0000FFFF;
//static const unsigned long UNI_MAX_UTF32 = 0x7FFFFFFF;

/* --------------------------------------------------------------------- */

/*
 * Index into the table below with the first byte of a UTF-8 sequence to
 * get the number of trailing bytes that are supposed to follow it.
 */
const char WWII::BytesFromCharacter[256] =
{
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
	3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3, 4,4,4,4,4,4,4,4,5,5,5,5,6,6,6,6
};

/*
 * Magic values subtracted from a buffer value during UTF-8 conversion.
 * This table contains as many values as there might be trailing bytes
 * in a UTF-8 sequence.
 */
static const UTF32 offsetsFromUTF8[7] = { 0L, 0x00000000UL, 0x00003080UL, 0x000E2080UL, 0x03C82080UL, 0xFA082080UL, 0x82082080UL };

/*
 * Once the bits are split out into bytes of UTF-8, this is a mask OR-ed
 * into the first byte, depending on how many bytes follow.  There are
 * as many entries in this table as there are UTF-8 sequence types.
 * (I.e., one byte sequence, two byte... six byte sequence.)
 */
static const UTF8 firstByteMark[7] = { 0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC };

/* --------------------------------------------------------------------- */

/* The interface converts a whole buffer to avoid function-call overhead.
 * Constants have been gathered. Loops & conditionals have been removed as
 * much as possible for efficiency, in favor of drop-through switches.
 * (See "Note A" at the bottom of the file for equivalent code.)
 * If your compiler supports it, the "isLegalUTF8" call can be turned
 * into an inline function.
 */

/* --------------------------------------------------------------------- */

bool
WWII::ConvertChars(const wchar_t* source, char* target, size_t sizeofTarget_InBytes) noexcept
{
	assert( source != nullptr );
	assert( target != nullptr );

	const size_t targetEntries = sizeofTarget_InBytes / sizeof(*target) - 1;
	assert( targetEntries > 0 );

	const char* const targetEnd = target + targetEntries;

	*target = 0;

	while ( *source && target < targetEnd )
	{
		UTF32 ch = (UTF32)*(source++);
		unsigned short bytesToWrite = 0;
		static const UTF32 byteMask = 0xBF;
		static const UTF32 byteMark = 0x80;
		// If we have a surrogate pair, convert to UTF32 first.
		if ( ch >= UNI_SUR_HIGH_START && ch <= UNI_SUR_HIGH_END && *source )
		{
			UTF32 ch2 = *source;
			if ( ch2 == 0 )
			{
				return false;
			}
			if ( ch2 >= UNI_SUR_LOW_START && ch2 <= UNI_SUR_LOW_END )
			{
				ch = ((ch - UNI_SUR_HIGH_START) << halfShift) + (ch2 - UNI_SUR_LOW_START) + halfBase;
				++source;
			}
		}
		/* Figure out how many bytes the result will require */
		if (ch < (UTF32)0x80)
			bytesToWrite = 1;
		else if (ch < (UTF32)0x800)
			bytesToWrite = 2;
		else if (ch < (UTF32)0x10000)
			bytesToWrite = 3;
		else if (ch < (UTF32)0x200000)
			bytesToWrite = 4;
		else
		{
			bytesToWrite = 2;
			ch = WWII::UNI_REPLACEMENT_CHAR;
		}

		target += bytesToWrite;
		if (target >= targetEnd)
			return false;
		switch (bytesToWrite)
		{	/* note: everything falls through. */
			case 4:	*(--target) = (UTF8)((ch | byteMark) & byteMask); ch >>= 6;
			case 3:	*(--target) = (UTF8)((ch | byteMark) & byteMask); ch >>= 6;
			case 2:	*(--target) = (UTF8)((ch | byteMark) & byteMask); ch >>= 6;
			case 1:	*(--target) = (UTF8)( ch | firstByteMark[bytesToWrite]);
		}
		target += bytesToWrite;
		*target = 0;
	}

	return true;
}

/* --------------------------------------------------------------------- */

/*
 * Utility routine to tell whether a sequence of bytes is legal UTF-8.
 * This must be called with the length pre-determined by the first byte.
 * If not calling this from ConvertUTF8to*, then the length can be set by:
 *	length = trailingBytesForUTF8[*source]+1;
 * and the sequence is illegal right away if there aren't that many bytes
 * available.
 * If presented with a length > 4, this returns false.  The Unicode
 * definition of UTF-8 goes up to 4-byte sequences.
 */

bool
WWII::IsLegalUTF8(const char* const source) noexcept
{
	const int length = WWII::BytesFromCharacter[(UTF8)*source];
	UTF8 a;
	const UTF8* srcptr = reinterpret_cast<const UTF8*>(source + length);
	switch (length)
	{
		default: return false;
			/* Everything else falls through when "true"... */
		case 4: if ((a = (*--srcptr)) < 0x80 || a > 0xBF) return false;
		case 3: if ((a = (*--srcptr)) < 0x80 || a > 0xBF) return false;
		case 2: if ((a = (*--srcptr)) > 0xBF) return false;
			switch ((UTF8)*source)
			{
		    	/* no fall-through in this inner switch */
		    	case 0xE0: if (a < 0xA0) return false; break;
		    	case 0xF0: if (a < 0x90) return false; break;
		    	case 0xF4: if (a > 0x8F) return false; break;
		    	default:  if (a < 0x80) return false;
			}
   		case 1: if ((UTF8)*source >= 0x80 && (UTF8)*source < 0xC2) return false;
			if ((UTF8)*source > 0xF4) return false;
		break;
	}
	return true;
}

/* --------------------------------------------------------------------- */

// Advances "source" beyond any leading whitespace.
// If we advance to an invalid character, returns false.

bool
WWII::SkipWhitespace(const char*& source) noexcept
{
	assert( source != nullptr );

	while ( *source )
	{
		if ( false == IsLegalUTF8(source) )
			return false;
		unsigned int length = WWII::BytesFromCharacter[(UTF8)*source];
		if ( length > 1 || !isspace(*source) )
			return true;
		source += length;
	}
	return true;
}

/* --------------------------------------------------------------------- */

bool
WWII::ConvertChars(const char* source, wchar_t* target, size_t sizeofTarget_InBytes) noexcept
{
	assert( source != nullptr );
	assert( target != nullptr );

	const size_t targetEntries = sizeofTarget_InBytes / sizeof(*target) - 1;
	assert( targetEntries > 0 );

	*target = 0;
	const wchar_t* const targetEnd = target + targetEntries;

	while ( *source && target < targetEnd )
	{
		unsigned int length = WWII::BytesFromCharacter[(UTF8)*source];

		/* Do this check whether lenient or strict */
		if ( false == IsLegalUTF8(source) )
			return false;

		UTF32 ch = 0;
		/*
		 * The cases all fall through. See "Note A" below.
		 */
		switch (length)
		{
			case 4: ch += (UTF8)*(source++); ch <<= 6;
			case 3:	ch += (UTF8)*(source++); ch <<= 6;
			case 2:	ch += (UTF8)*(source++); ch <<= 6;
			case 1:	ch += (UTF8)*(source++);
		}
		ch -= offsetsFromUTF8[length];

		if (ch <= UNI_MAX_BMP)
		{ /* Target is a character <= 0xFFFF */
		    *target++ = (wchar_t)ch;	/* normal case */
			*target = 0;
		}
		else if (ch > WWII::UNI_MAX_WCHAR)
		{
			*target++ = WWII::UNI_REPLACEMENT_CHAR;
			*target = 0;
		}
		else
		{
			/* target is a character in range 0xFFFF - 0x10FFFF. */
			if (target + 1 >= targetEnd)
				return true;
			ch -= halfBase;
			*target++ = (wchar_t)((ch >> halfShift) + UNI_SUR_HIGH_START);
			*target++ = (wchar_t)((ch & halfMask) + UNI_SUR_LOW_START);
			*target = 0;
		}
	}

	return true;
}

/* ---------------------------------------------------------------------

	Note A.
	The fall-through switches in UTF-8 reading code save a
	temp variable, some decrements & conditionals.  The switches
	are equivalent to the following loop:
		{
			int tmpBytesToRead = extraBytesToRead+1;
			do {
				ch += *source++;
				--tmpBytesToRead;
				if (tmpBytesToRead) ch <<= 6;
			} while (tmpBytesToRead > 0);
		}
	In UTF-8 writing code, the switches on "bytesToWrite" are
	similarly unrolled loops.

   --------------------------------------------------------------------- */
