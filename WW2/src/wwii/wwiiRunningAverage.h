#pragma once
// Date: Mon Mar 27 15:18:33 CST 2006
//
// Template class for efficiently maintaining a running
// average with a static histogram that is used cyclicly
// to avoid memoves/memcpys etc.
//
// Interface:
//	  RunningAverage<dataType, historySize> myAverage;
//	  dataType currentAvg = myAverage.add(newValue);
//	  myAverage.reset();
//
//  Additional functions:
//		myAverage.average() returns current average
//		myAverage.total() returns total of current histogram
//		myAverage.population() returns number of entries in the histogram
//
// Copyright (C) PlayNet, Inc 1999-2006 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include <assert.h>             // For assert()
#include <algorithm>            // For std::min()
#include <string.h>             // For memset()

template<typename _Tp, unsigned int _MAX_ENTRIES>
class RunningAverage
{
public: // *tors
    RunningAverage() { assert(_MAX_ENTRIES > 0) ; reset() ; }
    ~RunningAverage() {}

public:     /// Operations
	// Add a value to the histogram and update the average
    const _Tp& add(const _Tp& newValue)
    {
        // Swap out the old for the new
        m_total -= m_histogram[m_nextEntry] ;
        m_histogram[m_nextEntry] = newValue ;
        m_total += newValue ;

        // Increment the entry reference
        m_nextEntry = (m_nextEntry + 1) % _MAX_ENTRIES ;
        if ( m_population < _MAX_ENTRIES )
            ++m_population ;

        // Update the average
        m_avg = m_total / m_population ;

        return m_avg ;
    }

    // Restart everything
    void reset()
    {
        // Brute force approach
        ///CAVEAT: may make template unsuitable for some datatypes
        memset(this, 0, sizeof(*this)) ;
    }

public:     /// Accessors
    unsigned int population() const { return m_population ; }
    const _Tp& average() const { return m_avg ; }
    const _Tp& total() const { return m_total ; }
	unsigned int size() const { return _MAX_ENTRIES ; }

private:    /// Members
    _Tp             m_histogram[_MAX_ENTRIES] ;
    _Tp             m_total ;       // Current total
    _Tp             m_avg ;         // Running average
    unsigned int    m_nextEntry ;   // Which slot is about to be used
    unsigned int    m_population ;  // Number of slots used
} ;

