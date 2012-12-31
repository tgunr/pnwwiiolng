#pragma once
// Created: Thu Sep  7 16:12:14 2000
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Eric Hall

// Setup uthe callback x and y values
// relatively high priority callbacks

enum {
	DBSE_GENERAL_X = 19,
	MM_DBSE_X,
	CHAT_DBSE_X
};

// have the whole damn column to work with, 256 rows.
enum {
	DBSE_CLIENT_HANDSHAKE_Y=0,
	DBSE_SERVER_HANDSHAKE_Y
};

enum {
	//map database stuff here
	MMDB_XCHG_BRIGADE_DETAILS_Y
} ;

enum
{
	CHATDB_IGNORE_LIST_Y
,	CHATDB_MODIFY_IGNORE_Y
,	CHATDB_POLL_CHAT_PERMS_Y
,	CHATDB_SQUAD_INFO_Y
,	CHATDB_SQUAD_MEMBERS_Y
,	CHATDB_SQUAD_REQUEST_Y
,	CHATDB_SQUAD_MEMBER_UPDATE_Y
} ;

//Parser Return Codes
enum {
	PARSE_SUCCESS=0,			//dbParser has been succesfully initialized
	PARSE_DB_CONNECT_FAILED,	//the call to dbseCannect returned false
	PARSE_EXISTS				//a parser has already been initialized for the dbTpye specified, no action taken
};

