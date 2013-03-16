#pragma once
// Created: Wed Jul 27 10:53:46 CDT 2005
//
// Wrapper class that conceals the housekeeping concept
// of calling a particular function every X ms.
//
// Usages:
//
//  Teul::housekeeper myKeeper(void* fn(void), stock_ms_interval);
//    Declares a housekeeper for which fn() will be invoked
//    every time stock_ms_interval miliseconds elapses between
//    calls to myKeeper.poll();
//
//  myKeeper.poll();
//    calls myKeeper.poll(stock_ms_interval);
//
//  myKeeper.poll(ms_interval);
//    if ms_interval is zero, or ms_interval miliseconds
//    have elapsed since time we invokved fn() then
//    fn() is called and true returned. Otherwise
//    false is returned.
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

namespace Teul
{
class housekeeper
{
public:
	housekeeper(void (*fn)(void), UINT32 stockInterval_ms)
		: m_lastClock(teulClock64)
		, m_fn(fn)
		, m_stockInterval_ms(stockInterval_ms)
		{}

	bool poll(UINT32 interval_ms)
	{
		if ( interval_ms > 0 && teulClock64 - m_lastClock < (TeulTimeMS)interval_ms )
			return false ;
		m_fn() ;
		m_lastClock = teulClock64 ;
		return true ;
	}

	bool poll() { return poll(m_stockInterval_ms) ; }

private:
	TeulTimeMS		m_lastClock ;
	void			(*m_fn)(void) ;
	UINT32			m_stockInterval_ms ;
} ; // class
} // namespace
