#include "syscfg.h" // Path is project specific.
// This module provides support utilities used in
// the teul API. The end-user developer
// should not need the functions provided here.
//
// Copyright 1999 Playnet Inc, All rights reserved
// Not to be distributed in API form except as part of a compiled
// library
//
// Originating Programmer: Russell Mirabelli

#include "mem/mem.h"
#include "teul/teulCallback.h"
#include "teul/teulCallbackInit.h"
#include "teul/teulCallbackStruct.h"
#include "teul/teulEndpoint.h"
#include "teul/teulPackers.h"
#include "teul/teulPacketHeader.h"
#include "wwii/wwiiCallbacks.h"

TEUL_CALLBACK* TEUL_CALLBACK::s_callbackArray[256][256];

TEUL_CALLBACK*	teulAllocSystemCallback(Teul::CallbackInit* const ci);

// Callback Errors are handled differently here. This routine simply
// executes the Callback error routine that is found in the ERROR_STRUCT.
SINT32
teulPostCallbackError(const SINT32 teulIndex, const UCHAR* const data, const SINT32 size, const TEUL_CALLBACK* const object, const SINT32 error)
{
	if ( object == nullptr && data == nullptr )
		return -EINVAL;

	const TEUL_CALLBACK* const callback = (object == nullptr) ? TEUL_CALLBACK::Lookup(data) : object;
	if (callback == nullptr)
		return -EINVAL;

	if ( callback->error == nullptr )
	{
		dbugLog(_FLA, "t:%d missing error handler for " PRF_CALLBACK "", teulIndex, PRA_CALLBACK(*callback));
		return 0;
	}

	dbugLog(_FLW, "executing error for " PRF_CALLBACK "", PRA_CALLBACK(*callback));

	// Invoke the error handler.
	(callback->error)(teulIndex, data, size, callback, error);

	return 0;
}

// This routine initializes a TEUL_CALLBACK structure and points
// to it from the callback array.
TEUL_CALLBACK*
teulAllocCALLBACKi(Teul::CallbackInit* const ci)
{
	TEUL_CALLBACK* const prevObject = TEUL_CALLBACK::Lookup(ci->callX, ci->callY);

	TEUL_CALLBACK* realObject;
	if ( PN_UNLIKELY(prevObject != nullptr) )
	{
		if (  (ci->recvAction != nullptr && prevObject->recvAction() != nullptr)
			|| (ci->sendAction != nullptr && prevObject->sendAction() != nullptr)  )
		{
			dbugSetErrno(EINVAL);
			LogAbort("Attempt to re-allocate callback x:%u y:%u", ci->callX, ci->callY);
		}

		if (ci->sendAction != nullptr)
		{
			prevObject->m_sendAction = ci->sendAction;
		}
		if (ci->recvAction != nullptr)
		{
			prevObject->m_recvAction = ci->recvAction;
		}

		realObject = prevObject;
	}
	else
	{
		realObject = (TEUL_CALLBACK*)memCalloc(1, sizeof(TEUL_CALLBACK));

		realObject->m_sendAction = ci->sendAction;
		realObject->m_recvAction = ci->recvAction;
		realObject->m_callX = ci->callX;
		realObject->m_callY = ci->callY;
		realObject->m_callXName = ci->callXName;
		realObject->m_callYName = ci->callYName;

		TEUL_CALLBACK::s_callbackArray[realObject->m_callX][realObject->m_callY] = realObject;

		realObject->outData = nullptr;
		realObject->inData = nullptr;
	}

	realObject->error = ci->error;

	return realObject;
}

// There are actually two main types of callbacks, System and User.
// the main differentiation is that system callbacks are reinstalled
// in the teulClient (AKA teulKit) and teulServer and provide the
// base functionality of teul.  The User callbacks are usable by
// the application programmer for their application.

// this allocates User Callbacks
TEUL_CALLBACK*
teulAllocCallback(Teul::CallbackInit* const ci)
{
	if(	PN_UNLIKELY(ci->callX < Teul::RestrictedCallbackRange_X) )
	{
		dbugSetErrno(EINVAL);
		dbugLog(_FLA,"Invalid Parameter for callback x:%u y:%u", ci->callX, ci->callY );
		return nullptr;
	}
	if ( PN_UNLIKELY(ci->sendAction == nullptr && ci->recvAction == nullptr) )
	{
		dbugSetErrno(EINVAL);
		dbugLog(_FLA,"Invalid handlers for callback x:%u y:%u", ci->callX, ci->callY );
		return nullptr;
	}

#if defined(DEV_RELEASE)
	dbugLog(_FLD, "Alloc entered x:%d(%s) y:%d(%s) mode:%d "
		"sendA:%p recvA:%p timeout:%d"
		, ci->callX, ci->callXName, ci->callY, ci->callYName, ci->mode
		, ci->sendAction, ci->recvAction, ci->sendTimeout);

#endif

	return teulAllocCALLBACKi(ci);
}

// This allocates System Callbacks.
//note:	this function should not be available as an exported
//		symbol for released versions of teulClient (AKA teulKit).
TEUL_CALLBACK*
teulAllocSystemCallback(Teul::CallbackInit* const ci)
{
	if ( PN_UNLIKELY(ci->callX >= Teul::RestrictedCallbackRange_X) )
		LogAbort("cb:%u,%u: Invalid system callback", ci->callX, ci->callY);
	if ( PN_UNLIKELY(ci->sendAction == nullptr && ci->recvAction == nullptr) )
		LogAbort("cb:%u,%u: Invalid send/recv parameters for system callback", ci->callX, ci->callY);

	return teulAllocCALLBACKi(ci);
}

// should you need to reassign a callback you must first free it.
void
TEUL_CALLBACK::Release()
{
	dbugLog(_FLI, "freeing callback " PRF_CALLBACK "", PRA_CALLBACK(*this));

	s_callbackArray[callX()][callY()] = nullptr;
	memFree(this);
}

// sometimes you just have to free them all
void teulFreeAllCallbacks()
{
	for ( UINT32 x = 0; x < 256; ++x )
	{
		for ( UINT32 y = 0; y < 256; ++y )
		{
			TEUL_CALLBACK* objPtr = TEUL_CALLBACK::Lookup((UINT8)x, (UINT8)y);
			if ( objPtr != nullptr )
				objPtr->Release();
		}
	}
}

static SINT32
_doAction(ENDPOINT& endpoint
		  , TEUL_CALLBACK& callback
		  , UCHAR* const data
		  , const SINT32 dataSize_bytes)
{
	if (callback.outData == nullptr)
	{
		static char tempData[Teul::MaxPacket * 2];
		callback.outData = tempData;
	}

	ACTION_OBJECT actionObj;
	actionObj.endpoint = &endpoint;
	actionObj.teulIndex = endpoint.teulIndex();
	actionObj.actionObj = &callback;
	actionObj.localRawData = callback.outData;
	actionObj.packed = data;
	actionObj.offset = 0;
	actionObj.m_size_bytes = dataSize_bytes;
	actionObj.m_client = endpoint.appConnectionData;

	actionObj.fromAddress = 0;
	actionObj.fromPort = 0;

	const sockaddr_in& from = endpoint.fromSockAddress;
	actionObj.fromAddress = ntohl(from.sin_addr.s_addr);
	actionObj.fromPort = ntohs(from.sin_port);

	dbugLog(_FLD, "t:%d executing callback " PRF_CALLBACK " size:%d", endpoint.teulIndex(), PRA_CALLBACK(callback), dataSize_bytes);

	const SINT32 retval = callback.recvAction()(&actionObj);

	if (PN_UNLIKELY(retval < 0))
		dbugLog(_FLA, "t:%d callback " PRF_CALLBACK " err:%d size:%d", endpoint.teulIndex(), PRA_CALLBACK(callback), abs(retval), dataSize_bytes);

	return retval;
}

// this routine not only executes callbacks but also relays callbacks not
// destined for this module to the appropriate interface either directly
// with _relayCALLBACK() or _routeCALLBACK().
SINT32
TEUL_CALLBACK::processCallback(ENDPOINT& endpoint_, UCHAR* const callData_, const SINT32 dataSize_bytes)
{
	if ( PN_UNLIKELY(recvAction() == nullptr) )
	{
		dbugLog(_FLA, "t:%d " PRF_CALLBACK " null action", endpoint_.teulIndex(), PRA_CALLBACK(*this));
		return -EINVAL;
	}

	const SINT32 result = _doAction(endpoint_, *this, callData_, dataSize_bytes);
	if ( PN_UNLIKELY(result == -ETEUL_CB_REQ_DATA) )
	{
		endpoint_.sendError(ETEUL_CB_REQ_DATA);
	}
	return result;
}

SINT32
CallbackRegister::NoDataHandler(ACTION_OBJECT* const a)
{
	return a->offset;
}
