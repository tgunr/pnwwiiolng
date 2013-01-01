#pragma once

#include <assert.h>
#include "teul/teulCallback.h"
#include "teul/teulCallbackInit.h"

//////////////////////////////////////////////////////////////////////
// Macro functions for wrapping callback definitions
// Example usage:
//
//		static TEUL_CALLBACK* _SEND_PLAYER_INFO ;		// Send only
//		static TEUL_CALLBACK* _XCHG_MISSION_DELETION ;	// Bi-directional
//		...
//
//		static SINT32
//		_sendPlayerInfo(ACTION_OBJECT* const a) { ... }
//		...
//
//		SINT32				// Important
//		chatInstallSomeCallbacks()
//		{
//			BEGIN_CALLBACK_REGISTRATION(CHAT_X)
//				REGISTER_SENDER(Chat::PLAYER_INFO_Y, _sendPlayerInfo, _SEND_PLAYER_INFO);
//				// "SENDRECV" is* suggested* for requests while "RECVSEND" is suggested
//				// for the recipient of a request. They are distinct for purely aesthetic
//				// reasons, argument ordering being the only implementation difference.
//				REGISTER_RECVSEND(Chat::MISSION_DELETION_Y, _recvMissionDeletion, _sendMissionDeletion, _XCHG_MISSION_DELETION) ;
//				REGISTER_SENDRECV(Chat::PERSONA_INFO_Y, _sendPersonInfoRequest, _recvPersonInfoResponse, _XCHG_PERSONA_INFO) ;
//			END_CALLBACK_REGISTRATION
//		}
//

//! Begin registration of a callback set for a given CallbackX group.
#define BEGIN_CALLBACK_REGISTRATION(callX)	\
	CallbackRegister cbr(callX); \
	try	\
	{ \
	dbugLog(_FLI, "%s: installing " #callX " callbacks", __FUNCTION__) ;

//! Register a sender function.
#define REGISTER_SENDER(y, sendFn, cb) cbr.sender(y, sendFn, cb)
//! Register a trivial sender that has no data.
#define REGISTER_SENDER_NODATA(y, cb) cbr.sender(y, CallbackRegister::NoDataHandler, cb)
//! Register a receiver function.
#define	REGISTER_RECEIVER(y, recvFn) cbr.receiver(y, recvFn)
//! Register a function with both send and receive options.
#define	REGISTER_SENDRECV(y, sendFn, recvFn, cb) { assert(cb == NULL) ; cb = cbr.callback(y, sendFn, recvFn) ; }
//! Register a function with both receive and send options.
#define	REGISTER_RECVSEND(y, recvFn, sendFn, cb) { assert(cb == NULL) ; cb = cbr.callback(y, sendFn, recvFn) ; }

//! Register a bi-directional teul_network_array.
#define REGISTER_ARRAY(y, typeName) { assert(cb == NULL) ; cb = cbr.callback(y, typeName::callbackSend, typeName::callbackRecv) ; }

//! Register a teul_network entity sender.
#define REGISTER_MSG_SENDER(y, typeName) cbr.sender(y, typeName::callbackSend, typeName::callback())
//! Register a teul_network entity receiver.
#define REGISTER_MSG_RECEIVER(y, typeName) cbr.receiver(y, typeName::callbackRecv)
//! Register a bi-directional teul_network entity.
#define REGISTER_MSG(y, typeName) { assert(typeName::callback() == NULL) ; typeName::callback() = cbr.callback(y, typeName::callbackSend, typeName::callbackRecv) ; }

//! Receive message to _recv<ObjType>(ACTION_OBJECT*, ObjType&)
#define REGISTER_RECEIVEOBJ(y, objType, recvFn) cbr.receiveObj<objType>(y, recvFn)

//! Terminate registration block.
#define END_CALLBACK_REGISTRATION \
		dbugLog(_FLI, "%s: done", __FUNCTION__) ; \
		dbugSetErrno(0) ; \
		return 0 ; \
	} \
	catch (std::exception& e) \
	{ \
		LogAbort("%s: %s", __FUNCTION__, e.what()) ; \
		return -EINVAL ; \
	}

//////////////////////////////////////////////////////////////////////
//! Teulkit Callback registration-wrapper class.
//! Use the Macros in wwii/wwiiCallbacks.h instead of accessing
//! this class directly.

class CallbackRegister
{
public:
	CallbackRegister(UCHAR callX) ;
	~CallbackRegister() {}

	TEUL_CALLBACK* callback(UCHAR callY, ACTION_FN sendFn, ACTION_FN recvFn) ;
	void sender(UCHAR callY, ACTION_FN sendFn, TEUL_CALLBACK*& toRef) { assert(toRef == NULL) ; toRef = callback(callY, sendFn, nullptr) ; }
	void receiver(UCHAR callY, ACTION_FN recvFn) { callback(callY, nullptr, recvFn) ; }
	template<typename T_>
	void receiveObj(UCHAR callY, SINT32(*recvFn)(ACTION_OBJECT*, T_&))
	{
		auto delegateFn = [=] (ACTION_OBJECT* const a) -> SINT32
		{
			if ( a->canRead<T_>() == false )
				return -EINVAL;
			T_& objInstance = a->read<T_>();
			return recvFn(a, objInstance);
		};
		callback(callY, nullptr, delegateFn);
	}

	// No-op handler for sends
	static SINT32 NoDataHandler(ACTION_OBJECT* const a) ;

private:
	static void _error(SINT32 teulIndex, const UCHAR* const data, SINT32 size, const struct TEUL_CALLBACK* const object, SINT32 error) ;

private:
	static Teul::CallbackInit	sm_ci ;		// Defined in wwiiCallbacks.cpp
} ;

