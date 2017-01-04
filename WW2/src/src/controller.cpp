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

#include "teul/teulKit.h"
#include "wwii/wwiiNetPorts.h"
#include "src/controllerGameProcess.h"
#include "server/serverInitializeProcess.h"

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
static std::map<evutil_socket_t, bufferevent*> s_clients;

// TeulKit uses libevent and has an eventBase which it is going to initialize,
// so we want to expose it to ourselves.
namespace Teul
{
	extern event_base* eventBase;
}

// Name of our server.
const char* const g_serverName = "controller";

// The once-per-second heartbeat event.
static event*			s_heartbeatEvent;
static evutil_socket_t	s_managedProcessSocket = INVALID_SOCKET;
static event*			s_managedProcessEvent = nullptr;
static evutil_socket_t	s_repeaterSocket = INVALID_SOCKET;
static event*			s_repeaterEvent = nullptr;

bool g_singleShot = false;
bool g_shuttingDown = false;
bool g_enableStarts = true;

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
										, UINT32 ipAddr_hostOrdered_, UINT16 port_hostOrdered_
										, UINT32 acceptBacklog_, event_callback_fn acceptFn_
										)
{
	ABORT_IF(socket_ == nullptr);
	ABORT_IF(event_ == nullptr);

	*socket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	evutil_make_socket_nonblocking(*socket_);
	evutil_make_socket_closeonexec(*socket_);
	evutil_make_listen_socket_reuseable(*socket_);

	struct sockaddr_in sin{};
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = htonl(ipAddr_hostOrdered_);
	sin.sin_port = htons(port_hostOrdered_);

	if ( bind(*socket_, (struct sockaddr*)&sin, sizeof(sin)) < 0 )
		LogAbort("Unable to bind listening socket [already running?] %d (%08x:%u).", *socket_, ipAddr_hostOrdered_, port_hostOrdered_);

	if ( listen(*socket_, acceptBacklog_) < 0 )
		LogAbort("Unable to listen on listen socket %d (%08x:%u).", *socket_, ipAddr_hostOrdered_, port_hostOrdered_);

	// Create an event to listen for new connections on this socket.
	*event_ = event_new(Teul::eventBase, *socket_, EV_READ|EV_PERSIST, acceptFn_, nullptr);
	event_add(*event_, nullptr);
}

//////////////////////////////////////////////////////////////////////////
// Helper for dealing with an accept call on a libevent endpoint.

static evutil_socket_t _libEventAcceptHandler(evutil_socket_t listener, short event, sockaddr_in& cliAddr_)
{
	socklen_t caLen = sizeof(cliAddr_);
	evutil_socket_t cliSock;
	do
	{
		cliSock = accept(listener, (struct sockaddr*)&cliAddr_, &caLen);
	} while (cliSock < 0 && (errno == PLAT_EAGAIN || errno == PLAT_EINTR));
	if (cliSock >= 0) {
		evutil_make_socket_nonblocking(cliSock);
		evutil_make_socket_closeonexec(cliSock);
	}

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
	const evutil_socket_t srcSock = *(evutil_socket_t*)arg_;
	struct evbuffer* input = bufferevent_get_input(bev_);
	ABORT_IF(input == nullptr);

#ifdef CONTROLLER_STATS
	size_t numReads = 0;
	size_t bytesRead = 0;
	size_t numWrites = 0;
	size_t bytesWritten = 0;
#endif

	struct evbuffer_iovec ebiov;
	for ( ; ; )
	{
		size_t contigBytes = evbuffer_get_contiguous_space(input);
		if ( contigBytes == 0 )
			break;

		int n = evbuffer_peek(input, -1, nullptr, &ebiov, 1);
		if ( n <= 0 || ebiov.iov_base == nullptr || ebiov.iov_len == 0 )
			break;

		for ( auto it = s_clients.begin(), end = s_clients.end(); it != end; ++it )
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
	dbugLog(_FLD, "sock:" PRF_SOCKET " read %" PRSizeT " bytes in %" PRSizeT " reads; wrote %" PRSizeT " bytes in %" PRSizeT " writes."
					, PRA_SOCKET(srcSock)
					, bytesRead, numReads
					, bytesWritten, numWrites
					);
#endif
}

////////////////////////////////////////////////////////////////////////////////
// Handle an error event on the repeater interface - WITH FIRE (aka disconnection)

static void _onRepeaterError(struct bufferevent* bev_, short error_, void* arg_)
{
	evutil_socket_t cliSock = *(evutil_socket_t*)arg_;
    if (error_ & BEV_EVENT_EOF)
	{
		dbugLog(_FLN, "sock:" PRF_SOCKET " disconnected.", PRA_SOCKET(cliSock));
    }
	else if (error_ & BEV_EVENT_ERROR)
	{
		dbugLog(_FLW, "sock:" PRF_SOCKET "error:%d. disconnecting.", PRA_SOCKET(cliSock), dbugGetErrno());
    }
	else if (error_ & BEV_EVENT_TIMEOUT)
	{
		dbugLog(_FLW, "sock:" PRF_SOCKET " timeout. disconnecting.", PRA_SOCKET(cliSock));
    }
	else
	{
		dbugLog(_FLW, "sock:" PRF_SOCKET " error %d", PRA_SOCKET(cliSock), error_);
	}

    bufferevent_free(bev_);
	s_clients.erase(cliSock);
	delete (evutil_socket_t*)(arg_);
}

////////////////////////////////////////////////////////////////////////////////
// New connection on the repeater interface, add it to the pool.

static void _onRepeaterAccept(evutil_socket_t listener_, short event_, void*)
{
	struct sockaddr_in cliAddr;
	evutil_socket_t cliSock = _libEventAcceptHandler(listener_, event_, cliAddr);
	if ( cliSock < 0 )
	{
		dbugLog(_FLI, "accept error: %d", cliSock);
		return;
	}

	dbugLog(_FLN, "sock:" PRF_SOCKET " connected from %s:%u"
				, PRA_SOCKET(cliSock)
				, teulGetAddress(ntohl(cliAddr.sin_addr.s_addr)), ntohs(cliAddr.sin_port)
				);

	struct bufferevent* bev = bufferevent_socket_new(Teul::eventBase, cliSock, BEV_OPT_CLOSE_ON_FREE);
	bufferevent_setcb(bev, _onRepeaterRead, nullptr, _onRepeaterError, new evutil_socket_t(cliSock));
	bufferevent_enable(bev, EV_READ);

	const bool socketAlreadyRegistered = (s_clients.find(cliSock) != s_clients.end());
	ABORT_IF( socketAlreadyRegistered );

	s_clients[cliSock] = bev;
}

//////////////////////////////////////////////////////////////////////////
// Process management.

static std::map<evutil_socket_t /*socket*/, UINT64 /*processID*/> s_socketToProcess;
static std::map<UINT64 /*processID*/, evutil_socket_t /*socket*/> s_processToSocket;

//////////////////////////////////////////////////////////////////////////
// Managed process disconnecting.

static void _onManagedProcessError(struct bufferevent* bev_, short error_, void* arg_)
{
	evutil_socket_t cliSock = *(evutil_socket_t*)(arg_);
	if (error_ & BEV_EVENT_EOF)
	{
		dbugLog(_FLN, "sock:" PRF_SOCKET " disconnected.", PRA_SOCKET(cliSock));
	}
	else if (error_ & BEV_EVENT_ERROR)
	{
		dbugLog(_FLW, "sock:" PRF_SOCKET " error:%d. disconnecting.", PRA_SOCKET(cliSock), dbugGetErrno());
	}
	else if (error_ & BEV_EVENT_TIMEOUT)
	{
		dbugLog(_FLW, "sock:" PRF_SOCKET " timeout. disconnecting.", PRA_SOCKET(cliSock));
	}
	else
	{
		dbugLog(_FLW, "sock:" PRF_SOCKET " error %d", PRA_SOCKET(cliSock), error_);
	}

	bufferevent_free(bev_);

	auto it = s_socketToProcess.find(cliSock);
	if ( it != s_socketToProcess.end() )
	{
		dbugLog(_FLI, "sock:" PRF_SOCKET " pid:%" PR64BitUT " disconnecting", PRA_SOCKET(cliSock), it->second);
		delete (evutil_socket_t*)arg_;
		///TODO: Tell GameProcessManager that the process went away.
		if ( it->second != 0ULL )
			s_processToSocket.erase(it->second);
		s_socketToProcess.erase(cliSock);
	}
}

//////////////////////////////////////////////////////////////////////////
// Receiving data from a managed process, should only happen at startup.

static bool _handleIntro(evutil_socket_t srcSock, const ProcessIntroduction& intro)
{
	auto pit = s_processToSocket.find(intro.m_processID);
	ABORT_IF( pit != s_processToSocket.end() );
	auto process = GameProcessManager::LookupGameProcess(intro.m_processID);
	if (!process) {
		dbugLog(_FLW, "sock:" PRF_SOCKET " pid:%" PR64BitUT " no process entry.", PRA_SOCKET(srcSock), intro.m_processID);
		return false;
	}
	if (process->ServerType() != intro.m_processType) {
		dbugLog(_FLW, "sock:" PRF_SOCKET " pid:%" PR64BitUT " type mismatch (expected %u got %u).",
			PRA_SOCKET(srcSock), intro.m_processID, process->ServerType(), intro.m_processType
		);
		return false;
	}

	if (!process->OnStarted())
		return false;
	if (!process->IsRunning())
		return false;

	s_socketToProcess[srcSock] = intro.m_processID;
	s_processToSocket[intro.m_processID] = srcSock;
	dbugLog(_FLI, "sock:" PRF_SOCKET " pid:%" PR64BitUT " established", PRA_SOCKET(srcSock), intro.m_processID);

	return true;
}

static void _onManagedProcessRead(struct bufferevent* bev_, void* arg_)
{
	evutil_socket_t srcSock = *(evutil_socket_t*)arg_;
	auto sockIt = s_socketToProcess.find(srcSock);
	ABORT_IF( sockIt == s_socketToProcess.end() );
	if ( sockIt->second ) {
		dbugLog(_FLW, "sock:" PRF_SOCKET " intro on socket we have pid for", PRA_SOCKET(srcSock));
	} else {
		struct evbuffer* input = bufferevent_get_input(bev_);
		ABORT_IF(input == nullptr);

		if (evbuffer_get_length(input) != sizeof(ProcessIntroduction)) {
			dbugLog(_FLM, "length of data does not match ProcessIntroduction");
		} else {
			ProcessIntroduction intro;
			int n = evbuffer_remove(input, &intro, sizeof(intro));

			// We just checked if there was enough data, and there looked to be.
			ABORT_IF(n != sizeof(intro));

			if (_handleIntro(srcSock, intro))
				return;
		}
	}

	_onManagedProcessError(bev_, 0, arg_);
}

//////////////////////////////////////////////////////////////////////////
// Managed process connecting.

static void _onManagedProcessAccept(evutil_socket_t listener_, short event_, void*)
{
	struct sockaddr_in cliAddr;
	evutil_socket_t cliSock = _libEventAcceptHandler(listener_, event_, cliAddr);
	if ( cliSock < 0 )
	{
		dbugLog(_FLI, "managed process accept error: %d", cliSock);
		return;
	}

	dbugLog(_FLN, "sock:" PRF_SOCKET " process connected from %s:%u", PRA_SOCKET(cliSock), teulGetAddress(ntohl(cliAddr.sin_addr.s_addr)), ntohs(cliAddr.sin_port));

	struct bufferevent* bev = bufferevent_socket_new(Teul::eventBase, cliSock, BEV_OPT_CLOSE_ON_FREE);
	bufferevent_setcb(bev, _onManagedProcessRead, nullptr, _onManagedProcessError, new evutil_socket_t(cliSock));
	bufferevent_enable(bev, EV_READ);

	const bool socketAlreadyRegistered = (s_socketToProcess.find(cliSock) != s_socketToProcess.end());
	ABORT_IF( socketAlreadyRegistered );

	s_socketToProcess[cliSock] = 0ULL;
}

////////////////////////////////////////////////////////////////////////////////
// Get server type by name

static SERVER_TYPE _getServerType(const std::string& name)
{
	for (UINT32 i = 0; i < MAX_SERVER_TYPE; ++i)
	{
		if ( name == g_serverTypes[i] )
			return static_cast<SERVER_TYPE>(i);
	}
	return MAX_SERVER_TYPE;
}

////////////////////////////////////////////////////////////////////////////////
// Check to see if we need to manage any processes.

static void _addInitialProcesses()
{
	const char* p = cv_manageProcesses.GetString();
	while ( p && *p )
	{
		const char* end = strchr(p, ';');
		std::string name = (end) ? std::string(p, end - p) : std::string(p);
		auto serverType = _getServerType(name);
		if ( serverType >= MAX_SERVER_TYPE ) {
			LogAbort("'%s': '%s': Unrecognized server: %s",
						cv_manageProcesses.GetEnvVarName(),
						cv_manageProcesses.GetString(),
						name.c_str());
		}
		dbugLog(_FLI, "Managing %s", name.c_str());
		GameProcessManager::AddServer(serverType);
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

extern int teulModIdle(const TeulTimeMS);

static void heartbeatFunction(evutil_socket_t /*-1*/, short /*eventBits*/, void* /*arg*/)
{
	teulModIdle(0);
	GameProcessManager::Heartbeat(g_enableStarts);
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

static void _startShutdown(int /*signal*/)
{
	g_enableStarts = false;
	if ( !g_shuttingDown )
	{
		g_shuttingDown = true;
		event_base_loopbreak(Teul::eventBase);
		dbugLog(_FLA, "abort signal received: shutting down.");
	}
}

////////////////////////////////////////////////////////////////////////////////
// Toggle the spawning of new processes.

#if defined(SIGUSR1)
static void _toggleStarts(int /*signal*/)
{
	if ( !g_shuttingDown )
		g_enableStarts = !g_enableStarts;
}
#endif

////////////////////////////////////////////////////////////////////////////////
// Entry point.

int main(int argc, const char* argv[], const char* envp[])
{
	static bool s_singleStartMode = false;
	static std::vector<std::string> s_envQueries;
	Server::InitializeProcess(g_serverName, CONTROLLER_HOST, _errorFunction, nullptr, argc, argv, {
		{ "--configs", "List configuration variables and exit.", false,
			[](const char*) {
				Teul::ConfigVar::PrintCurrentVars();
				exit(0);
			},
			nullptr,
		},
		{ "--make-config-file", "Generate an environment.cfg file and exit.", true,
			[](const char* filename) {
				Teul::ConfigVar::Generate(filename);
				exit(0);
			},
			nullptr,
		},
		{ "--getenv", "Output value of specified environment variable(s) (in order) to stdout and exit.", true,
			[&](const char* varName) {
				// cmd line parsing precedes env loading, so we have to save these and query later.
				s_envQueries.push_back(varName);
			},
			[&]() {
				if (s_envQueries.empty() == false) {
					for (std::string varName : s_envQueries) {
						const char* value = Teul::ConfigVar::GetEnv(varName);
						if (!value) {
							fprintf(stderr, "unrecognized variable: %s\n", varName.c_str());
							exit(1);
						}
						printf("%s\n", value);
					}
					exit(0);
				}
			}
		},
		{ "--single-shot", "Launch each child process only once.", false,
			[&](const char*) {
				s_singleStartMode = true;
			}
		}
	});
#ifndef NDEBUG
    dbugSetLogLevel(Debug::ERR_DBUG | STD_ERR);
#endif

	if ( Teul::ConfigVar::LoadedConfig() == false )
	{
		Teul::ConfigVar::Generate("environment.cfg");
		LogAbort("environment.cfg could not be found - generated new one, please edit it.");
	}

	if ( !cv_manageProcesses.GetString() || !*cv_manageProcesses.GetString() ) {
		dbugLog(_FLM, "'%s' list is empty", cv_manageProcesses.GetEnvVarName());
	}

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
#if defined(SIGUSR1)
	signal(SIGUSR1, _toggleStarts);
#endif

	GameProcessManager::Initialize(envp, s_singleStartMode);

	// Our main loop is handled by libevent rather than teulKit.
	event_base_dispatch(Teul::eventBase);

	if ( g_shuttingDown == false )
	{
		dbugLog(_FLA, "exited event_base_dispatch without being flagged as shutting down.");
		g_shuttingDown = true;
	}

	// Kill all the child processes.
	GameProcessManager::Shutdown();

	_shutdownNetwork();

	teulCloseTransport();
}
