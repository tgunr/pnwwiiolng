#pragma once
// Created: Wed Jul 26 11:00:52 2000
//
// The main client-side API for the games to interface with the authHost
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Marty "Vandal" Poulin

#define AUTH_SEQ "Authentication Sequence Copyright (c)1999 Playnet Inc. All rights reserved."
#define AUTH_SEQ_SIZE	sizeof(AUTH_SEQ)

struct PLAYER_LOGIN
{
	UINT32 attemptID ;			// Client's login attempt identifier.
	UINT32 appVersion;			// version of application that is connecting
	UINT32 OTP;					// One Time passWord
	UCHAR CopyRightString[AUTH_SEQ_SIZE];

	SINT32 packer(LOGICAL mode, UCHAR* const message, SINT32 offset) ;
} ;

struct PLAYER_LOGIN_REPLY
{
	UINT32	attemptID ;			// ID of the attempt.
	SINT32	resultCode ;		// Attempt result.
} ;
