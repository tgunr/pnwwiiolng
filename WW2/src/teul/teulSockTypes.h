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

#include "teulSockConfig.h"
