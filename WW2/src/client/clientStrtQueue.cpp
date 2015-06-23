#include <syscfg.h> // Path is project specific.
#include <map>

#include "client/clientStrtQueue.h"
#include "trn/trnmain.h"
#include "client/client.h"
#include "client/GameClient.h"
#include "trn/trncd.h"
#include "trn/trnvars.h"
#include "strt/strtEventTypes.h"
#include "strt/strtCPFlags.h"
#include "inst/instDescriptor.h"
#include "miss/missionUtils.h"

/************************************************************************************************
	clientStrtQueue
*************************************************************************************************/

// Map of most recent vis level vs super-cell specific building ID.
//typedef std::map< UINT32 /* Building ID */, Strat::EventData /* strat info */ > BuildingStatus;

// Map of supercell <-> BuildingStatus map.
//typedef std::map< std::pair<int /*scx*/, int /*scy*/>, BuildingStatus > CellBuildingStatus;

//! All pending entities.
//static CellBuildingStatus _statusMap;


/************************************************************************************************
	_applyBuildingStatus
*************************************************************************************************/
/*
static void _applyBuildingStatus(TERRAIN_DETAIL* const td, UINT32 bldgId, SINT32 owner, SINT32 level)
{
	trnSetStratData(td, bldgId, owner, level);
}
*/

/************************************************************************************************
	_applyCellStatus
*************************************************************************************************/
/*
static void _applyCellStatus(TERRAIN_DETAIL* td, const BuildingStatus& cell)
{
	for ( BuildingStatus::const_iterator it = cell.begin(); it != cell.end(); ++it )
	{
		const UINT32 bldgId				= it->first;
		const Strat::EventData& data	= it->second;

		_applyBuildingStatus(td, bldgId, data.owner, data.level);
	}
}
*/

void clientStrtUpdateBuildings(TERRAIN_DETAIL* td)
{
	/*
	FULL_OBJ_ID fid;

	for(int i = 0; i < td->numBuildings; i++)
	{
		fid.cell.ox		= td->scx << 4;
		fid.cell.oy		= td->scy << 4;
		fid.id			= td->pbld[i].oid.id;
		fid.objClass	= td->pbld[i].oid.objClass;
		fid.objType		= td->pbld[i].oid.objType;

		StratDB::StratObject* building = StratDB::FindStratObject(fid);
		if(!building) continue;

		StratDB::StratObject* fac = building->ParentStratObject();;
		if(!fac) continue;

		trnSetStratData(td, (SINT32)td->pbld[i].oid.id, fac->Country(), building->VisLevel());
	}
	*/

	for(int i = 0; i < td->numBuildings; i++)
		trnSetStratData(td, (SINT32)td->pbld[i].oid.id);
}

/*
void clientStrtQueueUpdate()
{
	CellBuildingStatus::iterator it = _statusMap.begin();

	while ( it != _statusMap.end() )
	{
		const CellBuildingStatus::iterator cur = it;
		++it;

		TERRAIN_DETAIL* td = trnGetTerainDetail(cur->first.first, cur->first.second);
		if(!td) continue;

		_applyCellStatus(td, cur->second);

		_statusMap.erase(cur);
	}
}
*/

/************************************************************************************************
	clientStrtQueueReleaseSupercell
*************************************************************************************************/
/*
void clientStrtQueueReleaseSupercell(int sx, int sy)
{
	_statusMap.erase( std::make_pair(sx, sy) );
}
*/

/************************************************************************************************
	clientStrtCopyQuedData
*************************************************************************************************/
/*
void clientStrtCopyQuedData(TERRAIN_DETAIL* td)
{
	if(td == nullptr) return;

	CellBuildingStatus::iterator it =  _statusMap.find(std::make_pair(td->scx,td->scy));
	if(it == _statusMap.end()) return;

	_applyCellStatus(td, it->second);

	_statusMap.erase(it);
}
*/

/************************************************************************************************
	clientApplyStratEventData
*************************************************************************************************/

void clientApplyStratEventData(const FULL_OBJ_ID& fid)
{
	const int& sx = fid.cell.ox >> 4;
	const int& sy = fid.cell.oy >> 4;

	TERRAIN_DETAIL* const td = trnGetTerainDetail(sx, sy);
	if(td) trnSetStratData(td, fid.id);
}

/************************************************************************************************
	clientApplyStratEventData
*************************************************************************************************/

void clientApplyStratEventData(const Strat::EventData& d)
{
	const int& sx = d.fid.cell.ox >> 4;
	const int& sy = d.fid.cell.oy >> 4;

	TERRAIN_DETAIL* const td = trnGetTerainDetail(sx,sy);
	if(td) trnSetStratData(td, d.fid.id, d.owner, d.level);
}

/************************************************************************************************
	clientStrtFilterFlags
*************************************************************************************************/

#define ENEMY_EXCLUSION_MASK (CP_STAT_OWN_HAAC | CP_STAT_OVERRUN)

/*
CP_STAT_ENEMY_ARMOR | CP_STAT_ENEMY_INFANTRY | \
CP_STAT_ENEMY_SEACRAFT | CP_STAT_ENEMY_AIRCRAFT | CP_STAT_HEAVY_ENEMY_ARMOR | \
CP_STAT_HEAVY_ENEMY_INF | CP_STAT_HEAVY_ENEMY_SEA | CP_STAT_HEAVY_ENEMY_AIR)
*/

UINT32 clientStrtFilterFlags(WWII_SIDE side, UINT32 flags)
{
	if(!Online)
		return flags;

	if (PlayerClient()->GetInstance().category() > 1 )
	{
		///TODO:INST: Really need to return instanced flags.
		static const UINT32 instanceMask  = CP_STAT_HAS_NAVYBASE | CP_STAT_HAS_AIRBASE | CP_STAT_MAKES_RDP;
		static const UINT32 instanceFlags = CP_STAT_OPEN | CP_STAT_IN_SUPPLY;
		return (flags & instanceMask) | instanceFlags;
	}

	static const UINT32 sameSideOnlyMask = ENEMY_EXCLUSION_MASK;

	WWII_SIDE playerSide = wwiiSide(PlayerClient()->PersonaSelected().m_countryID);

	// Build a mask of flags that this player
	// should not be able to see.
	UINT32 exclusionMask = 0;

	if ( side != playerSide )
		exclusionMask |= sameSideOnlyMask;
	if ( playerSide != ALLIED )
		exclusionMask |= CP_ALLIED_OBJECTIVES;
	if ( playerSide != AXIS )
		exclusionMask |= CP_AXIS_OBJECTIVES;

	// And the flags with the inverse of the mask
	// so as to exclude the bits present in the mask.
	return flags & (~exclusionMask);
}