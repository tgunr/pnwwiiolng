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
#include <OpenThreads/Condition>

// Wrapper around the simple mutex object class; lock/unlock
// is the responsibility of the caller. See Lock class.
class Mutex
{
public:
	OpenThreads::Mutex m_mutex ;

	Mutex() : m_mutex() {}

	int lock() { return m_mutex.lock() ; }
	int tryLock() { return m_mutex.trylock() ; }
	int unlock() { return m_mutex.unlock() ; }

	virtual ~Mutex() {}
} ;

// Class which encapsulates the relationship between
// a condition variable and its mutex.
// Usage:
//  std::vector<int> threadSharedQueue ;
//  ...
//  consumer()
//  {
//    condition.lock() ;
//    while ( threadSharedQueue.empty() )
//    {
//      condition.wait() ;
//    }
//  }
//
//  producer(int value)
//  {
//    condition.lock() ;
//    threadSharedQueue.push_back(value) ;
//    condition.unlock() ;
//    condition.signal() ;
//  }

class Condition : public Mutex
{
public:
	OpenThreads::Condition m_cond ;

	Condition() : Mutex(), m_cond() {}

	int wait() { return m_cond.wait(&m_mutex) ; }
	int signal() { return m_cond.signal() ; }

	~Condition() {}
} ;

// Provides the Lock concept for the Condition class.
class ConditionLock
{
public:
	ConditionLock(Condition& cond, bool signal=false) : m_cond(cond), m_signal(signal) { m_cond.lock() ; }
	~ConditionLock()
	{
		m_cond.unlock() ;
		if ( m_signal ) m_cond.signal() ;
	}

	Condition& m_cond ;
	bool m_signal ;
} ;

// Implements a shared queue around an STL container class.
// For a non-blocking fetch, use tryGetNext().
// For a condition-variable based, blocking fetch, use getNext()
template<typename _DataType>
class ConditionQueue : public Condition
{
public:
	typedef std::queue<_DataType> QUEUE ;

public:
	ConditionQueue() : Condition(), m_hits(0LL), m_maxSize(0) {}

	QUEUE m_queue ;
	unsigned long long m_hits ;
	size_t m_maxSize ;

	// Non-blocking lock attempt.
	bool tryGetNext(_DataType& data)
	{
		if ( tryLock() != 0 )
			return false ;

		if ( m_queue.empty() )
		{
			unlock() ;
			return false ;
		}

		data = m_queue.front() ;
		m_queue.pop() ;
		unlock() ;

		return true ;
	}

	// Blocking lock attempt
	_DataType getNext()
	{
		// Wait until empty is false
		lock() ;
		while ( m_queue.empty() )
		{
			wait() ;
		}

		_DataType data = m_queue.front() ;
		m_queue.pop() ;

		unlock() ;

		return data ;
	}

	void push_back(const _DataType& data)
	{
		lock() ;
		m_queue.push(data) ;
		++m_hits ;
		if ( m_queue.size() > m_maxSize )
			m_maxSize = m_queue.size() ;
		unlock() ;
		signal() ;
	}
} ;

