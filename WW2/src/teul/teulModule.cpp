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
#include "teul/teulConstants.h"
#include "teul/teulEndpoint.h"
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

// Higher than SOMAXCONN.
enum { MAX_ACCEPT_BACKLOG = 1024 };

//////////////////////////////////////////////////////////////////////
// Retrieve a list of the IP addresses this host has on its primary interface.

#if platBuild == BUILD_SERVER
static void _getHostIPAddresses()
{
#if platOS == OS_LINUX || platOS == OS_MACOS
	const char *primaryInterfaceName = cv_defaultInterface.GetString();

	// Get a list of IPv4 network interfaces.
	struct ifaddrs *ifap = nullptr;
    
	if (getifaddrs(&ifap) < 0 || ifap == nullptr) {
		LogAbort("Unable to get network interfaces");
    }

	// Get the addresses of upto MaxHostIPs interfaces.
    struct ifaddrs *pIntf = nullptr;

    Server::myInterface.pid = getpid();
    Server::myInterface.numIP = 0;
	for (auto ifa = ifap; ifa && Server::myInterface.numIP <= Server::InterfaceDescription::MaxHostIPs; ifa = ifa->ifa_next, ++Server::myInterface.numIP) {
        if (!ifa->ifa_addr) {
            dbugLog(_FLN, "Skipping interface %s because it has no address", ifa->ifa_name);
            continue;
        }
        if (ifa->ifa_addr->sa_family != AF_INET) {
            dbugLog(_FLN, "Skipping interface %s because it is not an IPv4 interface", ifa->ifa_name);
            continue;
        }
        
        if (!pIntf && strcasecmp(ifa->ifa_name, primaryInterfaceName) == 0) {
            pIntf = ifa;
        }
        
        Server::myInterface.addr[Server::myInterface.numIP].naddr = reinterpret_cast<struct sockaddr_in *>(ifa->ifa_addr)->sin_addr.s_addr;
        Server::myInterface.addr[Server::myInterface.numIP].ip = ntohl(Server::myInterface.addr[Server::myInterface.numIP].naddr);
        dbugLog(_FLI, "read index %u interface %s, ip %s",
                    Server::myInterface.numIP,
                    ifa->ifa_name,
                    teulGetAddress(Server::myInterface.addr[Server::myInterface.numIP].ip));
    }
    
    if (!pIntf) {
        LogAbort("Can't find primary interface %s", primaryInterfaceName);
    }
	freeifaddrs(ifap);
#elif platOS == OS_WIN32
	PIP_ADAPTER_ADDRESSES pAddresses = nullptr;
	DWORD outBufLen = 32 * 1024;	// Start with 32Kb of buffer to retrieve data.
	DWORD dwRetVal = ERROR_BUFFER_OVERFLOW;
	DWORD getAddrFlags = GAA_FLAG_INCLUDE_PREFIX | GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST;
    
    pAddresses = static_cast<PIP_ADAPTER_ADDRESSES>(::calloc(1, outBufLen));
    dwRetVal = GetAdaptersAddresses(AF_INET, getAddrFlags, nullptr, pAddresses, &outBufLen);
    if (dwRetVal == ERROR_BUFFER_OVERFLOW) {
        ::free(pAddresses);
        pAddresses = static_cast<PIP_ADAPTER_ADDRESSES>(::calloc(1, outBufLen));
        dwRetVal = GetAdaptersAddresses(AF_INET, getAddrFlags, nullptr, pAddresses, &outBufLen);
    }
    ABORT_IF_EX(dwRetVal != ERROR_SUCCESS, "Unable to retrieve network interfaces.");

	PIP_ADAPTER_ADDRESSES preferredIf = nullptr;
	
    for (auto curAddr = pAddresses; curAddr != nullptr; curAddr = curAddr->Next) {
		// Only look for ethernet networks with IfOperStatusUp and a unicast address.
		if (curAddr->OperStatus != IfOperStatusUp || !curAddr->FirstUnicastAddress || (curAddr->Flags & IP_ADAPTER_RECEIVE_ONLY) != 0) {
			continue;
        }
        
		// The current interface is a better choice than what came before if it has at least one DNS server and Gateway.
		if (preferredIf && (!curAddr->FirstDnsServerAddress || !curAddr->FirstGatewayAddress)) {
            continue;
        }
		preferredIf = curAddr;
	}

	ABORT_IF_EX(!preferredIf, "Unable to determine primary network adapter. Warning: Unfortunately, DEFAULT_IFACE does not work on Windows.");

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

	::free(pAddresses);
#endif
}
#endif

SINT32 teulModStartup(ERROR_FUNCTION errFnPtr)
{
#if platBuild == BUILD_SERVER && platOS != OS_WIN32
	signal(SIGPIPE, SIG_IGN);
#endif

	dbugLog(_FLN, "intitializing teul module version: %s", Teul::VersionString);

	extern void teulInitModRecv();
    extern void teulInitEndpoints(ERROR_FUNCTION const);

	teulInitModRecv();
	teulInitEndpoints(errFnPtr);
	teulInitModCallbacks();
	teulSyncClock();
#if platBuild == BUILD_SERVER
	_getHostIPAddresses();
#endif
	return 0;
}

using ADDRESS_LIST = std::vector<UINT32>;

#if platBuild == BUILD_SERVER
void teulGetAddresses(ADDRESS_LIST& addresses)
{
	addresses.clear();
	for (UINT32 i = 0; i < Server::myInterface.numIP; ++i) {
		addresses.push_back(Server::myInterface.addr[i].ip);
	}
}

namespace Teul
{
	bool IsMyAddress(UINT32 address_)
	{
		if (address_ == INADDR_LOOPBACK) {
			return true;
        }
		for (UINT32 i = 0; i < Server::myInterface.numIP; ++i) {
			if (Server::myInterface.addr[i].ip == address_) {
				return true;
            }
		}
		return false;
	}
}
#endif

SINT32 teulModShutdown()
{
	using namespace Teul;
	
    for (auto *ep : idleEndpoints) {
        if (ep) {
            teulClose(*ep);
        }
    }

	teulModIdle(10);
	teulKillDeadEndpoints();
	// Free up the memory the endpoints are using
	teulCleanupEndpoints();
#if platOS == OS_WIN32
	WSACleanup();
#endif
	return 0;
}

//////////////////////////////////////////////////////////////////////
// Update the m_peer flag of an endpoint.
static void _setEndpointPeerState(ENDPOINT& endpoint)
{
#if platBuild == BUILD_SERVER
	if (endpoint.protocol == PROTOCOL_UDB || endpoint.protocol == PROTOCOL_CTRL || endpoint.state() == IS_LISTENING ||
        (endpoint.protocol == PROTOCOL_TCP && static_cast<UINT32>(endpoint.destPort() - WWII::Net::Ports::ServerPeering) < MAX_SERVER_TYPE)
    ) {
		endpoint.m_peer = true;
    } else
#endif
	endpoint.m_peer = false;
}

//////////////////////////////////////////////////////////////////////
// Socket configuration.

// TCP sockets are generally WAN, so they have different buffer sizes.
static constexpr const size_t TcpRcvBufSize = 8 * 1024;
static constexpr const size_t TcpSndBufSize = 4 * 1024;

// UDP sockets are all LAN and need large buffers.
static constexpr const size_t UdpRcvBufSize = 64 * 1024;
static constexpr const size_t UdpSndBufSize = 64 * 1024;

static constexpr const size_t sockbufChunkSize = 4096;        // 4Kb - must be power of 2.
static constexpr const size_t sockbufChunkMask = sockbufChunkSize - 1;


void teulConfigureEndpointSocket(ENDPOINT& endpoint)
{
    u_long onValue = 1;

#if platOS != OS_WIN32 // Windows does not use the same meanings for these
    fcntl(endpoint.socket, F_SETFL, fcntl(endpoint.socket, F_GETFL) | O_NONBLOCK | O_CLOEXEC);
#else
    ioctlsocket(endpoint.socket, FIONBIO, &onValue);
#endif

    u_long rcvBufSize = static_cast<u_long>(((endpoint.protocol == PROTOCOL_TCP ? TcpRcvBufSize : UdpRcvBufSize) + sockbufChunkSize) & (~sockbufChunkMask));
    u_long sndBufSize = static_cast<u_long>(((endpoint.protocol == PROTOCOL_TCP ? TcpSndBufSize : UdpSndBufSize) + sockbufChunkSize) & (~sockbufChunkMask));

#if defined(DEV_BUILD)
    dbugLog(_FLD, "t:%d sock:" PRF_SOCKET " setting rcvbuf to %lu bytes", endpoint.teulIndex(), PRA_SOCKET(endpoint.socket), rcvBufSize);
    dbugLog(_FLD, "t:%d sock:" PRF_SOCKET " setting sndbuf to %lu bytes", endpoint.teulIndex(), PRA_SOCKET(endpoint.socket), sndBufSize);
#endif

    setsockopt(endpoint.socket, SOL_SOCKET, SO_RCVBUF, reinterpret_cast<char const *>(&rcvBufSize), sizeof(rcvBufSize));
    setsockopt(endpoint.socket, SOL_SOCKET, SO_SNDBUF, reinterpret_cast<char const *>(&sndBufSize), sizeof(sndBufSize));

	if (endpoint.protocol == PROTOCOL_TCP || endpoint.protocol == PROTOCOL_CTRL) {
		// Disable Nagle so data doesn't wait to be pushed.
        setsockopt(endpoint.socket, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<char const *>(&onValue), sizeof(onValue));

		// Listening sockets need to re-allow use of the address on close.
#if platOS != OS_WIN32 // Windows does not use the same meaning for this
		if (endpoint.state() == IS_LISTENING) {
            setsockopt(endpoint.socket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<char const *>(&onValue), sizeof(onValue));
        }
#endif
#if platOS == OS_MACOS
        setsockopt(endpoint.socket, SOL_SOCKET, SO_NOSIGPIPE, reinterpret_cast<char const *>(&onValue), sizeof(onValue));
#endif
	} else if (endpoint.protocol == PROTOCOL_UDB) {
		// UDB sockets need to be broadcast-enabled.
        setsockopt(endpoint.socket, SOL_SOCKET, SO_BROADCAST, reinterpret_cast<char const *>(&onValue), sizeof(onValue));
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

static void _bindAndListen(ENDPOINT& endpoint)
{
	if (bind(endpoint.socket, (struct sockaddr*) &endpoint.localSockAddress, sizeof(endpoint.localSockAddress)) < 0) {
		throw Teul::OpenError("bind failed");
    }
    if (listen(endpoint.socket, MAX_ACCEPT_BACKLOG) < 0) {
		throw Teul::OpenError("listen failed");
    }
	dbugLog(_FLD, "t:%d sock:" PRF_SOCKET " listening socket on %s:%u", endpoint.teulIndex(), PRA_SOCKET(endpoint.socket), teulGetAddress(endpoint.destAddress()), endpoint.destPort());
}

//////////////////////////////////////////////////////////////////////
// Start an outgoing connection.
static void _connectSocket(ENDPOINT& endpoint)
{
	// Start the connection process.
    int connectResult = 0;
    int connectErr = 0;
    
    do {
        connectResult = connect(endpoint.socket, (struct sockaddr*)&endpoint.destSockAddress, sizeof(endpoint.destSockAddress));
    } while (connectResult == SOCKET_ERROR && (connectErr = dbugGetErrno()) == PLAT_EINTR);
   
    if (connectResult != SOCKET_ERROR || connectErr == PLAT_EWOULDBLOCK || connectErr == PLAT_EINPROGRESS) {
        endpoint.lastDataRecv.makeCurrent();
        endpoint.lastDataSent.makeCurrent();
        dbugLog(_FLD, "t:%d sock:" PRF_SOCKET " connecting on %s:%u", endpoint.teulIndex(), PRA_SOCKET(endpoint.socket), teulGetAddress(endpoint.destAddress()), endpoint.destPort());
        return;
    } else {
        throw Teul::OpenError(connectErr, "connect() call failed");
    }
}

//////////////////////////////////////////////////////////////////////
// Open a stream socket.
static void _openStream(ENDPOINT& endpoint)
{
	if (endpoint.state() == IS_LISTENING) {
		_bindAndListen(endpoint);
	} else if (endpoint.state() == IS_OPENING) {
		_connectSocket(endpoint);
	} else {
		throw Teul::OpenError(EINVAL, "invalid socket state during open");
    }
}

//////////////////////////////////////////////////////////////////////
// Bind a socket for a datagram (UDP) connection.
static void _bindDgramSocket(ENDPOINT& endpoint, sockaddr_in& sockAddr)
{
	socklen_t len = sizeof(sockAddr);

	if (bind(endpoint.socket, reinterpret_cast<struct sockaddr *>(&sockAddr), len) == SOCKET_ERROR) {
		throw Teul::OpenError("bind() failed");
    }

	if (getsockname(endpoint.socket, reinterpret_cast<struct sockaddr *>(&sockAddr), &len) == SOCKET_ERROR) {
		throw Teul::OpenError("getsockname() failed after bind");
    }

	dbugLog(_FLI, "t:%d sock:" PRF_SOCKET " bound to fam:%u addr:%s port:%hu",
					endpoint.teulIndex(),
					PRA_SOCKET(endpoint.socket),
					sockAddr.sin_family,
					teulGetAddress(ntohl(sockAddr.sin_addr.s_addr)),
					sockAddr.sin_port);
}

//////////////////////////////////////////////////////////////////////
// Open a teul connection
SINT32 teulOpenConnection(const bool listening, const SINT32 timeout, const PROTOCOL protocol, const UINT32 host, const UINT16 port,
                          CONNECT_FUNCTION connectFn, ERROR_FUNCTION errorFn, const bool initBackChannel)
{
	if (Teul::IsTransportClosing()) {
		dbugLog(_FLM, "open connection refused: transport closing");
		return -EDEADLK;
	}

	struct sockaddr_in sockAddress;
    
	memset(&sockAddress, 0, sizeof(sockAddress));
	sockAddress.sin_family = AF_INET;
	sockAddress.sin_addr.s_addr = htonl(host);
	sockAddress.sin_port = htons(port);

	ENDPOINT* newEndpoint = teulCreateEndpoint(listening, timeout, protocol);

	if (!newEndpoint) {
		return -EMFILE;
	}
	newEndpoint->connectFn = connectFn;
	newEndpoint->errorFn = errorFn;
	newEndpoint->socket = INVALID_SOCKET;

	if (newEndpoint->state() == IS_LISTENING) {
		newEndpoint->localSockAddress = sockAddress;
	} else {
		newEndpoint->fromSockAddress = sockAddress;
		newEndpoint->destSockAddress = sockAddress;

		if (newEndpoint->protocol == PROTOCOL_UDB) {
			newEndpoint->localSockAddress = sockAddress;
			// Translate the destination address into a broadcast.
			newEndpoint->destSockAddress.sin_addr.s_addr |= 0xff000000;
		} else {
			newEndpoint->destSockAddress = sockAddress;
			// Listen on all addresses, with an auto-port.
			newEndpoint->localSockAddress.sin_addr.s_addr = htonl(INADDR_ANY);
			newEndpoint->localSockAddress.sin_port = htons(0);
		}
	}
    
    try {
        switch (newEndpoint->protocol) {
            case PROTOCOL_TCP:
            case PROTOCOL_CTRL:
                newEndpoint->socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
                if (newEndpoint->socket == INVALID_SOCKET) {
                    throw Teul::OpenError("socket(SOCK_STREAM) failed");
                }
                teulConfigureEndpointSocket(*newEndpoint);
                _openStream(*newEndpoint);
                break;
            case PROTOCOL_UDP:
            case PROTOCOL_UDB:
                newEndpoint->socket = socket(AF_INET, SOCK_DGRAM, 0);
                if (newEndpoint->socket == INVALID_SOCKET) {
                    throw Teul::OpenError("socket(SOCK_DGRAM) failed");
                }
                teulConfigureEndpointSocket(*newEndpoint);
                _bindDgramSocket(*newEndpoint, newEndpoint->localSockAddress);
                break;
        }
        _setEndpointPeerState(*newEndpoint);
        if (newEndpoint->protocol == PROTOCOL_UDB || newEndpoint->protocol == PROTOCOL_CTRL) {
            newEndpoint->m_state = IS_OPEN; // broadcast and control sockets are immediately open and connected
            newEndpoint->setConnected();
            newEndpoint->doConnect();
        } else if (newEndpoint->state() != IS_LISTENING && !newEndpoint->m_reliable) {
            SINT32 sendErr = teulModSendCALLBACKe(*newEndpoint, *SYN_TOKEN); // Send SYN immediately on UDP connection sockets
            
            if (sendErr < 0) {
                throw Teul::OpenError(-sendErr, "failed to send new connection open");
            }
        }
        teulMRecvAddToPoll(*newEndpoint);
    } catch (Teul::OpenError& e) {
        const char *errStr = Teul::GetErrorName(e.errNo);

		dbugLog(_FLW, "t:%d sock:" PRF_SOCKET " open error:%d [%s] %s", newEndpoint->teulIndex(), PRA_SOCKET(newEndpoint->socket), e.errNo, errStr, e.msg);
		newEndpoint->doError(e.errNo);
		newEndpoint->shutdown();
		teulFreeEndpoint(newEndpoint);
		return e.errNo < 0 ? e.errNo : -EINVAL;
	}

	// Backchannels may exist on listening hosts and clients
    assert(!initBackChannel || newEndpoint->state() != IS_LISTENING); // listening endpoints never have backchannels

	if (initBackChannel) {
		newEndpoint->m_pairdUdpConn = new RakNetChannel(0, listening, port, newEndpoint);
		newEndpoint->m_pairdUdpConn->init();
	}
	return newEndpoint->teulIndex();
}

//////////////////////////////////////////////////////////////////////
// Mark an endpoint fully connected (TCP connect completed)
// This is HORRIBLY mis-factored relative to the UDB/CTRL endpoint handling above
SINT32 teulConnectENDPOINT(ENDPOINT& inEndpoint)
{
	if (inEndpoint.state() != IS_OPENING) {
		dbugLog(_FLA,"can't connect an endpoint in state %d", inEndpoint.state());
		return -EINVAL;
	}

	if (!inEndpoint.m_reliable) {
        int connErr = 0, connRes = 0;
        
        do {
		    connRes = connect(inEndpoint.socket, reinterpret_cast<struct sockaddr *>(&inEndpoint.destSockAddress), sizeof(inEndpoint.destSockAddress));
        } while (connRes == SOCKET_ERROR && (connErr = dbugGetErrno()) == PLAT_EINTR);
        
		if (connRes == SOCKET_ERROR) {
			dbugLog(_FLA, "t:%d couldn't connect socket",inEndpoint.teulIndex());
			return -connErr;
		}
	}
	inEndpoint.m_state = IS_OPEN;
	inEndpoint.setConnected();
    inEndpoint.updateSocketEvents();
	return 0;
}

//////////////////////////////////////////////////////////////////////
// Tear down an endpoint.
// This is one of the worst offenders of reentrancy in all Teul's network logic.
void teulClose(ENDPOINT& inEndpoint)
{
	if (!inEndpoint.m_inUse) {
		return;
    }

	// When an error occurs, the full cycle of shutdown may bring us back here.
	if (inEndpoint.state() == IS_DEAD) {
        //dbugLog(_FLD, "t:%d endpoint already IS_DEAD", inEndpoint->teulIndex());
		return;
	}

    dbugLog(_FLD, "t:%d closing st:%u cl:%s", inEndpoint.teulIndex(), inEndpoint.state(), inEndpoint.m_closed ? "yes" : "no");
	if (!inEndpoint.m_closed) {
		inEndpoint.doError(ENOTCONN);
    }

	// endpoint.doError() may have recursed, so check the state again
	if (inEndpoint.state() == IS_DEAD) {
        return;
    }
    
    // check status of paired backchannel, if client release - it'll get recreated when they log back in
    RakNetChannel::disconnectPeer(&inEndpoint);
    inEndpoint.shutdown();
    inEndpoint.m_state = IS_DEAD;
    inEndpoint.m_doTimeoutCheck = false;	// stop checking timeouts on me.
    teulRemoveEndpoint(&inEndpoint);
}

//////////////////////////////////////////////////////////////////////
// Tear down a endpoint by index.
void teulClose(const SINT32 teulIndex)
{
	if (ENDPOINT* const theEndpoint = teulGetEndpoint(teulIndex)) {
        teulClose(*theEndpoint);
    }
}
