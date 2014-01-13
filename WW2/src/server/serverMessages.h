#pragma once
// Created: Tue Jun 13 13:32:25 2000
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a compiled library.
//
// Originating Programmer:  Marty "Vandal" Poulin

//#include "server/serverTypes.h"
//#include "server/serverFuncTypes.h"

extern TEUL_CALLBACK*	SERVER_HEARTBEAT_CB;
extern TEUL_CALLBACK*	PERSONA_EXIT_CB;
extern TEUL_CALLBACK*	PLAYER_MIRROR_CB;
extern TEUL_CALLBACK*	CLUSTER_SHUTDOWN_CB;

SINT32 serverInitCallbacks(void);

