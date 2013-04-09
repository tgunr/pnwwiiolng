#pragma once
// Created: Thu Jun 15 15:08:59 2000
//
// Example constants file to setup a simple client/server app
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson

//Setup the callback x and y values

namespace Auth
{
	enum { CallbackX = 250 } ;
}

//Once again, we pick a high number that hopefully isn't taken by someone
//	else
enum
{
	LOGIN_REQ_Y	=	200,
	LOGIN_REPLY_Y,
	ARENA_REQ_Y,
	APP_VERIFY_Y,
	APP_LAUNCH_REQ_Y,
	APP_LAUNCH_Y,

	AUTH_DISPATCH_Y,
	GAME_MANAGEMENT_Y,
	AUTH_GAME_INIT_Y,
	AUTH_GAME_ACK_Y,
	GAME_CLEAR_LOGIN_TABLE_Y,

	_SPARE_Y_,
	PLAYER_LOGIN_Y,
	PLAYER_LOGIN_REPLY_Y,
	SERVER_LOAD_Y,
	GAME_CAPACITY_Y,

	ARENA_REQ2_Y,
	APP_FILE_CHECK_Y,
	APP_PLAYGATE_OK_Y,

	// Request a list of games.
	APP_GET_GAME_LIST_Y,			// Deprecated
	APP_XCHG_GAME_LIST_Y,			// New version

	AUTH_XCHG_LOGIN_Y,				// LoginRequest and Reply
	AUTH_LAUNCH_REQUEST_Y,			// Client requesting to launch
	AUTH_LAUNCH_SUCCESS_Y,			// Launch request granted
	AUTH_LAUNCH_REDIRECT_Y,			// Launch request denied
	AUTH_QUEUE_POSITION_Y			// You have been placed in a queue
};

