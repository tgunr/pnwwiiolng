#pragma once
// Created: Thu Jun 22 17:16:51 2000
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// The header file for the DB layer to the auth server
//
// Originating Programmer: Shane "Elmo" Wickson

#define ISFILE_URL 0
#define ISFILE_FILE 1
#define ISFILE_NUN 2

#define URL_STRING_MAX 128
#define APPLE_ID_MAX 16
#define DEF_PORT 21
#define DEF_PROTOCOL PROTOCOL_TCP

struct vers_manifest
{
    UINT32		oldNo;
    UINT32		newNo;
    UINT32		appid;
    UINT32		platform;
    SINT32		isfile;
    UINT32		oldcrc;
    UINT32		newcrc;

    UINT16		port;
    UINT16		protocol;
    SCHAR		location[URL_STRING_MAX+1];
    SCHAR		appleID[APPLE_ID_MAX+1];
    SCHAR		ftpURL[URL_STRING_MAX+1];
    SCHAR		ftpDir[URL_STRING_MAX+1];
    SCHAR		ftpFile[URL_STRING_MAX+1];
};

namespace Auth { struct Dispatch; struct ClientData; }
struct APP_LAUNCH_REQ;
struct APP_LAUNCH;
struct GAME;
struct PLAY_COMPUTER_INFO;

void authInitDB();
void authResetConnectInfo(UINT32 gameID);

//Validates a user against the database, and fills out the game cookie
//	if they are a good user
UINT32 authValidateUser(SINT32 teulIndex, const APP_LAUNCH_REQ& request, Auth::Dispatch& gamecookie, const Auth::ClientData& cdata);

// Inserts otp records int ot the authOTP table to be used by
// our ftp servers for authenication.
void authDBInsertOTP(const APP_LAUNCH& launch, const APP_LAUNCH_REQ& request, const char* dir);
bool authInitGameEntry(GAME& gameEntry);
void authInitApps();

//
// Search for a match between the encoded 'needle'
// and the config lines

//SINT32 matchversion(SINT32 needle,SINT32 appid);
struct vers_manifest* matchversion(UINT32 needle, SINT32 appid, UINT32 platform);

// Save a computer info packet arriving on a connection
void authdbStoreComputerInfo(const Auth::ClientData& data, const PLAY_COMPUTER_INFO& pci);

// Get a checksum from the database
bool authdbGetFileMD5(const UINT32 appID, const UINT32 platform, const char* filename, char* buffer, UINT32 bufLen);

void authdbSaveLastOTP(UINT64 otp);
UINT32 authdbGetLastOTP();

void authdbLogBadPlayername(UINT32 ip, const std::string& username, std::string reason);

void authdbUpdateArenaStatus(UINT32 arenaID, SINT32 lock, UINT32 players);

void authdbReadArenaAliases();

// Get the name of an arena or NULL.
const char* authdbGetArenaAlias(UINT32 arenaID);

void authdbSaveLogins();
