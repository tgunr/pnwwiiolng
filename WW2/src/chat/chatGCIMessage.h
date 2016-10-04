#pragma once
// Created: Sun Jan 13 18:57:18 CST 2008
//
// Chat::GCIMessage Structure.
// "GCI" = "Global Chat Interface", it was intended to be something
// along the lines of the LString system, but instead it only serves
// to send strat capture/ownership messages.
//
// Copyright (C) PlayNet, Inc 1999-2008 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "chat/chatDefines.h"			// For GCI_TYPE
#include "wwii/wwiiOwnershipEnums.h"	// For WWII_SIDE
#include "wwii/wwiiFullObjID.h"			// For FULL_OBJ_ID
#include "chat/chatGCITypes.h"

#pragma pack(push, 1)
namespace Chat
{
	struct GCIMessage
	{
		GCIMessage(GCI_TYPE gci_, WWII_SIDE side_, const FULL_OBJ_ID cp_, const FULL_OBJ_ID fac_, const char* name_)
			: gci(gci_), side(side_), where(cp_), facility(fac_)
		{
			strsafecpy(name, name_);
		}

		// Members
		GCI_TYPE		gci;
		WWII_SIDE		side;
		FULL_OBJ_ID		where;
		FULL_OBJ_ID		facility;
		SCHAR			name[64];
	};
}
#pragma pack(pop)

void chatSendGCIMessage(GCI_TYPE gci_, WWII_SIDE side_, const class CPStruct& cp, const FULL_OBJ_ID facID);
