#pragma once
// Created: Tue Apr 17 14:42:47 2001
//
// Pack functions for the merged mmhost, including the things necessary to
// posting and spawning "weapons" in the new and improved multi-crewed
// vehicle world
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson

extern SINT32 mmPackPostedWeapon(LOGICAL mode, UCHAR* data, SINT32 offset, struct MM_POSTED_WEAPON* object, SINT32 size);
extern SINT32 mmPackFEPostRequest(LOGICAL mode, UCHAR* data, SINT32 offset, struct FE_POST_REQUEST* object, SINT32 size);
extern SINT32 mmPackFEPostAccept(LOGICAL mode, UCHAR* data, SINT32 offset, struct FE_POST_ACCEPT* object, SINT32 size);
extern SINT32 mmPackFEPostDeny(LOGICAL mode, UCHAR* data, SINT32 offset, struct FE_POST_DENY* object, SINT32 size);
extern SINT32 mmPackFEPostListRequest(LOGICAL mode, UCHAR* data, SINT32 offset, struct FE_POST_LIST_REQUEST* object, SINT32 size);
extern SINT32 mmPackFEPostListResponse(LOGICAL mode, UCHAR* data, SINT32 offset, struct FE_POST_LIST_RESPONSE* object, SINT32 size);
extern SINT32 mmPackFEJoinRequest(LOGICAL mode, UCHAR* data, SINT32 offset, struct FE_JOIN_REQUEST* object, SINT32 size);
extern SINT32 mmPackFEJoinAccept(LOGICAL mode, UCHAR* data, SINT32 offset, struct FE_JOIN_ACCEPT* object, SINT32 size);
extern SINT32 mmPackFEJoinDeny(LOGICAL mode, UCHAR* data, SINT32 offset, struct FE_JOIN_DENY* object, SINT32 size);
extern SINT32 mmPackFEPostExpired(LOGICAL mode, UCHAR* data, SINT32 offset, struct FE_POST_EXPIRED* object, SINT32 size);
extern SINT32 mmPackFEJoinPermRequest(LOGICAL mode, UCHAR* data, SINT32 offset, struct FE_JOIN_PERM_REQUEST* object, SINT32 size);
extern SINT32 mmPackFECrewRequest(LOGICAL mode, UCHAR* data, SINT32 offset, struct FE_CREW_REQUEST* object, SINT32 size);
extern SINT32 mmPackFECrewAccept(LOGICAL mode, UCHAR* data, SINT32 offset, struct FE_CREW_ACCEPT* object, SINT32 size);
extern SINT32 mmPackCommandedWeapon(LOGICAL mode, UCHAR* data, SINT32 offset, struct COMMANDED_WEAPON* object, SINT32 size);
extern SINT32 mmPackFEJoinPermKill(LOGICAL mode, UCHAR* data, SINT32 offset, struct FE_JOIN_PERM_KILL* object, SINT32 size);
extern SINT32 mmPackFEPostCancel(LOGICAL mode, UCHAR* data, SINT32 offset, struct FE_POST_CANCEL* object, SINT32 size);
extern SINT32 mmPackFEClientSpawn(LOGICAL mode, UCHAR* data, SINT32 offset, struct FE_CLIENT_SPAWN* object, SINT32 size);
extern SINT32 mmPackFECrewBailed(LOGICAL mode, UCHAR* data, SINT32 offset, struct FE_CREW_BAILED* object, SINT32 size);
