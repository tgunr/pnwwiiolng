#pragma once

struct TERRAIN_DETAIL;
namespace Strat
{
	class Event ;
	class EventPacket ;

	struct EventData
	{
		FULL_OBJ_ID fid;
		SINT32 owner;
		SINT32 level;
	};
}

extern void clientApplyStratEventData(const FULL_OBJ_ID&);
extern void clientApplyStratEventData(const Strat::EventData&);
extern void clientStrtUpdateBuildings(struct TERRAIN_DETAIL*);
extern UINT32 clientStrtFilterFlags(WWII_SIDE side, UINT32 flags);
