#include <syscfg.h> // Path is project specific.
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

void
CallbackRegister::_error(SINT32 teulIndex, const UCHAR* const data, SINT32 size, const TEUL_CALLBACK* const object, SINT32 error)
{
	dbugLog(_FLA, "t:%d error %d in teulCallback %p data %p size %d", teulIndex, error, static_cast<const void*>(object), data, size);
}

CallbackRegister::CallbackRegister(UCHAR callX)
{
	sm_ci.callX = callX;
	sm_ci.error = CallbackRegister::_error;
}

TEUL_CALLBACK*
CallbackRegister::callback(UCHAR callY, ACTION_FN sendFn, ACTION_FN recvFn)
{
	try
	{
		sm_ci.callY = callY;
		sm_ci.sendAction = sendFn;
		sm_ci.recvAction = recvFn;
		TEUL_CALLBACK* const newRef = teulAllocCallback(&sm_ci);

		if ( newRef == NULL )
			throw std::logic_error("callback allocation failed");

		return newRef;
	}
	catch (std::exception& e)
	{
		char buf[201];
		strprintf(buf, "Unable to register %u/%u: %s", sm_ci.callX, callY, e.what());
		throw std::runtime_error(buf);	///Terminal
	}
}
