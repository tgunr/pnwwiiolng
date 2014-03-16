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
	std::mutex				m_mutex;
	std::condition_variable	m_condition;
	std::atomic<bool>		m_destructing;

public:
	typedef std::queue<_DataType*> QUEUE;

public:
	ConditionQueue() : m_mutex(), m_condition(), m_destructing(false), m_queue(), m_hits(0LL), m_maxSize(0) {}
	~ConditionQueue()
	{
		destroy();
		std::this_thread::yield();
		m_mutex.lock();
		m_mutex.unlock();
	}

	QUEUE m_queue;
	unsigned long long m_hits;
	size_t m_maxSize;

	void destroy()
	{
		if ( !m_destructing ) {
			m_destructing = true;
			m_mutex.lock();
			m_condition.notify_all();
			m_mutex.unlock();
		}
	}

	void doLocked(std::function<void(void)> lambda)
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		lambda();
	}

	bool isValid() const { return !m_destructing; }

	// Non-blocking lock attempt.
	bool tryGetNext(_DataType** data)
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

	// Blocking lock attempt
	_DataType* getNext()
	{
		// Wait until empty is false
		std::unique_lock<std::mutex> lock(m_mutex);
		m_condition.wait(lock, [=] () {
			return m_destructing || !m_queue.empty();
		});
		if ( m_destructing )
			return nullptr;

		_DataType* data = m_queue.front();
		m_queue.pop();

		return data;
	}

	void push_back(_DataType* data)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_queue.push(data);
		++m_hits;
		if ( m_queue.size() > m_maxSize )
			m_maxSize = m_queue.size();
		m_condition.notify_one();
	}
};
