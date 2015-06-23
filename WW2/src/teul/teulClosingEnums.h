#pragma once

namespace Teul
{
	namespace Closing
	{
		enum
		{
			SynAckAckFail	=	30000
		,	ConnectFail		=	30001
		,	FynRx			=	30002
		, 	Refused			=	30003
		,	ConnSend		=	30004
		,	ConnError		=	30005
		,	Closed			=	30006
		,	ReadErr			=	30007
		,	Prefix			=	30008
		,	ReadSize		=	30009

		,	BadSize			=	30010
		,	RecvError		=	30011
		,	EndpointError	=	30012
		,	RcvDgramError	=	30013
		,	FDClose			=	30014
		,	AuthCB			=	30015
		,	Timeout			=	30016
		,	SendErrorLimit	=	30017
		,	SendPipe		=	30018
		,	SendFail		=	30019

		,	Shutdown		=	30020
		,	RstToken		=	30021
		,	SynErr			=	30022
		,	FynAck			=	30023

		,	TeulClose		=	30100

		,	AuthLoginExcept	=	30101
		,	AuthLoginErr	=	30102
		,	AuthRelease		=	30103
		,	BCRNoPersona	=	30104
		,	MgrLoginErr		=	30105
		,	VJRNoPersona	=	30106
		,	VJRExited		=	30107
		,	RUINoPersona	=	30108
		,	DBDBad			=	30109
		,	DBDClose		=	30110
		,	DBDErr			=	30111

		,	ExitNoPlayer	=	30112
		,	ExitWithPlayer	=	30113
		,	PlayerLeaveNoPlayer = 30114
		,	Smite			=	30115
		,	PlayerErrFn		=	30116
		,	PlayerRelease	=	30117
		,	NullPlayerRelease	=	30118
		,	ReleaseEconn	=	30119

		,	DiscoCell		=	30120
		,	DiscoChat		=	30121
		,	ClientConnClose	=	30122
		,	OrderlyShutdown	=	30123
		,	RemoteDisc		=	30124
		,	LocalDisc		=	30125
		,	AbortedApp		=	30126
		,	PlayerDtor		=	30127

		,	PeerDoDisc		=	30200
		,	PeerBadSrc		=	30201
		,	PeerBadSrv		=	30202

		,	ChatSrvErr		=	30300
		,	MapSrvClose		=	30303
		};
	}
}
