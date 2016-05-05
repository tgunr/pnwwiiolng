#pragma once
//! @file teulSockConfig.h
//! @brief Cross-platform socket manipulation includes, macros etc.
//
// Copyright 1999-2011 Playnet Inc, All rights reserved
// Not to be distributed in API form except as part of a compiled
// library
//
// Originating Programmer: Martin Poulin

#if platOS == OS_WIN32

	#define socklen_t SINT32 FAR
	#define sockval_t SINT32 FAR

#else

	#include <signal.h>
	#include <sys/ioctl.h>
	#include <fcntl.h>
	#include <unistd.h>
	#define sockval_t int
	#define closesocket close
	#define ioctlsocket ioctl

#endif

#define INVALID_SOCKET  -1
#define SOCKET_ERROR -1
typedef int				SOCKET;
#define PRF_SOCKET "%d"
#define PRA_SOCKET(s) int(s)
