#pragma once
// Created: April 17 2002
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Marty "Vandal" Poulin

extern SINT32 wwiiHostInstallCallbacks(void);
extern SINT32 wwiiSendDialog(SINT32 connection, UINT32 dialog) ;
extern SINT32 wwiiSendActivate(SINT32 connection, struct WWII_ACTIVATE* activate);

