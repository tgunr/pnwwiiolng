#pragma once

struct CP_OBJ_EVENT;
struct FULL_OBJ_ID;
struct STRAT_SUPERCELL_PACKET;
struct STRAT_CELL_PACKET;
namespace Strat { class EventPacket ; }

/************************************************************************************************
	clientStrat
*************************************************************************************************/

void clientStratApplyEvent(CP_OBJ_EVENT const& event);
void clientStratControlUpdate(FULL_OBJ_ID const& id, int ownership, UINT32 flags);
void clientStratLevelUpdate(FULL_OBJ_ID const& id, int vislevel);
void clientStratApplySuperCellStatus(STRAT_SUPERCELL_PACKET const&);
void clientStratApplyCellStatus(STRAT_CELL_PACKET const&);
void clientStratProcessEventPacket(const Strat::EventPacket& packet) ;
