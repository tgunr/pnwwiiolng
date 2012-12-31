#include <syscfg.h> // Path is project specific.
#include "include/vars.h"
#include "sdm/StratDB.h"
#include "updt/eventTypes.h"
#include "wwii/wwiiTypes.h"
#include "client/clientStrat.h"
#include "strt/strtCPFlags.h"
#include "strt/strtEventTypes.h"

static void SetChildOwnership(StratDB::StratObject* parent);

/************************************************************************************************
	clientStrat
*************************************************************************************************/

void clientStratApplyEvent(CP_OBJ_EVENT const& event)
{
	if (event.type == WWII_CP_LEVEL)
	{
		clientStratLevelUpdate(event.object, event.actionData);
	}else if (event.type == WWII_CP_CONTROL)
	{
		clientStratControlUpdate(event.object, event.actionData, event.flags);
	}
}

void clientStratLevelUpdate(FULL_OBJ_ID const& id, int vislevel)
{
	StratDB::StratObject* stratobj = StratDB::FindStratObject(id);
	if(!stratobj) return;

	stratobj->SetVisLevel(vislevel);
	stratobj->SetUpdateTime(frametime);
}

void clientStratControlUpdate(FULL_OBJ_ID const& id, int ownership, UINT32 flags)
{
	StratDB::StratObject* stratobj = StratDB::FindStratObject(id);
	if(!stratobj) return;

	stratobj->SetCountry(ownership);
	stratobj->SetUpdateTime(frametime);

	if ( flags != (UINT32)~0 ) // -1 means we're not chaning the flags
		stratobj->SetStratFlags((UINT32)flags);

	SetChildOwnership(stratobj);
}

void clientStratProcessEventPacket(const Strat::EventPacket& packet)
{
	const size_t numEvents = packet.size() ;
	for ( size_t i = 0 ; i < numEvents ; ++i )
	{
		const Strat::Event& event = incoming[i] ;
		StratDB::StratObject* const stratobj = StratDB::FindStratObject(id) ;
		if ( !stratobj )
			return ;
		switch ( stratobj->StratClass() )
		{
		case SDM_CLASS_CP:
			stratobj->SetCountry(event.ownership()) ;
			stratobj->SetStratFlags(event.flags()) ;
			stratobj->SetUpdateTime(frametime) ;
		break ;
		case SDM_CLASS_FACILITY:
			stratobj->SetCountry(event.ownership()) ;
			stratobj->SetStratFlags(event.flags()) ;
			stratobj->SetUpdateTime(frametime) ;
			SetChildOwnership(stratobj) ;
		break ;
		case SDM_CLASS_BUILDING:
			stratobj->SetVisLevel(event.flags()) ;
		break ;
		default:
		}
	}
}

static void SetChildOwnership(StratDB::StratObject* parent)
{
	StratDB::StratObject::ChildList children = parent->ChildStratObjects();
	StratDB::StratObject::ChildList::iterator it = children.begin();
	for(; it != children.end(); ++it)
	{
		StratDB::StratObject* stratobj = *it;
		stratobj->SetCountry(parent->Country());
		stratobj->SetUpdateTime(frametime);
		if(	stratobj->StratClass() == SDM_CLASS_FACILITY &&
			stratobj->StratType() == SDM_FACILITY_FIREBASE )
		{
			stratobj->SetStratFlags(stratobj->StratFlags() & ~CP_STAT_OPEN);
			stratobj->SetUpdateTime(frametime);
		}
	}
}

void clientStratApplySuperCellStatus(STRAT_SUPERCELL_PACKET const& ssp)
{
	StratDB::LoadSuperCell(ssp.superPoint.sx, ssp.superPoint.sy);

	StratDB::StratObject* stratobj;
	StratDB::StratObjList objlist;
	StratDB::StratObjList::iterator it;

	StratDB::FacilitiesInSuperCell(objlist, ssp.superPoint.sx, ssp.superPoint.sy);
	if(objlist.size() != (size_t)ssp.numFacilities)
	{
//		printf("facility update mismatch! %d %d\n", ssp.numFacilities, objlist.size());
		return;
	}
	it = objlist.begin();
	for(int i = 0; it != objlist.end(); i++, ++it)
	{
		stratobj = *it;
		if(!stratobj) continue;
		FAC_INFO const& facinfo = ssp.finfo[i];

		stratobj->SetCountry(facinfo.ownership);
		stratobj->SetStratFlags(facinfo.flags);
		stratobj->SetUpdateTime(frametime);

		SetChildOwnership(stratobj);
	}

	StratDB::CPsInSuperCell(objlist, ssp.superPoint.sx, ssp.superPoint.sy);
	if(objlist.size() != (size_t)ssp.numCPs)
	{
//		printf("cp update mismatch! %d %d\n", ssp.numCPs, objlist.size());
		return;
	}
	it = objlist.begin();
	for(int i = 0; it != objlist.end(); i++, ++it)
	{
		stratobj = *it;
		if(!stratobj) continue;

		FAC_INFO const& cpinfo = ssp.cpinfo[i];

		stratobj->SetCountry(cpinfo.ownership);
		stratobj->SetStratFlags(cpinfo.flags);
		stratobj->SetUpdateTime(frametime);
	}
}

void clientStratApplyCellStatus(STRAT_CELL_PACKET const& scp)
{
	int sx = scp.octetPoint.ox >> 4;
	int sy = scp.octetPoint.oy >> 4;
	int ox =  scp.octetPoint.ox & 0x0f;
	int oy =  scp.octetPoint.oy & 0x0f;

	StratDB::StratObjList bldlist;
	StratDB::BuildingsInOctet(bldlist, sx, sy, ox, oy);

	if(scp.buildingOffset + scp.numBuildings > (int)bldlist.size())
	{
//		printf("cell packet update mismatch! %d %d %d %d\n", scp.buildingOffset
//				, scp.numBuildings, scp.buildingOffset + scp.numBuildings, bldlist.size() );
		return;
	}

	for (int i = 0; i < scp.numBuildings; i++)
	{
		StratDB::StratObject* stratobj = bldlist[i + scp.buildingOffset];
		stratobj->SetVisLevel(scp.binfo[i].level);
//printf("clientStratApplyCellStatus %s vislevel: %d\n", stratobj->Name(), stratobj->VisLevel());
		stratobj->SetUpdateTime(frametime);
	}
}
