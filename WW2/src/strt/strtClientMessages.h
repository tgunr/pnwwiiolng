#pragma once
// Created: Fri Nov  3 13:55:07 2000
//
// This module contains the setup for the Client API that allows the
// front-end to communicate with the strat system.
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson

//Installs all the client callbacks
extern SINT32 strtInstallClientCallbacks(void);
extern SINT32 strtSendEnemyReport(SINT32 teulIndex, struct STRAT_ENEMY_REPORT* report);
