#include <syscfg.h> // Path is project specific.
// Copyright 1999 Playnet Inc, All rights reserved
// Not to be distributed in API form except as part of a compiled
// library
//
// Originating Programmer: Martin Poulin
// Supporting Programmer: Russell Mirabelli

#include "chat/chatCellHostSupportMessages.h"
#include "comm/commHostMessages.h"
#include "strt/strtUpdaterSupportMessages.h"
#include "wwii/wwiiHostMessages.h"

SINT32	hostInstallAllCallbacks(void)
{
SINT32 retval;

	if ( (retval = commInstallHostCallbacks()) < 0)
		return retval;

	if ( (retval = strtInstallUpdaterSupportCallbacks()) < 0)
		return retval;

	if ( (retval = chatInstallCellHostSupportCallbacks()) < 0)
		return retval;

//	if ( (retval = installTestCallbacks()) < 0)
//		return retval;

	if ( (retval = wwiiHostInstallCallbacks()) < 0)
		return retval;

	return retval;
}

