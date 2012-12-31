#pragma once

#include <deque>
#include <unordered_set>

struct RoutingPropagator
{
	RoutingPropagator()
		: m_openSet()
		, m_closedSet()
		, m_currentRun(teulClock64)
	{
	}

public:
	void Run(const CPSET& suppliers)
	{
		for ( auto it = suppliers.begin() ; it != suppliers.end() ; ++it )
		{
			CPStruct* cp = *it ;
			if ( cp->m_routeingRun != m_currentRun && cp->side == cp->originalSide )
			{
				m_openSet.push_back(cp) ;

				m_side = cp->originalSide ;
				cp->m_routeingRun = m_currentRun ;

				m_closedSet.clear() ;

				while ( m_openSet.empty() == false )
				{
					cp = m_openSet.back() ;
					m_openSet.pop_back() ;
					Propagate(cp) ;
				}
			}
		}
	}

	void Apply(CPStruct* const cpList) const
	{
		for ( auto cp = cpList ; cp != NULL ; cp = cp->next )
		{
			if ( cp->isCity() == false || cp->side == NEUTRAL )
				continue ;
			bool isLinked = (cp->m_routeingRun == m_currentRun ) ;
			if ( isLinked != cp->m_isRouteingConnected )
				cp->setRouteing(isLinked) ;
		}
	}

private:
	bool IsOpenConnection(CPStruct* const cp) const
	{
		if ( cp->m_routeingRun >= m_currentRun )
			return false ;
		if ( cp->side == m_side && cp->side == cp->controllingSide )
		{
			cp->m_routeingRun = m_currentRun ;
			if ( cp->m_isRouteingConnected == false )
				cp->setRouteing(true) ;
			return true ;
		}
		if ( cp->country == WWII_CHINA )
		{
			return ( m_closedSet.insert(cp).second ) ;
		}
		return false ;
	}

private:
	// Check if this CP is an open connection, and if so,
	// put it's neighbors onto the
	// if not, check if it is in a visitable state.
	void Propagate(CPStruct* const cp)
	{
		for ( auto it = cp->m_links.begin() ; it != cp->m_links.end() ; ++it )
		{
			CPStruct* const neighbor = (*it)->endDepot->cp ;
			if ( IsOpenConnection(neighbor) )
			{
				m_openSet.push_back(neighbor) ;
			}
		}
	}

private:
	// List of CPs waiting to be visited.
	typedef std::deque<CPStruct*> OPEN_SET ;
	// List of CPs that have been visited.
	typedef std::unordered_set<CPStruct*> CLOSED_SET ;

	WWII_SIDE	m_side ;
	OPEN_SET	m_openSet ;
	mutable CLOSED_SET	m_closedSet ;
	TeulTimeMS	m_currentRun ;
} ;

