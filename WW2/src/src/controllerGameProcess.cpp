#include <syscfg.h>	// Path is project specific.
#include <list>

#if platOS == OS_WIN32
#include "WinBase.h"
#include "processthreadsapi.h"
#else
#include <sys/wait.h>
#endif

#include "teul/teulTime.h"
#include "src/controllerGameProcess.h"

extern bool g_shuttingDown;

static TeulTimeMS s_processLaunchTimeout = 30 * 1000;
static TeulTimeMS s_processShutdownTimeout = 5 * 1000;

// Launch singleton.
GameProcess* GameProcess::s_processStarting = nullptr;

static std::list<GameProcess> s_processes;

//////////////////////////////////////////////////////////////////////
// Helper to determine if a process is currently running.

bool GameProcess::IsRunning() const
{
	if (!m_processID)
		return false;
#if platOS == OS_WIN32
	// Check if the process is still running.
	auto result = WaitForSingleObject(m_processHandle, 0);
	return ( result != WAIT_OBJECT_0 && result != WAIT_FAILED );
#else
	int status = 0;
	auto result = waitpid(m_processID, &status, WEXITED | WNOHANG);
	return ( result == 0 );
#endif
}

//////////////////////////////////////////////////////////////////////
// GameProcess object ctor.

GameProcess::GameProcess(SERVER_TYPE type_)
	: m_type(type_)
	, m_desired(true)
	, m_wantRunning(false)
	, m_startSent(0ULL)
	, m_started(0ULL)
#if platOS == OS_WIN32
	, m_processHandle(0ULL)
#endif
	, m_processID(0ULL)
	, m_stopSent(0ULL)
	, m_stopped(0ULL)
	, m_launchAttempts(0)
{
}

//////////////////////////////////////////////////////////////////////
// Schedule a start next time we can find a launch spot.

void GameProcess::RequestStart()
{
	if ( WantRunning() )
		return;
	m_wantRunning = true;
	m_stopped = 0ULL;
	Log("requestStart");
}

//////////////////////////////////////////////////////////////////////
// Handle the process being successfully started - does not mean that
// it will succeed in startup, just that we've done everything that
// we needed to do to get the OS to run the executable.

void GameProcess::OnStarting(processhandle_t processHandle_, UINT64 processID_)
{
	ABORT_IF( StartSent() );
	ABORT_IF( Running() );
	ABORT_IF( ProcessID() );
	ABORT_IF( s_processStarting != nullptr );

	teulSyncClock();
	m_startSent = teulClock64;
	m_stopped = 0ULL;
#if platOS == OS_WIN32
	m_processHandle = processHandle_;
#endif
	m_processID = processID_;
	s_processStarting = this;
	++m_launchAttempts;

	Log("onStarting");
}

//////////////////////////////////////////////////////////////////////
// Handle the process having launched and registered itself with us.

bool GameProcess::OnStarted()
{
	if (!StartSent() || Running() || s_processStarting != this)
		return false;

	teulSyncClock();
	Log(Debug::ERR_NOTE, "started (attempt %u, took %llums)", m_launchAttempts, (teulClock64 - m_startSent));

	m_started = teulClock64;
	m_stopped = 0ULL;
	s_processStarting = nullptr;
	m_launchAttempts = 0;

	return true;
}

//////////////////////////////////////////////////////////////////////
// Handle a request to begin stopping the process.

void GameProcess::RequestStop()
{
	if ( m_wantRunning == false )
		return;
	m_wantRunning = false;
	Log("requestStop");

	// TODO: Close cleanly.
	Kill();
}

//////////////////////////////////////////////////////////////////////
// We want to stop running this process and not launch any more of it.

void GameProcess::RequestDiscontinue()
{
	// Ignore duplicate discontinue requests.
	if ( m_desired == false )
		return;
	m_desired = false;
	Log("requestDisc");
	RequestStop();
}

//////////////////////////////////////////////////////////////////////
// Handle a process that is no-longer running.

void GameProcess::OnStopped()
{
	if ( m_stopped != 0ULL )
	{
		ABORT_IF( Running() || StartSent() );
		return;
	}
	ABORT_IF( !Running() && !StartSent() );
	if ( Running() )
		Log(Debug::ERR_NOTE, "stopped after %llus", TimeRunning() / 1000);
	else
		Log(Debug::ERR_NOTE, "stopped during startup after %llums", TimeSinceStartSent());

	teulSyncClock();

	m_stopped = teulClock64;
	if ( s_processStarting == this )
		s_processStarting = nullptr;
	m_startSent = 0ULL;
	m_stopSent = 0ULL;
	m_started = 0ULL;
#if platOS == OS_WIN32
	if ( m_processHandle != 0ULL ) {
		CloseHandle(m_processHandle);
		m_processHandle = 0ULL;
	}
#endif
	m_processID = 0ULL;
}

//////////////////////////////////////////////////////////////////////
// Forcefully terminate a process and assume that'll work.

void GameProcess::Kill()
{
	if ( !StartSent() )
		return;
	Log(Debug::ERR_NOTE, "kill");
#if platOS == OS_WIN32
	if (m_processHandle) {
		TerminateProcess(m_processHandle, 1);
		CloseHandle(m_processHandle);
		m_processHandle = 0ULL;
	}
#else
	kill(m_processID, SIGKILL);
#endif
	m_processID = 0ULL;
	OnStopped();
}

//////////////////////////////////////////////////////////////////////
// Work the state machine for a process.

bool GameProcess::CheckStatus()
{
	if ( g_shuttingDown && Desired() )
	{
		Log(Debug::ERR_MINOR, "terminating for shutdown.");
		RequestDiscontinue();
	}

	// Kill processes that we're trying to stop.
	if ( StopSent() )
	{
		if ( TimeSinceStopSent() > s_processShutdownTimeout )
		{
			Log(Debug::ERR_WARN, "process took to long to stop, killing.");
			Kill();
			return false;
		}
		Log("waiting on shutdown.");
	}

	if ( StartSent() )
	{
		if ( ! IsRunning() )
		{
			Log("process stopped");
			OnStopped();
			return false;
		}
	}

	if ( Running() )
	{
		ABORT_IF(!ProcessID());
		ABORT_IF(s_processStarting == this);

		if ( WantRunning() == false )
		{
			ABORT_IF( StopSent() );

			Log("want stopped.");
			RequestStop();
			return false;
		}
		// Should not be possible for desired to be false while want running is true.
		ABORT_IF(!Desired());
	}
	else // Process is NOT running.
	{
		ABORT_IF(StopSent());
		ABORT_IF(!StartSent() && ProcessID());
		if ( Desired() )
		{
			if ( WantRunning() == false )
			{
				// Flag that we should start it, and let it get picked up this heartbeat cycle.
				Log("want running.");
				RequestStart();
				return true;
			}
			// Are we in the process of starting?
			if ( StartSent() )
			{
				ABORT_IF(s_processStarting != this);
				if ( TimeSinceStartSent() >= s_processLaunchTimeout )
				{
					Log(Debug::ERR_WARN, "startup timed out after %llums. Killing.", TimeSinceStartSent());
					Kill();
					return false;
				}
				Log("waiting for startup.");
				return false;
			}
			ABORT_IF(s_processStarting == this);
			Log("waiting on startup opportunity.");
			return true;
		}
		else if ( StartSent() )
		{
			// Process is no-longer wanted but we were starting up.
			// Ask the process not to bother.
			RequestStop();
			return false;
		}
		else
		{
			Log("waiting removal (!desired)");
			return false;
		}
	}

	return false;
}

static void _getCurrentProcessPath(char (& processPath)[PATH_MAX])
{
#if platOS == OS_WIN32
	GetModuleFileNameA(NULL, processPath, sizeof(processPath) - 1);
#else
	extern const char* g_argv0;
	strncpy(processPath, g_argv0, sizeof(processPath) - 1);
#endif
	char* endCharacter = strrchr(processPath, '/');
	if ( endCharacter != nullptr )
		*(endCharacter) = 0;
	else
		strncpy(processPath, ".", sizeof(processPath) - 1);
}

static bool _spawnProcess(GameProcess* const starter_, const char* const processName_, processhandle_t& process_, UINT64& pid_, int errno_)
{
	char processPath[PATH_MAX];
	processPath[sizeof(processPath) - 1] = 0;
	_getCurrentProcessPath(processPath);
	strncat(processPath, "/", sizeof(processPath) - 1);
	strncat(processPath, processName_, sizeof(processPath) - 1);

#if platOS == OS_WIN32
	char cmdLine[] = "";
	STARTUPINFOA sinf;
	PROCESS_INFORMATION pinf;
	memset(&sinf, 0, sizeof(sinf));
	memset(&pinf, 0, sizeof(pinf));

	char pwd[PATH_MAX];
	GetCurrentDirectoryA(sizeof(pwd), pwd);
	sinf.lpTitle = const_cast<LPSTR>(starter_->ServerTypeName());
	dbugLog(_FLD, "starting %s server %s", sinf.lpTitle, processPath);
	if ( CreateProcessA(processPath, cmdLine, nullptr, nullptr, FALSE, CREATE_NEW_CONSOLE, nullptr, pwd, &sinf, &pinf) == TRUE )
	{
		CloseHandle(pinf.hThread);
		process_ = (processhandle_t)pinf.hProcess;
		pid_ = pinf.dwProcessId;
		return true;
	}
	errno_ = GetLastError();
#else
	const pid_t childPid = vfork();
	switch ( childPid )
	{
	case -1:	// Error
	break;
	case 0:		// Child
		{
			fclose(stdin);
			fclose(stdout);
			fclose(stderr);
			char* execArgs[2] = { processPath, nullptr };
			int execResult = execv(processPath, execArgs);
			dbugLog(_FLA, "failed to start %s, exec returned %d (%u)", processPath, execResult, errno);
			// we're in a child process...
			pthread_exit(nullptr);
		}
	break;
	default:	// Parent getting PID
		process_ = childPid;
		pid_ = childPid;
		return true;
	break;
	}
	errno_ = errno;
#endif
	return false;
}

#if platOS == OS_WIN32
# define EXE_SUFFIX ".exe"
#else
# define EXE_SUFFIX ""
#endif

namespace GameProcessManager
{
	void Initialize()
	{
#if platOS != OS_WIN32
		signal(SIGCHLD, SIG_IGN);
#endif
	}

	// Include this server in the launch list.
	void AddServer(SERVER_TYPE type_)
	{
		ABORT_IF(static_cast<UINT32>(type_) >= MAX_SERVER_TYPE);
		auto it = std::find_if(s_processes.cbegin(), s_processes.cend(), [type_](const GameProcess& proc_) { return proc_.ServerType() == type_; });
		if ( it == s_processes.end() )
		{
			dbugLog(_FLI, "adding %s server to process management list.\n", g_serverTypes[type_]);
			s_processes.emplace_back(type_);
		}
	}

	// Remove this server from the launch list.
	void RemoveServer(SERVER_TYPE type_)
	{
		ABORT_IF(static_cast<UINT32>(type_) >= MAX_SERVER_TYPE);
		auto it = std::find_if(s_processes.begin(), s_processes.end(), [type_](const GameProcess& proc_) { return proc_.ServerType() == type_; });
		if ( it == s_processes.end() )
			return;
		GameProcess& proc = (*it);
		proc.RequestDiscontinue();
	}

	// Find a server by its processID.
	GameProcess* LookupGameProcess(uint64_t processID_)
	{
		auto it = std::find_if(s_processes.begin(), s_processes.end(), [processID_](const GameProcess& proc_) { return proc_.ProcessID() == processID_; });
		return (it != s_processes.cend()) ? &(*it) : nullptr;
	}

	// Check up on all processes.
	bool Heartbeat()
	{
		teulSyncClock();

		// Remove all processes we no-longer need.
		s_processes.remove_if([](const GameProcess& proc_) { return (proc_.ReadyForRemoval()); });

		if ( s_processes.empty() )
			return false;

		// Remember if we thought a process was starting before we checked statuses.
		bool wasStarting = (GameProcess::s_processStarting != nullptr);
		GameProcess* starter = nullptr;
		for ( auto it = s_processes.begin(); it != s_processes.end(); )
		{
			auto& proc = (*(it++));
			if ( proc.CheckStatus() )
			{
				if ( starter == nullptr || proc.ServerType() < starter->ServerType() )
					starter = &proc;
			}
		}

		if ( !wasStarting && starter != nullptr )
		{
			// Someone wants to start. Only start one at a time.
			if ( GameProcess::s_processStarting == nullptr )
			{
				///TODO: Make the host names match their app names,
				/// e.g. right now cell host logs to wwii_host.X.log but the executable is hostd.
				static const char* processNames[MAX_SERVER_TYPE] = { nullptr, "authHost" EXE_SUFFIX, "dbd" EXE_SUFFIX, "stratd" EXE_SUFFIX, "chatd" EXE_SUFFIX, "mmhostd" EXE_SUFFIX, "hostd" EXE_SUFFIX, nullptr };

				const char* processName = processNames[starter->ServerType()];
				processhandle_t child = 0ULL;
				UINT64 childPid = 0ULL;
				int spawnErrno = 0;
				bool success = _spawnProcess(starter, processName, child, childPid, spawnErrno);
				if ( success )
				{
					dbugLog(_FLI, "%s server started %s child %llu (pid %llu)", starter->ServerTypeName(), processName, child, childPid);
					starter->OnStarting(child, childPid);
					return true;
				}
				dbugLog(_FLA, "failed to start %s: %u", processName, spawnErrno);
				starter->RequestStop();
				return false;
			}
			else
			{
				dbugLog(_FLD, "we have a starter but %s is already starting.", starter->ServerTypeName());
			}
		}
		return true;
	}

	void Shutdown()
	{
		// Ask all processes to stop.
		for ( auto it = s_processes.begin(); it != s_processes.end(); ++it )
		{
			it->RequestDiscontinue();
		}

		teulSyncClock();
		const TeulTimeMS cutoff = teulClock64 + 10 * 1000;
		while ( teulClock64 < cutoff  && Heartbeat() == true )
		{
			// Free up the CPU for a while.
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			teulSyncClock();
		}

		// Kill any processes that are still running, forcibly.
		if ( s_processes.empty() == false )
		{
			fprintf(stderr, "killing lingering processes.\n");
			for ( auto it = s_processes.begin(); it != s_processes.end(); )
			{
				auto& proc = *(it++);
				proc.Log(Debug::ERR_ABORT, "still running at shutdown, terminating.");
				proc.Kill();
			}
		}
	}
}
