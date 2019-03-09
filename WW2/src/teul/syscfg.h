#ifndef PN_PROJECT_SYSCFG_H
#define PN_PROJECT_SYSCFG_H 1
// Can't use a pragma guard because there are multiple syscfgs.
// This is the teulKit client syscfg.h
// Copyright 1999 Playnet Inc, All rights reserved
// NOTE : There is NO corresponding source file, this is purely an abstract
// header file.
// Originating Programmer : Stephen W. Evans

#ifdef _DEBUG
	#define NO_DEBUG_WRITE
	#define NO_DEBUG_LOG
#endif

#include "include/platforms.h"

#include "teul/teulBasic.h"
#include "teul/teulClosingEnums.h"
#include "teul/teulEndpointDefs.h"
#include "teul/teulEndpoint.h"
#include "teul/teulPackers.h"
#include "teul/teulPacket.h"
#include "weap/weapSpec.h"

#define WriteDebug(...)

#endif
