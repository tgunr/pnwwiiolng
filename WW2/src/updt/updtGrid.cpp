#include <syscfg.h> // Path is project specific.
//
// Date: Wed Mar  11 16:43:42 CST 2007
//
// New replacement vehicle grid
//
// Copyright (C) PlayNet, Inc 1999-2007 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith
//

#include "dbug/dbug.h"							// For host "dbugLog" access
#include <assert.h>

#include "wwii/wwiiHash.h"
#include "strt/strtCPStruct.h"
#include "strt/strtFacilityStruct.h"
#include "strt/strtSupercell.h"
#include "teul/teulTime.h"

struct GridKey;

struct GridKey
{
	constexpr GridKey(SINT32 x, SINT32 y) : m_x(x), m_y(y) {}
	constexpr GridKey(UINT64 key) : m_key(key) {}
	constexpr SINT32 x() const { return m_x; }
	constexpr SINT32 y() const { return m_y; }
	constexpr const UINT64& key() const { return m_key; }

	union
	{
		struct { SINT32 m_x, m_y; };
		UINT64 m_key;
	};
};

using _WORLD_HASH = std::unordered_map<UINT64, class WorldGrid*>;
using _WORLD_GRIDS = std::set<class WorldGrid*>;
using _FREE_STACK = std::stack<WorldGrid*>;

using _VEHICLE_LIST = std::set<class HostVehicle*>;

static WorldGrid* _getGrid(const GridKey& key);

//////////////////////////////////////////////////////////////////////
// WorldGrid class

class WorldGrid
{
public:
	WorldGrid(const GridKey& key) ;
	~WorldGrid() ;

	static _WORLD_HASH		gridHash ;
	static _FREE_STACK		freeList ;

public:
	// How long does a grid entry stick around before expiring
	enum
	{
		_EXPIRY_MS = 60 * 1000			// 1 minute
	,	_KEY_SHIFT = 9					// Keys have 512m granularity
	,	_NODE_SIZE = 1 << _KEY_SHIFT
	,	_MAX_VIS_RANGE = 8192			// 8km
	,	_RING_RADIUS = ceil(_MAX_VIS_RANGE / _NODE_SIZE)
	,	_RING_DIAMETER = 2 * _STO_RING_RADIUS + 1
	,	_GRID_SIZE = _RING_DIAMETER * _RING_DIAMETER
	} ;

private:
	void _checkForExpiry() ;
	void _removeFromNeighborhood() ;

public:
	void activate() ;
	void deactivate() ;

	// Add or transfer a vehicle
	void addVehicle(Vehicle* vehicle, WorldGrid* oldGrid) ;
	// Remove a vehicle without any fuss
	void extractVehicle(Vehicle* vehicle) { m_vehicles.erase(vehicle) ; _checkForExpiry() ; }
	// Remove a vehicle with bells and whistles
	void removeVehicle(Vehicle* vehicle) ;

	bool expired() const ;

	bool inRange(const GridKey& botleft, const GridKey& topright) const
	{
		if ( gridKey().x() < botleft.x() || gridKey().y() < botleft.y() ) return false ;
		if ( gridKey().x() > topright.x() || gridKey().y() > topright.y() ) return false ;
		return true ;
	}

	const _VEHICLE_LIST& vehicles() const { return m_vehicles ; }
	const GridKey& gridKey() const { return m_gridKey ; }
	const _WORLD_GRIDS& neighbors() const { return m_neighbors ; }

private:
	GridKey	m_gridKey ;
	_WORLD_GRIDS	m_neighbors ;
	_VEHICLE_LIST	m_vehicles ;
	TeulTimeMS		m_expiry ;
	bool			m_active ;

	friend struct NeighborExtractionFunctor ;
	friend WorldGrid* _getGrid(const GridKey& key) ;
} ;

//////////////////////////////////////////////////////////////////////
// Constructor for a grid object

WorldGrid::WorldGrid(const GridKey& key)
	: m_gridKey(key)
	, m_vehicles()
	, m_expiry(0LL)
	, m_active(false)
{
	activate() ;
}

//////////////////////////////////////////////////////////////////////
// Destructor for a grid object

struct NeighborExtractionFunctor : public std::unary_function<WorldGrid*, void>
{
	NeighborExtractionFunctor(WorldGrid* leaver) : m_leaver(leaver) {}
	WorldGrid* m_leaver ;
	void operator()(WorldGrid* neighbor) const { neighbor->m_neighbors.erase(m_leaver) ; }
} ;

void
WorldGrid::_removeFromNeighborhood()
{
	m_neighbors.erase(this) ;
	for_each(m_neighbors.begin(), m_neighbors.end(), NeighborExtractionFunctor(this)) ;
}

WorldGrid::~WorldGrid()
{
	assert(m_vehicles.empty()) ;
	_removeFromNeighborhood() ;
}

//////////////////////////////////////////////////////////////////////
// Get a pointer to a grid OR null if it doesn't exist

static WorldGrid*
_findGrid(const GridKey& key)
{
	GRID_HASH::iterator it = WorldGrid::gridHash.find(key.key()) ;
	if ( it == WorldGrid::gridHash.end() )
		return NULL ;
	return it->second ;
}

//////////////////////////////////////////////////////////////////////
// Get a pointer to a grid, reserving it or allocating it if neccessary

static WorldGrid*
_getGrid(const GridKey& key)
{
	WorldGrid* grid = _findGrid(key) ;
	if ( grid != NULL )
		return grid ;

	if ( !_freeGrids.empty() )
	{
		grid = WorldGrid::freeList.top() ;
		WorldGrid::freeList.pop() ;
		grid->m_gridKey = key ;
		grid->activate() ;
		return grid ;
	}

	return new WorldGrid(key) ;
}
//////////////////////////////////////////////////////////////////////
// Check to see if a grid element should be marked for expiry

void
WorldGrid::_checkForExpiry()
{
	if ( !m_vehicles.empty() )
	{
		if ( m_expiry )
		{
//			GRID_INFO("cancelling expiration", 0) ;

			// We have data, so we can't expire
			m_expiry = 0LL ;

			if ( !m_active )
				activate() ;
		}
		return ;
	}

	// We don't have data, are we penned for expiry?
	if ( m_expiry > 0LL )
		return ;

	// Flag us for expiry
	m_expiry = teulClock64 + _EXPIRY_MS ;
}

//////////////////////////////////////////////////////////////////////
// Check if a grid has expired

bool
WorldGrid::expired() const
{
	return ( m_expiry > 0LL && m_expiry < teulClock64 ) ;
}

//////////////////////////////////////////////////////////////////////
// Get the neighbor list and register me with my neighbors

void
WorldGrid::activate()
{
//	GRID_INFO("activating", 0) ;
	m_expiry = 0LL ;

	assert( gridHash.find(gridKey().key()) == gridHash.end() ) ;
	gridHash[gridKey().key()] = this ;

	m_active = true ;

	_GRID_HASH::iterator it ;
	SINT32 x, y ;
	for ( y = -_RING_RADIUS ; y <= _RING_RADIUS ; ++y )
	{
		for ( x = -_RING_RADIUS ; x <= _RING_RADIUS ; ++x )
		{
			GridKey neighbor(gridKey().x() + x, gridKey().y() + y) ;
			it = gridHash.find(neighbor.key()) ;
			if ( it == gridHash.end() )
			{
				assert( !(neighbor.x() == gridKey().x() && neighbor.y() == gridKey().y()) ) ;
				continue ;
			}

			// Mutual visibility
			m_neighbors.insert(it->second) ;
			it->second->m_neighbors.insert(this) ;
		}
	}
}

void
WorldGrid::deactivate()
{
//	GRID_INFO("deactivating", 0) ;

	m_active = false ;

	_removeFromNeighborhood() ;

	m_neighbors.clear() ;
	m_vehicles.clear() ;
}

//////////////////////////////////////////////////////////////////////
// Add a vehicle to a new grid, possibly transfering from an old grid

void
WorldGrid::addVehicle(HostVehicle* vehicle, WorldGrid* oldGrid)
{
	// If oldGrid is true, this is a transfer
	if ( oldGrid == this )
		return ;

	m_vehicles.insert(vehicle) ;

	if ( oldGrid )
		oldGrid->extractVehicle(vehicle) ;

	_checkForExpiry() ;
}

//////////////////////////////////////////////////////////////////////
// Remove a vehicle from the grid without any fuss

void
WorldGrid::removeVehicle(HostVehicle* vehicle)
{
	extractVehicle(vehicle) ;
}

//////////////////////////////////////////////////////////////////////
// Patrol the expiry list for expiring Grid entries

bool
updtPatrolGridExpirations()
{
	_GRID_HASH::iterator it ;
	for ( it = gridHash.begin() ; it != gridHash.end() ;  )
	{
		WorldGrid* cell = it->second ;
		++it ;
		if ( cell->expired() )
		{
            dbugLog(_FLD, "g:%d,%d expiry time elapsed", cell->gridKey().x(), cell->gridKey().y()) ;
			gridHash.erase(cell->gridKey().key()) ;
			cell->deactivate() ;
			_freeGrids.push(cell) ;
		}
	}

	return true ;
}

//////////////////////////////////////////////////////////////////////
// Position a vehicle in the grid

void
HostVehicle::updateGridPosition()
{
	SINT32 newGX = (vehicleData.position.rawX / 100) >> WorldGrid::_KEY_SHIFT ;
	SINT32 newGY = (vehicleData.position.rawY / 100) >> WorldGrid::_KEY_SHIFT ;

	if ( m_onGrid && newGX == m_gridX && newGY == m_gridY )
		return ;

    dbugLog(_FLD, "v:%u c:%s moved [%d,%d] -> [%d,%d]"
                    , index, callsign()
                    , m_gridX, m_gridY
                    , newGX, newGY) ;

	WorldGrid* newGrid = _getGrid(GridKey(newGX, newGY)) ;
	WorldGrid* oldGrid = (m_onGrid) ? oldGrid = _findGrid(GridKey(m_gridX, m_gridY)) : NULL ;

	newGrid->addVehicle(this, oldGrid) ;

	m_onGrid = true ;
	m_gridX = newGX ;
	m_gridY = newGY ;
}

//////////////////////////////////////////////////////////////////////
// Remove a vehicle from the grid

void
HostVehicle::removeFromGrid()
{
	if ( !m_onGrid )
	{
		dbugLog(_FLD, "v:%u c:%s not on grid, not removing", index, callsign()) ;
		return ;
	}

    dbugLog(_FLD, "v:%u c:%s removing from grid [%d,%d]"
                    , index, callsign()
                    , m_gridX, m_gridY) ;

	WorldGrid* grid = _findGrid(GridKey(m_gridX, m_gridY)) ;
	if ( grid != NULL )
		grid->removeVehicle(this) ;

	m_gridX = m_gridY = 0 ;
	m_onGrid = false ;
}

void
updtGridGetVehiclesVector(const HostVehicle* vehicle, const SINT64& range, PLAYERS_ARRAY& cells)
{
	cells.clear() ;

	GridKey origin(vehicle->gridX(), vehicle->gridY()) ;
	WorldGrid* grid = _findGrid(origin) ;
	if ( grid == NULL )
	{
		dbugLog(_FLW, "t:%d p:%u c:%s g:%d,%d no grid?"
						, vehicle->teulIndex(), vehicle->vehicleID(), vehicle->callsign()
						, vehicle->gridX(), vehicle->gridY()) ;
		return ;
	}

	SINT32 posX = vehicle->vehicleData.position.rawX / 100 ;
	SINT32 posY = vehicle->vehicleData.position.rawY / 100 ;

	GridKey botleft((posX - (SINT32)WorldGrid::_RING_RADIUS) >> WorldGrid::_KEY_SHIFT, (posY - (SINT32)WorldGrid::_RING_RADIUS) >> WorldGrid::_KEY_SHIFT) ;
	GridKey topright((posX + (SINT32)WorldGrid::_RING_RADIUS) >> WorldGrid::_KEY_SHIFT, (posY + (SINT32)WorldGrid::_RING_RADIUS) >> WorldGrid::_KEY_SHIFT) ;

	_WORLD_GRIDS::const_iterator it ;

	betaLog(_FLD, "v:%u c:%s getting cells within [%d,%d],[%d,%d] %llu range: "
					, vehicle->index, vehicle->callsign()
					, botleft.x(), botleft.y(), topright.x(), topright.y()
					, WorldGrid::_RING_RADIUS) ;

	for ( it = grid->neighbors().begin() ; it != grid->neighbors().end() ; ++it )
	{
		const WorldGrid* node = (*it) ;
		if ( node->vehicles().empty() )
			continue ;
		if ( !node->inRange(botleft, topright) )
			continue ;
		cells.push_back(&node->vehicles()) ;
	}

    betaLog(_FLD, " total: %u cells", cells.size()) ;
}

// Structure indicates that I currently have you on my "vis list"
class VisLink
{
	HostVehicle*	m_vehicle ;
} ;
