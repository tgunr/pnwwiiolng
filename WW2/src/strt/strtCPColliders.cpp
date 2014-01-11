#include <syscfg.h> // Path is project specific.
//
// Created: Thu Mar  1 16:43:42 CST 2007
//
// Functions for building and managing the CP Collision tables
// and for detecting if a point is "within" a particular CP.
//
// The CPCollider system creates collision footprints for
// each CP in the SDMs based on the x/y position of facilities.
// This bounding boxes are then projected into a map indexed
// on <floor(x/2048), floor(y/2048)>.
//
// The map can then be used to quickly find CPs near a given
// co-ordinate in the world space by checking to see if there
// is an entry for <x/2048, y/2048>. If there is, it will
// contain a vector of pre-calculated bounding boxes to check
// the position against and so we can very quickly determine
// which CPs and facilities to measure distances against to
// tell what CP the player is near/overlapping.
//
// Copyright (C) PlayNet, Inc 1999-2007 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith
//

#include "dbug/dbug.h"							// For host "dbugLog" access

#include "wwii/wwiiHash.h"
#include "strt/strtCPStruct.h"
#include "strt/strtFacilityStruct.h"
#include "strt/strtSupercell.h"
#include "teul/teulTime.h"

class CPCollider
{
public:
	CPCollider() : x1(0x7fffffff), y1(0x7fffffff), x2(0x80000000), y2(0x80000000), cx(0), cy(0), cp(nullptr) {}
	SINT32	x1, y1;	// bottom left
	SINT32	x2, y2;	// top right
	SINT32	cx, cy;	// Center x/y
	CPStruct*	cp;
};

typedef std::pair<SINT32, SINT32> ColliderKey;
static const UINT32 COLLISION_GRID_SHIFT = 11;	// Each grid space is 2048^2m granularity
static const SINT32 COLLISION_MARGIN = 2000;	// Extend each bounding box out this far from town

namespace std
{
	template<>
	struct hash< ColliderKey >
	{
		size_t operator()(ColliderKey const& __x) const
		{
			return ((((size_t)__x.first)<<16) | ((size_t)__x.second));
		}
	};
}

typedef std::vector<CPCollider*> COLLIDERS;
typedef std::unordered_map<ColliderKey, COLLIDERS> CP_MAP;
static CP_MAP _cpMap;
extern CPStruct* _cpList;

//////////////////////////////////////////////////////////////////////
// Find the distance between meter-coordinates position and the center
// of a collider

static UINT64
_distance(const CPCollider& collider, SINT32 x, SINT32 y)
{
	const SINT64 xd = (x - collider.cx);
	const SINT64 yd = (y - collider.cy);
	return (UINT64)((xd * xd) + (yd * yd));
}

//////////////////////////////////////////////////////////////////////
// Test if co-ordinates overlap with a collider box

static inline bool
_collide(const CPCollider& collider, SINT32 x, SINT32 y)
{
	return ( x >= collider.x1 && x <= collider.x2 && y >= collider.y1 && y <= collider.y2 );
}

#ifdef PICKY
//////////////////////////////////////////////////////////////////////
// Given the upper/lower or left/right edges of a rectangle along a
// single axis, returns the distance from outside the edges to the
// closest edge. 0 indicates the point is between the edges along
// this axial plane.

static inline SINT64
_singleAxisDistance(SINT32 negativeEdge, SINT32 positiveEdge, SINT32 pointOnAxis)
{
	if ( negativeEdge > pointOnAxis )
	{
		// Point is below/left of the lower/left edge.
		return negativeEdge - pointOnAxis;
	}
	if ( pointOnAxis > positiveEdge )
	{
		// Point is above/right of the upper/right edge.
		return pointOnAxis - positiveEdge;
	}

	// Otherwise it is between the two edges
	return 0;
}

//////////////////////////////////////////////////////////////////////
// Calculate the distance^2 from a point to the closest edge of a
// bounding box.

static UINT64
_boxDistance2(const CPCollider& collider, SINT32 x, SINT32 y)
{
	// X1/Y1 are bottom left, X2/Y2 are top right
	SINT64 xd = _singleAxisDistance(collider.x1, collider.x2, x);
	SINT64 yd = _singleAxisDistance(collider.y1, collider.y2, y);
	return (UINT64)((xd * xd) + (yd * yd));
}

//////////////////////////////////////////////////////////////////////
// Distance from a facility

static UINT64
_facilityDistance(const FacilityStruct* facility, SINT32 x, SINT32 y)
{
	// X1/Y1 are bottom left, X2/Y2 are top right
	SINT64 xd = (x - facility->m_posX);
	SINT64 yd = (y - facility->m_posY);
	return (UINT64)((xd * xd) + (yd * yd));
}

//////////////////////////////////////////////////////////////////////
// Search a CP for the closest facility inside closestDist

static const FacilityStruct*
_checkFacilities(const CPStruct* cp, SINT32 x, SINT32 y, UINT64& closestDist, const FacilityStruct* closest)
{
	UINT64 dist;
	const FacilityStruct* fac;
	for ( fac = cp->facilityList; fac != NULL; fac = fac->next )
	{
		if ( fac->type == FACILITY_FIREBASE )
		{
			// Firebases are supposed to be far away from the home town,
			// so being near an FB doesn't count. EWS is about towns.
			continue;
		}
		dist = _facilityDistance(fac, x, y);
		if ( dist < closestDist )
		{
			closest = fac;
			closestDist = dist;
		}
	}

	return closest;
}
#endif

void
strtBuildCPColliders()
{
	teulSyncClock();
	TeulTimeMS startMs = teulClock64;

	CPStruct* cp;
	FacilityStruct* fac;
	for ( cp = _cpList; cp != nullptr; cp = cp->next )
	{
		if ( !cp->isCity() )
			continue;						// Bridges don't count.

		CPCollider* collider = new CPCollider;
		for ( fac = cp->facilityList; fac != nullptr; fac = fac->next )
		{
			if ( fac->type == FACILITY_FIREBASE )
			{
				// Again: Firebases are designed to be a long way away
				// from the CP they belong to, so they don't count.
				continue;
			}

			// Stretch the bounding box to encompass the position
			// of this facility.
			if ( fac->m_posX < collider->x1 )
				collider->x1 = fac->m_posX;
			if ( fac->m_posX > collider->x2 )
				collider->x2 = fac->m_posX;
			if ( fac->m_posY < collider->y1 )
				collider->y1 = fac->m_posY;
			if ( fac->m_posY > collider->y2 )
				collider->y2 = fac->m_posY;
		}

		collider->cp = cp;

		// Calculate the center of the bounding box.
		collider->cx = collider->x1 + (collider->x2 - collider->x1) / 2;
		collider->cy = collider->y1 + (collider->y2 - collider->y1) / 2;

		// The box we have has corners centered on individual facilities;
		// we add "COLLISION_MARGIN" to transform it into a larger
		// footprint.
		collider->x1 -= COLLISION_MARGIN;
		collider->x2 += COLLISION_MARGIN;
		collider->y1 -= COLLISION_MARGIN;
		collider->y2 += COLLISION_MARGIN;

		// Now point to this collider in every cell of the grid that
		// it overlaps with.
		SINT32 gx, gy;
		for ( gx = collider->x1 >> COLLISION_GRID_SHIFT;
			  gx <= collider->x2 >> COLLISION_GRID_SHIFT;
			  ++gx )
		{
			for ( gy = collider->y1 >> COLLISION_GRID_SHIFT;
				  gy <= collider->y2 >> COLLISION_GRID_SHIFT;
				  ++gy )
			{
				_cpMap[std::make_pair(gx,gy)].push_back(collider);
			}
		}
	}

	UINT32 cells = 0, entries = 0, overlaps = 0;
	for ( auto it = _cpMap.begin(), end = _cpMap.end(); it != end; ++it )
	{
		++cells;
		entries += it->second.size();
		if ( it->second.size() > 1 )
		{
			++overlaps;
		}
	}

	teulSyncClock();
	const TeulTimeMS endMs = teulClock64;

	dbugLog(_FLI, "colliders: %u cells, %u entries, %u overlaps, %f average/cell. Took %llums"
					, cells, entries, overlaps, ((FLOAT32)entries / (FLOAT32)cells)
					, endMs - startMs
					);
}

//////////////////////////////////////////////////////////////////////
// Given specific meter coordinates, find the closest CP

CPStruct*
strtFindClosestCP(SINT32 x, SINT32 y)
{
	CP_MAP::iterator it;
	it = _cpMap.find(std::make_pair(x>>COLLISION_GRID_SHIFT, y>>COLLISION_GRID_SHIFT));
	if ( it == _cpMap.end() )
		return nullptr;

	COLLIDERS& colliders = it->second;
	if ( colliders.size() == 1 && _collide(*colliders[0], x, y) )
		return colliders[0]->cp;

	// More than one CP, may be overlap
	bool gotCandidate = false;
	UINT32 candidate = 0;
#ifdef PICKY
	UINT32 candidate2 = 0;
#endif
	for ( UINT32 i = 0; i < colliders.size(); ++i )
	{
		if ( !_collide(*colliders[i], x, y) )
			continue;
		if ( gotCandidate )
		{
			UINT64 oldDist = _distance(*colliders[candidate], x, y);
			UINT64 newDist = _distance(*colliders[i], x, y);
			if ( oldDist <= newDist )
				continue;
#ifdef PICKY
			candidate2 = candidate;
#endif
		}

		gotCandidate = true;
		candidate = i;
	}

	if ( !gotCandidate )
		return nullptr;

#ifdef PICKY
	if ( candidate2 == 0 )
		return colliders[candidate]->cp;

	// Check facilities in candidate2 for something close to us
	UINT64 closestDist = (UINT64)-1LL;
	const FacilityStruct* closest;
	closest = _checkFacilities(colliders[candidate2]->cp, x, y, closestDist, closest);
	closest = _checkFacilities(colliders[candidate]->cp, x, y, closestDist, closest);

	if ( closest != NULL )
		return closest->cp;
#endif // PICKY

	return colliders[candidate]->cp;
}

