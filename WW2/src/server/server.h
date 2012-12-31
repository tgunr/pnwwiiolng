#pragma once
// Created: Tue Jun 13 13:32:25 2000
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer:  Marty "Vandal" Poulin

//----> player functions

#include "server/serverReleaseTypeEnum.h"
extern SINT32 serverReleasePlayer(SINT32 teulIndex, UINT32 id, UINT32 otp, UINT32 exitCode, Server::ReleaseType releaseType);

extern UINT32 serverGetAuthIP() ;

// Look up the maximum allowable cluster users.
extern UINT32 serverGetMaxClusterUsers() ;

extern void serverEnableDebugWebServer(unsigned short int port_=0) ;

extern void serverInitAddresses(const UINT32 baseAddress) ;

extern void serverOpenBroadcastIndexes() ;

extern void serverBroadcastHostConfig() ;

extern size_t serverGetPlayerCount(const UINT32 serverType_) ;

// Find out how many players the cluster can support, adjusted
// by different host types and configuration parameters.
extern size_t serverGetClusterCapacity() ;

