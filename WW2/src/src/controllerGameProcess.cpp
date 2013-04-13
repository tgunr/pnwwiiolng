#include <syscfg.h>	// Path is project specific.
#include <list>

#if platOS == OS_WIN32
#include "WinBase.h"
#include "processthreadsapi.h"
#endif

#include "dbug/dbug.h"
#include "teul/teulTime.h"
#include "src/controllerGameProcess.h"

extern bool g_shuttingDown;

static TeulTimeMS s_processLaunchTimeout = 30 * 1000;
static TeulTimeMS s_processShutdownTimeout = 5 * 1000;

// Launch singleton.
GameProcess* GameProcess::s_processStarting = nullptr;

static std::list<GameProcess> s_processes;

GameProcess::GameProcess(SERVER_TYPE type_)
	: m_type(type_)
	, m_desired(true)
	, m_wantRunning(false)
	, m_startSent(0ULL)
	, m_started(0ULL)
	, m_pid(0ULL)
	, m_process(0ULL)
	, m_stopSent(0ULL)
	, m_stopped(0ULL)
{
}

void GameProcess::RequestStart()
{
	if ( WantRunning() )
		return;
	m_wantRunning = true;
	m_stopped = 0ULL;
	Log("requestStart");
}

void GameProcess::OnStarting(UINT64 processID_, void* process_)
{
	ABORT_IF( StartSent() );
	ABORT_IF( Running() );
	ABORT_IF( ProcessID() );
	ABORT_IF( s_processStarting != nullptr );

	teulSyncClock();
	m_startSent = teulClock64;
	m_stopped = 0ULL;
	m_pid = processID_;
	m_process = process_;
	s_processStarting = this;
	++m_launchAttempts;

	Log("onStarting");
}

void GameProcess::OnStarted(UINT64 processID_)
{
	ABORT_IF( !StartSent() );
	ABORT_IF( Running() );
	ABORT_IF( ProcessID() != processID_ );
	ABORT_IF( s_processStarting != this );

	teulSyncClock();
	Log(Debug::ERR_NOTE, "started (attempt %u, took %llums)", m_launchAttempts, (teulClock64 - m_startSent));

	m_started = teulClock64;
	m_stopped = 0ULL;
	s_processStarting = nullptr;
	m_launchAttempts = 0;
}

void GameProcess::RequestStop()
{
	if ( m_wantRunning == false )
		return;
	m_wantRunning = false;
	Log("requestStop");
}

void GameProcess::RequestDiscontinue()
{
	// Ignore duplicate discontinue requests.
	if ( m_desired == false )
		return;
	m_desired = false;
	Log("requestDisc");
	RequestStop();
}

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
	m_pid = 0ULL;
#if platOS == OS_WIN32
	if ( m_process != 0ULL )
		CloseHandle(m_process);
#endif
	m_process = 0ULL;
}

void GameProcess::Kill()
{
	if ( !StartSent() )
		return;
	Log(Debug::ERR_NOTE, "kill");
#if platOS == OS_WIN32
	TerminateProcess(m_process, 1);
	CloseHandle(m_process);
	m_process = 0ULL;
#else
	kill(m_process, SIGKILL);
#endif
	OnStopped();
}

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
#if platOS == OS_WIN32
		// Check if the process is still running.
		auto result = WaitForSingleObject(m_process, 0);
		if ( result == WAIT_OBJECT_0 || result == WAIT_FAILED )
		{
			Log("process stopped");
			OnStopped();
			return false;
		}
#endif
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

#if platOS == OS_WIN32
# define EXE_SUFFIX ".exe"
#else
# define EXE_SUFFIX ""
#endif

namespace GameProcessManager
{
	// Include this server in the launch list.
	void AddServer(SERVER_TYPE type_)
	{
		ABORT_IF(static_cast<UINT32>(type_) >= MAX_SERVER_TYPE);
		auto it = std::find_if(s_processes.begin(), s_processes.end(), [=](const GameProcess& proc_) { return proc_.ServerType() == type_; });
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
		auto it = std::find_if(s_processes.begin(), s_processes.end(), [=](const GameProcess& proc_) { return proc_.ServerType() == type_; });
		if ( it == s_processes.end() )
			return;
		GameProcess& proc = (*it);
		proc.RequestDiscontinue();
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
		for ( auto it = s_processes.begin() ; it != s_processes.end() ; )
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
				static const char* processNames[MAX_SERVER_TYPE] = { nullptr, "authHost" EXE_SUFFIX, "dbd" EXE_SUFFIX, "stratd" EXE_SUFFIX, "chatd" EXE_SUFFIX, "mmhostd" EXE_SUFFIX, "pingHost" EXE_SUFFIX, "hostd" EXE_SUFFIX, nullptr };

				const char* processName = processNames[starter->ServerType()];
				char cmdLine[] = "";

#if platOS == OS_WIN32
				STARTUPINFOA sinf;
				PROCESS_INFORMATION pinf;
				memset(&sinf, 0, sizeof(sinf));
				memset(&pinf, 0, sizeof(pinf));

				bool success = false;
				char processPath[MAX_PATH];
				// Get the current executable path + name, remove the name and add the target executable path.
				GetModuleFileNameA(NULL, processPath, sizeof(processPath));
				char* endCharacter = strrchr(processPath, '\\');
				if ( endCharacter != NULL )
					*(endCharacter + 1) = 0;
				else
					strcpy_s(processPath, ".\\");
				if ( strcat_s(processPath, processName) == 0 )
				{
					char pwd[MAX_PATH];
					GetCurrentDirectoryA(sizeof(pwd), pwd);
					sinf.lpTitle = const_cast<LPSTR>(starter->ServerTypeName());
					dbugLog(_FLD, "starting %s server %s", sinf.lpTitle, processPath);
					if ( CreateProcessA(processPath, cmdLine, nullptr, nullptr, FALSE, CREATE_NEW_CONSOLE, nullptr, pwd, &sinf, &pinf) == TRUE )
					{
						dbugLog(_FLI, "%s server started %s pid %u", starter->ServerTypeName(), processPath, pinf.dwProcessId);
						CloseHandle(pinf.hThread);
						starter->OnStarting(static_cast<UINT64>(pinf.dwProcessId), pinf.hProcess);
					}
					else
					{
						dbugLog(_FLA, "failed to start %s: %u", processNames[starter->ServerType()], GetLastError());
						starter->RequestStop();
					}
				}
				else
				{
					dbugLog(_FLA, "unable to start %s: can't populate pathname", processNames[starter->ServerType()]);
					starter->RequestStop();
				}
#else
# error "Not implemented yet."
#endif
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
		for ( auto it = s_processes.begin() ; it != s_processes.end() ; ++it )
		{
			auto& proc = *(it++);
			proc.RequestDiscontinue();
		}

		teulSyncClock();
		const TeulTimeMS cutoff = teulClock64 + 10 * 1000;
		while ( teulClock64 < cutoff  && Heartbeat() == true )
		{
			// Free up the CPU for a while.
			OpenThreads::Thread::microSleep(100 * 1000);
			teulSyncClock();
		}

		// Kill any processes that are still running, forcibly.
		if ( s_processes.empty() == false )
		{
			fprintf(stderr, "killing lingering processes.\n");
			for ( auto it = s_processes.begin() ; it != s_processes.end() ; )
			{
				auto& proc = *(it++);
				proc.Log(Debug::ERR_ABORT, "still running at shutdown, terminating.");
				proc.Kill();
			}
		}
	}
}
