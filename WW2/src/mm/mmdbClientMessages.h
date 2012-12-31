#pragma once
// Created: Thu May 10 11:15:55 2001
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Eric "Meat" Hall

#include "wwii/wwiiOwnershipEnums.h"		// For WWII_SIDE

extern SINT32 mmdbInstallClientCallbacks();
extern LOGICAL mmdbRequestMissionList(void) ;
extern void mmdbSendPlayerSideLock(UINT32 pid, WWII_SIDE side, UINT32 locktime) ;
extern void mmdbRequestBrigadeDetailsUpdate() ;
extern void mmdbForceBrigadeDetailsUpdate() ;
extern void mmdbInitializeBrigadeLocations() ;

