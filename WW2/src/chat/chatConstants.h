#pragma once
// Created: Thu Jul 27 15:45:40 2000
//
// constants for the chat system
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson

//We want this to be a low priority callback
enum { CHAT_X = 205 };

namespace Chat
{
	enum { CallbackX = 205 };

	//Once again, we pick a high number that hopefully isn't taken by someone else
	enum
	{
		// 0
		MISSION_DELETION_Y = 0,	// Requests for and notifications of mission deletions
		GAME_HACKER_REPORT_Y,
		LOCATE_USER_Y,
		FOUND_USER_Y,
		EXCHANGE_SCMD_Y,		// Send and receive Strat CoMmanDs
		CMD_LIST_Y,				// List of dot commands
		BRIGADE_MOVE_Y,			// Notice of a brigade movement
		WHO_LIST_Y,				// Array of who messages
		MOD_REPORT_Y,			// Mod report
		MOBILE_SPAWN_WAYPOINT_Y,	// Mobile spawn waypoint updates
		// 10
		MISSION_LEADER_CHANGE_Y,	// Change of mission leader
		BUZZARD_SPAWN_Y,			// Spawn a buzzard at a particular location
		MAP_PERSONA_UPDATE_Y,
		BROADCAST_MESSAGE_Y,		// For Chat::Broadcast's
		TICK_SAMPLE_Y,				// Speed hack report
		ATTACK_ORDER_Y,				// Strat->Chat attack order replies
		HOST_LOCALITY_UPDATES_Y,	// Locality update batches.

		// 50 - Insert new client
		DOT_COMMAND_LIST_Y,	// Used to send the Dot Command Array
		AWS_Y,					// AWS request/reply
		HOLD_FALL_INFO_Y,		// Messages about hold/fall votes

		/////  ^^^ INSERT NEW CALLBACKS HERE^^^  /////
		OMG_Y = 100,				// OMG-numbered callbacks start here

		// 200
		LOGIN_Y	=	200,	// Old callbacks I don't wanna renumber
		LOGOUT_Y,
		CLIENT_MESSAGE_Y,
		SERVER_MESSAGE_Y,
		SVR_TUNE_TO_CHANNEL_Y,
		FE_TUNE_TO_CHANNEL_Y,
		UPDATE_CHANNEL_Y,
		UPDATE_HANDLE_Y,
		USER_NLA_Y,
		CHAT_MISS_STATUS_Y,
		__SPARE__4_Y,	// 210
		PLAYER_INFO_Y,
		SERVER_INFO_Y,
		PERSONA_UPDATE_Y,
		SMITE_USER_Y,
		GAME_GCI_MESSAGE_Y,
		CLIENT_GCI_MESSAGE_Y,
		UPDATE_PLAYER_PERM_Y,
		REPORT_Y,				// Report submission
		MISSION_MEMBER_UPDATE_Y,

		MOTD_REQ_Y,		// 220
		MOTD_REPLY_Y,
		LOCALIZED_MESSAGE_Y,
		OOB_MESSAGE_Y,
		PREFERENCE_Y,
		__SPARE__1_Y,
		SPAWN_RESTRICT_Y,
		SQUAD_Y,
		SQUAD_CONTROL_Y,
		EDIT_STRING_Y,			// Change a string
		__SPARE__3_Y,				// 230
		KICK_NOTICE_Y,
		CLIENT_LOCALITY_Y,
		GENERIC_STRING_Y,		// Exchange a string
		BANDWIDTH_NOTICE_Y,
		WAYPOINT_H,
		SIDE_MESSAGE_Y,		// Message to a side [and optional objective]
		___SPARE_CALLBACK_1___,
		WAYPOINT_Y,
		WAYPOINT_CTRL_Y,
		WAYPOINT_LIST_Y,		// 240: Host sending us a batch of waypoints
		MOVE_TO_Y,				// Chat "move to here" message
		BRIGADE_CONTROL_Y,		// BrigadeControl requests and notices
		BRIGADE_CONTROL_LIST_Y,// BrigadeControlList
		SERVER_TRACKED_OBJECTS_Y,
		STO_JOULES_Y,			// Apply joules to an STO
		TEST_PACKET_Y,
		RECRUITER_LIST_Y,		// list of squad recruiters
		RECRUITER_REQUEST_Y,	// client request recruiter list
		MISSION_INVITE_Y,		// invitation/directive to join a mission

		MAP_REFRESH_PLAYER_Y,	// Chat needs a refresh of a player.
		MAX_MESSAGE_Y
	};
}

