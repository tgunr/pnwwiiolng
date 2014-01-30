#pragma once

#include <thread>
#include <atomic>

class PNThread
{
	std::thread	m_thread;
	std::atomic<bool> m_started;
	std::atomic<bool> m_cancelled;

public:
	PNThread() : m_thread(), m_started(false), m_cancelled(false) {}
	virtual ~PNThread() {}

	void start()
	{
		if (!m_cancelled && !m_started.exchange(true))
			m_thread = std::thread([=] { run(); });
	}
	//! Cancel thread and return whether or not we performed cancellation.
	//! @return true if we changed the cancellation state.
	bool cancel()
	{
		bool wasCancelledBefore = m_cancelled.exchange(true);
		if (!wasCancelledBefore)
			onCancel();
		return !wasCancelledBefore;
	}

	//! behavior for handling with cancellation.
	virtual void onCancel()
	{
	}

	void join()
	{
		if (isRunning())
			m_thread.join();
	}

	bool isStarted() const { return m_started; }
	bool isRunning() const { return m_thread.joinable(); }
	bool isCancelled() const { return m_cancelled; }

	virtual void run() = 0;
};
