#pragma once

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
	void OnStarting(UINT64 processID_, void* process_);
	//! We have received contact from the process, it's running.
	void OnStarted(UINT64 processID_);
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

	UINT64 ProcessID() const { return m_pid; }
#if platOS == OS_WIN32
	HANDLE ProcessHandle() const { return static_cast<HANDLE>(m_process); }
#endif

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
	//! ProcessID of the process.
	UINT64			m_pid;
	//! For Windows - equivalent of a HANDLE.
	void*			m_process;
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
	void Log(int level, const char* fmt, ...)
	{
		dbugLog("GameProcess", 0, level | LOG_ALL_TO_STDERR, "s:%u(%s) pid:%llu ", ServerType(), ServerTypeName(), ProcessID());
		va_list args;
		va_start(args, fmt);
		dbugLogV("GameProcess", 0, level | LOG_ALL_TO_STDERR | BARE_DEBUG, fmt, args);
		va_end(args);
	}

	void Log(const char* msg)
	{
		dbugLog("GameProcess", 0, Debug::ERR_DBUG | LOG_ALL_TO_STDERR, "s:%u(%s) pid:%llu %s", ServerType(), ServerTypeName(), ProcessID(), msg);
	}
};

//! Game server manager API.
namespace GameProcessManager
{
	//! Ask the server manager to include a new server type.
	//! @note Only one instance of each server per manager.
	void AddServer(SERVER_TYPE type_);

	//! Ask the server manager to shutdown a particular server type.
	void RemoveServer(SERVER_TYPE type_);

	//! Ask the server manager to do it's thing.
	//! @return false when there are no processes to manage.
	bool Heartbeat();

	//! Shutdown.
	void Shutdown();

#pragma pack(push, 1)
	//! @struct GameProcessManager::Introduction
	//! @brief Identification message from a managed process.
	//! @detail Tells the controller who is attempting a connection.
	struct Introduction
	{
		UINT64	m_processID;
		UINT32	m_processType;
		UINT32	m_token;
	};
#pragma pack(pop)
}
