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

extern struct TEUL_CALLBACK*	authARENA_REQ;
extern struct TEUL_CALLBACK*	authAPP_LAUNCH_REQ;

extern SINT32 authLaunchApp(struct APP_LAUNCH* reply);
extern SINT32 authVerifyApp(SINT32 teulIndex, struct APP_VERIFY* reply);

extern SINT32 authClientInstallCallbacks();
