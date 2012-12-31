#pragma once
// This is where all declarations for call back constants that will
// be transmited accross the network must reside or be included into.
//
//	Copyright 1999 Playnet Inc, All rights reserved
//
//	Originating Programmer : Martin J. Poulin

namespace Teul
{
	// Teul::Module callbacks.
	namespace Module { enum { CallbackX = 0 } ; }
	// reserved a special row for callbacks that are system but must be remote
	namespace Serv { enum { CallbackX = 166 } ; }
}

// Teul::Module callbacks.
enum
{
	TM_ACCEPT_Y,				//0
	TM_CONNECT_Y,
	TM_ENDPOINT_ERROR_Y,
	TM_ERROR_Y,
	TM_RST_TOKEN_Y,

	TM_SYN_TOKEN_Y,				//5
	TM_SYN_ACK_TOKEN_Y,
	TM_SYN_ACK_ACK_TOKEN_Y,
	TM_FYN_TOKEN_Y,
	TM_FYN_ACK_TOKEN_Y,

	TM_KLIVE_TOKEN_Y,			//10
	TM_KLIVE_ACK_TOKEN_Y,
	TM_ACK_TOKEN_Y,
	TM_OLISTEN_Y,
	TM_OPEN_Y,

	_SPARE_TM_Y_,				//15
	TM_CLOSE_Y,
	TM_ALLOC_Y,
	TM_FREE_CALLBACK_Y,
	TM_INTERFACE_Y,

	TM_REQ_CONFIG_Y,			//20
	TM_CONFIG_Y,
	TM_CONNECTION_INSTRUCTION_Y,
	_TM_SPARE_4_Y,
	TM_CLOSED_Y,

	_TM_SPARE_1_Y,				//25
	TM_CANCEL_FILE_Y,
	TM_SEND_FILE_Y,
	_TM_SPARE_2_Y,
	TM_GENERIC_MESSAGE_Y,

	TM_SESSION_INIT_Y			//30
};

// Server callbacks.
enum
{
	SERV_HEARTBEAT_Y,
	PLAYER_MIRROR_Y,
	PLAYER_MIRROR_REPLY_Y,
	GROUP_XFER_Y,
	GROUP_XFER_REPLY_Y,
	PLAYER_EXIT_Y,
	PLAYER_UPDATE_Y,
	CLUSTER_SHUTDOWN_Y,
	HOST_CONFIG_Y
};
