#pragma once

struct CP_OBJ_EVENT;
struct FULL_OBJ_ID;
struct STRAT_SUPERCELL_PACKET;
struct STRAT_CELL_PACKET;
namespace Strat { class EventPacket ; }

/************************************************************************************************
	clientStrat
*************************************************************************************************/

extern void clientStratApplyEvent(CP_OBJ_EVENT const& event);
extern void clientStratControlUpdate(FULL_OBJ_ID const& id, int ownership, UINT32 flags);
extern void clientStratLevelUpdate(FULL_OBJ_ID const& id, int vislevel);
extern void clientStratApplySuperCellStatus(STRAT_SUPERCELL_PACKET const&);
extern void clientStratApplyCellStatus(STRAT_CELL_PACKET const&);
extern void clientStratProcessEventPacket(const Strat::EventPacket& packet) ;
