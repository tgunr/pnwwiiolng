#include <syscfg.h> // Path is project specific.
// This module provides access to the socket
// functions used by the teul API. The end-user developer
// should never need the functions provided here.
//
// Copyright 1999 Playnet Inc, All rights reserved
// Not to be distributed in API form except as part of a compiled
// library
//
// Originating Programmer: Martin Poulin

#include <fcntl.h>
#include <signal.h>
#include <event2/event.h>

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
#include "teul/teulSockConfig.h"
#include "teul/teulTime.h"
#include "teul/teulUtils.h"
#include "teul/teulVersion.h"
#include "wwii/wwiiNetPorts.h"

#if platOS == OS_WIN32
#include <winsock2.h>
#include <WinInet.h>
#include <Winineti.h>
#include <WinIoCtl.h>
#pragma comment(lib, "event.lib")
#pragma comment(lib, "ws2_32.lib")
#include <iphlpapi.h>
#pragma comment(lib, "IPHLPAPI.lib")
#else
#include <sys/ioctl.h>
#include <sys/socket.h>
#endif

#if platOS == OS_LINUX
#include "server/serverIfConf.h"
#endif

#if platBuild == TEUL_SERVER
#include "server/serverInterfaceDescription.h"
#endif

#if platOS == OS_LINUX
#include "teul/teulConfigVar.h"
CVAR_STRING(defaultInterface, NULL, "DEFAULT_IFACE", false
			, "Name of the network interface to treat as primary.");
#endif

#if platOS != OS_WIN32
	#include <netinet/tcp.h>
	#include <sys/un.h>
#endif

#define EXT_TOKENS		0x8000000

// Higher than SOMAXCONN.
enum { MAX_ACCEPT_BACKLOG = 1024 } ;

TeulTimeMS lastUpdateTime;

extern void teulInitEndpoints(ERROR_FUNCTION* const) ;

//////////////////////////////////////////////////////////////////////

// Check a network-order address.
static inline CONSTEXPR_FN bool IsMulticastNetworkAddress(const struct sockaddr_in& sockAddress)
{
	return (sockAddress.sin_addr.s_addr & 0xE0) == 0xE0;
}

//////////////////////////////////////////////////////////////////////
// Retrieve a list of the IP addresses this host has on eth0.

#if platBuild == TEUL_SERVER || platOS == OS_LINUX
static void
_getHostIPAddresses()
{
#if platOS == OS_LINUX
	static const char* primaryInterfaceName = cv_defaultInterface.GetString();

	// Get a list of IPv4 network interfaces.
	auto ifiList = get_ifi_info(AF_INET, TRUE);

	// Get the addresses of upto MaxHostIPs interfaces.
	auto& ifNo = Server::myInterface.numIP ;
	ifNo = 0 ;
	for ( auto ifi = ifiList; ifi != NULL ; ifi = ifi->ifi_next )
	{
		if ( strcmp(ifi->ifi_name, primaryInterfaceName) == 0 )
		{
			memcpy(&(Server::myInterface.mac), ifi->ifi_haddr, sizeof(Server::myInterface.mac));
			Server::myInterface.pid = getpid();
			//Server::myInterface.id = (Server::myInterface.pid &0x0000ffff) | Server::myInterface.mac;
			Server::myInterface.addr[ifNo].naddr = ((struct sockaddr_in*)(ifi->ifi_addr))->sin_addr.s_addr;
			Server::myInterface.addr[ifNo].ip = ntohl(Server::myInterface.addr[ifNo].naddr);
			dbugLog(_FLI, "read index %u interface %s, ip %s mac %llx pid %u"
						, ifNo
						, ifi->ifi_name
						, teulGetAddress(ntohl(Server::myInterface.addr[ifNo].ip))
						, Server::myInterface.mac
						, Server::myInterface.pid
						//, Server::myInterface.id
						);

			++ifNo ;
			if ( ifNo >= Server::InterfaceDescription::MaxHostIPs )
				break ;
		}
	}

	if ( ifiList != NULL )
		free_ifi_info(ifiList) ;
#elif platOS == OS_WIN32
	PIP_ADAPTER_ADDRESSES pAddresses = NULL;
	DWORD outBufLen = 32 * 1024;	// Start with 32Kb of buffer to retrieve data.
	DWORD dwRetVal = ERROR_BUFFER_OVERFLOW;

	DWORD getAddrFlags = GAA_FLAG_INCLUDE_PREFIX | GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST;

	for ( UINT32 attempt = 0; dwRetVal == ERROR_BUFFER_OVERFLOW && attempt < 6; ++attempt )
	{
		if (pAddresses)
			memFree(pAddresses);
		pAddresses = static_cast<PIP_ADAPTER_ADDRESSES>(memAlloc(outBufLen));
		ABORT_IF_EX(pAddresses == NULL, "Failed to allocate memory for listing network interfaces.");
		dwRetVal = GetAdaptersAddresses(AF_INET, getAddrFlags, NULL, pAddresses, &outBufLen);
	}

	ABORT_IF_EX(dwRetVal == ERROR_BUFFER_OVERFLOW, "Unable to retrieve network interfaces.");

	PIP_ADAPTER_ADDRESSES preferredIf = NULL;
	for ( auto curAddr = pAddresses; curAddr != NULL; curAddr = curAddr->Next )
	{
		// Only look for ethernet networks with IfOperStatusUp and a unicast address.
		if ( curAddr->OperStatus != IfOperStatusUp || curAddr->FirstUnicastAddress == NULL )
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
		if ( preferredIf != NULL && curAddr->FirstDnsServerAddress == NULL )
			continue;

		preferredIf = curAddr;
	}

	ABORT_IF_EX(preferredIf == NULL || preferredIf->FirstUnicastAddress == NULL
				, "Unable to find eth0. If you need to specify an interface, set the environment variable 'ETH0' to the friendly name of the adapter.");

	assert( preferredIf->PhysicalAddressLength < sizeof(Server::myInterface.mac) );

	memcpy(&(Server::myInterface.mac), preferredIf->PhysicalAddress, preferredIf->PhysicalAddressLength);
	Server::myInterface.pid = static_cast<UINT32>(GetCurrentProcessId());

	auto address = preferredIf->FirstUnicastAddress;
	auto& ifNo = Server::myInterface.numIP;
	for ( ifNo = 0; ifNo < Server::InterfaceDescription::MaxHostIPs && address != NULL; ++ifNo, address = address->Next)
	{
		struct sockaddr_in* saddr = (struct sockaddr_in*)(address->Address.lpSockaddr);
		Server::myInterface.addr[ifNo].naddr = saddr->sin_addr.S_un.S_addr;
		Server::myInterface.addr[ifNo].ip = ntohl(Server::myInterface.addr[0].naddr);

		TCHAR addrString[128];
		DWORD addrSize = sizeof(addrString);
		auto x = WSAAddressToString(address->Address.lpSockaddr, address->Address.iSockaddrLength, NULL, addrString, &addrSize);

		dbugLog(_FLI, "read index %u interface %ls, ip %s mac %llX pid %u"
						, ifNo
						, preferredIf->FriendlyName
						, addrString
						, Server::myInterface.mac
						, Server::myInterface.pid
						);
	}

	memFree(pAddresses);
#endif
}
#endif

SINT32 teulModStartup(ERROR_FUNCTION* errFnPtr)
{
#if platOS == OS_LINUX
	signal(SIGPIPE, SIG_IGN);
#endif

	dbugWrite(_FLN,"intitializing teul module verion: %x",Teul::VersionString);

	teulInitSend();

	teulInitEndpoints(errFnPtr);

	teulInitModCallbacks();

	teulSyncClock();
	lastUpdateTime = teulClock64 ;

#if platOS == OS_LINUX || platBuild == TEUL_SERVER
	_getHostIPAddresses() ;
#endif

	teulModRecvInit();

	return 0;
}

typedef std::vector<UINT32> ADDRESS_LIST ;

#if platBuild == TEUL_SERVER || platOS == OS_LINUX
void
teulGetAddresses(ADDRESS_LIST& addresses)
{
	addresses.clear();
	for ( UINT32 i = 0 ; i < Server::myInterface.numIP ; ++i )
	{
		addresses.push_back(Server::myInterface.addr[i].ip) ;
	}
}

namespace Teul
{
	bool IsMyAddress(UINT32 address_)
	{
		if ( address_ == INADDR_LOOPBACK )
			return true;
		for ( UINT32 i = 0 ; i < Server::myInterface.numIP ; ++i )
		{
			if ( Server::myInterface.addr[i].ip == address_ )
				return true;
		}
		return false;
	}
}
#endif

SINT32
teulModShutdown(void)
{
	Teul::ENDPOINT_LIST::iterator it = Teul::idleEndpoints.begin() ;
	while ( it != Teul::idleEndpoints.end() )
	{
		ENDPOINT* const endpoint = *it ;
		++it ;

		if ( endpoint != NULL )
			teulClose(*endpoint, Teul::Closing::Shutdown) ;
	}

	teulModIdle(10);

	teulKillDeadEndpoints() ;

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
	if ( endpoint.protocol == UDB || endpoint.state() == IS_LISTENING )
		endpoint.m_peer = true ;
	else if ( endpoint.protocol == TCP && endpoint.destPort() == WWII::Net::Ports::ServerPeering )
		endpoint.m_peer = true ;
}

//////////////////////////////////////////////////////////////////////
// Socket configuration.

// TCP sockets are generally WAN, so they have different buffer sizes.
static const size_t TcpRcvBufSize = 8 * 1024 ;
static const size_t TcpSndBufSize = 4 * 1024 ;

// UDP sockets are all LAN and need large buffers.
static const size_t UdpRcvBufSize = 64 * 1024 ;
static const size_t UdpSndBufSize = 64 * 1024 ;

static void
_configureEndpointSocket(ENDPOINT& endpoint, const bool isMulticast)
{
	evutil_make_socket_nonblocking(endpoint.socket);
	evutil_make_socket_closeonexec(endpoint.socket);

	const bool isTcp = (endpoint.protocol == TCP) ;

	endpoint.setRcvBufferSize( isTcp ? TcpRcvBufSize : UdpRcvBufSize ) ;
	endpoint.setSndBufferSize( isTcp ? TcpSndBufSize : UdpSndBufSize ) ;

	if ( isTcp )
	{
		// Disable Nagle so data doesn't wait to be pushed.
		endpoint.setNagleEnabled(false) ;

		// Listening sockets need to re-allow use of the address on close.
		if ( endpoint.state() == IS_LISTENING )
			evutil_make_listen_socket_reuseable(endpoint.socket) ;
	}
	else if ( endpoint.protocol == UDB )
	{
		if ( !isMulticast )
		{
			// UDB sockets need to be broadcast-enabled.
			int bcast = 1 ;
			setsockopt(endpoint.socket, SOL_SOCKET, SO_BROADCAST, (char*)&bcast, sizeof(bcast)) ;
		}
		else
			evutil_make_listen_socket_reuseable(endpoint.socket);
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

	SINT32 errNo ;
	const char* const msg ;

private:
	OpenError& operator = (const OpenError&) ;
} ;
}

//////////////////////////////////////////////////////////////////////
// Prepare a socket for listening -- bind it to the listen address,
// and tell the operating system to listen.

static void
_bindAndListen(ENDPOINT& endpoint)
{
	if ( bind(endpoint.socket, (struct sockaddr*) &endpoint.localSockAddress, sizeof(endpoint.localSockAddress)) < 0 )
		throw Teul::OpenError("bind failed") ;

	if ( listen(endpoint.socket, MAX_ACCEPT_BACKLOG) < 0 )
		throw Teul::OpenError("listen failed") ;

	dbugLog(_FLD, "t:%d sock:%d listening socket", endpoint.teulIndex(), endpoint.socket);
}

//////////////////////////////////////////////////////////////////////
// Start an outgoing connection.

static void
_connectSocket(ENDPOINT& endpoint)
{
	// Start the connection process.
	if ( connect(endpoint.socket, (struct sockaddr*)&endpoint.destSockAddress, sizeof(endpoint.destSockAddress)) < 0 )
	{
		const SINT32 error = dbugGetErrno() ;

		// Since our sockets are configured as non-blocking,
		// it's VERY unlikely the connection will happen this
		// quickly - packets have to be exchanged, etc. So the
		// operating system will return an error to tell us that
		// this connection is not a done deal, yet.
		// Any of EAGAIN, EWOULDBLOCK or EINPROGRESS means that
		// it's begun, but not finished.
		const bool inProgress = (error == EAGAIN) | (error == EWOULDBLOCK) | (error == EINPROGRESS);
		if ( ! inProgress )
			throw Teul::OpenError(error, "connect() call failed") ;

		endpoint.lastDataRecv.makeCurrent() ;
		endpoint.lastDataSent.makeCurrent() ;

#if platOS == OS_LINUX
		assert( endpoint.m_impl->pollSlot() < 0 ) ;
#endif

		dbugLog(_FLD, "t:%d sock:%d connecting", endpoint.teulIndex(), endpoint.socket);
	}
}

//////////////////////////////////////////////////////////////////////
// Open a stream socket.

static void
_openStream(ENDPOINT& endpoint)
{
	if (endpoint.state() == IS_LISTENING )
		_bindAndListen(endpoint) ;
	else if ( endpoint.state() == IS_OPENING )
		_connectSocket(endpoint) ;
	else
		throw Teul::OpenError(EINVAL, "invalid socket state during open") ;
}

//////////////////////////////////////////////////////////////////////
// Bind a socket for a datagram (UDP) connection.

static void
_bindDgramSocket(ENDPOINT& endpoint, const bool isMulticast)
{
	sockaddr_in& sockAddr = endpoint.localSockAddress ;
	socklen_t len = sizeof(sockAddr) ;	// Needed for getsockname.

	if ( bind(endpoint.socket, (struct sockaddr*)&sockAddr, len) < 0 )
		throw Teul::OpenError("bind() failed") ;

	if ( getsockname(endpoint.socket, (struct sockaddr*)&sockAddr, &len) < 0 )
		throw Teul::OpenError("getsockname() failed after bind") ;

	dbugLog(_FLI, "t:%d sock:%d bound to fam:%u addr:%s port:%hu"
					, endpoint.teulIndex(), endpoint.socket
					, sockAddr.sin_family
					, teulGetAddress(ntohl(sockAddr.sin_addr.s_addr))
					, sockAddr.sin_port
					) ;

	if ( isMulticast )
	{
		evutil_make_listen_socket_reuseable(endpoint.socket);

		struct ip_mreq mreq;
		mreq.imr_multiaddr = endpoint.destSockAddress.sin_addr;
		mreq.imr_interface.s_addr = htonl(INADDR_ANY);
		int res = setsockopt(endpoint.socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (const char*)&mreq, sizeof(mreq));
		if ( res < 0 )
			throw Teul::OpenError("multicast membership request failed");

		dbugLog(_FLI, "t:%d sock:%d joined multicast group %s", endpoint.teulIndex(), endpoint.socket, teulGetAddress(ntohl(endpoint.destSockAddress.sin_addr.s_addr)));
	}
}

// inActive : true if the TCP socket is to be used to send, false if used to listen/receive
SINT32
teulOpenConnection(const SINT32 teulIndex, const bool listening, const SINT32 timeout, const PROTOCOL protocol, const UINT32 host, const UINT16 port, CONNECT_FUNCTION* connectFn, ERROR_FUNCTION* errorFn)
{
	struct sockaddr_in sockAddress ;
	memset(&sockAddress, 0, sizeof(sockAddress)) ;
	sockAddress.sin_family = AF_INET;
	sockAddress.sin_addr.s_addr = htonl(host);
	sockAddress.sin_port = htons(port);

	ENDPOINT* newEndpoint = teulCreateEndpoint(teulIndex, listening, timeout, protocol) ;
	if ( newEndpoint == NULL )
	{
		dbugLog(_FLA, "Couldn't Allocate endpoint!!");
		return -ENOSPC;	// could not allocate buffer for endpoint
	}

	newEndpoint->connectFn = connectFn ;
	newEndpoint->errorFn = errorFn ;

	SINT32	retval = 0 ;

	// create the socket
	newEndpoint->socket = INVALID_SOCKET;

	bool isMulticast = IsMulticastNetworkAddress(sockAddress);

	if (newEndpoint->state() == IS_LISTENING)
	{
		newEndpoint->localSockAddress = sockAddress ;
	}
	else
	{
		newEndpoint->fromSockAddress = sockAddress ;
		newEndpoint->destSockAddress = sockAddress ;

		if (newEndpoint->protocol == UDB && !isMulticast )
		{
			newEndpoint->localSockAddress = sockAddress ;

			// Translate the destination address into a broadcast.
			newEndpoint->destSockAddress.sin_addr.s_addr |= 0xff000000 ;
		}
		else
		{
			newEndpoint->destSockAddress = sockAddress ;

			// Listen on all addresses, with an auto-port.
			newEndpoint->localSockAddress.sin_addr.s_addr = htonl( INADDR_ANY );
			newEndpoint->localSockAddress.sin_port = !isMulticast ? htons(0) : htons(port);
		}
	}

	newEndpoint->socket = socket(PF_INET, (newEndpoint->protocol == TCP) ? SOCK_STREAM : SOCK_DGRAM, 0) ;
	try
	{
		if ( newEndpoint->socket == INVALID_SOCKET )
			throw Teul::OpenError("socket() failed") ;

		_configureEndpointSocket(*newEndpoint, isMulticast) ;

		switch ( newEndpoint->protocol )
		{
		case TCP:
			_openStream(*newEndpoint) ;
		break ;
		case UDP:
		case UDB:
			_bindDgramSocket(*newEndpoint, isMulticast) ;
		break ;
		default:
			Teul::OpenError(EINVAL, "unknown protocol") ;
		break ;
		}
	}
	catch ( Teul::OpenError& e )
	{
		const char* err = strerror(abs(e.errNo)) ;
		if ( err == NULL )
			err = "unknown" ;

		dbugLog(_FLW, "t:%d sock:%d open error:%d [%s] %s", newEndpoint->teulIndex(), newEndpoint->socket, e.errNo, err, e.msg) ;

		newEndpoint->doError(e.errNo) ;
		newEndpoint->shutdown() ;
		teulFreeEndpoint(newEndpoint) ;

		return -EINVAL ;
	}

	retval = newEndpoint->teulIndex();

	_setEndpointPeerState(*newEndpoint) ;

	if ( newEndpoint->protocol == UDB )
	{
		// tell the app we are connected
		newEndpoint->m_state = IS_OPEN;
		newEndpoint->setConnected() ;
		newEndpoint->doConnect() ;
	}
	else if (newEndpoint->state() != IS_LISTENING && (newEndpoint->m_reliable == false) )
	{
		const SINT32 error = (SYN_TOKEN != NULL) ? teulModSendCALLBACKe(*newEndpoint, *SYN_TOKEN) : -EINVAL ;
		if ( error < 0 )
			dbugLog(_FLA,"t:%d err:%d couldn't send to new connection"
						, newEndpoint->teulIndex(), error);
	}

	teulMRecvAddToPoll(newEndpoint);

	return retval;
}

SINT32
teulConnectENDPOINT(ENDPOINT* const inEndpoint)
{
	SINT32	retval = 0;

	if (inEndpoint->state() != IS_OPENING)
	{
		dbugLog(_FLA,"can't connect an endpoint in state %d", inEndpoint->state());
		return -EINVAL;
	}

	if ( inEndpoint->m_reliable == false )
	{
		retval = connect(inEndpoint->socket, (struct sockaddr*)&inEndpoint->destSockAddress, sizeof(inEndpoint->destSockAddress)) ;
		if ( retval < 0 )
		{
			dbugWrite(_FLA, "t:%d couldn't connect socket",inEndpoint->teulIndex());
			return retval;
		}
	}

	inEndpoint->m_state = IS_OPEN;
	inEndpoint->setConnected() ;

	teulMRecvUpdatePoll(inEndpoint);

	return 0;
}

static void
_releaseEndpoint(ENDPOINT* const inEndpoint)
{
	if ( inEndpoint->m_closed == false )
	{
		if ( inEndpoint->socket != INVALID_SOCKET )
			teulModSendCALLBACKe(*inEndpoint, *FYN_TOKEN) ;

		inEndpoint->doError(ECONNABORTED);
		inEndpoint->shutdown() ;
	}
}

void
teulClose(ENDPOINT& inEndpoint, const UINT32 code)
{
	if( ! inEndpoint.m_inUse )
		return ;

	// When an error occurs, the full cycle of shutdown may bring us back here.
	if ( inEndpoint.state() == IS_DEAD )
	{
//		dbugLog(_FLD, "t:%d endpoint already IS_DEAD", inEndpoint->teulIndex()) ;
		return ;
	}

#if platOS == OS_WIN32
	Teul::g_logFnPtr("t:%d closing code:%u", inEndpoint.teulIndex(), code) ;
#endif
#if platOS == OS_LINUX
	if ( g_logDebug )
		dbugLog(_FLN, "t:%d closing [%u] st:%u cl:%s", inEndpoint.teulIndex(), code, inEndpoint.state(), inEndpoint.m_closed ? "yes" : "no");
#endif
	if ( inEndpoint.state() < IS_CLOSING || inEndpoint.m_closed == false )
		inEndpoint.doError(ENOTCONN) ;

	if ( (inEndpoint.state() < IS_CLOSING)
		& (inEndpoint.state() != IS_LISTENING)
		& (inEndpoint.protocol != TCP)
		& (inEndpoint.protocol != UDB)
		)
	{
		inEndpoint.m_state = IS_CLOSING;
		_releaseEndpoint(&inEndpoint);
		return ;
	}

	// dispose of socket
	inEndpoint.shutdown() ;

	inEndpoint.m_state = IS_DEAD;

	teulRemoveEndpoint(&inEndpoint);
}

void
teulClose(const SINT32 teulIndex, const UINT32 code)
{
	ENDPOINT* const theEndpoint = teulGetEndpoint(teulIndex) ;
	if ( theEndpoint != NULL )
		teulClose(*theEndpoint, code ? (UINT32)code : (UINT32)Teul::Closing::TeulClose);
}

