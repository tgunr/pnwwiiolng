#pragma once
/// @brief Provides the Strat::Events class.
/// Strat::Events describe various events that
/// need sending down to players.

#include "strt/strtProtos.h"
#include "wwii/wwiiFullObjID.h"

struct ACTION_OBJECT ;
struct TEUL_CALLBACK ;

namespace Strat
{
	namespace Events
	{
		/// Add a CP to the pending-events list.
		extern void Add(CPStruct* cp) ;
		/// Add a facility to the pending-events list.
		extern void Add(FacilityStruct* cp) ;
		/// Add an object to the pending-events list.
		extern void Add(ObjectStruct* cp) ;

		/// Send pending events to player.
		extern void Publish() ;

		/// Send map state to a given player.
		extern void SendMapState(SINT32 teulIndex, bool isCampaign, const class CPStruct* const cpList) ;

		extern SINT32 SendFunction(ACTION_OBJECT* const a) ;
		extern TEUL_CALLBACK* Callback ;
	}
}

