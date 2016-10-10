#pragma once
// Created: April 17 2002
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Marty "Vandal" Poulin

extern SINT32 wwiiClientInstallCallbacks();

extern SINT32 wwiiSendWWII_ERROR_INFO(SINT32 connection, struct WWII_ERROR_INFO* info);

#ifdef FRONT_END

extern SINT32 wwiiQueuedSend_WWII_ERROR_INFO(struct WWII_ERROR_INFO*);
extern SINT32 wwiiServiceQueue_WWII_ERROR_INFO();

extern SINT32 wwiiLog_WWII_ERR_INFO(struct WWII_ERROR_INFO*);
extern SINT32 wwiiSendLog_WWII_ERR_INFO();

#endif // FRONT_END

// Used by the covert and locality systems
extern void wwiiActivateCell(LOGICAL);
extern UINT32 wwiiGetUpdateNo(bool increment = true);
extern void wwiiResetUpdateNo();
extern UINT32 wwiiHitsPrime();
