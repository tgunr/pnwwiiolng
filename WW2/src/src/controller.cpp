#include <syscfg.h>	// Path is project specific.
// Distributor:
// - Accepts incoming connections,
// - Forwards incoming packets to all *other* connections

#include <fcntl.h>
#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/util.h>
#include <set>

#include "dbug/dbug.h"
#include "teul/teulKit.h"
#include "wwii/wwiiNetPorts.h"
#include "src/controllerGameProcess.h"

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

// List of current clients and the bufferevent associated with each.
#include <map>
static std::map<int, bufferevent*> s_clients;

// TeulKit uses libevent and has an eventBase which it is going to initialize,
// so we want to expose it to ourselves.
namespace Teul
{
	extern event_base* eventBase;
}
extern void teulPrintConfigVars();
extern void teulGenerateEnvironmentCfgFile(const char* const filename);

// Name of our server.
const char* const g_serverName = "controller";

// The once-per-second heartbeat event.
static event*			s_heartbeatEvent;
static evutil_socket_t	s_managedProcessSocket = INVALID_SOCKET;
static event*			s_managedProcessEvent = nullptr;
static evutil_socket_t	s_repeaterSocket = INVALID_SOCKET;
static event*			s_repeaterEvent = nullptr;

bool g_shuttingDown = false;

extern ConfigStr cv_manageProcesses;

////////////////////////////////////////////////////////////////////////////////
// TeulKit error handler.

static void _errorFunction(SINT32 /*teulIndex*/, const UCHAR* const /*data*/, SINT32 /*size*/, const TEUL_CALLBACK* const /*object*/, SINT32 /*error*/)
{
	LogAbort("Teul called cluster server error function -- shouldn't happen.");
}

//////////////////////////////////////////////////////////////////////////
// Helper for setting up a new libevent listening socket.

static void _prepareLibEventListenSocket(evutil_socket_t* socket_, event** event_
										, UINT32 ipAddr_hostOrdered_, UINT16 port_hostOrdered_, UINT32 acceptBacklog_
										, event_callback_fn acceptFn_
										)
{
	ABORT_IF(socket_ == nullptr);
	ABORT_IF(event_ == nullptr);

	*socket_ = socket(AF_INET, SOCK_STREAM, 0);
	evutil_make_socket_nonblocking(*socket_);
	evutil_make_listen_socket_reuseable(*socket_);

	struct sockaddr_in sin;
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = htonl(ipAddr_hostOrdered_);
	sin.sin_port = htons(port_hostOrdered_);

	if ( bind(*socket_, (struct sockaddr*)&sin, sizeof(sin)) < 0 )
		LogAbort("Unable to bind listening socket [already running?] %d (%08x:%u).", *socket_, ipAddr_hostOrdered_, port_hostOrdered_);

	if ( listen(*socket_, 16) < 0 )
		LogAbort("Unable to listen on listen socket %d (%08x:%u).", *socket_, ipAddr_hostOrdered_, port_hostOrdered_);

	// Create an event to listen for new connections on this socket.
	*event_ = event_new(Teul::eventBase, *socket_, EV_READ|EV_PERSIST, acceptFn_, (void*)Teul::eventBase);
	event_add(*event_, nullptr);
}

//////////////////////////////////////////////////////////////////////////
// Helper for dealing with an accept call on a libevent endpoint.

static evutil_socket_t _libEventAcceptHandler(evutil_socket_t listener, short event, void* arg, sockaddr_in& cliAddr_)
{
	auto base = static_cast<event_base*>(arg);
	socklen_t caLen = sizeof(cliAddr_);
	evutil_socket_t cliSock = EAGAIN;
	while ( (cliSock == EAGAIN) | (cliSock == EINTR) )
	{
		cliSock = accept(listener, (struct sockaddr*)&cliAddr_, &caLen);
	};

	return cliSock;
}

//////////////////////////////////////////////////////////////////////////
// Helper for shutting down a libevent endpoint.

static void _shutdownLibEventSocket(evutil_socket_t& socket_, event*& event_)
{
	if (event_)
	{
		event_free(event_);
		event_ = nullptr;
	}
	if (socket_ != INVALID_SOCKET)
	{
		evutil_closesocket(socket_);
		socket_ = INVALID_SOCKET;
	}
}

////////////////////////////////////////////////////////////////////////////////
// Handle a libevent ready-to-read event on the repeater interface, this means
// someone has sent data, we forward it to all other repeater clients.

static void _onRepeaterRead(struct bufferevent* bev_, void* arg_)
{
	int srcSock = (int)arg_;
	struct evbuffer* input = bufferevent_get_input(bev_);
	ABORT_IF(input == nullptr);

#ifdef CONTROLLER_STATS
	size_t numReads = 0;
	size_t bytesRead = 0;
	size_t numWrites = 0;
	size_t bytesWritten = 0;
#endif

	struct evbuffer_iovec ebiov;
	for ( ;; )
	{
		size_t contigBytes = evbuffer_get_contiguous_space(input);
		if ( contigBytes == 0 )
			break;

		int n = evbuffer_peek(input, -1, nullptr, &ebiov, 1);
		if ( n <= 0 || ebiov.iov_base == nullptr || ebiov.iov_len == 0 )
			break;

		for ( auto it = s_clients.begin(); it != s_clients.end(); ++it )
		{
			auto destSock = it->first;
			if ( destSock == srcSock )
				continue;
			auto output = bufferevent_get_output(it->second);
			ABORT_IF(output == nullptr);
			evbuffer_add(output, ebiov.iov_base, ebiov.iov_len);

#ifdef CONTROLLER_STATS
			++numWrites;
			bytesWritten += ebiov.iov_len;
#endif
		}

		evbuffer_drain(input, ebiov.iov_len);

#ifdef CONTROLLER_STATS
		++numReads;
		bytesRead += ebiov.iov_len;
#endif
	}

#ifdef CONTROLLER_STATS
	dbugLog(_FLD, "sock:%d read %" PRSizeT " bytes in %" PRSizeT " reads; wrote %" PRSizeT " bytes in %" PRSizeT " writes."
					, srcSock
					, bytesRead, numReads
					, bytesWritten, numWrites
					);
#endif
}

////////////////////////////////////////////////////////////////////////////////
// Handle an error event on the repeater interface - WITH FIRE (aka disconnection)

static void _onRepeaterError(struct bufferevent* bev_, short error_, void* arg_)
{
	int cliSock = (int)(arg_);
    if (error_ & BEV_EVENT_EOF)
	{
		dbugLog(_FLM, "sock:%d disconnected.", cliSock);
    }
	else if (error_ & BEV_EVENT_ERROR)
	{
		dbugLog(_FLW, "sock:%d error:%d. disconnecting.", cliSock, dbugGetErrno());
    }
	else if (error_ & BEV_EVENT_TIMEOUT)
	{
		dbugLog(_FLW, "sock:%d timeout. disconnecting.", cliSock);
    }
	else
	{
		dbugLog(_FLW, "sock:%d error %d", cliSock, error_);
	}

    bufferevent_free(bev_);
	s_clients.erase(cliSock);
}

////////////////////////////////////////////////////////////////////////////////
// New connection on the repeater interface, add it to the pool.

static void _onRepeaterAccept(evutil_socket_t listener_, short event_, void* arg_)
{
	struct sockaddr_in cliAddr;
	evutil_socket_t cliSock = _libEventAcceptHandler(listener_, event_, arg_, cliAddr);
	if ( cliSock < 0 )
	{
		dbugLog(_FLI, "accept error: %d", cliSock);
		return;
	}

	evutil_make_socket_nonblocking(cliSock);
	struct bufferevent* bev = bufferevent_socket_new(static_cast<event_base*>(arg_), cliSock, BEV_OPT_CLOSE_ON_FREE);
	bufferevent_setcb(bev, _onRepeaterRead, nullptr, _onRepeaterError, (void*)cliSock);
	bufferevent_enable(bev, EV_READ|EV_WRITE|EV_PERSIST);

	const bool socketAlreadyRegistered = (s_clients.find(cliSock) != s_clients.end());
	ABORT_IF( socketAlreadyRegistered );

	s_clients[cliSock] = bev;

	dbugLog(_FLN, "sock:%d connected from %s:%u", cliSock, teulGetAddress(ntohl(cliAddr.sin_addr.s_addr)), ntohs(cliAddr.sin_port));
}

//////////////////////////////////////////////////////////////////////////
// Process management.

static std::map<evutil_socket_t /*socket*/, UINT64 /*processID*/> s_processSockets;

//////////////////////////////////////////////////////////////////////////
// Managed process disconnecting.

static void _onManagedProcessError(struct bufferevent* bev_, short error_, void* arg_)
{
	int cliSock = (int)(arg_);
	if (error_ & BEV_EVENT_EOF)
	{
		dbugLog(_FLM, "sock:%d disconnected.", cliSock);
	}
	else if (error_ & BEV_EVENT_ERROR)
	{
		dbugLog(_FLW, "sock:%d error:%d. disconnecting.", cliSock, dbugGetErrno());
	}
	else if (error_ & BEV_EVENT_TIMEOUT)
	{
		dbugLog(_FLW, "sock:%d timeout. disconnecting.", cliSock);
	}
	else
	{
		dbugLog(_FLW, "sock:%d error %d", cliSock, error_);
	}

	bufferevent_free(bev_);

	auto it = s_processSockets.find(cliSock);
	if ( it != s_processSockets.end() && it->second != 0ULL )
	{
		dbugLog(_FLI, "sock:%d process:%llu disconnecting", cliSock, it->second);
		///TODO: Tell GameProcessManager that the process went away.
	}
}

//////////////////////////////////////////////////////////////////////////
// Receiving data from a managed process, should only happen at startup.

static void _onManagedProcessRead(struct bufferevent* bev_, void* arg_)
{
	int srcSock = (int)arg_;
	if ( s_processSockets.find(srcSock) != s_processSockets.end() )
	{
		dbugLog(_FLW, "unexpected data from managed process socket %d", srcSock);
		evutil_closesocket(srcSock);
	}

	struct evbuffer* input = bufferevent_get_input(bev_);
	ABORT_IF(input == nullptr);

	if (evbuffer_get_length(input) < sizeof(GameProcessManager::Introduction))
		return;

	GameProcessManager::Introduction intro;
	int n = evbuffer_remove(input, &intro, sizeof(intro));
	// We just checked if there was enough data, and there looked to be.
	ABORT_IF( n < 0 );

	///TODO: Tell GameProcessManager that this process just connected to us.
	//if (!GameProcessManager::OnConnected(srcSock, intro))
	//	evutil_closesocket(srcSock);
}

//////////////////////////////////////////////////////////////////////////
// Managed process connecting.

static void _onManagedProcessAccept(evutil_socket_t listener_, short event_, void* arg_)
{
	struct sockaddr_in cliAddr;
	evutil_socket_t cliSock = _libEventAcceptHandler(listener_, event_, arg_, cliAddr);
	if ( cliSock < 0 )
	{
		dbugLog(_FLI, "managed process accept error: %d", cliSock);
		return;
	}

	evutil_make_socket_nonblocking(cliSock);
	struct bufferevent* bev = bufferevent_socket_new(static_cast<event_base*>(arg_), cliSock, BEV_OPT_CLOSE_ON_FREE);
	bufferevent_setcb(bev, _onManagedProcessRead, nullptr, _onManagedProcessError, (void*)cliSock);
	bufferevent_enable(bev, EV_READ|EV_PERSIST);

	const bool socketAlreadyRegistered = (s_processSockets.find(cliSock) != s_processSockets.end());
	ABORT_IF( socketAlreadyRegistered );

	s_processSockets[cliSock] = 0ULL;

	dbugLog(_FLN, "sock:%d process connected from %s:%u", cliSock, teulGetAddress(ntohl(cliAddr.sin_addr.s_addr)), ntohs(cliAddr.sin_port));
}

////////////////////////////////////////////////////////////////////////////////
// Describe how to use this application.

static void  _usage(bool detailed_)
{
	fprintf(stderr, "Usage: controller [--help] [--configs] [--make-config-only]\n");
	if ( detailed_ )
	{
		fprintf(stderr, "  --help                 Show this information and exit.\n");
		fprintf(stderr, "  --configs              List configuration variables and exit.\n");
		fprintf(stderr, "  --make-config-file     Generate an environment.cfg file and exit.\n");
	}
}

////////////////////////////////////////////////////////////////////////////////
// Describe command line usage and abort.

static NORETURN_FN void _exitWithUsage(const char* const error_, const char* const cause_)
{
	fprintf(stderr, "ERROR: %s: %s\n", error_, cause_);
	_usage(false);
	exit(1);
}

////////////////////////////////////////////////////////////////////////////////
// Process command line arguments.

static void _processArguments(int argc, const char* argv[])
{
	for ( int argn = 1; argn < argc && argv[argn] != nullptr; ++argn )
	{
		const char* arg = nullptr;
		if ( argv[argn][0] == '/' )
			arg = argv[argn] + 1;
		else if ( argv[argn][0] == '-' && argv[argn][1] == '-' )
			arg = argv[argn] + 2;

		if ( arg != nullptr  )
		{
			if ( strcasecmp(arg, "help") == 0 || strcasecmp(arg, "?") == 0 )
			{
				_usage(true);
				exit(0);
			}

			if ( strcasecmp(arg, "configs") == 0 )
			{
				Teul::ConfigVar::InitializeConfigVars();
				teulPrintConfigVars();
				exit(0);
			}

			if ( strcasecmp(arg, "make-config-file") == 0 )
			{
				if ( argn + 1 >= argc )
					_exitWithUsage("make-config-file must be followed by a filename.", "missing file name");
				else if ( argv[argn + 1][0] == '-' )
					_exitWithUsage("make-config-file must be followed by a filename.", argv[argn+1]);

				teulGenerateEnvironmentCfgFile(argv[argn + 1]);

				exit(0);
			}
		}

		_exitWithUsage("Unrecognized command line option", argv[argn]);
	}
}

////////////////////////////////////////////////////////////////////////////////
// Check to see if we need to manage any processes.

static void _addInitialProcesses()
{
	const char* p = cv_manageProcesses.GetString();
	while ( p && *p )
	{
		const char* end = strchr(p, ';');
		for ( UINT32 i = 0 ; i < MAX_SERVER_TYPE ; ++i )
		{
			if ( strncmp(g_serverTypes[i], p, strlen(g_serverTypes[i])) == 0 )
			{
				GameProcessManager::AddServer(static_cast<SERVER_TYPE>(i));
				break;
			}
		}
		p = end;
		if ( p )	// it points to a semicolon
			++p;	// which we want to skip.
	}
}

////////////////////////////////////////////////////////////////////////////////
// Schedule the heartbeat.

static void _scheduleHeartbeatEvent()
{
	timeval one_sec = { 1, 0 };
	event_add(s_heartbeatEvent, &one_sec);
}

////////////////////////////////////////////////////////////////////////////////
// Once-per-second event handling/checking.

static void heartbeatFunction(evutil_socket_t /*-1*/, short /*eventBits*/, void* /*arg*/)
{
	GameProcessManager::Heartbeat();
	_scheduleHeartbeatEvent();
}

//////////////////////////////////////////////////////////////////////////
// Clean shutdown

static void _shutdownNetwork()
{
	for ( auto it = s_clients.begin(); it != s_clients.end(); )
	{
		evutil_socket_t sock = (it++)->first;
		evutil_closesocket(sock);
	}
	s_clients.clear();

	// If we reach this point, we are shutting down.
	if ( s_heartbeatEvent )
	{
		event_free(s_heartbeatEvent);
		s_heartbeatEvent = nullptr;
	}

	_shutdownLibEventSocket(s_repeaterSocket, s_repeaterEvent);
	_shutdownLibEventSocket(s_managedProcessSocket, s_managedProcessEvent);
}

//////////////////////////////////////////////////////////////////////////
// CTRL-C handler: Initiate shutdown.

void _startShutdown(int /*signal*/)
{
	if ( !g_shuttingDown )
	{
		g_shuttingDown = true;
		event_base_loopbreak(Teul::eventBase);
		dbugLog(_FLA, "abort signal received: shutting down.");
	}
}

////////////////////////////////////////////////////////////////////////////////
// Entry point.

int main(int argc, const char* argv[])
{
	// Check arguments.
	_processArguments(argc, argv);

	Teul::ConfigVar::InitializeConfigVars();
	if ( Teul::ConfigVar::LoadedConfig() == false )
	{
		teulGenerateEnvironmentCfgFile("environment.cfg");
		LogAbort("environment.cfg could not be found - generated new one, please edit it.");
	}

	Teul::SetAppName(g_serverName);
	if (teulInitTransport(LOG_TO_FILE, g_serverName, _errorFunction) < 0)
		LogAbort("Failed to initialize cluster boss");

	// Prepare the repeater socket, which forwards packets from one peer to all other peers.
	_prepareLibEventListenSocket(&s_repeaterSocket, &s_repeaterEvent, INADDR_ANY, WWII::Net::Ports::PrivateBcast, 16, _onRepeaterAccept);

	// Prepare the managed process control socket.
	_prepareLibEventListenSocket(&s_managedProcessSocket, &s_managedProcessEvent, INADDR_LOOPBACK, WWII::Net::Ports::ServerPeering, 8, _onManagedProcessAccept);

	// Look for processes we're supposed to manage.
	_addInitialProcesses();

	// Once-a-second heartbeat tick.
	s_heartbeatEvent = evtimer_new(Teul::eventBase, heartbeatFunction, nullptr);
	_scheduleHeartbeatEvent();

	signal(SIGINT, _startShutdown);

	// Our mainloop is handled by libevent rather than teulkit.
	event_base_dispatch(Teul::eventBase);

	if ( g_shuttingDown == false )
	{
		dbugLog(_FLAS, "exited event_base_dispatch without being flagged as shutting down.");
		g_shuttingDown = true;
	}

	// Kill all the child processes.
	GameProcessManager::Shutdown();

	_shutdownNetwork();
}
