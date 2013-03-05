#include <syscfg.h>	// Path is project specific.
// Distributor:
// - Accepts incoming connections,
// - Forwards incoming packets to all *other* connections

#include <fcntl.h>
#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/util.h>

#include "dbug/dbug.h"
#include "teul/teulKit.h"
#include "wwii/wwiiNetPorts.h"

#if platOS == OS_WIN32
#include <winsock2.h>
#include <WinInet.h>
#include <Winineti.h>
#include <WinIoCtl.h>
#include <Ws2tcpip.h>
#pragma comment(lib, "event.lib")
#pragma comment(lib, "ws2_32.lib")
#include <iphlpapi.h>
#pragma comment(lib, "IPHLPAPI.lib")
#else
#include <sys/ioctl.h>
#include <sys/socket.h>
#endif

// We need access to teul transport's event base.
namespace Teul
{
	extern event_base* eventBase;
}

// Name of our server.
const char* const g_serverName = "cluster";

static void _errorFunction(SINT32 /*teulIndex*/, const UCHAR* const /*data*/, SINT32 /*size*/, const TEUL_CALLBACK* const /*object*/, SINT32 /*error*/)
{
	LogAbort("Teul called cluster server error function -- shouldn't happen.");
}

#include <map>
static std::map<int, bufferevent*> clients;

static void
_onEventBufferRead(struct bufferevent* bev, void* arg)
{
	int srcSock = (int)arg;
	struct evbuffer* input = bufferevent_get_input(bev);
	ABORT_IF(input == nullptr);

	size_t numReads = 0;
	size_t bytesRead = 0;
	size_t numWrites = 0;
	size_t bytesWritten = 0;

	struct evbuffer_iovec ebiov;
	for ( ;; )
	{
		size_t contigBytes = evbuffer_get_contiguous_space(input);
		if ( contigBytes == 0 )
			break;

		int n = evbuffer_peek(input, -1, nullptr, &ebiov, 1);
		if ( n <= 0 || ebiov.iov_base == nullptr || ebiov.iov_len == 0 )
			break;

		for ( auto it = clients.begin(); it != clients.end(); ++it )
		{
			auto destSock = it->first;
			if ( destSock == srcSock )
				continue;
			auto output = bufferevent_get_output(it->second);
			ABORT_IF(output == nullptr);
			evbuffer_add(output, ebiov.iov_base, ebiov.iov_len);

			++numWrites;
			bytesWritten += ebiov.iov_len;
		}

		evbuffer_drain(input, ebiov.iov_len);
		++numReads;
		bytesRead += ebiov.iov_len;
	}

	if ( g_logDebug )
	{
		printf("sock:%d read %" PRSizeT " bytes in %" PRSizeT " reads; wrote %" PRSizeT " bytes in %" PRSizeT " writes.\n"
						, srcSock
						, bytesRead, numReads
						, bytesWritten, numWrites
						);
		dbugLog(_FLD, "sock:%d read %" PRSizeT " bytes in %" PRSizeT " reads; wrote %" PRSizeT " bytes in %" PRSizeT " writes."
						, srcSock
						, bytesRead, numReads
						, bytesWritten, numWrites
						);
	}
}

static void
_onEventBufferError(struct bufferevent* bev, short error, void* arg)
{
	int cliSock = (int)(arg);
    if (error & BEV_EVENT_EOF)
	{
		dbugLog(_FLM, "sock:%d disconnected.", cliSock);
    }
	else if (error & BEV_EVENT_ERROR)
	{
		dbugLog(_FLW, "sock:%d error:%d. disconnecting.", cliSock, dbugGetErrno());
    }
	else if (error & BEV_EVENT_TIMEOUT)
	{
		dbugLog(_FLW, "sock:%d timeout. disconnecting.", cliSock);
    }
	else
	{
		dbugLog(_FLW, "sock:%d error %d", cliSock, error);
	}

    bufferevent_free(bev);
	clients.erase(cliSock);
}

static void
_onAccept(evutil_socket_t listener, short event, void* arg)
{
	auto base = static_cast<event_base*>(arg);
	struct sockaddr_in cliAddr;
	socklen_t caLen = sizeof(cliAddr);
	int cliSock = EAGAIN;
	while ( (cliSock == EAGAIN) | (cliSock == EINTR) )
	{
		cliSock = accept(listener, (struct sockaddr*)&cliAddr, &caLen);
	};

	if ( cliSock < 0 )
	{
		dbugLog(_FLI, "accept error: %d", cliSock);
		return;
	}

	evutil_make_socket_nonblocking(cliSock);
	struct bufferevent* bev = bufferevent_socket_new(base, cliSock, BEV_OPT_CLOSE_ON_FREE);
	bufferevent_setcb(bev, _onEventBufferRead, nullptr, _onEventBufferError, (void*)cliSock);
	bufferevent_enable(bev, EV_READ|EV_WRITE|EV_PERSIST);

	const bool socketAlreadyRegistered = (clients.find(cliSock) != clients.end());
	ABORT_IF( socketAlreadyRegistered );

	clients[cliSock] = bev;

	dbugLog(_FLN, "sock:%d connected from %s:%u", cliSock, teulGetAddress(ntohl(cliAddr.sin_addr.s_addr)), ntohs(cliAddr.sin_port));
}

int
main(int argc, const char* argv[])
{
	Teul::SetAppName(g_serverName);
	if (teulInitTransport(LOG_TO_FILE, g_serverName, _errorFunction) < 0)
		LogAbort("Failed to initialize cluster boss");

	struct sockaddr_in sin;
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons(WWII::Net::Ports::PrivateBcast);

	auto listener = socket(AF_INET, SOCK_STREAM, 0);
	evutil_make_socket_nonblocking(listener);
	evutil_make_listen_socket_reuseable(listener);

	if ( bind(listener, (struct sockaddr*)&sin, sizeof(sin)) < 0 )
		LogAbort("Unable to bind listening socket [already running?]");

	if ( listen(listener, 16) < 0 )
		LogAbort("Unable to listen on listen socket.");

	// Create an event to listen for new connections on this socket.
	auto listenEvent = event_new(Teul::eventBase, listener, EV_READ|EV_PERSIST, _onAccept, (void*)Teul::eventBase);

	event_add(listenEvent, nullptr);

	event_base_dispatch(Teul::eventBase);
}
