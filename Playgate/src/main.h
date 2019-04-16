#pragma once

#define PG_VER_STRING		"1.1.9"
#define PLAY_GATE_VERSION	"0." PG_VER_STRING
#define	PG_COPYRIGHT		PG_VER_STRING " Copyright(C) Playnet Inc, 2001-2019"

#include "include/systypes.h"
#include <string>
#include <vector>

#define MSG_PNI            9265
#define MSG_BADLOGIN       9266
#define MSG_NEEDUPDATE     9267
#define MSG_SELECTUPDATE   9269
#define MSG_VERUPDATE      9270
#define MSG_DOWNLOAD       9271

#define ERROR_REDIRECT "@pgerror/?error="

#define ERR_GAME_FULL		ERROR_REDIRECT "full"
#define ERR_GAME_NOT_AVAIL	ERROR_REDIRECT "game"
#define ERR_AUTH_LOGIN		ERROR_REDIRECT "login"
#define ERR_AUTH_RESOLVE	ERROR_REDIRECT "authresolve"
#define ERR_AUTH_CONNECT	ERROR_REDIRECT "authconnect"
#define ERR_FTP_CONNECT		ERROR_REDIRECT "ftpconnect"
#define ERR_FTP_CANCEL		ERROR_REDIRECT "ftpcancel"
#define ERR_FTP_NOPATCH		ERROR_REDIRECT "nopatch"
#define ERR_GAME_REINSTALL	ERROR_REDIRECT "reinstall"
#define ERR_UNKNOWN			ERROR_REDIRECT "unknown"
#define ERR_LOCKED			ERROR_REDIRECT "locked"

extern HINSTANCE hMainInstance;

extern char szVersion[];
extern char szVersionString[];

#define IGNORE_REGISTRY 1

typedef std::vector<UINT32> IP_LIST;
extern IP_LIST	g_authHosts;
extern UINT16	g_authPort;
extern SINT32 webID; //used for web downloads
extern UINT32 webURL;
extern UINT32 g_playgateVersion;
extern std::wstring g_exeName, g_exeParams;
extern SINT32 g_sessionID;
extern UINT32 g_gameAppID;

#ifndef IGNORE_REGISTRY
	extern char g_registryKey[];
	extern wchar_t g_registryKeyW[];
#endif