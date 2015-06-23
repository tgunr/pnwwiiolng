#pragma once
// Created: Thu Mar 16 13:31:56 CST 2006
//
// Class that defines a database-retained incremental resource
// counter like serial numbers or mission ids
//
// Copyright (C) PlayNet, Inc 1999-2006 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

class SerialNumber
{
public:
	typedef unsigned long long serialno_t;

public:
	SerialNumber(const char* const name_, const serialno_t minimum_, const serialno_t interval_, const serialno_t maximum_)
		: m_counter(0), m_name(name_), m_minimum(minimum_), m_interval(interval_), m_maximum(maximum_) {}

public:
	serialno_t getNext();
	serialno_t operator ++ () { return getNext(); }

private:
	serialno_t	m_counter;				// The current value or zero if uninitialized
	const char*	m_name;				// Resource name in the database table
	serialno_t	m_minimum;				// Initial value
	serialno_t	m_interval;			// How often to save
	serialno_t	m_maximum;				// Roll-over value
};

