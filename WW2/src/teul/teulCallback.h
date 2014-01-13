#pragma once
//	@file teul/teulCallback.h
//	@brief Management functions and definitions for the teul callback matrix
//
//	Copyright 1999 Playnet Inc, All rights reserved
//	Not to be distributed in API form except as part of a compiled library
//
//	Originating Programmer: Russell J. Mirabelli

#include "teul/teulFuncTypes.h"

namespace Teul
{
	struct CallbackInit;

	enum { RestrictedCallbackRange_X = 15 };
}

void teulFreeAllCallbacks(void);

TEUL_CALLBACK* teulAllocSystemCallback(Teul::CallbackInit* ci );

//!	@brief Creates and installs a callback object into the matrix at location X,Y.
TEUL_CALLBACK* teulAllocCallback(struct Teul::CallbackInit* const ci);
