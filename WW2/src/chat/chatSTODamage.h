#pragma once
// Created: Fri Aug  6 17:18:11 CDT 2010
//
// Describes damage status of an STO.
//
// Copyright (C) PlayNet, Inc 1999-2010 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

class STODamage
{
public:
	//! Constructor.
	//! @param[in]	threshold_		Joules required to do any damage.
	//! @param[in]	capacity_		Total joules required to destroy object.
	STODamage(const UINT64 threshold_, const SINT64 capacity_)
		: m_threshold(threshold_), m_capacity(capacity_)
		, m_accumulated(0ULL)
		{}

	//! Construct from a subtype.
	//! @param[in]	subType			Populate from this subtype's config.
	explicit STODamage(UINT16) ;

	//! Apply damage or repairs.
	//! @param[in] joules		+ve joules of damage or -ve joules of repair.
	//! @return true if joules were applied, false otherwise.
	bool applyJoules(const SINT64& joules)
	{
		if ( m_capacity == 0 )
			return false ;

		if ( joules < 0 )
			return _repair((UINT64)(-joules)) ;
		else if ( joules > 0 )
			return _damage((UINT64)joules) ;
		return false ;
	}

	//! Test if the object is destroyed.
	//! @return true if the object has a non-zero capacity and
	//!         accumulated damage has exceeded it.
	bool isDestroyed() const
	{
		return ( m_capacity > 0 && m_accumulated >= m_capacity ) ;
	}

	//! Test if the object is partially damaged.
	//! @return true if the object has any damage, false otherwise.
	bool isDamaged() const
	{
		return ( m_accumulated > 0 ) ;
	}

private:
	//! @internal Apply repair joules.
	//! @return true if repairs were done, false otherwise.
	bool _repair(const UINT64& joules)
	{
		if ( !isDamaged() )
			return false ;

		if ( joules > m_accumulated )
			m_accumulated = 0 ;
		else
			m_accumulated -= joules ;

		return true ;
	}

	//! @internal Apply damage joules.
	//! @return true if repairs were done, false otherwise.
	bool _damage(const UINT64& joules)
	{
		if ( joules < m_threshold )
			return false ;
		// Avoid overflow.
		if ( joules >= m_capacity || m_capacity - joules < m_accumulated )
		{
			m_accumulated = m_capacity ;
			return true ;
		}

		m_accumulated += joules ;

		return true ;
	}

public:
	const UINT64& threshold() const { return m_threshold ; }
	const UINT64& capacity() const { return m_capacity ; }
	const UINT64& accumulated() const { return m_accumulated ; }

private:
	UINT64		m_threshold ;			//!< Damage requird to hurt me.
	UINT64		m_capacity ;			//!< Total damage required to destroy me.
	UINT64		m_accumulated ;			//!< Damage received.
} ;
