#pragma once
// Date: Mon May 21 16:27:01 2001
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson

//Installs the callbacks
SINT32 strtUpdateHostInstallCallbacks();

//Sends a status packet to the FE
struct STRAT_STATUS_PACKET;
SINT32 strtSendStatusPacket(SINT32 teulIndex, STRAT_STATUS_PACKET* packet);
