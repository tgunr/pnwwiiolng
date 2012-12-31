#pragma once
// Created: Wed Sep 21 10:52:17 CDT 2005
//
// Enumerations of squad control results
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

enum SQUAD_CONTROL_RESULT
{
	SQR_INVALID
,	SQR_DONE								// This command was done
,	SQR_CREATED								// Squad has been created
,	SQR_ALREADY_IN_SQUAD					// You're already in a squad
,	SQR_NOT_IN_A_SQUAD						// You're not in a squad
,	SQR_SQUAD_EXISTS						// create: squad exists
,	SQR_INVALID_SQUAD_SHORT					// Invalid squad short name
,	SQR_INVALID_SQUAD_FULL					// Invalid squad full name
,	SQR_NO_SUCH_PLAYER						// No such player
,	SQR_PLAYER_NOT_ONLINE					// Player isn't online
,	SQR_PLAYER_NOT_IN_SQUAD					// Player isn't in squad
,	SQR_PLAYER_IN_OTHER_SQUAD				// Your target already has a squad
,	SQR_PLAYER_IN_SQUAD						// Target is already in this squad
,	SQR_RANK_TOO_LOW						// Your rank is too low
,	SQR_DEMOTE_LIMIT						// Cannot demote player further
,	SQR_PROMOTE_LIMIT						// Cannot promote player further
,	SQR_POSITION_TAKEN						// Cannot promote player: position taken
,	SQR_PLAYER_DECLINED						// Player declined squad invite
,	SQR_PLAYER_CONSIDERING					// Your squad already invited them
,	SQR_PLAYER_CONSIDERING_OTHER			// TOO SLOW KIDDO!
,	SQR_NO_INVITE							// No invite to accept/decline
,	SQR_TEXT_TOO_LONG						// Your text was too long
,	SQR_TEXT_TOO_SHORT						// Your text was too short
,	SQR_TEXT_INVALID						// Your text was invalid
,	SQR_NOT_IMPLEMENTED						// Too soon!
,	SQR_CANT_DO_TO_SELF						// Can't do that to yourself
,	SQR_PLAYER_OUTRANKS						// Other player outranks you
,	SQR_SQUAD_UNAVAILABLE					// Server is fucked
,	SQR_MUST_RESIGN							// XO/CO must resign
,	SQR_OPERATION_FAILED					// Operation failed
,	SQR_POSITION_OCCUPIED					// Position is taken (e.g. XO/CO)
,	SQR_CREATE_MEMBER_ERROR					// Members not online/declined
,	SQR_CREATE_EXPIRED						// Create request expired
,	SQR_CREATE_IN_PROGRESS					// You already have a create in progress
,	SQR_CREATE_TOO_FEW_MEMBERS				// Not enough members to create
,	SQR_CREATE_WAITING_OTHERS				// Waiting on others
,	SQR_ACCEPTED							// Squad invite accepted - you are now a member of %s.
,	SQR_DECLINED							// Squad invite declined.
,	SQR_HAVE_KICKED							// You have removed %s from the squad.
,	SQR_LEFT								// You've left the squad
,	SQR_KICKED								// You've left the squad
,	SQR_RECRUITER_GONE						// Recruiter is no-longer recruiting, pick another
} ;

// Used by the host to indicate no response as yet
static const UINT32 SQR_IN_PROGRESS = 9999 ;
