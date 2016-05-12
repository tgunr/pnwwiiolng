#pragma once
// Created: Thu Jul  5 14:07:28 2001
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson

SINT32 chatInstallClientManagementCallbacks();

//Logs into the chat server to do management stuff
bool chatSendManagementLogin2(SINT32 chatIndex, const char* gameName);
bool chatSendManagementLogin3(SINT32 chatIndex, const char* gameName, UINT16 country);
