#pragma once
// Created: Mon Jan 22 10:21:18 2001
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Brian "Stick" Duelm

extern SINT32 missPackMISSION_POST_REPLY(LOGICAL mode, UCHAR* data, SINT32 offset, struct MISSION_POST_REPLY* object, SINT32 size);
extern SINT32 missPackMISSION_POST_REQ(LOGICAL mode, UCHAR* data, SINT32 offset, struct MISSION_POST_REQ* object, SINT32 size);
extern SINT32 missPackMISSION_SUMMARY(LOGICAL mode, UCHAR* data, SINT32 offset, struct MISSION_SUMMARY* object, SINT32 size);
extern SINT32 missPackWEP_LIST_REQ(LOGICAL mode, UCHAR* data, SINT32 offset, struct WEP_LIST_REQ* object, SINT32 size);

extern SINT32 missPackMISSION_RESULTS(LOGICAL mode, UCHAR* data, SINT32 offset, struct MISSION_RESULTS* object, SINT32 size);

extern SINT32 missPackMISS_ROOM_LIST(LOGICAL mode, UCHAR* data, SINT32 offset, class RoomList* object, SINT32 size);

extern SINT32 missPackMISS_LAUNCH_UPDATE(LOGICAL mode, UCHAR* data, SINT32 offset, struct MISS_LAUNCH_UPDATE* object, SINT32 size);

