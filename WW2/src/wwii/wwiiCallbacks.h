#pragma once

#include "teul/teulCallback.h"
#include "teul/teulCallbackInit.h"
#include "assert.h"

//////////////////////////////////////////////////////////////////////
// Macro functions for wrapping callback definitions
// Example usage:
//
//		static TEUL_CALLBACK* _SEND_PLAYER_INFO;		// Send only
//		static TEUL_CALLBACK* _XCHG_MISSION_DELETION;	// Bi-directional
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
//				REGISTER_RECVSEND(Chat::MISSION_DELETION_Y, _recvMissionDeletion, _sendMissionDeletion, _XCHG_MISSION_DELETION);
//				REGISTER_SENDRECV(Chat::PERSONA_INFO_Y, _sendPersonInfoRequest, _recvPersonInfoResponse, _XCHG_PERSONA_INFO);
//			END_CALLBACK_REGISTRATION
//		}
//

//! Begin registration of a callback set for a given CallbackX group.
#if defined(DEBUG) || defined(ALPHA_RELEASE)
#define BEGIN_CALLBACK_REGISTRATION(callX)	\
    CallbackRegister cbr(callX, TEUL_PACKET_NAMES ? #callX : ""); \
	try	\
	{ \
	dbugLog(_FLI, "%s: installing " #callX " callbacks", __FUNCTION__);
#else
#define BEGIN_CALLBACK_REGISTRATION(callX)	\
    CallbackRegister cbr(callX, TEUL_PACKET_NAMES ? #callX : ""); \
	try	\
	{
#endif

//! Register a sender function.
#define REGISTER_SENDER(y, sendFn, cb) cbr.sender(y, TEUL_PACKET_NAMES ? #y : "", sendFn, cb)

//! Register a trivial sender that has no data.
#define REGISTER_SENDER_NODATA(y, cb) cbr.sender(y, TEUL_PACKET_NAMES ? #y : "", CallbackRegister::NoDataHandler, cb)
//! Register a receiver function.
#define REGISTER_RECEIVER(y, recvFn) cbr.receiver(y, TEUL_PACKET_NAMES ? #y : "", recvFn)
//! Register a function with both send and receive options.
#define REGISTER_SENDRECV(y, sendFn, recvFn, cb) { assert(cb == nullptr); cb = cbr.callback(y, TEUL_PACKET_NAMES ? #y : "", sendFn, recvFn); }
//! Register a function with both receive and send options.
#define REGISTER_RECVSEND(y, recvFn, sendFn, cb) { assert(cb == nullptr); cb = cbr.callback(y, TEUL_PACKET_NAMES ? #y : "", sendFn, recvFn); }

//! Register a bi-directional teul_network_array.
#define REGISTER_ARRAY(y, typeName) { assert(cb == nullptr); cb = cbr.callback(y, typeName::callbackSend, typeName::callbackRecv); }

//! Register a teul_network entity sender.
#define REGISTER_MSG_SENDER(y, typeName) cbr.sender(y, TEUL_PACKET_NAMES ? #y : "", typeName::callbackSend, typeName::callback())
//! Register a teul_network entity receiver.
#define REGISTER_MSG_RECEIVER(y, typeName) cbr.receiver(y, TEUL_PACKET_NAMES ? #y : "", typeName::callbackRecv)
//! Register a bi-directional teul_network entity.
#define REGISTER_MSG(y, typeName) { assert(typeName::callback() == nullptr); typeName::callback() = cbr.callback(y, TEUL_PACKET_NAMES ? #y : "", typeName::callbackSend, typeName::callbackRecv); }

//! Receive message to _recv<ObjType>(ACTION_OBJECT*, ObjType&)
#define REGISTER_RECEIVEOBJ(y, ObjType, RecvFn) cbr.receiveObj<ObjType>(y, TEUL_PACKET_NAMES ? #y : "", RecvFn)

//! Handle OMG Objects
#define REGISTER_OMG_RECEIVER(ObjType, RecvFn) do { \
	if (ObjType::CallX != cbr.callX()) \
		LogAbort("CallX mismatch"); \
	cbr.receiveObj<ObjType>(ObjType::CallY, TEUL_PACKET_NAMES ? #ObjType : "", RecvFn); \
} while(false)

//! Terminate registration block.
#if defined(DEBUG) || defined(ALPHA_RELEASE)
#define END_CALLBACK_REGISTRATION \
		dbugLog(_FLI, "%s: done", __FUNCTION__); \
		dbugSetErrno(0); \
		return 0; \
	} \
	catch (std::exception& e) \
	{ \
		LogAbort("%s: %s", __FUNCTION__, e.what()); \
		return -EINVAL; \
	}
#else
#define END_CALLBACK_REGISTRATION \
		dbugSetErrno(0); \
		return 0; \
	} \
	catch (std::exception&) \
	{ \
		LogAbort("network error"); \
		return -EINVAL; \
	}
#endif

//////////////////////////////////////////////////////////////////////
//! Teulkit Callback registration-wrapper class.
//! Use the Macros in wwii/wwiiCallbacks.h instead of accessing
//! this class directly.

class CallbackRegister
{
public:
	CallbackRegister(UCHAR callX, const char * const callXName);
	~CallbackRegister() {}

	static UINT8 callX() { return sm_ci.callX; }

	TEUL_CALLBACK* callback(UCHAR callY, const char * const callYName, ACTION_FN sendFn, ACTION_FN recvFn);
	void sender(UCHAR callY, const char * const callYName, ACTION_FN sendFn, TEUL_CALLBACK*& toRef) { assert(toRef == nullptr); toRef = callback(callY, callYName, sendFn, nullptr); }
	void receiver(UCHAR callY, const char * const callYName, ACTION_FN recvFn) { callback(callY, callYName, nullptr, recvFn); }

	template<typename T_>
	void receiveObj(UCHAR callY, const char * const callYName, SINT32(*recvFn)(ACTION_OBJECT*, T_&))
	{
		auto delegateFn = [=] (ACTION_OBJECT* a) -> SINT32
		{
			if ( a->canRead<T_>() == false )
				return -EINVAL;
			T_& objInstance = a->read<T_>();
			return recvFn(a, objInstance);
		};
		callback(callY, callYName, nullptr, delegateFn);
	}

	// No-op handler for sends
	static SINT32 NoDataHandler(ACTION_OBJECT* const a);

private:
	static void _error(SINT32 teulIndex, const UCHAR* const data, SINT32 size, const struct TEUL_CALLBACK* const object, SINT32 error);

private:
	static Teul::CallbackInit	sm_ci;		// Defined in wwiiCallbacks.cpp
};

