#include <syscfg.h> // Path is project specific.
// Created: Fri Jun 15 15:21:04 CDT 2012
//
// Copyright (c) 2012 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Manage direct connections to each peer server in
// the current cluster so that we can send them
// big packets easily.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "server/serverIndexes.h"
#include "server/serverPeers.h"
#include "server/serverServerInfo.h"
#include "teul/teulCallbackStruct.h"
#include "teul/teulEndpoint.h"
#include "teul/teulPacket.h"
#include "teul/teulPacketHeader.h"
#include "wwii/wwiiNetPorts.h"

// Describe the mapping.
struct ServerConnection
{
	ServerConnection(const UINT32 ip_, UINT32 type_, const SINT16 teulIndex_) : m_ip(ip_), m_type(type_), m_teulIndex(teulIndex_), m_connected(false) {}
	ServerConnection() : m_ip(0), m_type(0), m_teulIndex(-1), m_connected(false) {}

	void doDisconnect() ;
	void doConnect() ;

	UINT32		m_ip ;			// IP address.
	UINT32		m_type ;		// Server Type (see wwiiServerTypeEnums.h)
	SINT16		m_teulIndex ;	// teulIndex.
	SINT16		m_connected ;	// true once connected.
} ;

// There are going to be very few of these, probably 3-4,
// so just store them as a vector.
typedef std::vector<ServerConnection> ServerConnections ;
static ServerConnections s_connections ;
static SINT32 s_peeringIndex = -1 ;

//////////////////////////////////////////////////////////////////////
// Find a connection in the connections list by IP address. Returns
// pointer or NULL.

static ServerConnection*
_findConnectionByIP(const UINT32 ip)
{
	const UINT32 localIP = Server::BroadcastMaskAddress(ip) ;
	const size_t numConnections = s_connections.size() ;
	for ( size_t i = 0 ; i < numConnections ; ++i )
	{
		if ( s_connections[i].m_ip == localIP )
			return &s_connections[i] ;
	}

	return NULL ;
}

//////////////////////////////////////////////////////////////////////
// Find a connection in the connections list by teulIndex. Returns
// pointer or NULL.

static ServerConnection*
_findConnectionByTeulIndex(const SINT32 teulIndex)
{
	// Not valid to search by negative index.
	if ( teulIndex < 0 )
		return NULL ;
	const size_t numConnections = s_connections.size() ;
	for ( size_t i = 0 ; i < numConnections ; ++i )
	{
		if ( s_connections[i].m_teulIndex == teulIndex )
			return &s_connections[i] ;
	}

	return NULL ;
}

//////////////////////////////////////////////////////////////////////
// Called onConnect for a server peer.

static void
_onConnect(ENDPOINT& endpoint)
{
	// Localize the address.
	ServerConnection* const connection = _findConnectionByIP(endpoint.destAddress()) ;
	if ( connection != NULL )
	{
		char addr[65] ;
		teulGetAddress(connection->m_ip, addr, sizeof(addr)) ;
		dbugLog(_FLD, "serv:%s t:%d connected on t:%d", addr, connection->m_teulIndex, endpoint.teulIndex()) ;

		// Paranoia: Make sure the teulIndexes match.
		if ( connection->m_teulIndex != endpoint.teulIndex() )
		{
			connection->m_teulIndex = endpoint.teulIndex() ;
		}
		
		connection->m_connected = true ;

		return ;
	}

	dbugLog(_FLW, "t:%d ip:%u reporting connected, not in s_connections list.", endpoint.teulIndex(), endpoint.fromAddress()) ;
}

//////////////////////////////////////////////////////////////////////
// Handle a connection error.

static void
_onError(SINT32 teulIndex, const UCHAR* const /*data*/, SINT32 /*size*/, const TEUL_CALLBACK* /*object*/, SINT32 error)
{
	ServerConnection* const connection = _findConnectionByTeulIndex(teulIndex) ;
	if ( connection == NULL )
		return ;

	char addr[65] ;
	teulGetAddress(connection->m_ip, addr, sizeof(addr)) ;

	if ( error == ENOTCONN )
	{
		if ( connection->m_connected )
		{
			dbugLog(_FLD, "serv:%s t:%d disconnected", addr, teulIndex) ;
			connection->m_teulIndex = -1 ;
			connection->m_connected = false ;
		}
		return ;
	}

	dbugLog(_FLM, "serv:%s t:%d connection error:%d", addr, teulIndex, error) ;
}

//////////////////////////////////////////////////////////////////////
// Common code to disconnect a given connection.

void
ServerConnection::doDisconnect()
{
	m_connected = false ;
	const SINT16 usedTeulIndex = m_teulIndex ;
	if ( usedTeulIndex >= 0 )
	{
		m_teulIndex = -1 ;
		teulClose(usedTeulIndex, Teul::Closing::PeerDoDisc) ;
	}
}

//////////////////////////////////////////////////////////////////////
// Establish connection to a server.

void
ServerConnection::doConnect()
{
	m_connected = false ;
	UINT16 port = WWII::Net::Ports::ServerPeering + m_type;
	m_teulIndex = teulOpenSend(m_ip, port, _onConnect, 20000, TCP, _onError) ;
	dbugLog(_FLD, "t:%d opening to %s:%u [%u]", m_teulIndex, teulGetAddress(m_ip), port, m_type) ;
}

//////////////////////////////////////////////////////////////////////
// Register a new peer to the list and make a connection to them.
// If they're already in the list, disconnect the old entry and use
// the new connection instead.

void
serverConnectPeer(const UINT32 forIP, const UINT32 type)
{
	char dest[65] ;
	teulGetAddress(forIP, dest, sizeof(dest)) ;
	dbugLog(_FLI, "requested connection: type:%u ip:%s", type, dest) ;
	const SINT32 net = forIP >> 24 ;
	if ( (net == 127) | (net == 0) )
	{
		// Ignore localhost and exit host.
		dbugLog(_FLIB, " - ignoring local/exit host.") ;
		return ;
	}
	// Ignore requests to connect to ourselves or to peers with lower IPs,
	// we only ever connect upwards. This helps us also avoid trying to
	// connect to the exit server (ip 0).
	if ( ((forIP >> 24) == 127) | (forIP <= Server::myServerInfo.stats.address) )
	{
		dbugLog(_FLIB, " - skipping this one") ;
		return ;
	}

	const UINT32 ip = Server::BroadcastMaskAddress(forIP) ;
	if ( (ip & 0xff) <= (Server::myServerInfo.stats.address & 0xff) )
	{
		dbugLog(_FLIB, " - leaving this [%u] one to connect to me [%u]", (ip & 0xff), (Server::myServerInfo.stats.address & 0xff)) ;
		return ;
	}

	// Only consider the last octet.
	ServerConnection* const connection = _findConnectionByIP(ip) ;
	if ( connection != NULL )
	{
		// The server has probably restarted, so we may already have
		// a connection open, which is no-longer valid.
		connection->doDisconnect() ;

		// Open the new connection.
		connection->doConnect() ;

		return ;
	}

	// Being still here indicates the server wasn't found.
	s_connections.push_back(ServerConnection(ip, type, -1)) ;
	s_connections.back().doConnect() ;
}

//////////////////////////////////////////////////////////////////////
// Disconnect a peer.

void
serverDisconnectPeer(const UINT32 ip)
{
	ServerConnection* const connection = _findConnectionByIP(ip) ;
	if ( connection != NULL )
	{
		dbugLog(_FLI, "t:%d serv:%s disconnecting", connection->m_teulIndex, teulGetAddress(ip)) ;
		connection->doDisconnect() ;
	}
}

//////////////////////////////////////////////////////////////////////
// Return the teulIndex of a connected peer or -1.

SINT16
serverGetPeerIndex(const UINT32 ip)
{
	const ServerConnection* const connection = _findConnectionByIP(ip) ;
	if ( connection == NULL || connection->m_connected == false )
		return -1 ;
	else
		return connection->m_teulIndex ;
}

//////////////////////////////////////////////////////////////////////
// Repeat a Teul::Packet to all servers of a given class.

bool
serverSendPacketToType(Teul::Packet& outgoing, const SERVER_TYPE serverType)
{
	bool sent = false ;

	outgoing.setUnencrypted() ;

	for ( auto server : s_connections )
	{
		// Find servers matching the given type.
		if ( (server.m_type != serverType) | (!server.m_connected) )
			continue ;

		const SINT32 err = outgoing.send(server.m_teulIndex) ;
		if ( err < 0 )
		{
			dbugLog(_FLW, "bx cb:%u,%u to t:%d ip:%s failed:%d", outgoing.header().m_callX, outgoing.header().m_callY, server.m_teulIndex, teulGetAddress(server.m_ip), err) ;
			continue ;
		}

		sent = true ;
	}

	return sent ;
}

//////////////////////////////////////////////////////////////////////
// Repeat a Teul::Packet to all peering servers.

bool
serverSendPacketToPeers(Teul::Packet& outgoing)
{
	bool sent = false ;

	outgoing.setUnencrypted() ;

	for ( auto server : s_connections )
	{
		// Find servers matching the given type.
		if ( !server.m_connected )
			continue ;

		const SINT32 err = outgoing.send(server.m_teulIndex) ;
		if ( err < 0 )
		{
			dbugLog(_FLW, "bx cb:%u,%u to t:%d ip:%s failed:%d", outgoing.header().m_callX, outgoing.header().m_callY, server.m_teulIndex, teulGetAddress(server.m_ip), err) ;
			continue ;
		}

		sent = true ;
	}

	return sent ;
}

//////////////////////////////////////////////////////////////////////
// Transmit a callback to all of the corresponding hosts of that type.

bool
serverSendCallbackToType(const SERVER_TYPE serverType, TEUL_CALLBACK* const tcb, void* data)
{
	if ( tcb == NULL )
	{
		dbugLog(_FLW, "tried to send NULL callback") ;
		return false ;
	}

	bool sent = false ;

	for ( auto server : s_connections )
	{
		// Find servers matching the given type.
		if ( (server.m_type != serverType) | (!server.m_connected) )
			continue ;

		const SINT32 err = teulSend(server.m_teulIndex, tcb, data) ;
		if ( err < 0 )
		{
			dbugLog(_FLW, "tbx cb:%u,%u to t:%d ip:%s failed:%d", tcb->callX(), tcb->callY(), server.m_teulIndex, teulGetAddress(server.m_ip), err) ;
			continue ;
		}

		sent = true ;
	}

	return sent ;
}

//////////////////////////////////////////////////////////////////////
// Accept an incoming peering connection.

static void
_onPeerConnect(ENDPOINT& endpoint)
{
	// Restrict to local addresses.
	if ( Server::BroadcastMaskAddress(endpoint.fromAddress()) != endpoint.fromAddress() )
	{
		dbugLog(_FLA, "t:%d ip:%s unexpected peering attempt", endpoint.teulIndex(), teulGetAddress(endpoint.fromAddress())) ;
		teulClose(endpoint, Teul::Closing::PeerBadSrc) ;
		return ;
	}

	Server::ServerInfo* info = Server::ServerInfo::GetInfo(endpoint.fromAddress()) ;
	if ( info == NULL )
	{
		dbugLog(_FLA, "t:%d ip:%s unknown server attempted connection", endpoint.teulIndex(), teulGetAddress(endpoint.fromAddress())) ;
		teulClose(endpoint, Teul::Closing::PeerBadSrv) ;
		return ;
	}

	UINT32 ip = Server::BroadcastMaskAddress(endpoint.fromAddress()) ;
	ServerConnection* connection = _findConnectionByIP(ip) ;
	if ( connection == NULL )
	{
		s_connections.push_back(ServerConnection(ip, info->stats.type, endpoint.teulIndex())) ;
		connection = &s_connections.back() ;
	}

	if ( connection->m_teulIndex != endpoint.teulIndex() )
		connection->doDisconnect() ;

	connection->m_teulIndex = endpoint.teulIndex() ;
	connection->m_ip = ip ;
	connection->m_type = info->stats.type ;
	connection->m_connected = true ;

	char addr[65] ;
	teulGetAddress(connection->m_ip, addr, sizeof(addr)) ;
	dbugLog(_FLI, "serv:%s t:%d connected as peer", addr, connection->m_teulIndex) ;
}

//////////////////////////////////////////////////////////////////////
// Handle an error on the peering interface.

static void
_onPeerError(SINT32 teulIndex, const UCHAR* const /*data*/, SINT32 /*size*/, const TEUL_CALLBACK* /*object*/, SINT32 error)
{
	dbugLog(_FLW, "t:%d[peerif] error:%d", teulIndex, error) ;
}

//////////////////////////////////////////////////////////////////////
// Initialize the peering interface.

void
serverInitializePeering()
{
	const UINT16 peeringPort = WWII::Net::Ports::ServerPeering + Server::myServerInfo.stats.type;
	s_peeringIndex = teulOpenListen(peeringPort, _onPeerConnect, 10000, TCP, _onPeerError) ;
	if ( s_peeringIndex < 0 )
		LogAbort("Unable to open peering interface") ;
	dbugLog(_FLN, "t:%d Server peering interface opened on port %u", s_peeringIndex, peeringPort) ;
}

//////////////////////////////////////////////////////////////////////
// Return true if we have an open peering with the specified
// server.

bool
serverPeeringWithIP(const UINT32 ip)
{
	const ServerConnection* const connection = _findConnectionByIP(ip) ;
	// We are "peering" if there exists a connection
	// and it has an IP address. We don't check if
	// the connection is established to avoid confusion
	// while a connection is opening.
	return ( connection != NULL && connection->m_teulIndex >= 0 ) ;
}

//////////////////////////////////////////////////////////////////////
// Return true if we are peering with one or more instance of the
// specified server type.

bool
serverPeeringWithType(const SERVER_TYPE serverType)
{
	for ( auto server : s_connections )
	{
		// Find servers matching the given type.
		if ( (server.m_type == serverType) & (server.m_connected) )
			return true ;
	}
	return false ;
}

//////////////////////////////////////////////////////////////////////
// Return an endpoint for a given server type.

ENDPOINT*
serverPeerEndpointByType(const SERVER_TYPE serverType)
{
	ENDPOINT* endpoint = NULL ;
	for ( auto server : s_connections )
	{
		// Find servers matching the given type.
		if ( (server.m_type != serverType) | (!server.m_connected) )
			continue ;
		if ( endpoint == NULL )
		{
			endpoint = teulGetEndpoint(server.m_teulIndex) ;
			continue ;
		}

		// We already have an endpoint.
		ENDPOINT* thisEndpoint = teulGetEndpoint(server.m_teulIndex) ;
		if ( thisEndpoint == NULL )
			continue ;

		dbugLog(_FLA, "multiple servers of type %u connected - unable to select for EndpointByType", serverType) ;
		return NULL ;
	}

	return endpoint ;
}

