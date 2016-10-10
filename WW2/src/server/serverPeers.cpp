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
#include "teul/teulConfigVar.h"
#include "teul/teulConstants.h"
#include "teul/teulEndpoint.h"
#include "teul/teulMicroWebServer.h"
#include "teul/teulPacket.h"
#include "teul/teulPacketHeader.h"
#include "wwii/wwiiNetPorts.h"
#include "wwii/wwiiCallbacks.h"

extern ConfigUint cv_peerConnectionTimeout_ms;

// Describe the mapping.
struct ServerConnection
{
	ServerConnection(const UINT32 ip_, UINT32 type_, const SINT16 teulIndex_) : m_ip(ip_), m_type(type_), m_teulIndex(teulIndex_), m_connected(false) {}
	ServerConnection() : m_ip(0), m_type(0), m_teulIndex(-1), m_connected(false) {}

	void doDisconnect();
	void doConnect();

	UINT32		m_ip;			// IP address.
	UINT32		m_type;		// Server Type (see wwiiServerTypeEnums.h)
	SINT16		m_teulIndex;	// teulIndex.
	SINT16		m_connected;	// true once connected.
};

#pragma pack(push, 1)
struct PeerHello
{
	PeerHello(SERVER_TYPE type=SERVER_TYPE(~0)) : m_type(type) {}
	SERVER_TYPE	m_type;
};
#pragma pack(pop)

// There are going to be very few of these, probably 3-4,
// so just store them as a vector.
using ServerConnections = std::vector<ServerConnection>;
static ServerConnections s_connections;
static SINT32 s_peeringIndex = -1;

// Peers who have connected but not hello'd yet.
static std::map<SINT16, TeulTimeMS> s_pendingPeers;

static std::mutex s_peerMutex;

//////////////////////////////////////////////////////////////////////
// JSON description of current peering info.

static void
_reportJSONPeers(SOCKET fd, const char* /*path*/, const char* /*args*/)
{
	size_t maxSize = 128 + (s_connections.size() * 128) + (s_pendingPeers.size() * 64) + 1;
	char* report = static_cast<char*>(alloca(maxSize));
	char* p = report + sprintf(report, "{\"peers\":[");
	{
		std::lock_guard<std::mutex> lock(s_peerMutex);
		for ( auto& it : s_connections )
		{
			p += sprintf(p, "{\"ip\":%u,\"type\":%u,\"idx\":%d,\"conn\":%u},", it.m_ip, it.m_type, it.m_teulIndex, it.m_connected);
		}
		p += sprintf(p, "],\"pending\":[");
		for ( auto& it : s_pendingPeers )
		{
			p += sprintf(p, "{\"ip\":%u,\"time\":%llu},", it.first, teulClock64 - it.second);
		}
	}
	p += sprintf(p, "]}\n");

	send(fd, report, int(p - report), 0);
}

//////////////////////////////////////////////////////////////////////
// Find a connection in the connections list by IP address. Returns
// pointer or nullptr.

static ServerConnection*
_findConnectionByIP(const UINT32 ip_, const UINT16 type_)
{
	const UINT32 localIP = Server::BroadcastMaskAddress(ip_);
	const size_t numConnections = s_connections.size();
	for ( size_t i = 0; i < numConnections; ++i )
	{
		if ( s_connections[i].m_ip == localIP && s_connections[i].m_type == type_ )
			return &s_connections[i];
	}

	return nullptr;
}

//////////////////////////////////////////////////////////////////////
// Find a connection in the connections list by teulIndex. Returns
// pointer or nullptr.

static ServerConnection*
_findConnectionByTeulIndex(const SINT32 teulIndex)
{
	// Not valid to search by negative index.
	if ( PN_UNLIKELY(teulIndex < 0) )
		return nullptr;
	const size_t numConnections = s_connections.size();
	for ( size_t i = 0; i < numConnections; ++i )
	{
		if ( s_connections[i].m_teulIndex == teulIndex )
			return &s_connections[i];
	}

	return nullptr;
}

//////////////////////////////////////////////////////////////////////
// Called onConnect for a server peer.

static void
_onConnect(ENDPOINT& endpoint)
{
	// Localize the address.
	const SERVER_TYPE peerType = static_cast<SERVER_TYPE>(endpoint.destPort() - WWII::Net::Ports::ServerPeering);
	ABORT_IF(peerType >= MAX_SERVER_TYPE);
	ServerConnection* const connection = _findConnectionByIP(endpoint.destAddress(), peerType);
	if ( connection == nullptr )
	{
		dbugLog(_FLW, "t:%d ip:%u reporting connected, not in s_connections list.", endpoint.teulIndex(), endpoint.fromAddress());
		return;
	}

	char addr[65];
	teulGetAddress(connection->m_ip, addr, sizeof(addr));
	dbugLog(_FLD, "ip:%s:%u t:%d %s connected on t:%d"
					, addr, endpoint.destPort()
					, connection->m_teulIndex, g_serverTypes[peerType], endpoint.teulIndex()
					);

	{
		std::lock_guard<std::mutex> lock(s_peerMutex);

		// Paranoia: Make sure the teulIndexes match.
		if ( connection->m_teulIndex != endpoint.teulIndex() )
			connection->m_teulIndex = endpoint.teulIndex();

		connection->m_connected = true;
	}

	Teul::Packet outgoing(Teul::PacketId(Teul::Serv::CallbackX, PEER_HELLO_Y));
	outgoing.construct<PeerHello>(Server::myServerInfo.stats.type);
	outgoing.send(endpoint);
}

//////////////////////////////////////////////////////////////////////
// Handle a connection error.

static void
_onError(SINT32 teulIndex, const UCHAR* const /*data*/, SINT32 /*size*/, const TEUL_CALLBACK* /*object*/, SINT32 error)
{
	if ( error == ENOTCONN )
	{
		std::lock_guard<std::mutex> lock(s_peerMutex);
		s_pendingPeers.erase(teulIndex);
	}

	ServerConnection* const connection = _findConnectionByTeulIndex(teulIndex);
	if ( connection == nullptr )
		return;

	char addr[65];
	teulGetAddress(connection->m_ip, addr, sizeof(addr));

	if ( error == ENOTCONN )
	{
		if ( connection->m_connected )
		{
			dbugLog(_FLD, "%s serv:%s t:%d disconnected", g_serverTypes[connection->m_type], addr, teulIndex);
			connection->m_teulIndex = -1;
			connection->m_connected = false;
		}
		return;
	}

	dbugLog(_FLM, "serv:%s t:%d connection error:%d", addr, teulIndex, error);
}

//////////////////////////////////////////////////////////////////////
// Common code to disconnect a given connection.

void
ServerConnection::doDisconnect()
{
	m_connected = false;
	const SINT16 usedTeulIndex = m_teulIndex;
	if ( usedTeulIndex >= 0 )
	{
		m_teulIndex = -1;
		teulClose(usedTeulIndex, Teul::Closing::PeerDoDisc);
	}
}

//////////////////////////////////////////////////////////////////////
// Establish connection to a server.

void
ServerConnection::doConnect()
{
	m_connected = false;
	UINT16 port = WWII::Net::Ports::ServerPeering + m_type;
	m_teulIndex = teulOpenSend(m_ip, port, _onConnect, cv_peerConnectionTimeout_ms.GetUint32(), PROTOCOL_TCP, _onError);
	dbugLog(_FLD, "t:%d opening to %s %s:%u [%u]", m_teulIndex, teulGetAddress(m_ip), g_serverTypes[m_type], port, m_type);
}

//////////////////////////////////////////////////////////////////////
// Register a new peer to the list and make a connection to them.
// If they're already in the list, disconnect the old entry and use
// the new connection instead.

void
serverConnectPeer(const UINT32 forIP_, const SERVER_TYPE type_)
{
	char dest[65];
	teulGetAddress(forIP_, dest, sizeof(dest));
	dbugLog(_FLI, "requested %s connection: type:%u ip:%s", g_serverTypes[type_], type_, dest);
	const SINT32 net = forIP_ >> 24;
	if ( (net == 127) | (net == 0) )
	{
		// Ignore localhost and exit host.
		dbugLog(_FLI, " - ignoring local/exit host.");
		return;
	}
	// Ignore requests to connect to ourselves or to peers with lower IPs,
	// we only ever connect upwards. This helps us also avoid trying to
	// connect to the exit server (ip 0).
	const UINT32 ip = Server::BroadcastMaskAddress(forIP_);
	if ( (forIP_ < Server::myServerInfo.stats.address)
		|| (forIP_ == Server::myServerInfo.stats.address && type_ <= Server::myServerInfo.stats.type)
		|| ((ip & 0xff) <= (Server::myServerInfo.stats.address & 0xff) && type_ <= Server::myServerInfo.stats.type)
		|| ((forIP_ >> 24) == 127)
		)
	{
		dbugLog(_FLI, " - leaving this [%u] one to connect to me [%u]", (ip & 0xff), (Server::myServerInfo.stats.address & 0xff));
		return;
	}

	std::lock_guard<std::mutex> lock(s_peerMutex);

	// Only consider the last octet.
	ServerConnection* const connection = _findConnectionByIP(ip, type_);
	if ( connection != nullptr )
	{
		// The server has probably restarted, so we may already have
		// a connection open, which is no-longer valid.
		connection->doDisconnect();

		// Open the new connection.
		connection->doConnect();

		return;
	}

	// Being still here indicates the server wasn't found.
	s_connections.push_back(ServerConnection(ip, type_, -1));

	s_connections.back().doConnect();
}

//////////////////////////////////////////////////////////////////////
// Disconnect a peer.

void
serverDisconnectPeer(Server::ServerInfo* server)
{
	ServerConnection* const connection = _findConnectionByIP(server->stats.address, server->stats.type);
	if ( connection != nullptr )
	{
		dbugLog(_FLI, "t:%d %s serv:%s disconnecting", connection->m_teulIndex, g_serverTypes[server->stats.type], teulGetAddress(ntohl(server->stats.address)));
		connection->doDisconnect();
	}
}

//////////////////////////////////////////////////////////////////////
// Return the teulIndex of a connected peer or -1.

teulindex_t
serverGetPeerIndex(const UINT32 ip_, const SERVER_TYPE serverType_)
{
	const ServerConnection* const connection = _findConnectionByIP(ip_, serverType_);
	if ( connection == nullptr || connection->m_connected == false )
		return -1;
	else
		return connection->m_teulIndex;
}

//////////////////////////////////////////////////////////////////////
// Repeat a Teul::Packet to all servers of a given class.

bool
serverSendPacketToType(Teul::Packet& outgoing, const SERVER_TYPE serverType)
{
	bool sent = false;

	outgoing.setUnencrypted();

	for ( auto& server : s_connections )
	{
		// Find servers matching the given type.
		if ( (server.m_type != serverType) | (!server.m_connected) )
			continue;

		const SINT32 err = outgoing.send(server.m_teulIndex);
		if ( err < 0 )
		{
			dbugLog(_FLW, "bx cb:%u,%u to t:%d %s ip:%s failed:%d", outgoing.header().m_callX, outgoing.header().m_callY, server.m_teulIndex, g_serverTypes[server.m_type], teulGetAddress(server.m_ip), err);
			continue;
		}

		sent = true;
	}

	return sent;
}

//////////////////////////////////////////////////////////////////////
// Repeat a Teul::Packet to all peering servers.

bool
serverSendPacketToPeers(Teul::Packet& outgoing)
{
	bool sent = false;

	outgoing.setUnencrypted();

	for ( auto& server : s_connections )
	{
		// Find servers matching the given type.
		if ( !server.m_connected )
			continue;

		const SINT32 err = outgoing.send(server.m_teulIndex);
		if ( err < 0 )
		{
			dbugLog(_FLW, "bx cb:%u,%u to t:%d %s ip:%s failed:%d", outgoing.header().m_callX, outgoing.header().m_callY, server.m_teulIndex, g_serverTypes[server.m_type], teulGetAddress(server.m_ip), err);
			continue;
		}

		sent = true;
	}

	return sent;
}

//////////////////////////////////////////////////////////////////////
// Transmit a callback to all of the corresponding hosts of that type.

bool
serverSendCallbackToType(const SERVER_TYPE serverType, TEUL_CALLBACK* const tcb, void* data)
{
	if ( PN_UNLIKELY(tcb == nullptr) )
	{
		dbugLog(_FLW, "tried to send NULL callback");
		return false;
	}

	bool sent = false;

	for ( auto& server : s_connections )
	{
		// Find servers matching the given type.
		if ( (server.m_type != serverType) | (!server.m_connected) )
			continue;

		const SINT32 err = teulSend(server.m_teulIndex, tcb, data);
		if ( PN_UNLIKELY(err < 0) )
		{
			dbugLog(_FLW, "tbx cb:%u,%u to t:%d %s ip:%s failed:%d", tcb->callX(), tcb->callY(), server.m_teulIndex, g_serverTypes[server.m_type], teulGetAddress(server.m_ip), err);
			continue;
		}

		sent = true;
	}

	return sent;
}

//////////////////////////////////////////////////////////////////////
// Accept an incoming peering connection.

static void
_onPeerConnect(ENDPOINT& endpoint)
{
	// Restrict to local addresses.
	if ( Server::BroadcastMaskAddress(endpoint.fromAddress()) != endpoint.fromAddress() )
	{
		dbugLog(_FLA, "t:%d ip:%s unexpected peering attempt", endpoint.teulIndex(), teulGetAddress(endpoint.fromAddress()));
		teulClose(endpoint, Teul::Closing::PeerBadSrc);
		return;
	}

	char addr[65];
	teulGetAddress(endpoint.fromAddress(), addr, sizeof(addr));
	dbugLog(_FLI, "t:%d ip:%s:%u peer connection", endpoint.teulIndex(), addr, endpoint.fromPort());

	{
		std::lock_guard<std::mutex> lock(s_peerMutex);
		s_pendingPeers[endpoint.teulIndex()] = teulClock64;
	}
}

static SINT32
_onPeerHello(ACTION_OBJECT* a, PeerHello& hello)
{
	ABORT_IF(a->endpoint == nullptr);

	ENDPOINT& endpoint = *a->endpoint;
	///TODO: We should mark this somehow for expiration.
	Server::ServerInfo* info = Server::ServerInfo::GetInfo(endpoint.fromAddress(), hello.m_type);
	if ( PN_UNLIKELY(info == nullptr) )
	{
		dbugLog(_FLA, "t:%d ip:%s type:%u unknown server attempted connection", endpoint.teulIndex(), teulGetAddress(endpoint.fromAddress()), hello.m_type);
		teulClose(endpoint, Teul::Closing::PeerBadSrv);
		return -EINVAL;
	}
	ABORT_IF( info->stats.type != hello.m_type );

	std::lock_guard<std::mutex> lock(s_peerMutex);

	UINT32 ip = Server::BroadcastMaskAddress(endpoint.fromAddress());
	ServerConnection* connection = _findConnectionByIP(ip, hello.m_type);
	if ( connection == nullptr )
	{
		s_connections.push_back(ServerConnection(ip, info->stats.type, endpoint.teulIndex()));
		connection = &s_connections.back();
	}
	ABORT_IF( connection->m_type != hello.m_type );

	if ( connection->m_teulIndex != endpoint.teulIndex() )
		connection->doDisconnect();

	connection->m_teulIndex = endpoint.teulIndex();
	connection->m_ip = ip;
	connection->m_type = info->stats.type;
	connection->m_connected = true;

	char addr[65];
	teulGetAddress(connection->m_ip, addr, sizeof(addr));
	dbugLog(_FLI, "t:%d ip:%s:%u %s peer connection complete"
					, connection->m_teulIndex, addr, endpoint.fromPort()
					, g_serverTypes[hello.m_type]
					);

	s_pendingPeers.erase(endpoint.teulIndex());

	return a->offset;
}

//////////////////////////////////////////////////////////////////////
// Handle an error on the peering interface.

static void
_onPeerError(SINT32 teulIndex, const UCHAR* const /*data*/, SINT32 /*size*/, const TEUL_CALLBACK* /*object*/, SINT32 error)
{
	dbugLog(_FLW, "t:%d[peerif] error:%d", teulIndex, error);
}

//////////////////////////////////////////////////////////////////////
// Register peering-specific callbacks.

static SINT32
_registerPeeringCallbacks()
{
	BEGIN_CALLBACK_REGISTRATION(Teul::Serv::CallbackX)
	{
		REGISTER_RECEIVEOBJ(PEER_HELLO_Y, PeerHello, _onPeerHello);
	}
	END_CALLBACK_REGISTRATION(Teul::Serv::CallbackX)
}

//////////////////////////////////////////////////////////////////////
// Initialize the peering interface.

void
serverInitializePeering()
{
	if ( _registerPeeringCallbacks() < 0 )
		LogAbort("Unable to register peering callbacks");
	const UINT16 peeringPort = WWII::Net::Ports::ServerPeering + Server::myServerInfo.stats.type;
	s_peeringIndex = teulOpenListen(peeringPort, _onPeerConnect, cv_peerConnectionTimeout_ms.GetUint32(), PROTOCOL_TCP, _onPeerError);
	if ( s_peeringIndex < 0 )
		LogAbort("Unable to open peering interface");

	dbugLog(_FLN, "t:%d server peering interface opened on port %u", s_peeringIndex, peeringPort);

	Teul::uWeb::s_debugServer.addVirtualFile("Peering Status", "/peering.json", _reportJSONPeers);
}

//////////////////////////////////////////////////////////////////////
// Expire any peer connections that haven't hello'd.

void
serverExpirePeerConnections()
{
	if ( s_pendingPeers.empty() )
		return;
#if defined(DEBUG)
	// You have 15 seconds to identify yourself.
	const TeulTimeMS cutoff = teulClock64 - 5000;
#else
	// You get 500ms.
	const TeulTimeMS cutoff = teulClock64 - 500;
#endif
	std::vector<SINT16> discos;
	for ( auto& it : s_pendingPeers )
	{
		if ( it.second < cutoff )
			discos.push_back(it.first);
	}

	for ( auto& teulIndex : discos )
	{
		dbugLog(_FLI, "t:%d no hello received - disconnecting.", teulIndex);
		teulClose(teulIndex);
	}
}

//////////////////////////////////////////////////////////////////////
// Return true if we have an open peering with the specified
// server.

bool
serverPeeringWithIP(const UINT32 ip, const SERVER_TYPE type_)
{
	const ServerConnection* const connection = _findConnectionByIP(ip, type_);
	// We are "peering" if there exists a connection and it has an IP address.
	// We don't check if the connection is established to avoid confusion
	// while a connection is opening.
	return ( connection != nullptr && connection->m_teulIndex >= 0 );
}

//////////////////////////////////////////////////////////////////////
// Return true if we are peering with one or more instance of the
// specified server type.

bool
serverPeeringWithType(const SERVER_TYPE serverType)
{
	for ( auto& server : s_connections )
	{
		// Find servers matching the given type.
		if ( (server.m_type == serverType) & (server.m_connected) )
			return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////
// Return an endpoint for a given server type.

ENDPOINT*
serverPeerEndpointByType(const SERVER_TYPE serverType)
{
	ENDPOINT* endpoint = nullptr;
	for ( auto& server : s_connections )
	{
		// Find servers matching the given type.
		if ( (server.m_type != serverType) | (!server.m_connected) )
			continue;
		if ( endpoint == nullptr )
		{
			endpoint = teulGetEndpoint(server.m_teulIndex);
			continue;
		}

		// We already have an endpoint.
		ENDPOINT* thisEndpoint = teulGetEndpoint(server.m_teulIndex);
		if ( thisEndpoint == nullptr )
			continue;

		dbugLog(_FLA, "multiple servers of type %u (%s) connected - unable to select for EndpointByType"
						, serverType, g_serverTypes[serverType]
						);
		return nullptr;
	}

	return endpoint;
}

