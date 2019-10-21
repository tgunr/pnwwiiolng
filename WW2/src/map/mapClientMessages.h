#pragma once
// Date: Thu Jan 25 11:04:11 2001
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.

//This is the function to call before any of the others.
//Installs all the necessary callbacks for the map system
extern SINT32 mapInstallClientCallbacks();

extern SINT32 mapSendExitWWIIOL(SINT32 hostIndex);

extern void mapSendOpinionPollResult(SINT32 teulIndex, UINT32 pollID, UINT32 entryID);

extern SINT32 mapQueuedSend_WWII_ERROR_INFO(struct WWII_ERROR_INFO*);
extern SINT32 mapServiceQueue_WWII_ERROR_INFO();
extern SINT32 mapLog_WWII_ERR_INFO(struct WWII_ERROR_INFO*);
extern SINT32 mapSendLog_WWII_ERR_INFO();
