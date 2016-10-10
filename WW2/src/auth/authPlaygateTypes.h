#pragma once
// Created: Fri Dec 12 11:38:23 CST 2008
// Copyright (C) PlayNet, Inc 2008-2011 - All rights reserved.
// Not for redistribution.
//
// Additional authTypes for Playgate
//
// Originating Programmer: Oliver "kfsone" Smith

#include "include/arenaLockEnums.h"	// Used by AppGameEntry
#include "teul/teulPackers.h"
#include "wwii/wwiiPackString.h"

#include "auth/authTypes.h"	// For MAX_USERNAME/PASSWORD values

namespace Auth
{

//////////////////////////////////////////////////////////////////////
struct GameEntry
{
	// Constructors
	GameEntry() : m_name(""), m_arenaID(0), m_lockState(AL_OFFLINE), m_populationPctg(0), m_appID(0), m_minVer(0), m_maxVer(0) {}
	// Network Ctor
	GameEntry(ACTION_OBJECT* const a) { a->offset = packer(TEUL_UNPACK, a); }

	// Serializing function
	SINT32 packer(LOGICAL mode, ACTION_OBJECT* a)
	{
		a->offset = wwiiPackString(mode, a->packed, a->offset, m_name, 128);
		a->packer(mode, &m_arenaID, 8);
		a->packer(mode, &m_lockState, 8);
		a->packer(mode, &m_populationPctg, 8);
		return a->offset;
	}

	std::string	m_name;			//!< Arena's name
	UCHAR		m_arenaID;			//!< Server's ID for the arena
	SCHAR		m_lockState;		//!< Is it locked?
	UCHAR		m_populationPctg;	//!< How close to full?
	UCHAR		m_appID;			//!< Server's AppID for local lookup
	UINT32		m_minVer;			//!< Minimum version required
	UINT32		m_maxVer;			//!< Maximum version required
};

//////////////////////////////////////////////////////////////////////
// Host->Client list of current game availability. See arenaLockEnums.h for
// meanings of lockState.
class GameList
{
public:
	// Constructor
	GameList() : m_games() {}
	// Network Ctor
	GameList(ACTION_OBJECT* const a) { packer(TEUL_UNPACK, a); }
	// Serializing function
	SINT32 packer(LOGICAL mode, ACTION_OBJECT* a)
	{
		UINT32 numGames = (UINT32)m_games.size();
		a->packer(mode, &numGames, 8);
		if ( mode == TEUL_UNPACK )
		{
			m_games.clear();
			m_games.resize(numGames);
		}
		for ( UINT32 i = 0; i < numGames; ++i )
		{
			m_games[i].packer(mode, a);
		}
		for ( UINT32 i = 0; i < numGames && i < _MAX_GAMES; ++i )
		{
			a->packer(mode, &m_games[i].m_appID);
			a->packer(mode, &m_games[i].m_minVer);
			a->packer(mode, &m_games[i].m_maxVer);
		}
		return a->offset;
	}
	// Client: Handle the receipt of a list
	void process();
	// Host: populate a list for sending to client
	void populate(const std::vector<UCHAR>& clientSubs);

	UINT32 size() const { return (m_games.empty() == false) ? (UINT32)m_games.size() : 0; }

	static const UINT32 _MAX_GAMES = 15;
	using APP_GAMES = std::vector<GameEntry>;
	APP_GAMES	m_games;
	bool isPlaynet;
};

//////////////////////////////////////////////////////////////////////
// User requesting to log in
struct LoginRequest
{
	enum { PLAT_PC = 0, PLAT_MAC = 1 };
	// Constructor.
	LoginRequest(const std::string& account, const UCHAR* passwordMD5)
		: m_account(account), m_passwordMD5((const char*)passwordMD5)
		, m_platform(platOS)
		{}
	// Network ctor.
	LoginRequest(ACTION_OBJECT* const a) { a->offset = packer(TEUL_UNPACK, a); }
	// Serializing function
	SINT32 packer(LOGICAL mode, ACTION_OBJECT* a)
	{
		a->offset = wwiiPackString(mode, a->packed, a->offset, m_account, MAX_UTF8_USERNAME);
		a->offset = wwiiPackString(mode, a->packed, a->offset, m_passwordMD5, MAX_PASSWORD);
		a->offset = teulPackUCHAR(mode, a->packed, a->offset, &m_platform, 8);
		return a->offset;
	}
	// Host: Log the guy in.
	SINT32 process(SINT32 teulIndex);
	// Client: Send this request
	void send();
	// Host: authenticate
	UINT32 validate(SINT32 teulIndex, struct ClientData& cdata);

	std::string	m_account;		//!< Account name
	std::string	m_passwordMD5;	//!< MD5 encoded password
	UCHAR		m_platform;	//!< 0 = Windows, 1 = Mac
};

//////////////////////////////////////////////////////////////////////
// After an Auth::LoginRequest the host validates your request and determines
// whether your account has permission to log in or not. The result of this
// authentication is described through an Auth::LoginReply. "code" is a
// 1-4 digit human readable code which should reference localized strings.
// If code is empty, then the login was successful.
struct LoginReply
{
	// Constructor
	LoginReply(bool isPlaynet, const std::string& code) : m_playnet(isPlaynet), m_code(code) {}
	// Network Ctor.
	LoginReply(ACTION_OBJECT* const a) : m_playnet(false), m_code("") { a->offset = packer(TEUL_UNPACK, a); }
	// Serializing function
	SINT32 packer(LOGICAL mode, ACTION_OBJECT* const a)
	{
		a->packer(mode, m_playnet);
		a->offset = wwiiPackString(mode, a->packed, a->offset, m_code, 16);
		return a->offset;
	}
	// Client: Process this response.
	void process();
	// Host: Send the result
	void send(SINT32 teulIndex);

	bool wasSuccessful() const { return m_code.empty(); }

	bool 		m_playnet;
	std::string	m_code;
};

//////////////////////////////////////////////////////////////////////
struct LaunchRequest
{
	// Constructor
	LaunchRequest(SINT32 arenaID, UINT32 appID, UINT32 appVersion)
		: m_arenaID(arenaID)
		, m_appID(appID)
		, m_appVersion(appVersion)
		{}
	// Network Ctor
	LaunchRequest(ACTION_OBJECT* const a)
		: m_arenaID(0), m_appID(0), m_appVersion(0)
		{
			a->offset = packer(TEUL_UNPACK, a);
		}
	// Serializing function
	SINT32 packer(LOGICAL mode, ACTION_OBJECT* const a)
	{
		a->packer(mode, &m_arenaID, 10);
		a->packer(mode, &m_appID, 10);
		a->packer(mode, &m_appVersion, 0);
		return a->offset;
	}
	// Host: process this request
	void process(SINT32 teulIndex);

	UINT32	m_arenaID;			// Arena ID to be logged in to
	UINT32	m_appID;			// ID of the application we want to log into
	UINT32	m_appVersion;		// Version of the app requester has installed
};

//////////////////////////////////////////////////////////////////////
// If a launch request was successful, you are sent details of where to
// proceed to.
struct LaunchSuccess
{
	// Constructor
	LaunchSuccess(UINT32 _otp, UINT32 _serverIP, UINT16 _serverPort)
		: m_otp(_otp), m_serverIP(_serverIP), m_serverPort(_serverPort)
		{}
	// Network Ctor
	LaunchSuccess(ACTION_OBJECT* const a) { packer(TEUL_UNPACK, a); }

	// Serializing function
	SINT32 packer(LOGICAL mode, ACTION_OBJECT* a)
	{
		a->packer(mode, &m_serverPort, 15);
		a->packer(mode, &m_otp);
		a->packer(mode, &m_serverIP);
		return a->offset;
	}

	UINT32	m_otp;				// One Time Password (basically, session ID)
								// Or zero to indicate "failed".
	UINT32	m_serverIP;		// IP address of server to connect to
	UINT16	m_serverPort;		// Port to connect to
};

//////////////////////////////////////////////////////////////////////
// A launch request may be denied because you don't have permission
// to access a server or because a patch is required.
struct LaunchRedirect
{
	// Constructor
	LaunchRedirect(bool _patchRequired, const std::string& _url)
		: m_patchRequired(_patchRequired)
		, m_url(_url)
		{
		}
	// Network Ctor
	LaunchRedirect(ACTION_OBJECT* const a) : m_patchRequired(false), m_url("")
	{
		packer(TEUL_UNPACK, a);
	}

	// Serializing function
	SINT32 packer(LOGICAL mode, ACTION_OBJECT* a)
	{
		a->offset = wwiiPackString(mode, a->packed, a->offset, m_url, 500);
		a->packer(mode, m_patchRequired);
		return a->offset;
	}

	bool		m_patchRequired;	// True if this is a patch redirection
	std::string	m_url;				// URL to proceed to
};

struct VersionCheck
{
	VersionCheck(UINT32 platform=0, UINT32 version=0)
		: m_platform(platform)
		, m_clientVersion(version)
		{}
	VersionCheck(ACTION_OBJECT* const a)
		: m_platform(0)
		, m_clientVersion(0)
		{
			packer(TEUL_UNPACK, a);
		}

	void process();		// Host function
	void packer(LOGICAL mode, ACTION_OBJECT* a)
	{
		a->packer(mode, &m_platform);
		a->packer(mode, &m_clientVersion);
	}

	UINT32	m_arenaID;			// Arena you want to check
	UINT32	m_platform;		// Windows/Mac/etc
	UINT32	m_clientVersion;	// What version is the client running
};

struct VersionResponse
{
	VersionResponse(UINT32 arenaID=0, bool updateNeeded=false, bool updateAvailable=false)
		: m_arenaID(arenaID)
		, m_updateNeeded(updateNeeded)
		, m_updateAvailable(updateAvailable)
		{}
	VersionResponse(ACTION_OBJECT* const a) : m_arenaID(0), m_updateNeeded(false), m_updateAvailable(false) { packer(TEUL_UNPACK, a); }

	void process();		// Client function
	void packer(LOGICAL mode, ACTION_OBJECT* a)
	{
		a->packer(mode, &m_arenaID);
		a->packer(mode, m_updateNeeded);
		a->packer(mode, m_updateAvailable);
	}

	UINT32	m_arenaID;
	bool	m_updateNeeded;
	bool	m_updateAvailable;
};

} // namespace Auth

// Old, deprecated AppGameList from experimental playgates.
// Host->Client list of current game availability. See arenaLockEnums.h for
// meanings of lockState.
struct AppGameList
{
	static const UINT32 _MAX_GAMES = 8;
	UINT32  numGames;
	UINT32  arenaID[_MAX_GAMES];
	SINT32  lockState[_MAX_GAMES];
	SINT32	minVer[_MAX_GAMES];
	SINT32	maxVer[_MAX_GAMES];

	void process();
	SINT32 packer(LOGICAL mode, ACTION_OBJECT* a)
	{
		a->packer(mode, &numGames, 8);

		for ( UINT32 i = 0; i < numGames && i < _MAX_GAMES; ++i )
		{
			a->packer(mode, &arenaID[i], 8);
			a->packer(mode, &lockState[i], 8);
		}

		return a->offset;
	}
};
