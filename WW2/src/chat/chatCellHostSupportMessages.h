#pragma once
// Created: Thu Jul 12 10:37:48 2001
//
// The code the updater uses to talk to the chat server
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson

//This function installs all the host callbacks
SINT32 chatInstallCellHostSupportCallbacks();

// Notify chat we have kicked someone
bool chatSendKickNotice(const playerid_t playerID, const SCHAR* callsign, const SCHAR* problem);

