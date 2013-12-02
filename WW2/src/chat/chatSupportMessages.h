#pragma once
// Created: Mon Jul 31 13:29:38 2000
//
// The chat system host messages.  This module is actually included in the
// game host, not the chat host.
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson

struct MobileSpawnWaypoint;

//This function installs all the host callbacks
extern SINT32 chatInstallSupportCallbacks(void);

//This function sends a message to a client apprising it of the location of
//	the chat server.  To be used after chatSendPlayerInfo tells the chat
//	server about the client.
//	<teulIndex> is the index you want to send the information on
//	<OTP> is the otp to use
//	<ipAddress> is the address of the chat server
//	<port> is the port of the chat server
extern LOGICAL chatSendServerInfo(SINT32 teulIndex, UINT32 otp, UINT32 ipAddress, UINT16 port);

extern SINT32 chatSendMissionDeletionNotice(const UINT32& missionID);

extern SINT32 chatSendMissionMobileSpawnWaypoint(const MobileSpawnWaypoint& wp);
