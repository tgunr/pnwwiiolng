#ifndef PN_PROJECT_SYSCFG_H
#define PN_PROJECT_SYSCFG_H 1
// Can't use a pragma guard because there are multiple syscfgs.
// This is the teulKit client syscfg.h
// Copyright 1999 Playnet Inc, All rights reserved
// NOTE : There is NO corresponding source file, this is purely an abstract
// header file.
// Originating Programmer : Stephen W. Evans

#if !defined(WIN32) && !defined(WIN64) // OS_WIN32 is not available before platforms.h

#define HAVE_UNISTD_H 1
#define HAVE_SYS_TIME_H 1
#define HAVE_SYSLOG_H 1
#define HAVE_SIGNAL_H 1
#define HAVE_ARPA_INET_H 1
#define HAVE_SIGACTION 1

#endif

#include "include/platforms.h"

#include "teul/teulBasic.h"
#include "teul/teulClosingEnums.h"
#include "teul/teulEndpointDefs.h"
#include "teul/teulEndpoint.h"
#include "teul/teulPackers.h"
#include "teul/teulPacket.h"
#include "weap/weapSpec.h"

#endif
