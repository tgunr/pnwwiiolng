#pragma once

#include "src/controllerGameIntro.h"

#if platOS == OS_WIN32
using processhandle_t = HANDLE;
#else
using processhandle_t = UINT64;
#endif

//! Structure that encapsulates the operation of a single game server process.
struct GameProcess
{
	//! Default constructor.
	GameProcess(SERVER_TYPE type_);

	//! If we reach the destructor untidily, kill the process.
	~GameProcess() { Kill(); }

	//! We're ready to start this process.
	void RequestStart();
	//! The OS has been asked to launch this process.
	void OnStarting(processhandle_t processHandle_, UINT64 processID_);
	//! We have received contact from the process, it's running.
	//! Returns: true if we expected this, false otherwise.
	bool OnStarted();
	//! We have decided this process should be terminated.
	void RequestStop();
	//! If the process is still running, send it a kill and mark it as stopped.
	void Kill();
	//! We have been told not to run this process.
	void RequestDiscontinue();
	//! The process has stopped running.
	void OnStopped();

	//! Check the state of the object.
	//! @return true if the process is waiting to be started.
	bool CheckStatus();

	bool IsRunning() const;

	SERVER_TYPE ServerType() const { return m_type; }
	const char* ServerTypeName() const { return g_serverTypes[m_type]; }
	bool Desired() const { return m_desired; }
	bool WantRunning() const { return m_wantRunning; }
	bool StartSent() const { return (m_startSent != 0); }
	bool Running() const { return (m_started != 0); }
	bool StopSent() const { return (m_stopSent != 0); }
	bool Stopped() const { return (m_stopped != 0); }
	UINT32 GetLaunchAttempts() const { return m_launchAttempts; }

	//! Determine if the process no-longer needs to be managed.
	bool ReadyForRemoval() const { return !Running() && !StartSent() && !Desired(); }

	UINT64 ProcessID() const { return m_processID; }

	TeulTimeMS TimeRunning() const { return Running() ? (teulClock64 - m_started) : ~0ULL; }
	TeulTimeMS TimeSinceStartSent() const { return StartSent() ? (teulClock64 - m_startSent) : ~0ULL; }
	TeulTimeMS TimeSinceStopSent() const { return StopSent() ? (teulClock64 - m_stopSent) : ~0ULL; }
	TeulTimeMS TimeSinceStopped() const { return Stopped() ? (teulClock64 - m_stopped) : ~0ULL; }

private:
	//! Which server type we're expecting.
	SERVER_TYPE		m_type;
	//! Whether or not we are actively maintaining this process.
	bool			m_desired;
	//! Whether we are ready to have this process running or not.
	bool			m_wantRunning;
	//! When we started trying to run it.
	TeulTimeMS		m_startSent;
	//! When we got confirmation the process was running.
	TeulTimeMS		m_started;
#if platOS == OS_WIN32
	processhandle_t	m_processHandle;
#endif
	//! Process ID (linux) or Process Handle (Windows)
	UINT64			m_processID;
	//! When we asked the process to stop.
	TeulTimeMS		m_stopSent;
	//! When we got confirmation the process had stopped.
	TeulTimeMS		m_stopped;
	//! How many times have we attempted this launch?
	UINT32			m_launchAttempts;

public:
	// We only start one process at a time.
	static GameProcess* s_processStarting;

public:
	void Log(int level, const char* text)
	{
		dbugLog("GameProcess", 0, level | LOG_ALL_TO_STDERR, "s:%u(%s) pid:%" PR64BitUT " %s", ServerType(), ServerTypeName(), ProcessID(), text);
	}
	template<typename... Args>
	void Log(const char* const filename, const SINT32 lineno, Debug::ERROR_TYPE level, const char* fmt, Args&&... args)
	{
		char myfmt[1024];
		strprintf(myfmt, "s:%%u(%%s) pid:%%" PR64BitUT " %s", fmt);
		dbugLog(filename, lineno, level, myfmt, ServerType(), ServerTypeName(), ProcessID(), std::forward<Args>(args)...);
	}

	void Log(const char* const filename, const SINT32 lineno, const char* msg)
	{
		dbugLog(filename, lineno, Debug::ERR_DBUG, "s:%u(%s) pid:%" PR64BitUT " %s", ServerType(), ServerTypeName(), ProcessID(), msg);
	}
};

//! Game server manager API.
namespace GameProcessManager
{
	//! Tear up.
	void Initialize(const char** envp, bool singleStartMode);

	//! Ask the server manager to include a new server type.
	//! @note Only one instance of each server per manager.
	void AddServer(SERVER_TYPE type_);

	//! Ask the server manager to shutdown a particular server type.
	void RemoveServer(SERVER_TYPE type_);

	//! Find a process by it's process ID.
	GameProcess* LookupGameProcess(uint64_t processID_);

	//! Ask the server manager to do it's thing.
	//! @return false when there are no processes to manage.
	bool Heartbeat(bool allowStarts);

	//! Shutdown.
	void Shutdown();
}

