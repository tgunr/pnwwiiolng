#pragma once

#include "server/serverStatusEnum.h"
#include "wwii/wwiiServerTypeEnums.h"

namespace Server
{

// each stand alone process must send its own heartbeat
// e.g. chatd and mmhostd would both send heatbeats, but
// hostd and stratUpdate would only send one.
struct HeartBeat
{
	HeartBeat() : id(0ULL), address(0), port(0), type(MAX_SERVER_TYPE), teulVersion(0), procVersion(0), appVersion(0), m_numPlayers(0), m_maxPlayers(0), status(SERVER_LOGIN), arenaID(0) {}

	UINT64			id;				// mac address of eth0 + pid
	UINT32			address;		// ip address of eth0
	UINT16			port;			// port to accept players on.
	SERVER_TYPE		type;			// server type, servers are grouped by type
	UINT32			teulVersion;	// version of teul being run
	UINT32			procVersion;	// version of process being run
	UINT32			appVersion;		// version of the client application
	SINT32			m_numPlayers ;	//! current number of logged-in players.
	SINT32			m_maxPlayers ;	//! maximum number of players (capacity).
	Server::Status	status;			// server status
	UINT32			arenaID;

// Member functions:
public:
	SINT32 numPlayers() const { return (m_numPlayers > 0) ? m_numPlayers : 0 ; }
	SINT32 maxPlayers() const { return m_maxPlayers ; }

	void addPlayer() { ++m_numPlayers ; }
	void removePlayer() { if ( m_numPlayers > 0 ) --m_numPlayers ; }

	SINT32 availPlayers() { return (maxPlayers() > numPlayers()) ? maxPlayers() - numPlayers() : 0 ; }

	void process() ;
} ;

}

