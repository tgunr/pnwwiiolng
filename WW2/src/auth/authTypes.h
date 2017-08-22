#pragma once
// Created: Tue Jun 13 13:32:25 2000
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Data Definitions for the Auth Server
//
// Originating Programmer: Shane "Elmo" Wickson

#include "teul/teulEnums.h"

//uncomment the following to allow the password field of the APP_LAUNCH_REQ
//to be sent up as a 33 byte (32 byte hex rep + '\0'
//
#define MD5_PASSWORD

#define MAX_PARAM 255
#define MAX_OLD_USERNAME 16

#define MAX_WCHAR_USERNAME 12
#define	MAX_UTF8_USERNAME (MAX_WCHAR_USERNAME * 4)					// Worst case: 4 bytes per character

#ifdef MD5_PASSWORD
#define MAX_PASSWORD 33
#else
#define MAX_PASSWORD 16
#endif

struct ARENA_REQ
{
	SINT32	arenaID;
	UINT32	sessionID;
	LOGICAL	infoOnly;
	UINT32	presenterVersion;	//presenter must always send up it's version
	UINT8	oVersion;			// used by packer for expansion of fields.
	SCHAR	username[MAX_OLD_USERNAME+1];
	UCHAR	password[MAX_PASSWORD+1]; // UCHAR expected for MD5
};

enum AUTH_STATUS
{
	AUTH_OKAY=0,
	AUTH_GENERAL_ERROR,
	AUTH_GAME_HOST_DOWN,
	AUTH_AUTH_DOWN,
	AUTH_GAME_FULL,
	AUTH_MAX
};

struct APP_VERIFY
{
	SINT32	arenaID;
	SINT32	sessionID;
	SINT32	status;			// server status < 0 is an error, > 0 is # of players
	SINT32	appID;			// Application ID
	SCHAR	appleID[4];
	SINT32	appVersionMax;	// Max Version of the app required
	LOGICAL	requireCD;		// require CD to be in drive
};

struct APP_LAUNCH_REQ
{
	SCHAR	username[MAX_OLD_USERNAME+1];
	UCHAR	password[MAX_PASSWORD+1]; // UCHAR expected for MD5
	SINT32	arenaID;
	UINT32	sessionID;
	SINT32	appID;				//Application ID
	UINT32	appVersion;			//Min Version of the app required
	SINT32	status;				// cd in drive etc...
	SINT32	cdKeySize;
	UCHAR	cdKey[MAX_PARAM];	//Application data.  Like parameters and such.
	LOGICAL	ignoreMinorUpdates; //Tells the auth Server to ignore the minor
								//	revision number of the game and launch
								//	anyway
	UINT8	oVersion;			// used by packer for expansion of fields.

	UCHAR	fileKey[MAX_PASSWORD+1];	//	UCHAR expected for MD5
	UCHAR	manifestKey[MAX_PASSWORD+1]; //	UCHAR expected for MD5

};

struct APP_FILE_CHECK
{
	UINT32	presenterVersion;			//	presenter must always send up it's version
	SINT32	appID;						//	Application ID
	UCHAR	fileKey[MAX_PASSWORD+1];	//	UCHAR expected for MD5
	UCHAR	manifestKey[MAX_PASSWORD+1]; //	UCHAR expected for MD5
};


enum APPDL_ACTION
{
	APPDL_NODL,
	APPDL_INSTALL,
	APPDL_REPAIR
};

struct APP_LAUNCH
{
	SINT32	arenaID;
	UINT32	sessionID;		//The sessionID is used by the chat server, as
							//	well as any game service that wants sessions
	SINT32	appID;			//Application ID
	SINT32	appVersion;		//Version of the app required
	UINT32	otp;			//One time password, provided authentication was
							//	successful
	UINT32	serverIP;		//IP address of the server to connect to
	UINT32	serverPort;		//port of the game server
	PROTOCOL protocol;		//Protocol ID
	SINT32	status;			// server status < 0 is an error, > 0 is # of players
	SINT32	dataSize;
	SCHAR	appleID[4];
	char	data[MAX_PARAM];		//Application data.  Like parameters and such.

	APPDL_ACTION action;	// what to do with a file that we download
	SINT32	dlAppID;		//Application ID

};

#if platBuild == BUILD_SERVER
	#include "auth/authServerTypes.h"
#endif
