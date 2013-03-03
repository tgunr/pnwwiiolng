#include <syscfg.h> // Path is project specific.
//
// Date: Tue Mar 28 12:14:43 CST 2006
//
// Functions for managing/operating/interacting with the
// new grid system
//
// Copyright (C) PlayNet, Inc 1999-2006 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith
//

#include "versionNo.h"						// For version dependent code
#include "dbug/dbug.h"						// For host "dbugLog" access
#include <assert.h>

#include <string>
#include <set>
#include <stack>
#include <algorithm>

#include "chat/chatGrid.h"					// Our header file
#include "chat/chatGridTypes.h"
#include "chat/chatList.h"					// For playerIndex
#include "chat/chatPlayer.h"				// For Player
#include "teul/teulTime.h"					// For teulClock64
#include "wwii/wwiiHash.h"
#include "chat/chatMakeTempString.h"
#include "chat/chatServerSTO.h"
#include "chat/chatBroadcastObject.h"
#include "chat/chatHostMessages.h"
#include "chat/chatVehicleSTO.h"
#include "chat/chatMissions.h"
#include "chat/chatMissionTypes.h"
#include "chat/chatHulkSTO.h"
#include "chat/chatOrdnanceSTO.h"
#include "chat/chatUMSSTO.h"
#include "chat/chatDotCommand.h"
#include "include/mobileSpawnEnums.h"
#include "miss/missionMobileSpawnWaypoint.h"
#include "wwii/wwiiThreadText.h"
#include "wwii/wwiiDBThreads.h"
#include "wwii/wwiiThreads.h"
#include "wwii/wwiiConfig.h"
#include "wwii/wwiiIRC.h"

//////////////////////////////////////////////////////////////////////
// Private encapsulation of the ChatGrid index type.

struct GridKey
{
	//! Constructor.
	//! GridKeys are instance dependent so that STOs don't cross
	//! instance boundaries.
	GridKey(const instanceid_t& _instanceID, SINT32 x, SINT32 y)
		: m_instanceID(_instanceID), m_x(x), m_y(y)
		{}
	GridKey(UINT64 _key) : m_key(_key) {}

	const instanceid_t& instanceID() const { return m_instanceID ; }
	SINT32 X() const { return m_x ; }
	SINT32 Y() const { return m_y ; }
	const UINT64& key() const { return m_key ; }

	bool operator==(const GridKey& rhs) const { return instanceID() == rhs.instanceID() && key() == rhs.key() ; }

	instanceid_t m_instanceID ;
	union
	{
		struct { SINT32 m_x, m_y ; } ;
		UINT64 m_key ;
	} ;
} ;

// STL hash functor for the GridKey class.
namespace std { template<> struct hash<GridKey> { UINT64 operator()(const GridKey& key) const { return (key.instanceID() << 5) ^ key.key() ; } } ; }

typedef std::unordered_map<GridKey, class ChatGrid*> _CHAT_GRID ;
typedef std::set<class ChatGrid*> _GRIDS ;

static ChatGrid* _getGrid(const GridKey& key) ;

//////////////////////////////////////////////////////////////////////
//! Private ChatGrid class

class ChatGrid
{
public:
	ChatGrid(const GridKey& key) ;
	~ChatGrid() ;

public:
	enum
	{
#ifdef CLOSED_BETA
		_EXPIRY_MS = 20 * 1000			//!< 20 seconds
#else
		_EXPIRY_MS = 60 * 1000			//!< 1 minute
#endif
	,	_KEY_SHIFT = 9					//!< Keys have 512m granularity
	,	_NODE_SIZE = 1 << _KEY_SHIFT
	,	_CLIENT_RING_RADIUS = 1
	,	_STO_RING_RADIUS = 6			//!< Aprox. 3km range
	,	_STO_RING_DIAMETER = 2 * _STO_RING_RADIUS + 1
	,	_GRID_SIZE = _STO_RING_DIAMETER * _STO_RING_DIAMETER
	} ;

private:
	void _checkForExpiry() ;
	void _removeFromNeighborhood() ;

public:
	void activate() ;
	void deactivate() ;

	//! Add or transfer a player
	void addPlayer(Player* player, ChatGrid* oldGrid) ;
	//! Remove a player without any fuss
	void extractPlayer(Player* player) ;

	void addSTO(ServerSTO& sto) { m_stos.insert(&sto) ; _checkForExpiry() ; }
	void removeSTO(ServerSTO& sto) { m_stos.erase(&sto) ; _checkForExpiry() ; }

	bool expired() const ;

	bool inRange(const GridKey& botleft, const GridKey& topright) const
	{
		if ( gridKey().X() < botleft.X() || gridKey().Y() < botleft.Y() ) return false ;
		if ( gridKey().X() > topright.X() || gridKey().Y() > topright.Y() ) return false ;
		return true ;
	}

	const PLAYERS& players() const { return m_players ; }
	const GridKey& gridKey() const { return m_gridKey ; }
	const _GRIDS& neighbors() const { return m_neighbors ; }
	const STOS& stos() const { return m_stos ; }
	STOS& stos() { return m_stos ; }

private:
	GridKey			m_gridKey ;
	_GRIDS			m_neighbors ;
	PLAYERS			m_players ;
	STOS			m_stos ;
	TeulTimeMS		m_expiry ;
	bool			m_active ;

	friend struct NeighborExtractionFunctor ;
	friend ChatGrid* _getGrid(const GridKey& key) ;
} ;

#if defined(CLOSED_BETA)
# define GRID_INFO(mask, ...) dbugLog(_FLD, "g:%llu:%d,%d " mask, gridKey().instanceID(), gridKey().X(), gridKey().Y(), __VA_ARGS__)
# define GRID_INFOADD(mask, ...) dbugLog(_FLDB, "+ " mask, __VA_ARGS__)
#else
# define GRID_INFO(mask, ...)
#endif

//////////////////////////////////////////////////////////////////////

static _CHAT_GRID _gridIndex ;

typedef std::stack<ChatGrid*> _FREE_GRIDS ;
static _FREE_GRIDS _freeGrids ;

static STOBatch _stoBatch ;

//////////////////////////////////////////////////////////////////////
// Constructor for a grid object

ChatGrid::ChatGrid(const GridKey& key)
	: m_gridKey(key)
	, m_players()
	, m_stos()
	, m_expiry(0LL)
	, m_active(false)
{
	activate() ;
}

//////////////////////////////////////////////////////////////////////
// Destructor for a grid object

struct NeighborExtractionFunctor : public std::unary_function<ChatGrid*, void>
{
	NeighborExtractionFunctor(ChatGrid* leaver) : m_leaver(leaver) {}
	ChatGrid* m_leaver ;
	void operator()(ChatGrid* neighbor) const { neighbor->m_neighbors.erase(m_leaver) ; }
} ;

void
ChatGrid::_removeFromNeighborhood()
{
	m_neighbors.erase(this) ;

	for_each(m_neighbors.begin(), m_neighbors.end(), NeighborExtractionFunctor(this)) ;
}

ChatGrid::~ChatGrid()
{
	assert(m_players.empty()) ;
	assert(m_stos.empty()) ;

	_removeFromNeighborhood() ;
}

//////////////////////////////////////////////////////////////////////
// Get a pointer to a grid OR null if it doesn't exist

static ChatGrid*
_findGrid(const GridKey& key)
{
	_CHAT_GRID::iterator it = _gridIndex.find(key) ;
	if ( it == _gridIndex.end() )
		return NULL ;
	return it->second ;
}

//////////////////////////////////////////////////////////////////////
// Get a pointer to a grid, reserving it or allocating it if neccessary

static ChatGrid*
_getGrid(const GridKey& key)
{
	ChatGrid* grid = _findGrid(key) ;
	if ( grid != NULL )
		return grid ;

	if ( !_freeGrids.empty() )
	{
		grid = _freeGrids.top() ;
		_freeGrids.pop() ;
		grid->m_gridKey = key ;
		grid->activate() ;
		return grid ;
	}

	return new ChatGrid(key) ;
}
//////////////////////////////////////////////////////////////////////
// Check to see if a grid element should be marked for expiry

void
ChatGrid::_checkForExpiry()
{
	if ( !m_players.empty() || !m_stos.empty() )
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
// Get the neighbor list and register me with my neighbors

void
ChatGrid::activate()
{
//	GRID_INFO("activating", 0) ;
	m_expiry = 0LL ;

	assert( _gridIndex.find(gridKey()) == _gridIndex.end() ) ;
	_gridIndex[gridKey()] = this ;

	m_active = true ;

	_CHAT_GRID::iterator it ;
	SINT32 x, y ;
	for ( y = -_STO_RING_RADIUS ; y <= _STO_RING_RADIUS ; ++y )
	{
		for ( x = -_STO_RING_RADIUS ; x <= _STO_RING_RADIUS ; ++x )
		{
			GridKey neighbor(gridKey().instanceID(), gridKey().X() + x, gridKey().Y() + y) ;
			it = _gridIndex.find(neighbor) ;
			if ( it == _gridIndex.end() )
			{
				assert( !(neighbor.X() == gridKey().X() && neighbor.Y() == gridKey().Y()) ) ;
				continue ;
			}

			// Mutual visibility
			m_neighbors.insert(it->second) ;
			it->second->m_neighbors.insert(this) ;
		}
	}
}

//////////////////////////////////////////////////////////////////////
// Add a player to a new grid, possibly transfering from an old grid

void
ChatGrid::addPlayer(Player* player, ChatGrid* oldGrid)
{
	_GRIDS::iterator it ;

	if ( oldGrid )
	{
		GRID_INFO("transfering c:%s from %d,%d", player->callsign(), oldGrid->gridKey().X(), oldGrid->gridKey().Y()) ;
	}
	else
	{
		GRID_INFO("adding c:%s", player->callsign()) ;
	}

	if ( player->teulIndex() >= 0 )
	{
		// Grids that have moved out of range
		if ( oldGrid != NULL )
		{
			for ( it = oldGrid->m_neighbors.begin() ; it != oldGrid->m_neighbors.end() ; ++it )
			{
				if ( (*it)->stos().empty() )
					continue ;
				if ( std::find(m_neighbors.begin(), m_neighbors.end(), *it) == m_neighbors.end() )
				{
					_stoBatch.add((*it)->stos(), true) ;
				}
			}

			if ( _stoBatch.size() > 0 )
			{
				SINT64 xCoord = static_cast<SINT64>(oldGrid->gridKey().X()) << _KEY_SHIFT;
				SINT64 yCoord = static_cast<SINT64>(oldGrid->gridKey().Y()) << _KEY_SHIFT;
				_stoBatch.setCoords(static_cast<FLOAT64>(xCoord), static_cast<FLOAT64>(yCoord));
				_stoBatch.send(player->teulIndex()) ;
			}

			// Grids that have entered into range
			for ( it = m_neighbors.begin() ; it != m_neighbors.end() ; ++it )
			{
				if ( (*it)->stos().empty() )
					continue ;
				if ( std::find(oldGrid->m_neighbors.begin(), oldGrid->m_neighbors.end(), *it) == oldGrid->m_neighbors.end() )
				{
					_stoBatch.add((*it)->stos(), false) ;
				}
			}
		}
		else
		{
			for ( it = m_neighbors.begin() ; it != m_neighbors.end() ; ++it )
			{
				if ( !(*it)->stos().empty() )
				{
					_stoBatch.add((*it)->stos(), false) ;
				}
			}
		}

		if ( _stoBatch.size() > 0 )
		{
			SINT64 xCoord = static_cast<SINT64>(gridKey().X()) << _KEY_SHIFT;
			SINT64 yCoord = static_cast<SINT64>(gridKey().Y()) << _KEY_SHIFT;
			_stoBatch.setCoords(static_cast<FLOAT64>(xCoord), static_cast<FLOAT64>(yCoord));
			_stoBatch.send(player->teulIndex()) ;
		}
	}

	if ( oldGrid )
	{
		GRID_INFO("extracting %s after transfer", player->callsign()) ;
		oldGrid->extractPlayer(player) ;
	}

	m_players.insert(player) ;

	_checkForExpiry() ;
}

//////////////////////////////////////////////////////////////////////
// Deactivate a chat-grid node and clean it up.

void
ChatGrid::deactivate()
{
//	GRID_INFO("deactivating", 0) ;

	m_active = false ;

	_removeFromNeighborhood() ;

	m_neighbors.clear() ;
	m_stos.clear() ;
	m_players.clear() ;
}

//////////////////////////////////////////////////////////////////////
// Check if a grid has expired

bool
ChatGrid::expired() const
{
	return ( m_expiry > 0LL && m_expiry < teulClock64 ) ;
}

//////////////////////////////////////////////////////////////////////
// Extract a player from this grid.

void
ChatGrid::extractPlayer(Player* const player)
{
	GRID_INFO("removing c:%s", player->callsign()) ;

	// Remove the player from all of our STOs.
	for ( auto it = stos().begin() ; it != stos().end() ; ++it )
	{
		ServerSTO* const sto = (ServerSTO*)*it ;
		sto->extractPlayer(player) ;
	}

	m_players.erase(player) ;
	_checkForExpiry() ;
}

//////////////////////////////////////////////////////////////////////
// Check a grid cell for an over-population of hulks
// If the number of hulks is greater than limit,
// **ONE** hulk will be removed.
// NOTE: countHulksInArea depends on only one hulk being removed

static UINT32
_checkHulksInCell(ChatGrid* cell, UINT32 limit)
{
	STOS::iterator it ;
	UINT32 hulks = 0  ;
	ServerSTO* oldest = NULL ;

	// Count the hulks and track which is oldest incase
	// we need to remove it
	for ( it = cell->stos().begin()
			; it != cell->stos().end()
			; ++it )
	{
		if ( (*it)->type() != ServerTrackedObject::_HULK )
			continue ;
		++hulks ;

		if ( oldest == NULL || (*it)->age() > oldest->age() )
			oldest = (*it) ;
	}

	// Under either of these conditions, we don't need to proceed
	if ( hulks <= 0 )
		return 0 ;
	if ( hulks <= limit )
		return hulks ;

	// Simple sanity check
	assert( oldest != NULL ) ;

	// We've now determined that there is one or more hulk here,
	// and that we need to remove at least one hulk
	// NOTE: countHulksInArea depends on only one hulk being removed
	dbugLog(_FLI, "Removing oldest hulk: %u", oldest->uid()) ;
	oldest->deactivate() ;

	return hulks - 1 ;
}

//////////////////////////////////////////////////////////////////////
// Check the hulk population in the neighborhood of a particular cell
// and within each cell in that area too.
// Depends on _checkHulksInCell to only remove 1 hulk at a time.

static void
_checkHulksInArea(ChatGrid* cell)
{
	// If per-area limiting is not enabled, we're done
	if ( host_config.sto.hulkLimitPerArea <= 0 )
	{
		// Enforce per-cell if they exist
		if ( host_config.sto.hulkLimitPerCell > 0 )
		{
			_checkHulksInCell(cell, host_config.sto.hulkLimitPerCell - 1) ;
		}
		return ;
	}

	ChatGrid* highestPopulatedCell = NULL ;
	_GRIDS::iterator it ;
	UINT32 hulksInArea = 0 ;
	UINT32 highestPopulation = 0 ;

	// Find the most overpopulated cell
	for ( it = cell->neighbors().begin()
			; it != cell->neighbors().end()
			; ++it )
	{
		UINT32 hulksInCell = _checkHulksInCell(*it, host_config.sto.hulkLimitPerCell) ;

		if ( hulksInCell <= 0 )
			continue ;
		if ( highestPopulatedCell != NULL && hulksInCell < highestPopulation )
			continue ;
		if ( hulksInCell == highestPopulation
				&& highestPopulatedCell->players().size() >= (*it)->players().size() )
			continue ;

		///TODO: We should really pick the oldest between matches like this, huh?
		// Rather than leaving it like this

		highestPopulatedCell = *it ;
		highestPopulation = hulksInCell ;

		hulksInArea += hulksInCell ;
	}

	// Are there too many hulks in this area?
	if ( hulksInArea >= host_config.sto.hulkLimitPerArea )
	{
		// Simple sanity check
		assert( highestPopulatedCell != NULL ) ;
		_checkHulksInCell(highestPopulatedCell, 0) ;
	}
}

//////////////////////////////////////////////////////////////////////
// Add/remove a server tracked object to the grid

std::vector<SINT32> b_teulBroadcastIndexes ;

static void IndexPlayer(const wwiiPlayer* player)
{
	if ( player->teulIndex() >= 0 ) b_teulBroadcastIndexes.push_back(player->teulIndex()) ;
}

static void AddPlayersToIndex(const ChatGrid* grid)
{
	for_each(grid->players().begin(), grid->players().end(), IndexPlayer) ;
}

static bool
_announceSTOToGrid(ServerSTO& sto, bool isCreation)
{
	SINT32 gx = (SINT32)sto.position().x() >> ChatGrid::_KEY_SHIFT ;
	SINT32 gy = (SINT32)sto.position().y() >> ChatGrid::_KEY_SHIFT ;
	GridKey key(sto.m_instanceID, gx, gy) ;

	static bool inUse = false ;

	if ( inUse )
		LogAbort("recursive use of announceSTOToGrid") ;

	inUse = true ;

	STOS data ;
	data.insert(&sto) ;

	_stoBatch.clear() ;
	_stoBatch.setCoords(sto.position().x(), sto.position().y()) ;

	ChatGrid* cell ;
	const char* op ;
	if ( isCreation )
	{
		op = "add" ;
		cell = _getGrid(key) ;

		// DEBUG: Make sure we're not adding the same guy
		assert( cell != NULL ) ;
		assert( cell->stos().find(&sto) == cell->stos().end() ) ;

		cell->addSTO(sto) ;

		_stoBatch.add(data, false) ;
	}
	else
	{
		op = "remove" ;
		cell = _findGrid(key) ;

		assert( cell != NULL ) ;
		assert( cell->stos().find(&sto) != cell->stos().end() ) ;

		cell->removeSTO(sto) ;

		_stoBatch.add(data, true) ;
	}

	if ( g_logDebug )
		dbugLog(_FLD, "sto:%u %s [%f/%f/%f => %d/%d] %" PRSizeT " cells"
						, sto.uid(), op
						, sto.position().x()
						, sto.position().y()
						, sto.position().z()
						, key.X(), key.Y(), cell->neighbors().size()) ;

	// Are we forcibly removing an STO rather than expiring one due to old age?
	if ( isCreation == false && sto.needsBroadcastRemoval() == true )
	{
		chatBroadcastObject(playerIndex.begin(), playerIndex.end(), CHAT_XCHG_SERVER_TRACKED_OBJS, _stoBatch) ;
	}
	else
	{
		b_teulBroadcastIndexes.clear() ;

		for_each(cell->neighbors().begin(), cell->neighbors().end(), AddPlayersToIndex) ;

		// Don't send STO notices for a ordnance to the originating player
		if ( isCreation && sto.type() == ServerTrackedObject::_ORDNANCE )
		{
			SINT32 fromIndex = ((OrdnanceSTO&)sto).fromTeulIndex() ;
			if ( fromIndex != -1 )
			{
				std::vector<SINT32>::iterator it = std::find(b_teulBroadcastIndexes.begin(), b_teulBroadcastIndexes.end(), fromIndex) ;
				if ( it != b_teulBroadcastIndexes.end() )
					b_teulBroadcastIndexes.erase(it) ;
			}
		}

		if ( !b_teulBroadcastIndexes.empty() )
		{
			chatBroadcastObject(b_teulBroadcastIndexes.begin(), b_teulBroadcastIndexes.end(), CHAT_XCHG_SERVER_TRACKED_OBJS, _stoBatch) ;
			b_teulBroadcastIndexes.clear() ;
		}
	}

	_stoBatch.clear() ;

	inUse = false ;

	return true ;
}

//////////////////////////////////////////////////////////////////////
// Generic ServerSTO activation.

bool
ServerSTO::activate()
{
	dbugLog(_FLD, "%s:%u activating", typeName(), uid()) ;
	if ( activated() )
		return false ;

	if ( !_announceSTOToGrid(*this, true) )
	{
		dbugLog(_FLA, "%s announce failed", typeName()) ;
		return false ;
	}

	bool result = ServerTrackedObject::activate() ;
	if ( result == true )
		_audit(Creation) ;

	return result ;
}

//////////////////////////////////////////////////////////////////////
// Generic ServerSTO deactivation.

void
ServerSTO::deactivate()
{
	dbugLog(_FLD, "%s:%u deactivating", typeName(), uid()) ;

	_announceSTOToGrid(*this, false) ;

	_audit(Destruction) ;

	ServerTrackedObject::deactivate() ;
}

//////////////////////////////////////////////////////////////////////
// UMS STO activate function

bool
UMSSTO::activate()
{
	if ( activated() )
		return false ;

	// Make sure we can look this dude up
	Player* const player = PlayerInterface::Lookup(creatorID()) ;
	if ( player == NULL || player->missionID() == 0 )
		return false ;

	try
	{
		ChatMission* mission = chatFindMission(player->missionID()) ;
		if ( mission == NULL )
			throw MSDR_MISSION_EXPIRED ;

		if ( player->isMissionLeader() == false && mission->m_stoUID != 0 )
			throw MSDR_1PER ;

//		// Allow 10 seconds for a response from the mmhost
//		m_lifespan = 1 * 1000 ;
		mission->sendMobileSpawn(player->playerID(), player->matchHandle(), position().x(), position().y(), position().z(), true, uid()) ;
	}
	catch (MSPAWN_RESULT error)
	{
		char errorMsg[256] ;
		snprintf(errorMsg, 255, "t:%d p:%u c:%s m:%u %s activate failed: MSDR %u"
						, player->teulIndex(), player->playerID(), player->callsign(), player->missionID(), typeName(), error) ;
		dbugLog(_FLA, "%s", errorMsg) ;
		wwiiIRCChannelMessage("-sys", errorMsg) ;
		///TODO: What was I going to do here?
		return false ;
	}

	if ( !_announceSTOToGrid(*this, true) )
	{
		///TODO: Delete the msp waypoint too
		return false ;
	}

	_audit(Creation) ;

	return ServerTrackedObject::activate() ;
}

//////////////////////////////////////////////////////////////////////
// Patrol the expiry list for expiring Grid entries

bool
chatPatrolGridExpirations()
{
	_CHAT_GRID::iterator it ;
#ifdef HEAVY_DEBUG
	if ( !_gridIndex.empty() )
	{
		dbugLog(_FLD, "cell:%u entries. ", _gridIndex.size()) ;
		for ( it = _gridIndex.begin() ; it != _gridIndex.end() ; ++it )
		{
			ChatGrid* node = it->second ;
			if ( g_logDebug )
				dbugLog(_FLDB, "[g:%llu:%d,%d %p players:%u, stos:%u] "
								, node->gridKey().instanceID(), node->gridKey().X(), node->gridKey().Y(), node
								, node->players().size(), node->stos().size()) ;
		}
	}
#endif

	for ( it = _gridIndex.begin() ; it != _gridIndex.end() ;  )
	{
		ChatGrid* cell = it->second ;
		++it ;
		if ( cell->expired() )
		{
			if ( g_logDebug )
				dbugLog(_FLD, "g:%llu:%d,%d expiry time elapsed"
								, cell->gridKey().instanceID(), cell->gridKey().X(), cell->gridKey().Y()) ;
			_gridIndex.erase(cell->gridKey()) ;
			cell->deactivate() ;
			_freeGrids.push(cell) ;
		}
		else if ( !cell->stos().empty() )
		{
			if ( host_config.sto.hulkLimitPerArea > 0 )
				_checkHulksInArea(cell) ;
			else if ( host_config.sto.hulkLimitPerCell > 0 )
				_checkHulksInCell(cell, host_config.sto.hulkLimitPerCell) ;
		}
	}

	return true ;
}

//////////////////////////////////////////////////////////////////////
// Position a player in the grid

void
Player::updateGridPosition()
{
	SINT32 newGX = ((SINT32)m_lastX) >> ChatGrid::_KEY_SHIFT ;
	SINT32 newGY = ((SINT32)m_lastY) >> ChatGrid::_KEY_SHIFT ;

	if ( newGX == m_gridX && newGY == m_gridY )
		return ;

	ChatGrid* oldGrid = _findGrid(GridKey(instance().id(), m_gridX, m_gridY)) ;
	ChatGrid* newGrid = _getGrid(GridKey(instance().id(), newGX, newGY)) ;

	if ( g_logDebug )
	{
		dbugLog(_FLD, "t:%d p:%u c:%s moved into [%d,%d] -> [%d,%d]"
						, teulIndex(), playerID(), callsign()
						, m_gridX, m_gridY
						, newGX, newGY) ;
	}

#ifdef HEAVY_DEBUG
	char buffer[4096] ;
	int off ;
	off = sprintf(buffer, "Grid:%d,%d off:%d,%d. Neighborhood:"
							, newGX, newGY
							, lastX - (newGX << ChatGrid::_KEY_SHIFT)
							, lastY - (newGY << ChatGrid::_KEY_SHIFT)
							) ;
	_GRIDS::const_iterator it ;
	for ( it = newGrid->neighbors().begin() ; it != newGrid->neighbors().end() ; ++it )
	{
		off += sprintf(buffer + off, "%d,%d(c:%" PRSizeT ",s:%" PRSizeT ") ", (*it)->gridKey().X(), (*it)->gridKey().Y(), (*it)->players().size(), (*it)->stos().size()) ;
	}
	message(LSTR_NOTE, LSTR_GENERIC, buffer) ;
#endif

	newGrid->addPlayer(this, oldGrid) ;

	m_gridX = newGX ;
	m_gridY = newGY ;
}

//////////////////////////////////////////////////////////////////////
// Remove a player from the grid

void
Player::removeFromGrid()
{
	if ( m_gridX == INVALID_GRID_COORD && m_gridY == m_gridX )
		return ;

	if ( g_logDebug )
		log(_FLD, "removing from grid [%d,%d]", m_gridX, m_gridY) ;

	ChatGrid* grid = _findGrid(GridKey(instance().id(), m_gridX, m_gridY)) ;
	if ( grid != NULL )
		grid->extractPlayer(this) ;

	m_gridX = m_gridY = INVALID_GRID_COORD ;
}

void
chatGridGetPlayersVector(const Player* player, const SINT64& range, PLAYERS_ARRAY& cells)
{
	cells.clear() ;

	GridKey origin(player->instance().id(), player->gridX(), player->gridY()) ;
	ChatGrid* grid = _findGrid(origin) ;
	if ( grid == NULL )
	{
		player->log(_FLW, "g:%d,%d no grid?", player->gridX(), player->gridY()) ;
		return ;
	}

	GridKey botleft(player->instance().id(), (SINT32)(player->lastX() - range) >> ChatGrid::_KEY_SHIFT, (SINT32)(player->lastY() - range) >> ChatGrid::_KEY_SHIFT) ;
	GridKey topright(player->instance().id(), (SINT32)(player->lastX() + range) >> ChatGrid::_KEY_SHIFT, (SINT32)(player->lastY() + range) >> ChatGrid::_KEY_SHIFT) ;

	_GRIDS::const_iterator it ;
	PLAYERS::iterator cit ;

	betaLog(_FLD, "t:%d p:%u c:%s in:%u.%u.%llu.%llu getting cells within [%d,%d],[%d,%d] %lld range: "
					, player->teulIndex(), player->playerID(), player->callsign()
					, player->instance().category(), player->instance().type(), player->instance().handle(), player->instance().id()
					, botleft.X(), botleft.Y(), topright.X(), topright.Y()
					, range) ;

	for ( it = grid->neighbors().begin() ; it != grid->neighbors().end() ; ++it )
	{
		const ChatGrid* node = (*it) ;
		if ( node->players().empty() )
		{
#ifdef CLOSED_BETA
			if ( g_logDebug )
			{
				betaLog(_FLDB, "[%d,%d:empty] ", node->gridKey().X(), node->gridKey().Y()) ;
			}
#endif
			continue ;
		}
		if ( !node->inRange(botleft, topright) )
		{
#ifdef CLOSED_BETA
			if ( g_logDebug )
			{
				betaLog(_FLDB, "[%d,%d:too far] ", node->gridKey().X(), node->gridKey().Y()) ;
			}
#endif
			continue ;
		}
		cells.push_back(&node->players()) ;
#ifdef CLOSED_BETA
		if ( g_logDebug )
		{
			betaLog(_FLDB, "%d,%d ", node->gridKey().X(), node->gridKey().Y()) ;
		}
#endif
	}

#ifdef CLOSED_BETA
	if ( g_logDebug )
	{
		betaLog(_FLDB, " total: %u cells", cells.size()) ;
	}
#endif
}

static bool g_enableSnapshot = false ;
static bool g_enableFullSnapshot = false ;

bool
chatGridSnapshot()
{
	if ( ! g_enableSnapshot )
		return true ;

	static TeulTimeMS nextSnap = teulClock64 ;
	if ( teulClock64 < nextSnap )
		return true ;

	nextSnap = teulClock64 + 1 * 60 * 1000 ;	// Once a minute

	char query[4096] ;
	int len = 0 ;

	_CHAT_GRID::const_iterator it ;
	for ( it = _gridIndex.begin() ; it != _gridIndex.end() ; ++it )
	{
		const ChatGrid* node = it->second ;
		if ( !g_enableFullSnapshot && !node->players().size() && !node->stos().size() )
			continue ;
		if ( len == 0 )
		{
			len = sprintf(query, "INSERT INTO grid_snapshot (gx,gy,recorded,players,stos) VALUES ") ;
		}
		else
		{
			query[len++] = ',' ;
		}
		len += sprintf(query + len, "(%d,%d,now(),%" PRSizeT ",%" PRSizeT ")"
						, node->gridKey().X(), node->gridKey().Y()
						, node->players().size(), node->stos().size()) ;
		if ( len > 4000 )
		{
			(new DBThreadQuery(DBThread::Default, "GridSnapshot1", query, DBThreadQuery::_IMMEDIATE))->Queue() ;
			len = 0 ;
		}
	}

	if ( len > 0 )
		(new DBThreadQuery(DBThread::Default, "GridSnapshot2", query, DBThreadQuery::_IMMEDIATE))->Queue() ;

	return true ;
}

class DotGridSnapshot : public DotCommand
{
public:
	DotGridSnapshot() : DotCommand("gridsnapshot", "Enable/disable grid snapshot recording", NO_MASK, LSTR_GRIDSNAPSHOT_USAGE, LSTR_GRIDSNAPSHOT_DETAIL) {}

	virtual void actionWith(Player& player, Chat::ClientMessage* /*message*/, char* arguments) const OVERRIDE_FN
	{
		if ( strcmp(arguments, "on") == 0 )
		{
			g_enableSnapshot = true ;
			g_enableFullSnapshot = false ;
			player.message(LSTR_NOTE, LSTR_GRIDSNAPSHOT_ENABLED) ;
		}
		if ( strcmp(arguments, "full") == 0 )
		{
			g_enableSnapshot = g_enableFullSnapshot = false ;
			player.message(LSTR_NOTE, LSTR_GRIDSNAPSHOT_FULL) ;
		}
		else if ( strcmp(arguments, "off") == 0 )
		{
			g_enableSnapshot = false ;
			player.message(LSTR_NOTE, LSTR_GRIDSNAPSHOT_DISABLED) ;
		}
		else
			help(player) ;
	}
} dotGridSnapshot ;

