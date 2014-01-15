#pragma once
// Created: Wed Jul 26 11:00:52 2000
//
// The main client-side API for the chat system
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Marty "Vandal" Poulin

extern	struct TEUL_CALLBACK*	authAPP_VERIFY;
extern	struct TEUL_CALLBACK*	authAPP_LAUNCH;

extern	struct TEUL_CALLBACK*	authDISPATCH;
extern	struct TEUL_CALLBACK*	authGAME_ACK;

//Initialization function declaration
SINT32 authHostInstallCallbacks(void);

SINT32 authSendAppPlaygateOk(SINT32 teulIndex);

