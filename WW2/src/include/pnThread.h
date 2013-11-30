#pragma once

#include <thread>

class PNThread
{
	std::thread	m_thread;
	bool m_started;
	bool m_cancelled;

public:
	PNThread() : m_thread(), m_started(false), m_cancelled(false) {}
	virtual ~PNThread() {}

	void start() { if (m_started) return; m_thread = std::thread([=] { run(); }); m_started = true; }
	virtual int cancel() { m_cancelled = true; return 0; }
	void join() { m_thread.join(); }

	bool isStarted() const { return m_started; }
	bool isRunning() const { return m_thread.joinable(); }
	bool isCancelled() const { return m_cancelled; }

	virtual void run() = 0;
};
