#include "syscfg.h" // Path is project specific.
// This module provides access to the socket
// functions used by the teul API. The end-user developer
// should never need the functions provided here.
//
// Copyright 1999 Playnet Inc, All rights reserved
// Not to be distributed in API form except as part of a compiled
// library
//
// Originating Programmer: Martin Poulin

#include "mem/mem.h"
#include "teul/teulBasic.h"
#include "teul/teulCallback.h"
#include "teul/teulCallbackStruct.h"
#include "teul/teulClosingEnums.h"
#include "teul/teulConstants.h"
#include "teul/teulEndpoint.h"
#include "teul/teulEndpointIdleList.h"
#include "teul/teulEndpointImpl.h"
#include "teul/teulModCalls.h"
#include "teul/teulModule.h"
#include "teul/teulTime.h"
#include "teul/teulUtils.h"
#include "teul/teulVersion.h"
#include "wwii/wwiiNetPorts.h"
#include "teul/raknetConn.h"
#include "include/configVars.h"

#if platOS == OS_WIN32
#pragma comment(lib, "event.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "IPHLPAPI.lib")
#endif

#if platBuild == BUILD_SERVER
#include "server/serverInterfaceDescription.h"
#endif

#define EXT_TOKENS		0x8000000

// Higher than SOMAXCONN.
enum { MAX_ACCEPT_BACKLOG = 1024 };

TeulTimeMS lastUpdateTime;

extern void teulInitEndpoints(ERROR_FUNCTION const);

//////////////////////////////////////////////////////////////////////
// Retrieve a list of the IP addresses this host has on eth0.

#if platBuild == BUILD_SERVER
static void
_getHostIPAddresses()
{
#if platOS == OS_LINUX || platOS == OS_MACOS
	static const char* primaryInterfaceName = cv_defaultInterface.GetString();

	// Get a list of IPv4 network interfaces.
	struct ifaddrs* ifap = nullptr;
	if ( getifaddrs(&ifap) == -1 || ifap == nullptr )
		LogAbort("Unable to get network interfaces");

	// Get the addresses of upto MaxHostIPs interfaces.
	auto& ifNo = Server::myInterface.numIP;
	ifNo = 0;
	for ( auto ifa = ifap; ifa != nullptr; ifa = ifa->ifa_next)
	{
		if ( ifa->ifa_addr == nullptr || ifa->ifa_addr->sa_family != AF_INET )
			continue;

		if ( strcmp(ifa->ifa_name, primaryInterfaceName) != 0 )
			continue;

		Server::myInterface.pid = getpid();
		Server::myInterface.addr[ifNo].naddr = ((struct sockaddr_in*)(ifa->ifa_addr))->sin_addr.s_addr;
		Server::myInterface.addr[ifNo].ip = ntohl(Server::myInterface.addr[ifNo].naddr);
		dbugLog(_FLI, "read index %u interface %s, ip %s pid %u"
					, ifNo
					, ifa->ifa_name
					, teulGetAddress(Server::myInterface.addr[ifNo].ip)
					, Server::myInterface.pid
					//, Server::myInterface.id
					);

		++ifNo;
		if ( ifNo >= Server::InterfaceDescription::MaxHostIPs )
			break;
	}

	freeifaddrs(ifap);
#elif platOS == OS_WIN32
	PIP_ADAPTER_ADDRESSES pAddresses = nullptr;
	DWORD outBufLen = 32 * 1024;	// Start with 32Kb of buffer to retrieve data.
	DWORD dwRetVal = ERROR_BUFFER_OVERFLOW;

	DWORD getAddrFlags = GAA_FLAG_INCLUDE_PREFIX | GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST;

	for ( UINT32 attempt = 0; dwRetVal == ERROR_BUFFER_OVERFLOW && attempt < 6; ++attempt )
	{
		if (pAddresses)
			memFree(pAddresses);
		pAddresses = static_cast<PIP_ADAPTER_ADDRESSES>(memAlloc(outBufLen));
		ABORT_IF_EX(pAddresses == nullptr, "Failed to allocate memory for listing network interfaces.");
		dwRetVal = GetAdaptersAddresses(AF_INET, getAddrFlags, nullptr, pAddresses, &outBufLen);
	}

	ABORT_IF_EX(dwRetVal == ERROR_BUFFER_OVERFLOW, "Unable to retrieve network interfaces.");

	PIP_ADAPTER_ADDRESSES preferredIf = nullptr;
	for ( auto curAddr = pAddresses; curAddr != nullptr; curAddr = curAddr->Next )
	{
		// Only look for ethernet networks with IfOperStatusUp and a unicast address.
		if ( curAddr->OperStatus != IfOperStatusUp || curAddr->FirstUnicastAddress == nullptr )
			continue;
		if ( curAddr->Flags & IP_ADAPTER_RECEIVE_ONLY )
			continue;

		switch (curAddr->IfType)
		{
		case IF_TYPE_PPP:
		case IF_TYPE_SOFTWARE_LOOPBACK:
		case IF_TYPE_TUNNEL:
		case IF_TYPE_IEEE1394:
			continue;
		break;
		default:
		break;
		}

		// Try to improve on any previous interface address.
		if (preferredIf)
		{
			if (curAddr->FirstDnsServerAddress == nullptr)
				continue;
			if (preferredIf->FirstDnsServerAddress)
			{
#if (NTDDI_VERSION >= NTDDI_VISTA)
				if (curAddr->FirstGatewayAddress == nullptr)
					continue;
#endif
			}
		}

		preferredIf = curAddr;
	}

	ABORT_IF_EX(preferredIf == nullptr
				, "Unable to determine primary network adapter. "
				  "If you need to specify an interface, set 'DEFAULT_IFACE' to the friendly name of the adapter.");

	Server::myInterface.pid = static_cast<UINT32>(GetCurrentProcessId());

	auto address = preferredIf->FirstUnicastAddress;
	auto& ifNo = Server::myInterface.numIP;
	for ( ifNo = 0; ifNo < Server::InterfaceDescription::MaxHostIPs && address != nullptr; ++ifNo, address = address->Next)
	{
		struct sockaddr_in* saddr = (struct sockaddr_in*)(address->Address.lpSockaddr);
		Server::myInterface.addr[ifNo].naddr = saddr->sin_addr.S_un.S_addr;
		Server::myInterface.addr[ifNo].ip = ntohl(Server::myInterface.addr[0].naddr);

		TCHAR addrString[128];
		DWORD addrSize = sizeof(addrString) / sizeof(TCHAR);
		WSAAddressToString(address->Address.lpSockaddr, address->Address.iSockaddrLength, nullptr, addrString, &addrSize);

		dbugLog(_FLI, "read index %u interface %ls, ip %s pid %u"
						, ifNo
						, preferredIf->FriendlyName
						, addrString
						, Server::myInterface.pid
						);
	}

	memFree(pAddresses);
#endif
}
#endif

SINT32 teulModStartup(ERROR_FUNCTION errFnPtr)
{
#if platBuild == BUILD_SERVER && platOS != OS_WIN32
	signal(SIGPIPE, SIG_IGN);
#endif

	dbugWrite(_FLN, "intitializing teul module version: %s", Teul::VersionString);

	extern void teulInitModRecv();

	teulInitModRecv();

	teulInitEndpoints(errFnPtr);

	teulInitModCallbacks();

	teulSyncClock();
	lastUpdateTime = teulClock64;

#if platBuild == BUILD_SERVER
	_getHostIPAddresses();
#endif

	return 0;
}

using ADDRESS_LIST = std::vector<UINT32>;

#if platBuild == BUILD_SERVER
void
teulGetAddresses(ADDRESS_LIST& addresses)
{
	addresses.clear();
	for ( UINT32 i = 0; i < Server::myInterface.numIP; ++i )
	{
		addresses.push_back(Server::myInterface.addr[i].ip);
	}
}

namespace Teul
{
	bool IsMyAddress(UINT32 address_)
	{
		if ( address_ == INADDR_LOOPBACK )
			return true;
		for ( UINT32 i = 0; i < Server::myInterface.numIP; ++i )
		{
			if ( Server::myInterface.addr[i].ip == address_ )
				return true;
		}
		return false;
	}
}
#endif

SINT32
teulModShutdown()
{
	using namespace Teul;
	for ( auto it = idleEndpoints.begin(), end = idleEndpoints.end(); it != end; )
	{
		ENDPOINT* const endpoint = *(it++);
		if ( PN_LIKELY(endpoint != nullptr) )
			teulClose(*endpoint, Teul::Closing::Shutdown);
	}

	teulModIdle(10);

	teulKillDeadEndpoints();

	// Free up the memory the endpoints are using
	teulCleanupEndpoints();

#if platOS == OS_WIN32
	WSACleanup( );
#endif
	return 0;
}

//////////////////////////////////////////////////////////////////////
// Update the m_peer flag of an endpoint.

static void
_setEndpointPeerState(ENDPOINT& endpoint)
{
#if platBuild == BUILD_SERVER
	if ( endpoint.protocol == PROTOCOL_UDB || endpoint.protocol == PROTOCOL_CTRL || endpoint.state() == IS_LISTENING )
		endpoint.m_peer = true;
	else if ( endpoint.protocol == PROTOCOL_TCP && (static_cast<UINT32>(endpoint.destPort() - WWII::Net::Ports::ServerPeering) < (UINT32)MAX_SERVER_TYPE) )
		endpoint.m_peer = true;
#else
	endpoint.m_peer = false;
#endif
}

//////////////////////////////////////////////////////////////////////
// Socket configuration.

// TCP sockets are generally WAN, so they have different buffer sizes.
static const size_t TcpRcvBufSize = 8 * 1024;
static const size_t TcpSndBufSize = 4 * 1024;

// UDP sockets are all LAN and need large buffers.
static const size_t UdpRcvBufSize = 64 * 1024;
static const size_t UdpSndBufSize = 64 * 1024;

void teulConfigureEndpointSocket(ENDPOINT& endpoint)
{
#if platOS != OS_WIN32 // Windows does not use the same meanings for these
    fcntl(endpoint.socket, F_SETFL, fcntl(endpoint.socket, F_GETFL) | O_NONBLOCK | O_CLOEXEC);
#endif

	endpoint.setRcvBufferSize( endpoint.protocol == PROTOCOL_TCP ? TcpRcvBufSize : UdpRcvBufSize );
	endpoint.setSndBufferSize( endpoint.protocol == PROTOCOL_TCP ? TcpSndBufSize : UdpSndBufSize );

	if ( endpoint.protocol == PROTOCOL_TCP || endpoint.protocol == PROTOCOL_CTRL )
	{
		// Disable Nagle so data doesn't wait to be pushed.
		endpoint.setNagleEnabled(false);

		// Listening sockets need to re-allow use of the address on close.
		if ( endpoint.state() == IS_LISTENING ) {
#if platOS != OS_WIN32 // Windows does not use the same meaning for this
            setsockopt(endpoint.socket, SOL_SOCKET, SO_REUSEADDR, (const int []){ 1 }, sizeof(int));
#endif
        }
#if platOS == OS_MACOS
        setsockopt(endpoint.socket, SOL_SOCKET, SO_NOSIGPIPE, (const int []){ 1 }, sizeof(int));
#endif
	}
	else if ( endpoint.protocol == PROTOCOL_UDB )
	{
		// UDB sockets need to be broadcast-enabled.
		int bcast = 1;
		setsockopt(endpoint.socket, SOL_SOCKET, SO_BROADCAST, (char*)&bcast, sizeof(bcast));
	}
}

//////////////////////////////////////////////////////////////////////
// Error entity for throwing errors during the connection open process

namespace Teul
{
struct OpenError
{
	OpenError(SINT32 errNo_, const char* const msg_) : errNo(errNo_), msg(msg_) {}
	OpenError(const char* const msg_) : errNo(dbugGetErrno()), msg(msg_) {}
    OpenError(const OpenError & rhs) : errNo(rhs.errNo), msg(rhs.msg) { }

	SINT32 errNo;
	const char* const msg;

private:
	OpenError& operator = (const OpenError&);
};
}

//////////////////////////////////////////////////////////////////////
// Prepare a socket for listening -- bind it to the listen address,
// and tell the operating system to listen.

static void
_bindAndListen(ENDPOINT& endpoint)
{
	if ( bind(endpoint.socket, (struct sockaddr*) &endpoint.localSockAddress, sizeof(endpoint.localSockAddress)) < 0 )
		throw Teul::OpenError("bind failed");

	if ( listen(endpoint.socket, MAX_ACCEPT_BACKLOG) < 0 )
		throw Teul::OpenError("listen failed");

	dbugLog(_FLD, "t:%d sock:" PRF_SOCKET " listening socket on %s:%u", endpoint.teulIndex(), PRA_SOCKET(endpoint.socket), teulGetAddress(endpoint.destAddress()), endpoint.destPort());
}

//////////////////////////////////////////////////////////////////////
// Start an outgoing connection.

static void
_connectSocket(ENDPOINT& endpoint)
{
	// Start the connection process. Give the OS a few chances to allocate an address for us.
	for (size_t attempt = 0; attempt < 64; ++attempt)
	{
		int connectResult = connect(endpoint.socket, (struct sockaddr*)&endpoint.destSockAddress, sizeof(endpoint.destSockAddress));
		if ( connectResult >= 0 )
			break;

		const SINT32 error = dbugGetErrno();

		if ( error == PLAT_ECONNREFUSED )
			throw Teul::OpenError(error, "connection refused");

		if ( error == EADDRNOTAVAIL )
			continue;

		// Since our sockets are configured as non-blocking,
		// it's VERY unlikely the connection will happen this
		// quickly - packets have to be exchanged, etc. So the
		// operating system will return an error to tell us that
		// this connection is not a done deal, yet.
		// Any of EAGAIN, EWOULDBLOCK or EINPROGRESS means that
		// it's begun, but not finished.
		const bool inProgress = (error == PLAT_EAGAIN) || (error == PLAT_EWOULDBLOCK) || (error == PLAT_EINPROGRESS);
		if ( ! inProgress )
			throw Teul::OpenError(error, "connect() call failed");

		endpoint.lastDataRecv.makeCurrent();
		endpoint.lastDataSent.makeCurrent();

		assert( endpoint.m_impl->pollSlot() < 0 );

		dbugLog(_FLD, "t:%d sock:" PRF_SOCKET " connecting on %s:%u", endpoint.teulIndex(), PRA_SOCKET(endpoint.socket), teulGetAddress(endpoint.destAddress()), endpoint.destPort());

		break;
	}
}

//////////////////////////////////////////////////////////////////////
// Open a stream socket.

static void
_openStream(ENDPOINT& endpoint)
{
	if (endpoint.state() == IS_LISTENING )
		_bindAndListen(endpoint);
	else if ( endpoint.state() == IS_OPENING )
		_connectSocket(endpoint);
	else
		throw Teul::OpenError(EINVAL, "invalid socket state during open");
}

//////////////////////////////////////////////////////////////////////
// Bind a socket for a datagram (UDP) connection.

static void
_bindDgramSocket(ENDPOINT& endpoint, sockaddr_in& sockAddr)
{
	socklen_t len = sizeof(sockAddr);	// Needed for getsockname.

	if ( bind(endpoint.socket, (struct sockaddr*)&sockAddr, len) < 0 )
		throw Teul::OpenError("bind() failed");

	if ( getsockname(endpoint.socket, (struct sockaddr*)&sockAddr, &len) < 0 )
		throw Teul::OpenError("getsockname() failed after bind");

	dbugLog(_FLI, "t:%d sock:" PRF_SOCKET " bound to fam:%u addr:%s port:%hu"
					, endpoint.teulIndex()
					, PRA_SOCKET(endpoint.socket)
					, sockAddr.sin_family
					, teulGetAddress(ntohl(sockAddr.sin_addr.s_addr))
					, sockAddr.sin_port
					);
}

// inActive : true if the TCP socket is to be used to send, false if used to listen/receive
SINT32
teulOpenConnection(const bool listening, const SINT32 timeout, const PROTOCOL protocol, const UINT32 host, const UINT16 port, CONNECT_FUNCTION connectFn, ERROR_FUNCTION errorFn, const bool initBackChannel)
{
	if (PN_UNLIKELY(Teul::IsTransportClosing())) {
		dbugLog(_FLM, "open connection refused: transport closing");
		return -EDEADLK;
	}

	struct sockaddr_in sockAddress;
	memset(&sockAddress, 0, sizeof(sockAddress));
	sockAddress.sin_family = AF_INET;
	sockAddress.sin_addr.s_addr = htonl(host);
	sockAddress.sin_port = htons(port);

	ENDPOINT* newEndpoint = teulCreateEndpoint(listening, timeout, protocol);
	if ( PN_UNLIKELY(newEndpoint == nullptr) )
	{
		return -ENOSPC;	// could not allocate buffer for endpoint
	}

	newEndpoint->connectFn = connectFn;
	newEndpoint->errorFn = errorFn;

	// create the socket
	newEndpoint->socket = INVALID_SOCKET;

	if (newEndpoint->state() == IS_LISTENING)
	{
		newEndpoint->localSockAddress = sockAddress;
	}
	else
	{
		newEndpoint->fromSockAddress = sockAddress;
		newEndpoint->destSockAddress = sockAddress;

		if (newEndpoint->protocol == PROTOCOL_UDB )
		{
			newEndpoint->localSockAddress = sockAddress;

			// Translate the destination address into a broadcast.
			newEndpoint->destSockAddress.sin_addr.s_addr |= 0xff000000;
		}
		else
		{
			newEndpoint->destSockAddress = sockAddress;

			// Listen on all addresses, with an auto-port.
			newEndpoint->localSockAddress.sin_addr.s_addr = htonl( INADDR_ANY );
			newEndpoint->localSockAddress.sin_port = htons(0);
		}
	}

	bool useTCP = (newEndpoint->protocol == PROTOCOL_TCP || newEndpoint->protocol == PROTOCOL_CTRL);
	newEndpoint->socket = socket(AF_INET, useTCP ? SOCK_STREAM : SOCK_DGRAM, 0);
	try
	{
		if ( newEndpoint->socket == INVALID_SOCKET )
			throw Teul::OpenError("socket() failed");

		teulConfigureEndpointSocket(*newEndpoint);

		switch ( newEndpoint->protocol )
		{
		case PROTOCOL_TCP:
		case PROTOCOL_CTRL:
			_openStream(*newEndpoint);
		break;
		case PROTOCOL_UDP:
		case PROTOCOL_UDB:
			_bindDgramSocket(*newEndpoint, newEndpoint->localSockAddress);
		break;
		default:
			throw Teul::OpenError(EINVAL, "internal error (protocol)");
		}

		_setEndpointPeerState(*newEndpoint);

		if (newEndpoint->protocol == PROTOCOL_UDB || newEndpoint->protocol == PROTOCOL_CTRL)
		{
			// tell the app we are connected
			newEndpoint->m_state = IS_OPEN;
			newEndpoint->setConnected();
			newEndpoint->doConnect();
		}
		else if (newEndpoint->state() != IS_LISTENING && (newEndpoint->m_reliable == false))
		{
			const SINT32 error = (SYN_TOKEN != nullptr) ? teulModSendCALLBACKe(*newEndpoint, *SYN_TOKEN) : -EINVAL;
			if ( error < 0 ) {
				throw Teul::OpenError(error, "couldn't send to new connection");
			}
		}

		teulMRecvAddToPoll(*newEndpoint);
	}
	catch ( Teul::OpenError& e )
	{
		const char* err = strerror(abs(e.errNo));
		if ( err == nullptr )
			err = "unknown";

		dbugLog(_FLW, "t:%d sock:" PRF_SOCKET " open error:%d [%s] %s"
					, newEndpoint->teulIndex(), PRA_SOCKET(newEndpoint->socket)
					, e.errNo, err, e.msg
					);

		newEndpoint->doError(e.errNo);
		newEndpoint->shutdown();
		teulFreeEndpoint(newEndpoint);

		return e.errNo < 0 ? e.errNo : -EINVAL;
	}

	// Backchannels may exist on listening hosts and clients
	if (initBackChannel)
	{
		newEndpoint->m_pairdUdpConn = new RakNetChannel(0, listening, port, newEndpoint);
		newEndpoint->m_pairdUdpConn->init();
	}


	return newEndpoint->teulIndex();
}

SINT32 teulConnectENDPOINT(ENDPOINT& inEndpoint)
{
	if (PN_UNLIKELY(inEndpoint.state() != IS_OPENING))
	{
		dbugLog(_FLA,"can't connect an endpoint in state %d", inEndpoint.state());
		return -EINVAL;
	}

	if ( inEndpoint.m_reliable == false )
	{
		SINT32 retval = connect(inEndpoint.socket, (struct sockaddr*)&inEndpoint.destSockAddress, sizeof(inEndpoint.destSockAddress));
		if ( retval < 0 )
		{
			dbugWrite(_FLA, "t:%d couldn't connect socket",inEndpoint.teulIndex());
			return retval;
		}
	}

	inEndpoint.m_state = IS_OPEN;
	inEndpoint.setConnected();

	teulMRecvUpdatePoll(inEndpoint);

	return 0;
}

static void
_releaseEndpoint(ENDPOINT* const inEndpoint)
{
	if ( inEndpoint->m_closed == false )
	{
		if ( inEndpoint->socket != INVALID_SOCKET )
			teulModSendCALLBACKe(*inEndpoint, *FYN_TOKEN);

		inEndpoint->doError(ECONNABORTED);
		inEndpoint->shutdown();
	}
}

void
teulClose(ENDPOINT& inEndpoint, const UINT32 code)
{
	if( ! inEndpoint.m_inUse )
		return;

	// When an error occurs, the full cycle of shutdown may bring us back here.
	if ( inEndpoint.state() == IS_DEAD )
	{
//		dbugLog(_FLD, "t:%d endpoint already IS_DEAD", inEndpoint->teulIndex());
		return;
	}

	dbugLog(_FLD, "t:%d closing [%u] st:%u cl:%s", inEndpoint.teulIndex(), code, inEndpoint.state(), inEndpoint.m_closed ? "yes" : "no");
	if ( inEndpoint.state() < IS_CLOSING || inEndpoint.m_closed == false )
		inEndpoint.doError(ENOTCONN);
        // Returning here results in an infinite loop

	if ( (inEndpoint.state() < IS_CLOSING)
		&& (inEndpoint.state() != IS_LISTENING)
		&& (inEndpoint.protocol != PROTOCOL_TCP)
		&& (inEndpoint.protocol != PROTOCOL_UDB)
		&& (inEndpoint.protocol != PROTOCOL_CTRL)
		)
	{
		inEndpoint.m_state = IS_CLOSING;
		_releaseEndpoint(&inEndpoint);
		return;
	}

	// NOTE:
	//    This check is needed because the call to doError above **may** result in teulClose being re-entrant.
	//    If this happens the endpoing state will be dead and we should not continue, so we check again.
	if ( inEndpoint.state() != IS_DEAD )
	{
		// check status of paired backchannel, if client release - it'll get recreated when they log back in
		RakNetChannel::disconnectPeer(&inEndpoint);
    
		// dispose of socket
		inEndpoint.shutdown();

		inEndpoint.m_state = IS_DEAD;
		inEndpoint.m_doTimeoutCheck = false;	// stop checking timeouts on me.

		teulRemoveEndpoint(&inEndpoint);
	}
}

void
teulClose(const SINT32 teulIndex, const UINT32 code)
{
	ENDPOINT* const theEndpoint = teulGetEndpoint(teulIndex);
	if ( theEndpoint != nullptr )
		teulClose(*theEndpoint, code ? (UINT32)code : (UINT32)Teul::Closing::TeulClose);
}

