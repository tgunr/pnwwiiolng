#pragma once
// Created: Sun Jun 19 15:07:27 CDT 2005
//
// Prototypes for mmSpawnGrantResponse.cpp
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

//handles the spawn grant response
extern LOGICAL mmProcessSpawnGrantResponse(const struct MM_SPAWN_GRANT_RESPONSE& spawnGrant);

//Request for a fake (command line client) spawn trigger
extern void mmFakeSpawnGrantResponse(class Player* player, UINT32 serialNumber);

