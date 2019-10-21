#pragma once
// Created: Tue 03 Feb 2004, 11:53:00
// Copyright (c) 2004 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Protos etc for infoHostMessages.cpp
//
// Originating Programmer: Oliver "kfsone" Smith

// Ask the client to send us a computer info packet
extern SINT32 infoSendCompInfoRequest(SINT32 connection, const char* message);

// Install the callbacks for dealing with non-auth playgate connections
extern SINT32 infoInstallHostCallbacks();
