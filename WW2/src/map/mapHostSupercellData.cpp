#include <syscfg.h> // Path is project specific.
// Created: 2011/9/20 16:01 CDT
//
// Copyright (c) 2011 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Oliver "kfs1" Smith

#include "strt/strtSupercell.h"
#include "strt/strtSupercells.h"
#include "strt/strtObjectStruct.h"

#include "map/mapHostSupercellVislevelXfer.h"

// For sending data.
#include "map/mapConstants.h"
#include "teul/teulPacket.h"
#include "map/mapPlayer.h"
#include "host/hostPlayers.h"

// Vector of supercells with dirty data.
static std::vector<Supercell*> _dirtyCells;
// Amount of data this represents.
static size_t _dirtyData;

//////////////////////////////////////////////////////////////////////
// Create one huge buffer of all the supercell vis level bits as a
// single, contiguous data stream ready for sending to players.

class HostSupercellData
{
private:
	void* m_allocation;
	size_t m_size;

	// Constructor.
public:
	HostSupercellData()
		: m_allocation(NULL)
		, m_size(0)
	{
		UINT32 numBuildings = 0, maxBuildings = 0;
		std::pair<void*, size_t> result = HostSupercellVislevelXfer::allocateFor(g_supercells.begin(), g_supercells.end(), numBuildings, maxBuildings);
		m_allocation = result.first;
		m_size = result.second;

		dbugLog(_FLI, "Created HostSupercellData: %" PRSizeT " supercells, max %u buildings in a supercell, total %u buildings requiring %" PRSizeT " bytes"
					, g_supercells.size(), maxBuildings, numBuildings, m_size);
	}

	// Destructor.
public:
	~HostSupercellData()
	{
		if ( m_allocation != NULL )
			memFree(m_allocation);
	}

	// Update the bits for a given building.
public:
	bool updateBuilding(const ObjectStruct* const obj)
	{
		if ( obj->supercell == NULL || obj->supercell->m_xfer == NULL )
			return false;
		return obj->supercell->m_xfer->setBuildingLevel(obj->m_bid, obj->effectiveVisLevel());
	}

	// Return the size of the payload.
public:
	size_t size() const { return m_size; }

	// Return the data as an opaque blob.
public:
	const void* data() const { return m_allocation; }
};

//////////////////////////////////////////////////////////////////////
// Wrapper for a private instance of HostSupercellData.

static HostSupercellData&
_getHostSupercellData()
{
	static HostSupercellData hsd;
	return hsd;
}

//////////////////////////////////////////////////////////////////////
// Expose the initialization function.

void
mapInitializeHostSupercellData()
{
	_getHostSupercellData();
}

//////////////////////////////////////////////////////////////////////
// Send all of the current data to a specific teulIndex.

SINT32
mapSendHostSupercellData(SINT32 teulIndex)
{
	HostSupercellData& hsd = _getHostSupercellData();
	const size_t size = hsd.size();

	Teul::Packet outgoing(size, Map::CallbackX, Map::BUILDING_VISLEVELS_Y);
	outgoing.setPayloadSizeBytes(size);
	memcpy(outgoing.payload(), hsd.data(), size);
	dbugLog(_FLD, "t:%d sending %" PRSizeT " byte building damage level packet", teulIndex, size);

	return outgoing.send(teulIndex);
}

//////////////////////////////////////////////////////////////////////
// Modify the vislevel data for a particular building.

void
mapUpdateBuildingVisLevel(const ObjectStruct* const obj)
{
	HostSupercellData& hsd = _getHostSupercellData();
	const bool updated = hsd.updateBuilding(obj);
	if ( updated && obj->supercell->m_modified == false )
	{
		_dirtyCells.push_back(obj->supercell);
		_dirtyData += obj->supercell->m_xfer->size_bytes();
		obj->supercell->m_modified = true;
	}
}

//////////////////////////////////////////////////////////////////////
// Send all the dirty supercell data to the current players.

static void _clearData()
{
	const size_t cells = _dirtyCells.size();
	for ( size_t i = 0; i < cells; ++i )
	{
		_dirtyCells[i]->m_modified = false;
	}

	_dirtyCells.clear();
	_dirtyData = 0;
}

void
mapSendDirtySupercellData()
{
	if ( _dirtyData == 0 )
	{
		return;
	}

	if ( g_players.empty() )
	{
		_clearData();
		return;
	}

	std::vector<SINT32> indexes;
	for ( auto it = g_players.begin(); it != g_players.end(); ++it )
	{
		Player* const player = (Player*)(*it);
		if ( player->isConnected() && player->isSpawned() && player->instance().isCampaign() )
			indexes.push_back(player->teulIndex());
	}

	if ( indexes.empty() )
	{
		_clearData();
		return;
	}

	dbugLog(_FLI, "Sending %" PRSizeT " supercells to %" PRSizeT " players in %" PRSizeT " bytes", _dirtyCells.size(), indexes.size(), _dirtyData);

	// Create a packet with all of the data.
	Teul::Packet outgoing(_dirtyData, Map::CallbackX, Map::BUILDING_VISLEVELS_Y);
	outgoing.setUnencrypted();

	const size_t numDirtyCells = _dirtyCells.size();
	for ( size_t i = 0; i < numDirtyCells; ++i )
	{
		Supercell* const sc = _dirtyCells[i];
		outgoing.append(sc->m_xfer, sc->m_xfer->size_bytes());
	}

	for ( auto it = indexes.begin(); it != indexes.end(); ++it )
	{
		outgoing.send(*it);
	}

	_clearData();
}

