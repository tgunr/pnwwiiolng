#include <syscfg.h> // Path is project specific.
// Created: Tue Dec 14 12:10:03 CST 2010
//
// Copyright (C) PlayNet, Inc 1999-2010 - All rights reserved.
// Not for redistribution except as part of a compiled library.
//
// @file	host/hostGridCell.cpp
// @author	Oliver 'kfs1' Smith <osmith@playnet.com>
// @brief 	Implements the Host::Cell class.

#include "host/hostGridCellImpl.h"
#include "host/hostGridTypes.h"
#include "host/hostVehicle.h"

#ifndef ENABLE_TESTS
static Grid::InstanceHash instanceHash ;
#else
Grid::InstanceHash instanceHash ;
#endif

static std::unordered_set<Grid::Cell*> pendingExpirations ;

//////////////////////////////////////////////////////////////////////
// Constructor for a grid object

Grid::Cell::Cell(const Key& _key, CellHash& _hash)
	: m_key(_key)
	, m_hash(_hash)
	, m_vehicles()
	, m_vislist()
	, m_expiry(teulClock64 + ExpiryTime_ms)
	, m_active(false)
{
	pendingExpirations.insert(this) ;
	activate() ;
}

//////////////////////////////////////////////////////////////////////
// Destructor for a grid object

void
Grid::Cell::_removeFromNeighborhood()
{
	m_neighbors.erase(this) ;
	m_vislist.clear() ;
	for_each(m_neighbors.begin(), m_neighbors.end(), NeighborExtractionFunctor(this)) ;
}

//////////////////////////////////////////////////////////////////////
// Deactivation occurs prior to destruction: Extract ourselves from
// the neighborhood.

Grid::Cell::~Cell()
{
//	GRID_INFO("deactivating", 0) ;
	if ( m_active == false )
		return ;

	m_active = false ;

	assert( m_vehicles.empty() ) ;

	_removeFromNeighborhood() ;

	m_hash[m_key.y()].erase(m_key.x()) ;
}

//////////////////////////////////////////////////////////////////////
// Check to see if a grid element should be marked for expiry

void
Grid::Cell::_checkForExpiry()
{
	if ( !m_vehicles.empty() )
	{
		if ( m_expiry )
		{
//			GRID_INFO("cancelling expiration", 0) ;

			// We have data, so we can't expire
			m_expiry = 0LL ;
			pendingExpirations.erase(this) ;

			if ( !m_active )
				activate() ;
		}
		return ;
	}

	// We don't have data, are we penned for expiry?
	if ( m_expiry > 0LL )
		return ;

	// Flag us for expiry
	m_expiry = teulClock64 + ExpiryTime_ms ;
	pendingExpirations.insert(this) ;
}

//////////////////////////////////////////////////////////////////////
// Activation is separate from construction: Activated cells are
// registered with any active neighbors.

void
Grid::Cell::activate()
{
//	GRID_INFO("activating", 0) ;

	m_active = true ;

	// Any given Cell { cx, cy } has a bounding box
	// of { { cx-RADIUS, cy-RADIUS }, { cx+RADIUS, cy+RADIUS } }.
	// CellHash is a map of y-value -> { x-value -> cell* },
	// so we can iterate through the y values to look for
	// rows with cells in our x bounding range.

	const Key::Coord firstY = key().y() - Radius_cells ;
	const Key::Coord lastY = key().y() + Radius_cells ;
	const Key::Coord leftX = key().x() - Radius_cells ;
	const Key::Coord rightX = key().x() + Radius_cells ;

	CellHash::iterator yit = m_hash.lower_bound(firstY) ;
	while ( yit != m_hash.end() && yit->first <= lastY )
	{
		CellXHash& xHash = yit->second ;
		CellXHash::iterator xit = xHash.lower_bound(leftX) ;
		while ( xit != xHash.end() && xit->first <= rightX )
		{
			Cell* const neighbor = xit->second ;
			if ( inRange(neighbor->key()) )
			{
				neighbor->m_neighbors.insert(this) ;
				m_neighbors.insert(neighbor) ;
				m_vislist.insert(neighbor->vehicles().begin(), neighbor->vehicles().end()) ;
			}
			++xit ;
		}
		++yit ;
	}

	m_hash[key().y()][key().x()] = this ;
	m_neighbors.insert(this) ;
}

//////////////////////////////////////////////////////////////////////
// Add a vehicle to a new grid, possibly transfering from an old grid

class Cell : public Grid::Cell
{
	friend struct RemoveVehicleFromCellVisList ;
} ;

struct RemoveVehicleFromCellVisList
{
	RemoveVehicleFromCellVisList(HostVehicle* const _vehicle) : vehicle(_vehicle) {}
	void operator()(Grid::Cell* const cell) { ((Cell*)cell)->m_vislist.erase(vehicle) ; }
	HostVehicle* const vehicle ;
} ;

void
Grid::Cell::addVehicle(HostVehicle* const vehicle)
{
//	std::for_each(neighbors().begin(), neighbors().end(), [&](Grid::Cell* restrict const cell) { cell->m_vislist.erase(vehicle) ; }) ;
//	RemoveVehicleFromCellVisList eraser(vehicle) ;
//	std::for_each(neighbors().begin(), neighbors().end(), eraser) ;

	m_vehicles.insert(vehicle) ;

	// If we were about to expire, cancel expiration.
	_checkForExpiry() ;
}

//////////////////////////////////////////////////////////////////////
// Patrol a given row of cells.

static void
_checkRowForExpiry(Grid::CellXHash& hash)
{
	for ( Grid::CellXHash::iterator it = hash.begin() ; it != hash.end() ; )
	{
		Grid::Cell* cell = it->second ;
		++it ;

		if ( !cell || cell->expired() )
		{
			delete cell ;
		}
	}
}

//////////////////////////////////////////////////////////////////////
// Patrol the instance hashes for expirations.

bool
Grid::PatrolExpirations()
{
	for ( InstanceHash::iterator instit = instanceHash.begin() ; instit != instanceHash.end() ; )
	{
		InstanceHash::iterator curInstIt = instit ;
		++instit ;

		CellHash& hash = curInstIt->second ;
		for ( CellHash::iterator it = hash.begin() ; it != hash.end() ; )
		{
			CellHash::iterator curRowIt = it ;
			++it ;
			_checkRowForExpiry(curRowIt->second) ;
			if ( curRowIt->second.empty() )
			{
				size_t before = hash.size() ;
				hash.erase(curRowIt) ;
				assert( before != hash.size() ) ;
			}
		}

		if ( hash.empty() )
		{
			instanceHash.erase(curInstIt) ;
		}
	}

	return true ;
}

//////////////////////////////////////////////////////////////////////
// Update the location of a vehicle.

bool
HostVehicle::updateGrid()
{
	const Grid::Key::Coord newGX = (Grid::Key::Coord)(x_cm() / (100 * Grid::Cell::Size_m)) ;
	const Grid::Key::Coord newGY = (Grid::Key::Coord)(y_cm() / (100 * Grid::Cell::Size_m)) ;

	Grid::Key newKey(newGX, newGY) ;
	if ( m_cell && newKey == m_cell->key() )
		return false ;

	// Extract ourselves from the current cell.
	removeFromGrid() ;

	Grid::CellHash& hash = instanceHash[instID()] ;
	Grid::CellXHash& xhash = hash[newGY] ;
	auto it = xhash.find(newGX) ;
	if ( it == xhash.end() )
	{
		// No cell exists for these coordinates: create one.
		m_cell = new Grid::Cell(newKey, hash) ;
	}
	else
	{
		m_cell = it->second ;
	}

	// Add ourselves to this node.
	m_cell->addVehicle(this) ;

	return true ;
}

//////////////////////////////////////////////////////////////////////
// Remove a vehicle..

void
HostVehicle::removeFromGrid()
{
	if ( m_cell )
	{
		m_cell->removeVehicle(this) ;
		m_cell = NULL ;
	}
}

