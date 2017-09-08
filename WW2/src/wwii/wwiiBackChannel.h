#pragma once
// Created: Fri Sep 29 18:09:45 CST 2006
//
// Management of the 'backchannel' connection via raknet
// Currently the backchannel system is a 1-per-game-instance connection
// to the cell hosts using Raknet's reliable ordered UDP.
//
// Copyright (C) PlayNet, Inc 1999-2017 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith
// 
// Ask the back channel system to open a connection to a particular host,
// using the credentials supplied. One back channel per player client.
// Backchannel system is not currently encapsulated to handle multiple
// connections per client

extern bool wwiiBackChannelConnect(SINT32 hostTeulIndex, const char* serverIP, UINT16 port, SINT32 teulIndex, UINT32 playerId, UINT32 otp, char * packedPublicKey) ;

// Disconnect from the current back channel
extern void wwiiBackChannelDisconnect( SINT32 teulIndex ) ;
