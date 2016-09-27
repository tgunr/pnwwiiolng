#include "pnThread.h"

void PNThread::start() {

	if (!m_cancelled && !m_started.exchange(true))
	{
		m_thread = std::thread(&PNThread::run, this);
	}
}

bool PNThread::cancel() {
	bool wasCancelledBefore = m_cancelled.exchange(true);
	if (!wasCancelledBefore)
		onCancel();
	return !wasCancelledBefore;
}

void PNThread::join() {
	if (isRunning()) {
		m_thread.join();
	}

}
