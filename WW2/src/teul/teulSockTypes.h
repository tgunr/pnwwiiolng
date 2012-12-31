#pragma once
// This module provides high-level access to the transport
// functions used in the teul API.
//
// Copyright 1999 Playnet Inc, All rights reserved
// Not to be distributed in API form except as part of a compiled
// library
//
// Originating Programmer: Martin Poulin

#if !defined(WIN32) && !defined(WIN64)
#include <sys/socket.h>
#include <netinet/in.h>
#else
#include <winsock2.h>
#endif

#if platOS == OS_MACOS
	#include <sys/types.h>
#endif

#if !defined(WIN32) && !defined(WIN64)
	#define SOCKET_ERROR -1
	#define INVALID_SOCKET  -1
	typedef int				SOCKET;
/*
#if platOS == OS_FREEBSD
	typedef SINT32 sockval_t;
#endif
*/

#else // platOS != OS_WIN32
	#define socklen_t SINT32 FAR
	#define sockval_t SINT32 FAR
#endif

#if platOS == OS_MACOS
	#include <signal.h>
	#include <sys/ioctl.h>
	#include <fcntl.h>
	#include <unistd.h>
	#define sockval_t SINT32

#elif platOS == OS_LINUX
	#ifndef sockval_t
		#define sockval_t char
	#endif

#endif

