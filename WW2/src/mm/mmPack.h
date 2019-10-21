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

extern SINT32 mmPackPostedWeapon(PackMode mode, UCHAR* data, SINT32 offset, struct MM_POSTED_WEAPON* object, SINT32 size);
extern SINT32 mmPackFEPostRequest(PackMode mode, UCHAR* data, SINT32 offset, struct FE_POST_REQUEST* object, SINT32 size);
extern SINT32 mmPackFEPostAccept(PackMode mode, UCHAR* data, SINT32 offset, struct FE_POST_ACCEPT* object, SINT32 size);
extern SINT32 mmPackFEPostDeny(PackMode mode, UCHAR* data, SINT32 offset, struct FE_POST_DENY* object, SINT32 size);
extern SINT32 mmPackFEPostListRequest(PackMode mode, UCHAR* data, SINT32 offset, struct FE_POST_LIST_REQUEST* object, SINT32 size);
extern SINT32 mmPackFEPostListResponse(PackMode mode, UCHAR* data, SINT32 offset, struct FE_POST_LIST_RESPONSE* object, SINT32 size);
extern SINT32 mmPackFEJoinRequest(PackMode mode, UCHAR* data, SINT32 offset, struct FE_JOIN_REQUEST* object, SINT32 size);
extern SINT32 mmPackFEJoinAccept(PackMode mode, UCHAR* data, SINT32 offset, struct FE_JOIN_ACCEPT* object, SINT32 size);
extern SINT32 mmPackFEJoinDeny(PackMode mode, UCHAR* data, SINT32 offset, struct FE_JOIN_DENY* object, SINT32 size);
extern SINT32 mmPackFEPostExpired(PackMode mode, UCHAR* data, SINT32 offset, struct FE_POST_EXPIRED* object, SINT32 size);
extern SINT32 mmPackFEJoinPermRequest(PackMode mode, UCHAR* data, SINT32 offset, struct FE_JOIN_PERM_REQUEST* object, SINT32 size);
extern SINT32 mmPackFECrewRequest(PackMode mode, UCHAR* data, SINT32 offset, struct FE_CREW_REQUEST* object, SINT32 size);
extern SINT32 mmPackFECrewAccept(PackMode mode, UCHAR* data, SINT32 offset, struct FE_CREW_ACCEPT* object, SINT32 size);
extern SINT32 mmPackCommandedWeapon(PackMode mode, UCHAR* data, SINT32 offset, struct COMMANDED_WEAPON* object, SINT32 size);
extern SINT32 mmPackFEJoinPermKill(PackMode mode, UCHAR* data, SINT32 offset, struct FE_JOIN_PERM_KILL* object, SINT32 size);
extern SINT32 mmPackFEPostCancel(PackMode mode, UCHAR* data, SINT32 offset, struct FE_POST_CANCEL* object, SINT32 size);
extern SINT32 mmPackFEClientSpawn(PackMode mode, UCHAR* data, SINT32 offset, struct FE_CLIENT_SPAWN* object, SINT32 size);
extern SINT32 mmPackFECrewBailed(PackMode mode, UCHAR* data, SINT32 offset, struct FE_CREW_BAILED* object, SINT32 size);
