#include <syscfg.h> // Path is project specific.
// Date: Fri May 25 16:13:37 2001
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson
// Supporting Programmer:  Eric "Meat" Hall

#include "host/hostVehicleRequest.h"
#include "mm/mmConstants.h"
#include "wwii/wwiiCallbacks.h"

TEUL_CALLBACK*	Host::VehicleRequest::s_callback ;

namespace Scoring
{
	TEUL_CALLBACK* xchgEventCallback = NULL ;
}

SINT32
mmCellHostSupportCallbacks(void)
{
	BEGIN_CALLBACK_REGISTRATION(MMHost::CallbackX)

		REGISTER_RECEIVER(MMHost::WEAPONS_HANDOFF_Y, Host::VehicleRequest::_recvCallback) ;

	END_CALLBACK_REGISTRATION
}

