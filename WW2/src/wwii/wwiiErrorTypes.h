#pragma once
// Created: Tue Jan 15 13:58:31 CST 2008
//
// Structures for communicating a client crash/error of some kind.
//
// Copyright (C) PlayNet, Inc 1999-2008 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

enum WWII_ERR_TYPES
{
	WW_ERR_UNKNOWN,
	WW_ERR_EXCEPTION, // these seems to be the only ones used
	WW_ERR_PIEXIT, // these seems to be the only ones used
	WW_ERR_VALIDITY_CHECK,
	WW_ERR_SYSTEM_INFO
} ;

struct WWII_ERR_STRING
{
	static const UINT32 _MAX_LEN = 127;

	SINT32	length;
	SCHAR 	string[_MAX_LEN + 1];
} ;

struct WWII_PIEXIT_INFO // these seems to be the only ones used
{
	// piExit
	SINT32 srcLine;
	WWII_ERR_STRING srcFilename;
	WWII_ERR_STRING errorMessage;
} ;

struct WWII_EXCEPTION_INFO // these seems to be the only ones used
{
	static const UINT32 _STACK_LEN = 10;
	static const UINT32 _STR_LEN = 255-_STACK_LEN*sizeof(UINT32);
	// Exception
	UINT32 exceptionCode;
	UINT32 exceptionAddr;
	UINT32 stackAddrs[_STACK_LEN];
	SCHAR infoText[_STR_LEN + 1];
} ;

struct WWII_VALIDITY_INFO
{
	static const UINT32 _MD5_LEN = 32;

	// Validity check
	WWII_ERR_STRING archive;
	WWII_ERR_STRING filename;
	UINT32 crc;
	UCHAR md5[_MD5_LEN + 1];
} ;

struct WWII_SYSTEM_INFO
{
	static const UINT32 _MAX_LEN = 256;

	UINT32	numPackets;
	UINT32	packetID;	// id of info packet
	SINT32	length;
	SCHAR	infoText[_MAX_LEN + 1];
} ;

union WWII_ERROR_DATA
{
	WWII_PIEXIT_INFO	piExitInfo;
	WWII_EXCEPTION_INFO exceptInfo;
	WWII_VALIDITY_INFO	validityInfo;
	WWII_SYSTEM_INFO	systemInfo;
} ;

struct WWII_ERROR_INFO
{
	WWII_ERR_TYPES errorType;

	UINT32 eventID; // used to associate error info with an event
	UINT32 clientVersion;
	UINT32 clientTime;
	UINT32 hostTime;
	UINT32 hostIP;
	LOGICAL archiveWasCorrupt;

	WWII_ERROR_DATA data;
	UINT32	clientSourceRevision;
} ;
