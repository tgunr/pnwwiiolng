#pragma once
// Created: Wed Jul 26 14:13:03 2000
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a compiled library.
//
// Handle manipulation routines for the chat system
//
// Originating Programmer: Shane "Elmo" Wickson
/*
 * Callsigns are no-longer 8 characters long and may contain unicode characters:
 *
 * chatHandle.h :: MAX_HANDLE_SIZE  => char[] STORAGE size for all callsign variations (including \0)
 * chatHandle.h :: MAX_HANDLE_LEN => max strlen() for all callsign variations.
 *
 * REMEMBER: UTF8 characters require 3bytes when stored in a char[] array.
 * So a Chinese callsign may uses 12 /char/s but would only require 4 /wchar_t/s.
 *
 * chatHandle.h :: MAX_HANDLE_WSIZE => wchar_t STORAGE size for all callsign variations (including \0)
 * chatHandle.h :: MAX_HANDLE_WLEN => max wcslen() for all wchar_t callsign variations.
 *
 * EXAMPLE:
 *     ChatHandle chinaman = chatGetHandle(L"..."); // Get a unicode string handle
 *     char printableName[MAX_HANDLE_SIZE];
 *     wchar_t wideCharName[MAX_HANDLE_WSIZE];
 *     chatGetCallsign(printableName, chinaman);
 *     chatGetCallsign(wideCharName, chinaman);
 *     int printableLen = strlen(printableName); // 2 x 3 character bytes + 2 ascii bytes = strlen of 8
 *     int wideLen = wcslen(wideCharName);   // each character fits into a wchar_t so wcslen returns 4.
 *
 *  Utility functions:
 *     bool chatIsWideCallsign( char* | wchar_t* | ChatHandle) will tell you if it does.
 *     bool chatIsValidHandle(ChatHandle) will check if a 64-bit handle looks good.
 *     bool chatIsValidCalsign( char* | wchar_t* ) will check the various constraints.
 *
 */

// Maximum bits in a handle is 64, at 6 bits a character that limits us to 10 characters
static const size_t MAX_HANDLE_LEN = 13;		// 4 x 3-byte wide chars + 1 regular => 13
static const size_t MAX_HANDLE_SIZE = 14;		// MAX_HANDLE_LEN + 1
static const size_t MAX_HANDLE_WLEN = 10;		// Maximum standard character handle length
static const size_t MAX_HANDLE_WSIZE = MAX_HANDLE_WLEN + 1;	// Max wchars needed to store a handle
static const UINT64 WIDE_HANDLE_BIT = (1ULL << 63);

// We need the full 64 bits to encompass the Wide Handle Bit now.
static const size_t GAME_HANDLE_BITS = 64;

// The number of characters supported in chat handles
namespace Chat
{
	static const UINT16 WIDE_HANDLE_CHARACTERS_SUPPORTED = 6827;
}

extern bool chatIsValidHandle(ChatHandle handle);

//These functions retrieve the unique handleID from the handle and
//	the handle from the unique handleID
//tolua_begin
extern ChatHandle chatGetHandle(const char* callsign);
extern ChatHandle chatGetHandle(const wchar_t* callsign);

/// Wide-string variant
extern ChatHandle chatGetHandle(const wchar_t* callsign);
// Same function, but uses the lowercase version of the handle
extern ChatHandle chatGetLowercaseHandle(const char* callsign);
extern ChatHandle chatGetLowercaseHandle(const wchar_t* callsign);
extern ChatHandle chatGetLowercaseHandle(const wchar_t* callsign);
// Force a handle into lowercase
extern ChatHandle chatGetLowercaseHandle(ChatHandle handle);
//tolua_end

//tolua_begin
// Convert a handle into a callsign again
extern const char* chatGetCallsign(const ChatHandle& handle);
extern const wchar_t* chatGetWideCallsign(const ChatHandle& handle);
// Convert a handle into the supplied buffer
extern bool chatGetCallsign(char* into, const ChatHandle& from);
// Wide string variant
extern bool chatGetCallsign(wchar_t* into, const ChatHandle& from);

extern bool chatIsValidCallsign(const char* callsign);
extern bool chatIsValidCallsign(const wchar_t* callsign);

// Looks a wide character up in the character map and returns the symbol # for it
extern UINT16 chatGetHandleCharacterSymbol(const wchar_t character);
//tolua_end

// Return true if a callsign contains wide characters
static inline bool chatIsWideCallsign(const ChatHandle& handle) { return (handle & WIDE_HANDLE_BIT) != 0; }
static inline bool chatIsWideCallsign(const char* callsign) { return (*(const unsigned char*)callsign > 0x7f); }
static inline bool chatIsWideCallsign(const wchar_t* callsign) { return (callsign[0] > 0x7f); }

