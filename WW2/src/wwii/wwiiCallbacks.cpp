#include "syscfg.h" // Path is project specific.
// Created: Mon Mar 21 12:48:09 CST 2005
//
// Wrapper for simplifying the process of creating teulCallbacks
//
// Copyright (C) PlayNet, Inc. 2005 - All rights reserved.
// Not for redistribution without permission as part of an API.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "wwii/wwiiCallbacks.h"

Teul::CallbackInit CallbackRegister::sm_ci;

CallbackRegister::CallbackRegister(UCHAR x, const char * const name)
{
	sm_ci.callX = x;
	sm_ci.callXName = name;
}

TEUL_CALLBACK*
CallbackRegister::callback(UCHAR callY, const char * const callYName, ACTION_FN sendFn, ACTION_FN recvFn)
{
	try
	{
		sm_ci.callY = callY;
		sm_ci.callYName = callYName;
		sm_ci.sendAction = sendFn;
		sm_ci.recvAction = recvFn;
		TEUL_CALLBACK* const newRef = teulAllocCallback(&sm_ci);

		if ( newRef == nullptr )
			throw std::logic_error("callback allocation failed");

		return newRef;
	}
	catch (std::exception& e)
	{
		char buf[201];
		strprintf(buf, "Unable to register %u/%u: %s", sm_ci.callX, callY, e.what());
		throw std::runtime_error(buf); ///Terminal
	}
}
