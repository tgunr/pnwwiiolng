#pragma once
// Created: Thu Jun 15 16:17:47 2000
//
// The pack routines for the Authentication Server and client
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson

#ifndef FRONT_END

extern SINT32 authPackSERVER_LOAD(LOGICAL mode, UCHAR* data, SINT32 offset, struct AUTH_SERVER_LOAD* object, SINT32 size);

//This function packs/unpacks the game host hello message
extern SINT32 authPackGAME_INIT(LOGICAL mode, UCHAR* data, SINT32 offset, struct AUTH_GAME_INIT* init, SINT32 size);

//This function packs/unpacks the hello ack message
extern SINT32 authPackGAME_ACK(LOGICAL mode, UCHAR* data, SINT32 offset, struct AUTH_GAME_ACK* object, SINT32 size);

//This function packs/unpacks the init login message
extern SINT32 authPackClearLoginTable(LOGICAL mode, UCHAR* data, SINT32 offset, struct CLEAR_LOGIN_TABLE* object, SINT32 size);

#endif

extern SINT32 authPackAPP_LAUNCH_REQ2(LOGICAL mode, UCHAR* data, SINT32 offset, struct APP_LAUNCH_REQ* request, SINT32 size);

extern SINT32 authPackAPP_FILE_CHECK(LOGICAL mode, UCHAR* data, SINT32 offset, struct APP_FILE_CHECK* request, SINT32 size);

extern SINT32 authPackAPP_VERIFY(LOGICAL mode, UCHAR* data, SINT32 offset, struct APP_VERIFY* object, SINT32 size);

extern SINT32 authPackAPP_LAUNCH_REQ(LOGICAL mode, UCHAR* data, SINT32 offset, struct APP_LAUNCH_REQ* object, SINT32 size);

//This function takes out Login Reply structure and packs it or unpacks it (depreciated)
extern SINT32 authPackAPP_LAUNCH(LOGICAL mode, UCHAR* data, SINT32 offset, struct APP_LAUNCH* object, SINT32 size);

extern SINT32 authPackARENA_REQ(LOGICAL mode, UCHAR* data, SINT32 offset, struct ARENA_REQ* object, SINT32 size);
extern SINT32 authPackARENA_REQ2(LOGICAL mode, UCHAR* data, SINT32 offset, struct ARENA_REQ* object, SINT32 size);

