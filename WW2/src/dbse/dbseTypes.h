#pragma once
// Created: Thu Sep  7 17:11:01 2000
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Eric Hall

enum CONNECTION_STATUS
{
	CONNECT_NONE,
	CONNECT_BAD,
	CONNECT_OPEN,
	CONNECT_REFUSE,
	CONNECT_ACCEPT,
	CONNECT_REDIRECTED,
	CONNECT_DROPPED
} ;

struct HANDSHAKE
{
	UINT32 				host;
	CONNECTION_STATUS 	status;
	UINT32				version;
} ;
extern SINT32 dbsePackHandshake(LOGICAL mode, UCHAR* data, SINT32 offset, HANDSHAKE* object, SINT32 size);

enum DBSE_RANK
{
	DBSE_PRIMARY = 0,
	DBSE_SECONDARY,
	DBSE_SLAVE,
	DBSE_REDIRECT,
	DBSE_MAX_RANK
} ;

enum
{
	SUCCESS = 0,
	FAIL_VERSION,
	FAIL_CONNECT
};
