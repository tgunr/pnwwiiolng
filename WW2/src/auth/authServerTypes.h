#pragma once
// Created: Tue Sep 26 2002
//
// Data Definitions for the Auth Server
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Marty "Vandal" Poulin

#define AUTH_HELLO_VALID 1
#define MAX_DESCRIPTION 255
#define APPLE_ID_LEN 5
// Support for widechars
static const UINT32 MAX_ARENA_ALIAS_LEN = 3 * 16 ;
static const UINT32 MAX_ARENA_ALIAS_SIZE = MAX_ARENA_ALIAS_LEN + 1 ;

enum CLIENT_ACTIONS
{
	CLIENT_ENTER,
	CLIENT_LEAVE
} ;

enum AUTHCODE
{
	ACCOUNT_VALID,
	UNKNOWN_USER,
	ACCOUNT_SUSPENDED_NONPAYMENT
} ;


struct APP_INFO
{
	UINT32  	min;		//if you have below this version, you must upgrade
	UINT32  	max;		//the latest version available

} ;

struct GAME ;

struct SERVER
{
	SERVER() : ipAddress(0), teulIndex(-1), port(0), sourcePort(0), currentUsers(0), maxUsers(0), game(NULL)
		{}
	~SERVER() ;

	UINT32	ipAddress;			//The IP address of the server
	SINT32	teulIndex;			//The teulIndex of the server
	UINT16	port;				//The port the server is running on
	UINT16	sourcePort;
	SINT32	currentUsers;		//current useres on this server
	UINT32	maxUsers;			//maximum users allowed on this server
	GAME*	game;				//pointer back to the game the server is
								//	running
} ;

//----------------------- MESSAGE SECTION -----------------------------

//The game entry alerting the auth server to a new game
struct AUTH_GAME_INIT
{
	UINT32	arenaID;		// The arena this game serves
	UINT32	hostVersion;	// The version of the arena the game is running
	UINT32	appID;			// The application used to access this game
	UINT32	appVersion;		// Min Version of the app required
							//	to connect to this arena
	UINT32	maxUsers;		// The max users this server can handle
	UINT32	ipAddress;		// IP address of server
	UINT32	port;			// Port of server
	SCHAR	hello[16];		// A hello message
} ;

//This is the game init acknowledgement.  It could later contain all
//	kinds of fun things.
struct AUTH_GAME_ACK
{
	UINT32 gotcha;
} ;

// tell the auth server to re-init the login table for this game
struct CLEAR_LOGIN_TABLE
{
	UINT32	init;
} ;


//This is a connectinfo message from a game server to the Auth server
//	Later we have to figure out a good system to have multiple game servers
//	of the same kind
struct MANAGEMENT_MSG
{
	MANAGEMENT_MSG(UINT32 customerID, UINT32 arenaID, CLIENT_ACTIONS action, UINT32 ip)
		: m_customerID(customerID), m_arenaID(arenaID), m_action(action), m_ip(ip)
		{}
	MANAGEMENT_MSG(ACTION_OBJECT* const a) ;
	void process() const ;			// Auth host function

	UINT32			m_customerID;
	UINT32			m_arenaID;
	CLIENT_ACTIONS	m_action;
	UINT32			m_ip;
} ;

namespace Auth { struct Dispatch ; }

//! @struct OTP_NODE
//! @brief	Describes all the parameters associated with a given
//!			one-time password.
struct OTP_NODE
{
	OTP_NODE() : timeOut(0ULL), OTP(0), customerID(0), address(0), appVersion(0), fromAuth(false) { handle[0] = 0 ; }
	OTP_NODE(Auth::Dispatch& dispatch, bool fromAuth) ;

	~OTP_NODE() ;

	TeulTimeMS	timeOut;					//!< Timestamp of expiration
	UINT32		OTP;						//!< Actual one-time password
	UINT32		customerID;					//!< Customer (auth) ID of client
	UINT32		address;					//!< IP address of client
	UINT32		appVersion;					//!< What version of the application this session used.
	char		handle[MAX_HANDLE_SIZE];	//!< Customer (auth) handle
	bool		fromAuth;					//!< Whether inserted from auth or locally.
} ;

//sent to the authhost to let them know current load status of a
//server
struct AUTH_SERVER_LOAD
{
	SINT32 numPlayers;
	UINT32 hostAddress;
	UINT32	arenaID;		//The arena this game serves
} ;

//sent to the authhost to let them know current load status of a
//server
struct AUTH_GAME_CAPACITY
{
	UINT32 	capacity;		//how many users all the cell hosts can take
	SINT32 	totalPlayers;	//how many users on all the mm + cell hosts
	UINT32	arenaID;		//The arena this game serves
	SINT32	lock;			// Based on config.arena.lock, see AL_* enums
} ;

