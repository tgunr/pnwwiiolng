#include <syscfg.h> // Path is project specific.
// Copyright 1999 Playnet Inc, All rights reserved
// Not to be distributed in API form except as part of a compiled
// library
//
// Originating Programmer: Martin Poulin

//#undef NO_DEBUG_WRITE
#undef EXTREME_DEBUG
#undef ENABLE_VIS_DEBUG

#include "comm/commConstants.h"
#include "host/hostGridCellImpl.h"
#include "host/host.h"
#include "host/hostPlayer.h"
#include "host/hostVehicle.h"
#include "host/hostVehicleViewing.h"
#include "inst/instTypes.h"
#include "teul/teulPacket.h"
#include "updt/updtFraming.h"
#include "updt/updtTypes.h"
#include "weap/weapList.h"
#include "wwii/wwiiConfig.h"
#include "wwii/wwiiHash.h"			// For unordered map
#include "wwii/wwiiThreads.h"
#include "dbug/dbug.h"

#undef EXTREME_DEBUG
#ifdef EXTREME_DEBUG
# define EXTREME(...) dbugLog(__VA_ARGS__)
#else
# define EXTREME(...)
#endif

//////////////////////////////////////////////////////////////////////
// Dump a vis-list.

#include <type_traits>

#if defined(ENABLE_VIS_DEBUG)
#define VIS_DEBUG(...) dbugLog(__VA_ARGS__)
static void
_dumpVisListEntry(const Viewing& entry)
{
	char buffer[64];
	const char* lastSent;
	if ( entry.m_lastSent > 0 )
	{
		strprintf(buffer, "%" PR64BitUT "", teulClock64 - entry.m_lastSent);
		lastSent = buffer;
	}
	else
		lastSent = "never";
	dbugLog(_FLI, "%s:%s:%" PR64BitT ",", entry.m_vehicle.callsign(), lastSent, entry.m_priority);
}
#else
#define VIS_DEBUG(...)
#endif

template<typename _Container>
static void
_dumpVisList(const char* const label, const _Container& list)
{
#if defined(ENABLE_VIS_DEBUG)
	const size_t size = list.size();
	dbugLog(_FLI, "%s: (%u entries) ", label, size);
	if ( size > 0 )
	{
		dbugLog(_FLI, "[");
		for ( auto it = list.begin(); it != list.end(); ++it )
		{
			_dumpVisListEntry(*it);
		}
		dbugLog(_FLI, "]");
	}
#endif
}

//////////////////////////////////////////////////////////////////////
// Remove an viewing on my current list to the end of the list
// and queue it onto the remove list. Return an iterator to one after
// the removed viewing to support continued iteration.

Viewings::iterator
HostVehicle::removeFromVisList(Viewings::iterator& it)
{
	if ( it->m_lastSent > 0ULL )
	{
		m_worldUpdate->removeList[m_worldUpdate->numRemove++] = it->m_index;
	}
    return m_visList.erase(it);
}

//////////////////////////////////////////////////////////////////////
// Send a list of vehicles to stop rendering to all clients attached
// to a particular vehicle.

void
HostVehicle::SendRemoveList()
{
	if ( m_worldUpdate->numRemove == 0 )
		return;

	const UCHAR count = m_worldUpdate->numRemove;
	const size_t entrySize = sizeof(*m_worldUpdate->removeList);
	const size_t dataSize  = entrySize * count;

	Teul::Packet outgoing(Teul::PacketId(Comm::CallbackX, REMOVE_LIST_Y));
	outgoing.setUnencrypted();
	outgoing.reserve(sizeof(count) + dataSize);
	outgoing.append(count);
	outgoing.append(m_worldUpdate->removeList, dataSize);

	sendPacket(outgoing);

	m_worldUpdate->numRemove = 0;
}

//////////////////////////////////////////////////////////////////////
// Check if I perceive a given vehicle to be mandatory viewing

static bool
_isMandatoryVehicle(const HostVehicle& myVehicle, const HostVehicle& newVehicle)
{
	const bool isMyVehicle = (newVehicle.index == myVehicle.index);
	const bool isMulticrew = (newVehicle.isMultiCrewed());
	const bool isMyRide    = (myVehicle.vehicleData.rideIndex == newVehicle.index);

	return ( (isMyVehicle && isMulticrew) || isMyRide );
}

//////////////////////////////////////////////////////////////////////
// Calculate the square of the distance between a vehicle and my posn.

static UINT64
_getVehicleDistance_cm2(const HostVehicle& lhs, const HostVehicle& rhs)
{
	SINT64 sqXcm 	= 	(SINT64)(lhs.x_cm() - (SINT64)rhs.x_cm());
	SINT64 sqYcm 	= 	(SINT64)(lhs.y_cm() - (SINT64)rhs.y_cm());
	SINT64 sqZcm 	= 	(SINT64)(lhs.z_cm() - (SINT64)rhs.z_cm());

	sqXcm *= sqXcm;
	sqYcm *= sqYcm;
	sqZcm *= sqZcm;

	return (UINT64)(sqXcm + sqYcm + sqZcm);
}

//////////////////////////////////////////////////////////////////////
// Test if a vehicle is visible to me based on our vehicle specs.

static bool
_isVehicleVisible(const HostVehicle& vehicle, const UINT64& dist_cm2, const bool forFighter)
{
	if ( dist_cm2 > (UINT64)Grid::MAX_RANGE_CM_SQ )
	{
		EXTREME(_FLD, "!wayfar");
		return false;
	}

	UINT64 maxViewingDistance_cm2 = vehicle.m_maxLod_cm2;
	if ( forFighter && (vehicle.spec().wCategory == IN_ || vehicle.spec().wCategory == GV_) )
	{
		EXTREME(_FLD, ":forFighter");
		maxViewingDistance_cm2 >>= 2;
	}

	EXTREME(_FLD, ":dist=%" PR64BitUT ",maxV=%" PR64BitUT ",inrange=%s", (UINT64)dist_cm2, (UINT64)maxViewingDistance_cm2, dist_cm2 <= maxViewingDistance_cm2 ? "yes" : "no");

	if ( dist_cm2 <= maxViewingDistance_cm2 )
		return true;
	return false;
}

//////////////////////////////////////////////////////////////////////
// Generate weights for all the vehicles currently on my update list;
// their presence on our list means they already have holders.

static void
_weightCurrentVisList(HostVehicle& myVehicle)
{
	auto it = myVehicle.m_visList.begin();

	while ( it != myVehicle.m_visList.end() )
	{
		const HostVehicle& subject = it->m_vehicle;

		// Setting inUpdate to true, even if we decide they
		// will not be getting sent prevents them from
		// being added anyway in merge vehicles...
		// See if the vehicle has exited
		if ( subject.index != it->m_index )
		{
			EXTREME(_FLD, " vv:%u looks like it went away", it->m_index);
			it = myVehicle.removeFromVisList(it);
			continue;
		}

		if ( _isMandatoryVehicle(myVehicle, subject) )
		{
			EXTREME(_FLD, " vv:%u mandatory", it->m_index);
			it->m_priority = -1;
		}
		else
		{
			if ( subject.isHiddenBuzzard() && ! myVehicle.canSeeHiddenBuzzards() )
			{
				EXTREME(_FLD, " vv:%u hidden", it->m_index);
				it = myVehicle.removeFromVisList(it);
				continue;
			}
			it->m_priority = _getVehicleDistance_cm2(subject, myVehicle);
			if ( _isVehicleVisible(subject, it->m_priority, myVehicle.m_isFighter) == false )
			{
				EXTREME(_FLD, " vv:%u out of range", it->m_index);
				// Gone out of range
				it = myVehicle.removeFromVisList(it);
				continue;
			}
		}

		EXTREME(_FLD, " vv:%u visible", it->m_index);
		++it;
	}
}

//////////////////////////////////////////////////////////////////////
// Check if a given vehicle is in our update list.

static bool
_hasVehicleHadUpdate(UINT32 index, HostVehicle& myVehicle, const TeulTimeMS since)
{
	auto it = myVehicle.m_visList.begin();
	while ( it != myVehicle.m_visList.end() )
	{
		if ( index == it->m_vehicle.index )
			return (it->m_lastSent >= since);
		++it;
	}

	return false;
}

static bool
_vehicleIsCurrentlyInList(UINT32 index, HostVehicle& myVehicle)
{
	return _hasVehicleHadUpdate(index, myVehicle, 0ULL);
}

//////////////////////////////////////////////////////////////////////
// Add the vehicles from a given grid node into our vis list.

// Vehicles we know to have riders.
using CARRIERS = std::unordered_map<UINT32 /*carrierIndex*/, SINT32 /*riderCount or -1*/>;

static bool
_addCellNodesToVisList(HostVehicle& myVehicle, const Grid::VehicleSet& vehicles, CARRIERS& carriers)
{
	bool additions = false;

	auto it = vehicles.begin();
	while ( it != vehicles.end() )
	{
		const HostVehicle& subject = *(*it);
		++it;

		EXTREME(_FLD, " cand:%u:%s", subject.index, subject.callsign());

		if ( _vehicleIsCurrentlyInList(subject.index, myVehicle) == true )
		{
			EXTREME(_FLD, "!in");
			continue;
		}

		SINT64 vehicleDist_cm2 = -1LL;
		if ( _isMandatoryVehicle(myVehicle, subject) == false )
		{
			EXTREME(_FLD, "=norm");
			const bool isHidden = subject.isHiddenBuzzard() && ! myVehicle.canSeeHiddenBuzzards();
			if ( subject.index == myVehicle.index || ! subject.isActive()  || isHidden )
			{
				EXTREME(_FLD, "!vis");
				continue;
			}
			vehicleDist_cm2 = _getVehicleDistance_cm2(subject, myVehicle);
			if ( _isVehicleVisible(subject, vehicleDist_cm2, myVehicle.m_isFighter) == false )
			{
				EXTREME(_FLD, "!range");
				continue;
			}

			if ( vehicleDist_cm2 == 0 )
				vehicleDist_cm2 = 1;

			// Count the number of riders on each vehicle so that we can make
			// each subsequent rider less important to our vis list. This way
			// we will see less riders as our vis list becomes more crowded,
			// except as the carrying vehicle becomes closer to us.
			const UINT32 rideIndex = subject.vehicleData.rideIndex;
			if ( rideIndex != 0 )
			{
				SINT32& riderCount = carriers[rideIndex];
				// Never show me a rider for a vehicle I haven't seen yet.
				if ( riderCount == -1 )
				{
					EXTREME(_FLD, "!rider");
					continue;
				}
				// If this is the first update in which we are seeing the
				// ride, then don't send me the riders just yet.
				if ( riderCount == 0 && rideIndex != myVehicle.index && _hasVehicleHadUpdate(rideIndex, myVehicle, 1) == false )
				{
					EXTREME(_FLD, "!ride");
					riderCount = -1;
					continue;
				}
				vehicleDist_cm2 += (vehicleDist_cm2 / (host_config.update.riderBiasing + 1) * riderCount++);
			}
		}

		EXTREME(_FLD, "+add");
		myVehicle.m_visList.push_back(Viewing(subject, vehicleDist_cm2));
		additions = true;
	}

	return additions;
}

//////////////////////////////////////////////////////////////////////
// Go through the vis list and limit the number of potential additions.

static void
_cullAdditions(HostVehicle& myVehicle)
{
	UINT32 newVehiclesRemaining = host_config.update.limitNewVehicles;
	if ( newVehiclesRemaining == 0 )
		return;
	auto it = myVehicle.m_visList.begin();
	while ( it != myVehicle.m_visList.end() )
	{
		// Already in the update.
		if ( it->m_lastSent == 0 )
		{
			if ( newVehiclesRemaining == 0 )
			{
				EXTREME(_FLD, " vc:%u no room for new updates", it->m_index);
				it = myVehicle.removeFromVisList(it);
				continue;
			}
			--newVehiclesRemaining;
		}

		++it;
	}
}

//////////////////////////////////////////////////////////////////////
// Pop excess entries from the end of the vis list.

static void
_limitVisListSize(HostVehicle& myVehicle, const UINT32 numVehicles)
{
    if (myVehicle.m_visList.size() > numVehicles) {
        for (auto it = myVehicle.m_visList.begin() + numVehicles; it != myVehicle.m_visList.end(); ) {
            it = myVehicle.removeFromVisList(it);
        }
    }
}

#if defined(ENABLE_BIASING)

//////////////////////////////////////////////////////////////////////
// Because moving viewings around is non-trivial, while doing
// the biasing, we only really care about the positions (indexes)
// of the viewings in the vis list.

struct ViewingWeightOrder
{
	ViewingWeightOrder(HostVehicle& forVehicle)
		: myVehicle(forVehicle)
		, indexes()
		, biasedVehicles(0)
		, newVehiclesLeft(host_config.update.limitNewVehicles ? host_config.update.limitNewVehicles : 255)
		, size(forVehicle.m_visList.size())
	{
		indexes.resize(size);
		for ( UINT32 i = 0; i < size; ++i )
		{
			indexes[i] = i;
		}
		// Sort the indexes by the bias weighting of
		// the vehicle they reference.
		std::sort(indexes.begin(), indexes.end(), *this);
	}

	// Sort the indexes by the weight of the corresponding
	// vis list viewing.
	bool operator()(UINT32 lhs, UINT32 rhs) const
	{
		SINT64 myWeight = myVehicle.m_visList[lhs].m_priority;
		// Existing vehicles are always more important than new vehicles.
		if ( myVehicle.m_visList[lhs].m_lastSent == 0 && myVehicle.m_visList[rhs].m_lastSent != 0 )
			myWeight *= 3;
		return myWeight < myVehicle.m_visList[rhs].m_priority;
	}

	void adjustWeightsForBias(const WEAP_BIAS& bias);

	HostVehicle& myVehicle;
	std::vector<UINT32> indexes;
	UINT32 biasedVehicles;
	UINT32 newVehiclesLeft;
	const size_t size;
};

//////////////////////////////////////////////////////////////////////
// Determine which, if any, side a bias spec requires of a vehicle.

static WWII_SIDE
_getBiasSpecSide(const WEAP_BIAS& bias, const HostVehicle& myVehicle)
{
	if ( bias.spec.wCountry == Parse::FY_ )
		return myVehicle.side();
	if ( bias.spec.wCountry == Parse::EY_ )
		return wwiiOtherSide(myVehicle.side());
	return WWII_NO_SIDE;
}

//////////////////////////////////////////////////////////////////////
// Adjust the weights of unbiased vehicles that match a given bias spec

void
ViewingWeightOrder::adjustWeightsForBias(const WEAP_BIAS& bias)
{
	// Create a mask based on the weapon spec minus the country.
	const WWII_SIDE specSide = _getBiasSpecSide(bias, myVehicle);
	const UINT32 specMask = bias.spec.asUINT32() & 0x00ffffff;

	UINT32 slotsUsed = 0;
	auto& list = myVehicle.m_visList;

	// Start from the highest slot to contain a vehicle that
	// was positioned as a result of biasing.
	// Vehicles that match the bias pattern should be moved
	// down to the lower part of the list.
	for ( size_t current = biasedVehicles; current < size; ++current )
	{
		assert( current < indexes.size() );
		const UINT32 viewingIndex = indexes[current];
		assert( viewingIndex < list.size() );
		const Viewing& viewing = list[viewingIndex];
		assert( (UINT32)(&viewing.m_vehicle) > 1000 );
		const HostVehicle& subject = viewing.m_vehicle;

		// Find vehicles matching our bias pattern.
		if ( specSide && subject.side() != specSide || (specMask & subject.spec().asUINT32()) != specMask )
			continue;

		if ( current > biasedVehicles )
		{
			// Since the array is 0 based, the count of biased vehicles
			// is also the index of the first unbiased entry. Move this
			// vehicle down to that slot so that the biased vehicles
			// form a contiguous block.
			std::swap(indexes[current], indexes[biasedVehicles]);
		}

		// Add one to the biased count; all of the biased vehicles
		// will be at the bottom of the list.
		++biasedVehicles;

		++slotsUsed;
		if ( slotsUsed >= bias.slots )
			break;
	}
}

//////////////////////////////////////////////////////////////////////
// Re-weight the vis list based on biasing.

static void
_biasWeightVisList(HostVehicle& myVehicle)
{
	ViewingWeightOrder ordering(myVehicle);

	// For each bias we have, adjust the vehicle list weights.
	for ( UINT32 i = 0; i < MAX_BIAS_INDEX; ++i )
	{
		const WEAP_BIAS& bias = myVehicle.m_bias[i];
		if ( SPEC_IS_NULL(bias.spec) )
			break;

		ordering.adjustWeightsForBias(bias);
	}

	// Now assign the biasedVehicles weights based on their ordering position.
	for ( UINT32 i = 0; i < ordering.biasedVehicles; ++i )
	{
		SINT64& priority = myVehicle.m_visList[ordering.indexes[i]].m_priority;
		if ( priority >= 0 )
			priority = i;
	}

	// Now ensure that the remaining vehicles have weights higher than the highest bias value.
	for ( UINT32 i = ordering.biasedVehicles; i < myVehicle.m_visList.size(); ++i )
	{
		SINT64& priority = myVehicle.m_visList[ordering.indexes[i]].m_priority;
		if ( priority >= 0 )
			priority += i;
	}
}

#endif

//////////////////////////////////////////////////////////////////////
// Search cells adjacent to ours for vehicles that might be vis list
// candidates.

static void
_checkForNewVisListCandidates(HostVehicle& myVehicle)
{
	CARRIERS carriers;
	auto neighborhood = myVehicle.m_cell->neighbors();
	bool additions = false;
	for (auto it = neighborhood.begin(), end = neighborhood.end(); it != end; ++it)
	{
		const bool additionsThisCell = _addCellNodesToVisList(myVehicle, (*it)->vehicles(), carriers);
		additions |= additionsThisCell;
	}

	const UINT32 maxVisVehicles = framings[myVehicle.framingMode].maxVehicles;
	if ( additions == false && myVehicle.m_visList.size() <= maxVisVehicles )
	{
		// Incase vis-list size changed.
		EXTREME(_FLD, " no additions");
		return;
	}

	if ( additions )
	{
		EXTREME(_FLD, " additions");
		// Ensure that we don't try to add too many new vehicles.
		_cullAdditions(myVehicle);
	}

	// If we don't have a full vis list, we don't need to muck about
	// with what's in the list.
	myVehicle.m_visListPressure = myVehicle.m_visList.size();
	if ( myVehicle.m_visList.size() <= maxVisVehicles )
	{
		EXTREME(_FLD, " vl:%u, mvl:%u, plenty of room.", myVehicle.m_visList.size(), maxVisVehicles);
		if ( myVehicle.m_visList.size() > framings[myVehicle.framingMode].vehiclesPerFrame )
		{
			std::sort(myVehicle.m_visList.begin(), myVehicle.m_visList.end());
		}
		return;
	}

#if defined(ENABLE_BIASING)
	_biasWeightVisList(myVehicle);
#endif

	// Now sort the resulting visList and trim it down to size.
	std::sort(myVehicle.m_visList.begin(), myVehicle.m_visList.end());
	_limitVisListSize(myVehicle, maxVisVehicles);
}

//////////////////////////////////////////////////////////////////////
// Process "myVehicle"s

SINT32
HostVehicle::UpdateVehicleList()
{
	EXTREME(_FLD, "UPDT v:%u updating list", index);

	// Find where I am on the grid.
	if ( m_cell == nullptr )
	{
		EXTREME(_FLD, " cell=NULL");
		if ( m_visList.empty() == false )
		{
			EXTREME(_FLD, " purge");
			_limitVisListSize(*this, 0);
		}

		return -EINVAL;
	}

	// sort current update list, sets inUpdate to TRUE
	_weightCurrentVisList(*this);

	_dumpVisList("weighted", this->m_visList);

	// Only rebuild vis list once a second.
	if ( m_lastVisListUpdate <= teulClock64 - 400 )
	{
		m_lastVisListUpdate = teulClock64;
		_checkForNewVisListCandidates(*this);
	}

	_dumpVisList("with additions", this->m_visList);

	return 0;
}

bool SortViewingsByPriority(const Viewing* const lhs, const Viewing* const rhs)
{
	return lhs->m_priority < rhs->m_priority;
}

SINT32
HostVehicle::CreateWorldUpdate()
{
	WORLD_UPDATE& wu = *(this->m_worldUpdate);

	wu.header.random1 = static_cast<UINT8>(teulClock64 % 0x9e3779b9);

	if ( m_despawnTimerActive )
		wu.header.timerInfo = despawnTime().m_despawn;
	else
		wu.header.timerInfo = -1;
	wu.header.center = vehicleData.position;

	wu.header.framingMode = framingMode;
	wu.header.numVehicles = 0;
	wu.myVehicle = this;
	wu.header.actualVehiclesInRange = m_visListPressure;

	// A distant time guaranteed to be older than the oldest update
	// we might have sent.
	const TeulTimeMS epoch = (teulClock64 - (TeulTimeMS)(HOUR_S) * 1000ULL);

	const size_t visListSize = m_visList.size();
	const size_t maxVehiclesPerFrame = framings[framingMode].maxVehiclesPerFrame();
	VIS_DEBUG(_FLI, "framing:%u mv:%u mvpf:%u vls:%u", framingMode, framings[framingMode].maxVehicles, maxVehiclesPerFrame, visListSize);

	const bool avoidRepeats = host_config.update.avoidRepeats;

	if ( visListSize <= maxVehiclesPerFrame )
	{
		if ( avoidRepeats )
		{
			VIS_DEBUG(_FLI, " small, counting those with updates.");

			for ( size_t i = 0; i < visListSize; ++i )
			{
				Viewing& viewing = m_visList[i];
				if ( viewing.m_lastSent > 1 && viewing.m_vehicle.lastRX < viewing.m_lastSent )
					continue;
				wu.viewings[wu.header.numVehicles++] = &viewing;
			}
		}
		else
		{
			for ( size_t i = 0; i < visListSize; ++i )
			{
				wu.viewings[wu.header.numVehicles++] = &m_visList[i];
			}
		}

		VIS_DEBUG(_FLI, " going with %u", wu.header.numVehicles);

#if defined(ENABLE_VIS_DEBUG)
		if ( wu.header.numVehicles > 0 )
		{
			VIS_DEBUG(_FLI, "final list: ");
			for ( UINT32 i = 0; i < wu.header.numVehicles; ++i )
			{
				_dumpVisListEntry(*wu.viewings[i]);
			}
		}
#endif

		return wu.header.numVehicles;
	}

	/////
	///TODO:UPDATE+: We need to add extra code to do additional prioritization
	///TODO:UPDATE+: as and when the vislist reaches various sizes.
	///	if ( size > perFrame )
	///	{
	///		sortByTimeSinceLastUpdate
	/// 	if ( size > 2 x perFrame )
	///		{
	///			need to make sure enemies are getting updated,
	///			need to make sure people firing/moving get precedence next,
	///			if ( size > 3 x perFrame )
	///			{
	///				enemies within 250m need major prioritisation
	///			}

	std::vector<Viewing*> updateViewings;
	updateViewings.reserve(std::min(m_visList.size(), (size_t)framings[framingMode].maxVehicles));

	wu.header.numVehicles = 0;
	for ( UINT32 i = 0; i < m_visList.size(); ++i )
	{
		Viewing& viewing = m_visList[i];
		if ( avoidRepeats && viewing.m_vehicle.lastRX < viewing.m_lastSent )
			continue;

		//viewing.m_oneTimeBits = viewing.m_vehicle.getOneTimeBitsFor(this);

		// Top-priority stuff
		if ( viewing.m_priority < 0 || viewing.m_lastSent == 0 )
		{
			viewing.m_priority = -1;
		}
		else
		{
			// Otherwise the weight is the age of the last sent time.
			// subtract epoch so that more recent values are larger and thus lower priority.
			viewing.m_priority = (SINT64)(viewing.m_lastSent - epoch);

			// Try to prioritize vehicles that are firing.
			if ( viewing.m_vehicle.vehicleData.fire.masterClock64 > viewing.m_lastSent )
				viewing.m_priority /= 4;
		}

		updateViewings.push_back(&viewing);
	}

	// Size-up the final list.
	const size_t numVehicles = std::min(updateViewings.size(), maxVehiclesPerFrame);
	ABORT_IF(numVehicles > 0xff);

#if defined(ENABLE_VIS_DEBUG)
	dbugLog(_FLI, " final list: nv=min(%u, %u) = %u", updateViewings.size(), maxVehiclesPerFrame, numVehicles);
#endif

	// Sort the list into priority order and then whatever is at the preferred end of the list gets sent.
	std::sort(updateViewings.begin(), updateViewings.end(), SortViewingsByPriority);
	memcpy(wu.viewings, &updateViewings[0], sizeof(Viewing*) * numVehicles);

	wu.header.numVehicles = static_cast<UINT8>(numVehicles);

	_dumpVisList("vis list", this->m_visList);

	return wu.header.numVehicles;
}
