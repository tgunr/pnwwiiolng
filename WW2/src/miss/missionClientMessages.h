#pragma once
// Created: Tue Jan 23 21:04:52 2001
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Brian "Stick" Duelm

#include "wwii/wwiiFullObjID.h"

extern struct TEUL_CALLBACK*	missSEND_INFO_REQ;
extern struct TEUL_CALLBACK*	missSEND_ACCEPT;

extern SINT32 missionInstallCallbacks();

extern SINT32 missionSendPostRequest(SINT32 hostIndex, struct MISSION_POST_REQ const* request);

extern SINT32 missionSendWeaponListRequest(SINT32 hostIndex, FULL_OBJ_ID facility, LOGICAL allWeapons, LOGICAL isRefresh);
extern void missionSendDetailRequest(SINT32 hostIndex, UINT32 missionID, bool joinRequest);

extern SINT32 missionSendExit(SINT32 hostIndex, UINT32 cancelMissionID=0);

//! Re-request mission results from the host.
//! @param[in] teulIndex The teulindex to send the request on (map connection).
//! @param[in] sortieID The sortie ID we want details for (should be last sortie).
//! @returns Negative error code on error, otherwise 0 or greater.
extern SINT32 missRequestMissionResults(const SINT32 teulIndex, const UINT32 sortieID);
