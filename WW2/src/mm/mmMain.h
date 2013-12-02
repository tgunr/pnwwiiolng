#pragma once
// Created: Thu Jan 18 08:39:28 2001
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Brian "Stick" Duelm

extern UINT32 changeToBroadcast(UINT32 address);
extern SINT32 mmSendToChat(struct TEUL_CALLBACK* callback, void* data);

extern SINT32 mapGetStratIndex(void);
extern void mapSendActivate(SINT32 teulIndex, const playerid_t playerID, const char* callsign);

