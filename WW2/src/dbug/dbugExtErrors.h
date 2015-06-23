#pragma once
// Copyright 1999-2011 Playnet Inc, All rights reserved
// Not to be distributed in API form except as part of a compiled
// library
//
// Originating Programmer: Marty Poulin

enum
{
	ETEUL_UNDEF_CALLBACK		=	20000	//! Tried to call a callback that's not registered.
,	ETEUL_PACKET_TIMED_OUT		=	20001	//! A sent packet has not acknowledged being received.
,	ETEUL_SERVER_SHUTDOWN		=	20002	//! Server shutdown.
,	ETEUL_DUP_PACKET			=	20004	//! duplicate packet, function fail.
,	ETEUL_INVAL_BUFFER			=	20005	//! @internal invalid buffer specified.
,	ETEUL_INVALID_PACKET		=	20006	//! @internal invalid packet or packet structure.
,	ETEUL_ILLEGAL_TICKS			=	20008	//! Getting ahead of yourself, aren't you?

,	ETEUL_ENOSPC_SEND_BUFFER	=	20009	//! no space in send buffer.
,	ETEUL_ENOSPC_REL_BUFFER		=	20011	//! @internal no space in recv buffer.
,	ETEUL_ENOSPC_SEND_SOCKET	=	20012	//! no space in socket, pair down the sends.

,	ETEUL_AUTH_RESTARTED		=	20030	//! @internal Auth was restarted.
,	ETEUL_AUTH_DOWN				=	20031	//! @internal Could not make connection to auth.
,	ETEUL_ILLEGAL_PACKET_ORDER	=	20032	//! Out of order packet on TCP connection.
											//! (probably connection manipulation by player)
,	ETEUL_CONNECTION_STALLED	=	20033	//! Difficulty sending keepalives.
,	ETEUL_CB_CONN_DEAD			=	20034	//! Connection not accepting callbacks.
,	ETEUL_CB_SEND_NULL			=	20035	//! Tried to send nullptr.
,	ETEUL_CB_REQ_DATA			=	20036	//! Callback requires data.
};

namespace Teul
{
	namespace Errors
	{
		// Errors numbered 4ccxx are critical (end session stuff),
		// Errors numbered 3ccxx are bad (need player attention),
		// Errors numbered 2ccxx are messy.
		enum
		{
			LoginCategory = 40100
		,	LoginFail_ActivateNoLocalInfo = 40101
		,	LoginFail_ActivateFailed = 40102
		,	LoginFail_DespawnNoPlayer = 40103
		,	LoginFail_DespawnError = 40104
		,	LoginFail_ActivateNoPlayerInfo = 40105
		,	LoginFail_AcceptBadIndex = 40106
		,	LoginFail_AcceptBadTeul = 40107
		,	LoginFail_AddPlayer = 40108
		,	LoginFail_SessionValidation = 40109
		,	LoginFail_NoSession = 40110
		,	LoginFail_Client = 40111
		,	LoginFail_SessionOwn = 40112
		,	LoginFail_Validation = 40113
		,	LoginFail_Collision = 40114

		// Range 50xxx-51xxx are send failures.
		,	CategoryMultiplier = 10000
		,	PackCallbackCategory = 5
		,	SendFailCategory = 6
		,	PushPacketCategory = 7
		};

		static inline SINT32 combineCodes(const UINT32 category_, const SINT32 error_)
		{
			const UINT32 unsignedError = (UINT32)((error_ < 0) ? -error_ : error_);
			return (unsignedError >= CategoryMultiplier) ? unsignedError : (category_ * CategoryMultiplier) + unsignedError;
		}
	}
}
