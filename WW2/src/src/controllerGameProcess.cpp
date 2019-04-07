#include <syscfg.h>	// Path is project specific.
#include <list>

#if platOS == OS_WIN32
#include "WinBase.h"
#include "processthreadsapi.h"
#else
#include <sys/wait.h>
#include <spawn.h>
#endif

#include "teul/teulTime.h"
#include "src/controllerGameProcess.h"

extern bool g_shuttingDown;

static TeulTimeMS s_processLaunchTimeout = 30 * 1000;
static TeulTimeMS s_processShutdownTimeout = 10 * 1000;

// Launch singleton.
GameProcess* GameProcess::s_processStarting = nullptr;
// Timestamp of last launch.
static TeulTimeMS s_lastLaunch = 0ULL;
// Delay between process launches
static const TeulTimeMS s_launchInterval_ms = 2000;

static std::vector<GameProcess> s_processes;

static const char** s_savedEnvp = nullptr;
static bool s_singleStartMode = false;

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
    ABORT_IF_EX(m_processID > static_cast<decltype(m_processID)>(std::numeric_limits<pid_t>::max()),
        "Child process ID %" PR64BitUT " must fit in the current architecture's pid_t type - it doesn't.", m_processID);

	int status = 0;
	auto result = waitpid(static_cast<pid_t>(m_processID), &status, WNOHANG);

    if (result == 0) {
        dbugLog(_FLD, "waitpid() says process %" PR64BitUT " is fine.", m_processID);
        return true; // nothing to report
    }

    if (result == -1 && errno == ECHILD) {
        return false; // no such child or no children at all
    }

    ABORT_IF_EX(result == -1, "waitpid() returned not-ECHILD error %d", errno);

    ABORT_IF_EX(result != static_cast<pid_t>(m_processID),
        "waitpid() returned pid %d which isn't %" PR64BitUT ", kernel's broken", result, m_processID);

    if (WIFSTOPPED(status)) {
        // Stopped by a signal (most likely SIGSTOP) probably means a debugger attached to it
        dbugLog(_FLI, "Child %" PR64BitUT " stopped by signal %d, treating that as running.", m_processID, WSTOPSIG(status));
        return true;
    } else if (WIFCONTINUED(status)) {
        // Continued by SIGCONT is harmless
        return true;
    } else if (WIFSIGNALED(status)) {
        // Killed by a signal
        dbugLog(_FLI, "Child %" PR64BitUT " killed by signal %d.", m_processID, WTERMSIG(status));
        return false;
    } else if (WIFEXITED(status)) {
        // Exited normally
        dbugLog(_FLI, "Child %" PR64BitUT " exited with status %d.", m_processID, WEXITSTATUS(status));
        return false;
    }

    ABORT_IF_EX(true, "waitpid() returned status %d which isn't stopped, signaled, or exited.", status);
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
	Log(_FL, "requestStart");
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

	Log(_FL, "launched");
}

//////////////////////////////////////////////////////////////////////
// Handle the process having launched and registered itself with us.

bool GameProcess::OnStarted()
{
	if (!StartSent() || Running() || s_processStarting != this)
		return false;

	teulSyncClock();
	Log(_FLN, "started (attempt %u, took %" PR64BitUT "ms)", m_launchAttempts, (teulClock64 - m_startSent));

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
	Log(_FL, "requestStop");

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
	Log(_FL, "requestDisc");
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
		Log(_FLN, "stopped after %" PR64BitUT "s", TimeRunning() / 1000);
	else
		Log(_FLN, "stopped during startup after %" PR64BitUT "ms", TimeSinceStartSent());

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
	Log(_FLN, "kill");
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
		Log(_FLM, "terminating for shutdown.");
		RequestDiscontinue();
	}

	// Kill processes that we're trying to stop.
	if ( StopSent() )
	{
		if ( TimeSinceStopSent() > s_processShutdownTimeout )
		{
			Log(_FLW, "process took to long to stop, killing.");
			Kill();
			return false;
		}
		Log(_FL, "waiting on shutdown.");
	}

	if ( StartSent() )
	{
		if ( ! IsRunning() )
		{
			Log(_FL, "process stopped");
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

			Log(_FL, "want stopped.");
			RequestStop();
			return false;
		}
		// Should not be possible for desired to be false while want running is true.
		ABORT_IF(!Desired());
		return false;
	}

	// Process is NOT running.
	ABORT_IF(StopSent());
	ABORT_IF(!StartSent() && ProcessID());
	if ( Desired() )
	{
		if ( WantRunning() == false )
		{
			// Flag that we should start it, and let it get picked up this heartbeat cycle.
			Log(_FL, "want running.");
			RequestStart();
			return true;
		}
		// Are we in the process of starting?
		if ( StartSent() )
		{
			ABORT_IF(s_processStarting != this);
			if ( TimeSinceStartSent() >= s_processLaunchTimeout )
			{
				Log(_FLW, "startup timed out after %" PR64BitUT "ms. Killing.", TimeSinceStartSent());
				Kill();
				return false;
			}
			Log(_FL, "waiting for startup.");
			return false;
		}

		ABORT_IF_EX(s_processStarting == this, "if we're not the starting process, we'd better not be marked as such");
		Log(_FL, "waiting on startup opportunity.");
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
		Log(_FL, "awaiting removal (!desired)");
		return false;
	}
}

#if platOS != OS_WIN32
#include <dlfcn.h>
#endif

template<size_t MaxPath>
static void _getCurrentProcessPath(char (& processPath)[MaxPath])
{
	// request full path of current executable and then perform
	// a basename on it to get the cwd.
#if platOS == OS_WIN32
	GetModuleFileNameA(NULL, processPath, MaxPath - 1);
#else
    extern int main(int argc, const char* argv[], const char* envp[]);
    Dl_info info;
    if (dladdr(reinterpret_cast<const void *>(main), &info) == 0) {
        LogAbort("Can't determine current process name");
    }
    strncpy(processPath, info.dli_fname, MaxPath - 1);
#endif
	// terminate at the last path separator, otherwise it's just
	// a filename and we use '.' as the path.
	char* endCharacter = strrchr(processPath, '/');
	if ( endCharacter != nullptr )
		*(endCharacter) = 0;
	else
		strsafecpy(processPath, ".");
}

#if platOS != OS_WIN32
static void _buildCommandLineArguments(const char* processName_, std::vector<const char*>& execArgs_, std::vector<std::string>& moreArgs_)
{
	std::string envSetting { processName_ };
	envSetting += "_LAUNCHER";
	const char* launcher = getenv(envSetting.c_str());
	if ( !launcher || !*launcher )
		return;

	const char* start = launcher;
	do
	{
		const char* end = strchr(start, ' ');
		if ( end )
		{
			moreArgs_.emplace_back(start, end);
			while ( *end == ' ' )
				++end;
		}
		else
		{
			moreArgs_.emplace_back(start);
		}
		start = end;
	} while ( start && *start );
	std::vector<const char*> newArgs;
	for_each(moreArgs_.begin(), moreArgs_.end(), [&](const std::string& str){ newArgs.push_back(str.c_str()); });
	execArgs_.insert(execArgs_.begin(), newArgs.begin(), newArgs.end());
}
#endif

static bool _spawnProcess(GameProcess* const starter_, const char* const processName_, processhandle_t& process_, UINT64& pid_, int errno_)
{
	char basePath[PATH_MAX]{};
	_getCurrentProcessPath(basePath);

	std::string processPath(basePath);
	processPath += '/';
	processPath += processName_;

#if platOS == OS_WIN32
	char cmdLine[] = "";
	STARTUPINFOA sinf{};
	PROCESS_INFORMATION pinf{};

	char pwd[PATH_MAX]{};
	GetCurrentDirectoryA(sizeof(pwd), pwd);
	sinf.lpTitle = const_cast<LPSTR>(starter_->ServerTypeName());
	dbugLog(_FLD, "starting %s server %s", sinf.lpTitle, processPath.c_str());
	if ( CreateProcessA(processPath.c_str(), cmdLine, nullptr, nullptr, FALSE, CREATE_NEW_CONSOLE, nullptr, pwd, &sinf, &pinf) == TRUE )
	{
		CloseHandle(pinf.hThread);
		process_ = (processhandle_t)pinf.hProcess;
		pid_ = pinf.dwProcessId;
		return true;
	}
	errno_ = GetLastError();
	return false;
#else
	pid_t childPid{};
	posix_spawn_file_actions_t file_actions{};
	//posix_spawn_file_actions_addclose(&file_actions, STDIN_FILENO);
	//posix_spawn_file_actions_addclose(&file_actions, STDOUT_FILENO);
	//posix_spawn_file_actions_addclose(&file_actions, STDERR_FILENO);
	std::vector<const char*> execArgs { processPath.c_str(), "--child", nullptr };
	std::vector<std::string> moreArgs {};
	_buildCommandLineArguments(processName_, execArgs, moreArgs);
	dbugLog(_FLI, "launching");
	for ( auto&& arg : execArgs ) {
		if (arg)
			dbugLog(_FLI, " %s", arg);
	}
	int s = posix_spawn(&childPid, execArgs[0],
				&file_actions, nullptr,
				const_cast<char * const *>(execArgs.data()),
                const_cast<char * const *>(s_savedEnvp));
	if (s != 0) {
		errno_ = errno;
		char error[512]{};
        strerror_r(errno_, error, sizeof(error));
		dbugLog(_FLA, "failed to start %s err %d: %s", processPath.c_str(), errno_, error);
		return false;
	}

	process_ = childPid;
	pid_ = childPid;
	return true;
#endif
}

#if platOS == OS_WIN32
# define EXE_SUFFIX ".exe"
#else
# define EXE_SUFFIX ""
#endif

namespace GameProcessManager
{
	void Initialize(const char** envp, bool singleStartMode)
	{
#if platOS != OS_WIN32
		signal(SIGCHLD, SIG_IGN);
#endif
		s_savedEnvp = envp;
		s_singleStartMode = singleStartMode;
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
	bool Heartbeat(bool allowStarts)
	{
		// Remove all processes we no-longer need.
        s_processes.erase(
            std::remove_if(s_processes.begin(), s_processes.end(), [](const GameProcess& proc_) { return (proc_.ReadyForRemoval()); }),
            s_processes.end());

		if ( s_processes.empty() )
			return false;

		// Remember if we thought a process was starting before we checked statuses.
		static UINT32 launchCount[MAX_SERVER_TYPE];
		bool wasStarting = (GameProcess::s_processStarting != nullptr);
		GameProcess* starter = nullptr;
		for ( auto it = s_processes.begin(); it != s_processes.end(); )
		{
			auto& proc = (*(it++));
			if ( !proc.CheckStatus() || !allowStarts )
				continue;
			if ( s_singleStartMode && launchCount[proc.ServerType()] > 0 )
				continue;
			if ( starter == nullptr || proc.ServerType() < starter->ServerType() )
				starter = &proc;
		}

		if ( !wasStarting && starter != nullptr )
		{
			// Someone wants to start. Only start one at a time.
			if ( GameProcess::s_processStarting == nullptr )
			{
				if ( s_lastLaunch + s_launchInterval_ms > teulClock64 ) {
					dbugLog(_FLD, "waiting for next launch window");
					return true;
				}
				s_lastLaunch = teulClock64;
				///TODO: Make the host names match their app names,
				/// e.g. right now cell host logs to wwii_host.X.log but the executable is hostd.
				static const char* processNames[MAX_SERVER_TYPE] = { nullptr, "authHost" EXE_SUFFIX, "dbd" EXE_SUFFIX, "stratd" EXE_SUFFIX, "chatd" EXE_SUFFIX, "mmhostd" EXE_SUFFIX, "hostd" EXE_SUFFIX, nullptr };

				launchCount[starter->ServerType()]++;

				const char* processName = processNames[starter->ServerType()];
				processhandle_t child = 0ULL;
				UINT64 childPid = 0ULL;
				int spawnErrno = 0;
				bool success = _spawnProcess(starter, processName, child, childPid, spawnErrno);
				if ( success )
				{
					dbugLog(_FLI, "%s server started %s #%u child %" PR64BitUT " (pid %" PR64BitUT ")",
							starter->ServerTypeName(), processName, launchCount[starter->ServerType()], child, childPid);
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
		for ( auto& proc : s_processes )
			proc.RequestDiscontinue();

		teulSyncClock();
		const TeulTimeMS cutoff = teulClock64 + s_processShutdownTimeout;
		while ( teulClock64 < cutoff && Heartbeat(false) == true )
		{
			// Free up the CPU for a while.
			std::this_thread::sleep_for(std::chrono::milliseconds(200));
			teulSyncClock();
		}

		// Kill any processes that are still running, forcibly.
		if ( s_processes.empty() == false )
		{
			fprintf(stderr, "killing lingering processes.\n");
			for ( auto it = s_processes.begin(); it != s_processes.end(); )
			{
				auto& proc = *(it++);
				proc.Log(_FLA, "still running at shutdown, terminating.");
				proc.Kill();
			}
		}
	}
}
