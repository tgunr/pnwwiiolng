#pragma once

struct TERRAIN_DETAIL;
namespace Strat
{
	class Event;
	class EventPacket;

	struct EventData
	{
		FULL_OBJ_ID fid;
		SINT32 owner;
		SINT32 level;
	};
}

void clientApplyStratEventData(const FULL_OBJ_ID);
void clientApplyStratEventData(const Strat::EventData&);
void clientStrtUpdateBuildings(struct TERRAIN_DETAIL*);
UINT32 clientStrtFilterFlags(WWII_SIDE side, UINT32 flags);
