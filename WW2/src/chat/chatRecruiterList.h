#pragma once
// Created: Thu Jul  7 14:25:39 CDT 2005
//
// Mechanism for passing a list of Recruiters around
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Troy "RAMP" Lowe

#include "teul/teulNetworkArray.h"			// For teul_network_array
#include "wwii/wwiiSquads.h"				// For Recruiter

typedef teul_network_array<Recruiter> RECRUITER_LIST ;
