#pragma once
// Created: Mon Jun 26 17:28:54 2000
//
// This contains the routine necessary to turn the auth server (and later,
// any server) into a daemon process
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson

//This function turns the caller into a daemon.  onSigTerm is a function
//	that will be assigned to handle SIGTERMS
typedef void (ERROR_FUNCTION) (SINT32 teulIndex, const UCHAR* const data, SINT32 size, const struct TEUL_CALLBACK* const object, SINT32 error);
extern void authInitDaemon(ERROR_FUNCTION* exitHandler);

