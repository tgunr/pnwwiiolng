#include <syscfg.h> // Path is project specific.
// This module provides support utilities used in
// the teul API. The end-user developer
// should not need the functions provided here.
//
// Copyright 1999 Playnet Inc, All rights reserved
// Not to be distributed in API form except as part of a compiled
// library
//
// Originating Programmer: Russell Mirabelli

#include "teul/teulCallback.h"
#include "teul/teulCallbackInit.h"
#include "teul/teulCallbackStruct.h"
#include "teul/teulEndpoint.h"
#include "teul/teulPackers.h"
#include "teul/teulPacketHeader.h"
#include "wwii/wwiiCallbacks.h"

#include "host/hostTiming.h"

static LOGICAL _callbacksInited = FALSE;

TEUL_CALLBACK* TEUL_CALLBACK::s_callbackArray[256][256];

TEUL_CALLBACK*	teulAllocSystemCallback(Teul::CallbackInit* const ci);

// Callback Errors are handled differently here. This routine simply
// executes the Callback error routine that is found in the ERROR_STRUCT.
SINT32
teulPostCallbackError(const SINT32 teulIndex, const UCHAR* const data, const SINT32 size, const TEUL_CALLBACK* const object, const SINT32 error)
{
	if ( object == NULL && data == NULL )
		return -EINVAL ;

	const TEUL_CALLBACK* const callback = object == NULL ? TEUL_CALLBACK::Lookup(data) : object ;
	if (callback == NULL)
		return -EINVAL;

	dbugLog(_FLW, "executing error for callX:%d callY:%d", callback->callX(), callback->callY()) ;

	if ( callback->error == NULL )
	{
		dbugLog(_FLW, "t:%d err:%u,%u can't execute error callback, no handler registered", teulIndex, callback->callX(), callback->callY()) ;
		return 0 ;
	}

	// Invoke the error handler.
	(callback->error)(teulIndex, data, size, callback, error);

	return 0;
}

// This routine initializes a TEUL_CALLBACK structure and points
// to it from the callback array.
TEUL_CALLBACK*
teulAllocCALLBACKi(Teul::CallbackInit* const ci)
{
	TEUL_CALLBACK* const prevObject = TEUL_CALLBACK::Lookup(ci->callX, ci->callY) ;

	TEUL_CALLBACK* realObject;
	if ( prevObject != NULL)
	{
		if (  (ci->recvAction != NULL && prevObject->recvAction() != NULL)
			|| (ci->sendAction != NULL && prevObject->sendAction() != NULL)  )
		{
			dbugSetErrno(EINVAL);
			dbugLog(_FLA,"Attempt to allocate an in use callback for x:%d y:%d sendA:%p recvA:%p"
						, ci->callX, ci->callY
						, ci->sendAction, ci->recvAction);
			return NULL;
		}

		if (ci->sendAction != NULL)
		{
			prevObject->m_sendAction = ci->sendAction;
		}
		if (ci->recvAction != NULL)
		{
			prevObject->m_recvAction = ci->recvAction;
		}

		realObject = prevObject;
	}
	else
	{
		realObject = (TEUL_CALLBACK*)memCalloc(1, sizeof(TEUL_CALLBACK));
		assert( NULL != realObject ) ;

		realObject->m_sendAction = ci->sendAction;
		realObject->m_recvAction = ci->recvAction;
		realObject->m_callX = ci->callX;
		realObject->m_callY = ci->callY;

		TEUL_CALLBACK::s_callbackArray[realObject->m_callX][realObject->m_callY] = realObject;

		realObject->outData = NULL;
		realObject->inData = NULL ;
	}

	realObject->error = ci->error;

	return realObject ;
}

// There are actually two main types of callbacks, System and User.
// the main diferentiation is that system callbacks are preinstalled
// in the teulClient (AKA teulKit) and teulServer and provide the
// base functionality of teul.  The User callbacks are usable by
// the aplication programmer for his application.

// this allocates User Callbacks
TEUL_CALLBACK*
teulAllocCallback(Teul::CallbackInit* const ci)
{
	if(	ci->callX < Teul::RestrictedCallbackRange_X )
	{
		dbugSetErrno(EINVAL);
		dbugLog(_FLA,"Invalid Parameter for callback %d: %d", ci->callX, ci->callY );
		return NULL;
	}
	if ( ci->sendAction == NULL && ci->recvAction == NULL )
	{
		dbugSetErrno(EINVAL);
		dbugLog(_FLA,"Invalid handlers for callback %d: %d", ci->callX, ci->callY );
		return NULL;
	}

#if defined(DEV_RELEASE)
	dbugLog(_FLD,"Alloc entered x:%d y:%d sendA:%p recvA:%p"
				, ci->callX, ci->callY, ci->sendAction, ci->recvAction);
#endif

	return teulAllocCALLBACKi(ci);
}

// This allocates System Callbacks.
//note:	this function should not be available as an exported
//		symbol for released versions of teulClient (AKA teulKit).
TEUL_CALLBACK*
teulAllocSystemCallback(Teul::CallbackInit* const ci)
{
	if( ci->callX >= Teul::RestrictedCallbackRange_X || (ci->sendAction == NULL && ci->recvAction == NULL) )
	{
		dbugSetErrno(EINVAL);
		dbugLog(_FLA,"Invalid Parameter for system callback %d: %d send:%p recv:%p"
				, ci->callX, ci->callY,ci->sendAction, ci->recvAction);
		return NULL;
	}

	return teulAllocCALLBACKi(ci);
}

// should you need to reasign a callback you must first free it.
void
TEUL_CALLBACK::Release()
{
	dbugLog(_FLI, "cb:%u,%u freeing callback", callX(), callY()) ;

	s_callbackArray[callX()][callY()] = NULL;

	memFree(this) ;
}

// sometimes you just have to free them all
void teulFreeAllCallbacks()
{
	for ( UINT32 x = 0 ; x < 256 ; ++x )
	{
		for ( UINT32 y = 0 ; y < 256 ; ++y )
		{
			TEUL_CALLBACK* objPtr = TEUL_CALLBACK::Lookup((UINT8)x, (UINT8)y) ;
			if ( objPtr != NULL)
			{
				objPtr->Release() ;
			}
		}
	}

	_callbacksInited = TRUE;
}

static SINT32
_doAction(ENDPOINT& endpoint
		  , TEUL_CALLBACK& callback
		  , UCHAR* const data
		  , const SINT32 dataSize_bytes)
{
	TIMER_USE_HERE
	TIMER_START

	if (callback.outData == NULL)
	{
		static char tempData[Teul::MaxPacket * 2] ;
		callback.outData = tempData ;
	}

	ACTION_OBJECT actionObj;
	actionObj.endpoint = &endpoint ;
	actionObj.teulIndex = endpoint.teulIndex() ;
	actionObj.actionObj = &callback;
	actionObj.localRawData = callback.outData;
	actionObj.packed = data;
	actionObj.offset = 0;
	actionObj.m_size_bytes = dataSize_bytes;
	actionObj.m_client = endpoint.appConnectionData ;

	actionObj.fromAddress = 0;
	actionObj.fromPort = 0;

	const sockaddr_in& from = endpoint.fromSockAddress;
	actionObj.fromAddress = ntohl(from.sin_addr.s_addr);
	actionObj.fromPort = ntohs(from.sin_port);

	TIMER_STOP("doact:start")

	TIMER_START
	const SINT32 retval = callback.recvAction()(&actionObj);
	TIMER_STOP("doact:recv")

	if (retval < 0)
		dbugLog(_FLA, "t:%d error %d executing callback x:%d y:%d size:%d", endpoint.teulIndex(), abs(retval), callback.callX(), callback.callY(), dataSize_bytes);

	return retval;
}

// this routine not only executes callbacks but allso relays callbacks not
// destined for this module to the apropriate interface either directly
// with _relayCALLBACK() or _routeCALLBACK().
SINT32
TEUL_CALLBACK::processCallback(ENDPOINT& endpoint_, UCHAR* const callData_, const SINT32 dataSize_bytes)
{
	if ( recvAction() == NULL )
	{
		dbugLog(_FLA, "t:%d cb:%u,%u null action", endpoint_.teulIndex(), callX(), callY());
		return -EINVAL;
	}

	const SINT32 result = _doAction(endpoint_, *this, callData_, dataSize_bytes);
	if ( result == -ETEUL_CB_REQ_DATA )
	{
		endpoint_.sendError(ETEUL_CB_REQ_DATA) ;
	}
	return result ;
}

SINT32
CallbackRegister::NoDataHandler(ACTION_OBJECT* const a)
{
	return a->offset ;
}
