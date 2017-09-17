#pragma once
// Created: Mon Dec 12 16:12:44 CST 2005
//
// Private types used by the threading system
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include <queue>
#include <condition_variable>
#include <atomic>

// Implements a shared queue around an STL container class.
// For a non-blocking fetch, use tryGetNext().
// For a condition-variable based, blocking fetch, use getNext()
template<typename _DataType>
class ConditionQueue
{
public:
	using QUEUE = std::queue<_DataType*>;

private:
	std::mutex				m_mutex;
	std::condition_variable	m_condition;
	std::atomic<bool>		m_destructing { false };
	QUEUE m_queue;
	unsigned long long m_hits { 0 };
	size_t m_maxSize { 0 };

public:
	~ConditionQueue() noexcept
	{
		destroy();
		std::lock_guard<std::mutex> lock(m_mutex);
		while (!m_queue.empty()) {
			auto p = m_queue.front();
			m_queue.pop();
			delete p;
		}
	}

	void destroy() noexcept
	{
		if ( !m_destructing )
			m_destructing = true;
		m_condition.notify_all();
	}

	void doLocked(std::function<void()> lambda)
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		lambda();
	}

	bool isValid() const noexcept { return !m_destructing; }

	// Non-blocking lock attempt.
	bool tryGetNext(_DataType** data) noexcept
	{
		std::unique_lock<std::mutex> lock(m_mutex, std::try_to_lock);
		if ( m_destructing || lock.owns_lock() == false )
			return false;

		if ( m_queue.empty() )
			return false;

		*data = m_queue.front();
		m_queue.pop();

		return true;
	}

    // Blocking lock attempt with timeout
    template <class Rep, class Period>
    bool getNextWithin(const std::chrono::duration<Rep, Period> &duration, _DataType **data) noexcept
    {
        // Wait until empty is false
        std::unique_lock<std::mutex> lock(m_mutex);
        if (!m_condition.wait_for(lock, duration, [=] () {
            return m_destructing || !m_queue.empty();
        })) {
            *data = nullptr;
            return false;
        }
        if ( m_destructing ) {
            destroy();
            *data = nullptr;
        } else {
            *data = m_queue.front();
            m_queue.pop();
        }
        return true;
    }

	// Blocking lock attempt
	_DataType* getNext() noexcept
	{
		// Wait until empty is false
		std::unique_lock<std::mutex> lock(m_mutex);
		m_condition.wait(lock, [=] () {
			return m_destructing || !m_queue.empty();
		});
		if ( m_destructing ) {
			destroy();
			return nullptr;
		}

		_DataType* data = m_queue.front();
		m_queue.pop();

		return data;
	}

	void push_back(_DataType* data)
	{
		if ( PN_UNLIKELY(m_destructing) )
			return;
		std::lock_guard<std::mutex> lock(m_mutex);
		m_queue.push(data);
		++m_hits;
		if ( m_queue.size() > m_maxSize )
			m_maxSize = m_queue.size();
		m_condition.notify_one();
	}

	auto size_locked() noexcept  // lock first!
	{
		return m_queue.size();
	}

	auto maxSize_locked() noexcept  // lock first!
	{
		return m_maxSize;
	}

	auto hits_locked() noexcept  // lock first!
	{
		return m_hits;
	}
};
