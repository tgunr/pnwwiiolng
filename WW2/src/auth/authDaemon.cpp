#include <syscfg.h> // Path is project specific.
// Created: Mon Jun 26 17:33:37 2000
//
// This library contains all the code to turn a process into a daemon
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson

#include "dbug/dbug.h"

#include <signal.h>
#include "auth/authDaemon.h"

typedef void (SIG_FUNCTION) ( SINT32 signo );

static ERROR_FUNCTION* _userExitHandler = NULL;

static void
_sigTermHandler(int /*signo*/)
{
	_userExitHandler (0, NULL, 0, NULL, SIGTERM);
//	exit(0);
}

void
authInitDaemon(ERROR_FUNCTION* exitHandler)
{
//pid_t	pid;

	_userExitHandler = exitHandler;

	//Then usurp SIGTERM.  We setup the function they pass into us
	//	as our SIGTERM handler

	if (signal(SIGTERM, _sigTermHandler) == SIG_ERR)
	{
		dbugLog(_FLA,"can't catch SIGTERM");
		exit(1);
	}

	if (signal(SIGINT, _sigTermHandler) == SIG_ERR)
	{
		dbugLog(_FLA,"can't catch SIGTERM");
		exit(1);
	}
}

