#pragma once

#include "teul/teulTime.h"

namespace ww2 { class Client ; }
struct ENDPOINT ;

//! @namespace Teul
//! @brief TeulKit Networking API members.

namespace Teul
{

/** @class Teul::Connection
 *  Abstract base class for asynchronous opening and authentication of a teulkit connection.
 *
 *  Provides a black-box base class for connecting to game
 *  servers.
 *
 *  All relevant steps for opening, connecting and authenticating
 *  are handled discretely with significant event points being
 *  delivered notification through event handlers.
 *
 *  Usage:
 *
 *  Derive an instance of Teul::Connection and allocate with
 *  operator "new". This creates an instance with all necessary
 *  data values.
 *
 *  Once you are ready to begin establishing the connection,
 *  call the start() member function with a pointer to the
 *  ww2Client that will be responsible for the connection.
 *
 *  start() will return false if there are resource issues
 *  or an absence of network connectivity preventing a
 *  connecting being created.
 *
 *  After start() has been successfully called, updates will
 *  be processed as they arrive by calling teulUpdateTransport
 *  (or in the client, client->NetUpdate()).
 *
 *  If the connection is not torn up in the time allotted,
 *  onError will be called with an error code of -ETIMEDOUT.
 *
 *  m_finished will become "true" once the connection has
 *  either completed and logged in or has reached an error
 *  condition forcing login attempts to be abandoned.
 *
 *  Once teulKit has established preliminary communications
 *  with the remote server, member onConnect will be called.
 *  This does NOT indicate readiness for use. At this point,
 *  login() should be called to send the necessary
 *  authentication credentials to establish a player-level
 *  conversation with the server.
 *
 *  After login() has been called, one of three handlers
 *  may be called:
 *   onError() :-
 *     if login times out, or if a network error occurs,
 *   onLoginFail() :-
 *     if the login is refused by the server,
 *   onLoggedIn() :-
 *     once the connection becomes client ready.
 *
 *  "m_connected" is set to true once a TCP connection exists.
 *
 *  "m_loggedIn" is set to true once a player-level connection
 *  has been established and the conncetion is ready for client-
 *  level usage.
 *
 *  "m_finished" is set to true once either the connection has
 *  been logged in and authenticated or has reached a terminal
 *  networking condition.
 *
 *  "m_disposable" is set to true when a connection is closed
 *  or has expired. The object will be deleted on the next
 *  invocation of Teul::Connection::expire()
 *
 */

//////////////////////////////////////////////////////////////////////
// Helper classes.

//! @class Teul::ServerInfo
//! @brief IP address and port of a server.
//! @note In future may also contain server-specific authentication info.
struct ServerAddr
{
	UINT32 m_hostIP ;			//! IPv4 address of remote hsot.
	UINT16 m_port ;				//! Port to connect to.
} ;

//! @class Teul::PlayerIdent
//! @brief Description of a player for authentication/login purposes:
struct PlayerIdent
{
	playerid_t m_playerID ;		//! 0 or the playerID for auth-bypass.
	UINT32 m_otp ;				//! One Time Password used for authentication.
	UINT32 m_clientVersion ;	//! UINT32 representation of client version.
} ;

class Connection
{
	//! Enumeration of host types.
	public: enum HostType { Map, Chat, Cell, NumHostTypes } ;

	//! Default constructor disallowed.
	private: Connection() {}

	//! @brief Initiate a new connection and, upon completion, automatically perform authentication.
	//! @note On-stack construction is not allowed; must be created with operator "new".
	public: Connection(HostType _type, const ServerAddr& _serverInfo, const PlayerIdent& _playerIdent, UINT32 _timeoutMS) ;

	// Set to protected to disable on-stack construction.
	protected: virtual ~Connection() ;

	//////////////////////////////////////////////////////////////////////
	//// Main operational functions.

	//! @brief Start the connection (connect to host).
	public: bool start(ww2::Client* client) ;

	//! @brief Send authentication to host.
	//! Should be called from within onConnect().
	public: void login() ;

	//! @brief Close up and free a connection.
	public: virtual void close() ;

	//////////////////////////////////////////////////////////////////////
	//// Event handlers.

	//! @brief Per-object notification of a successful connection.
	public: virtual void onConnect(ww2::Client* client) = 0 ;

	//! @brief Per-object notification of an error.
	public: virtual void onError(ww2::Client* client, SINT32 error) ;

	//! @brief Per-object notification of a successful login -- connection is now in service.
	public: virtual void onLogin(ww2::Client* client) = 0 ;

	//! @brief Per-object notification of a login failure.
	//! @param[in] client The Client responsible for this connection.
	//! @param[in] result Negative errno code (e.g. -Teul::Errors::LoginFail_SessionValidation).
	public: virtual void onLoginFail(ww2::Client* client, SINT32 result) = 0 ;

	//////////////////////////////////////////////////////////////////////
	// Evaluators.

	//! @brief Determine if this connection is in a serviceable state.
	public: bool isInService() const { return m_loggedIn && connected() ; }

	//! @brief Determine if we think this connection is connected to the server.
	public: bool connected() const { return m_teulIndex >= 0 && m_connected == true ; }

	//! @brief Determine if the connection has completed connection/login.
	public: bool complete() const { return (m_finished || m_loggedIn) ; }

	//! @brief Determine if the connection has decided it needs to di.
	public: bool terminated() const { return (m_disposable) ; }

	//! @brief Determine if the login attempt has failed.
	public: bool hasExpired() const { return (m_started + m_timeoutMS <= teulClock64) ; }

public:
	//////////////////////////////////////////////////////////////////////
	//// [INTERNAL] Service-level event handling.

	//! @brief Low-level (teulKit) interception of login results; do not override.
	public: void onLoginResult(SINT32 result) ;

	//! @brief Handle a connection success and forward it to the correct object.
	public: static void s_onConnect(ENDPOINT& endpoint) ;

	//! @brief Handle a connection error and forward it tothe correct object.
	public: static void s_onError(SINT32 teulIndex, const UCHAR* data, SINT32 size, const TEUL_CALLBACK* callback, SINT32 error) ;

	//! @brief expire connections.
	public: static void expire() ;

public:
	HostType	m_type ;		//! Which type of host attempt is for.
	ServerAddr	m_serverAddr ;	//! IP & port of server, etc.
	PlayerIdent	m_playerIdent ;	//! Player ID, OTP, etc.
	TeulTimeMS	m_timeoutMS ;	//! How long to allow the connection.

	SINT32		m_teulIndex ;	//! TeulIndex of the connection attempting the login.
	UINT32		m_attemptID ;	//! Client's reference for this attempt.

	TeulTimeMS	m_started ;		//! When the attempt started (for timeouts).
	bool		m_connected ;	//! True once we're connected.
	bool		m_loggedIn ;	//! Set true once we have logged in.
	bool		m_finished ;	//! True when we have completed, failed or timed out.
	bool		m_disposable ;	//! Ready to be garbage collected.
};

} // namespace Teul
