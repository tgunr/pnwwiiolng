#pragma once
// Created: Tue Sep 20 17:56:58 CDT 2005
//
// Defines a structure for squad commands
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith
// Supporting Programmer: Troy "ramp" Lowe

#include "teul/teulPackers.h"
#include <string>
#include <vector>
#include "chat/chatSquadControlResults.h"

class SquadControl
{
public:
	enum { _INVALID, _CREATE, _DISBAND, _RECRUIT, _ACCEPT, _DECLINE, _KICK, _LEAVE, _PROMOTE, _DEMOTE, _TEXT, _RESIGN, _INTERNAL, _JOIN, _INTEGER } ;
	enum { TEXT_NAME = 1, TEXT_DESCRIPTION, TEXT_HOMEPAGE, TEXT_MOTD, TEXT_VOICE, TEXT_NIGHT, TEXT_SLOGAN } ;
	enum { INT_LANGUAGE = 1, INT_RECRUITING, INT_VOTING } ;

public:
	SquadControl() : m_action(0), m_handle(0LL), m_text(""), m_players(), m_playerIDs(), m_key(0), m_value(0) {}
	SquadControl(ACTION_OBJECT* const a)
		: m_action(0), m_handle(0LL), m_text(""), m_players(), m_playerIDs(), m_key(0), m_value(0)
		{ a->offset = packer(TEUL_UNPACK, a) ; }
	~SquadControl() {}

public:
	SINT32 receive(SINT32 teulIndex) ;
	SINT32 send(SINT32 teulIndex) ;

	SINT32 _sendBasic(SINT32 connection, UINT32 action, const ChatHandle& handle=0LL, const std::string& reason="") ;

	SINT32 sendCreate(SINT32 connection, const ChatHandle& handle, const std::string& squadName, const std::vector<ChatHandle>& players)
		{
			m_players = players ;
			return _sendBasic(connection, _CREATE, handle, squadName) ;
		}
	SINT32 sendDisband(SINT32 connection)
		{ return _sendBasic(connection, _DISBAND) ; }
	SINT32 sendRecruit(SINT32 connection, const ChatHandle& handle)
		{ return _sendBasic(connection, _RECRUIT, handle) ; }
	SINT32 sendAccept(SINT32 connection)
		{ return _sendBasic(connection, _ACCEPT) ; }
	SINT32 sendDecline(SINT32 connection)
		{ return _sendBasic(connection, _DECLINE) ; }
	SINT32 sendKick(SINT32 connection, const ChatHandle& handle, const std::string& reason)
		{ return _sendBasic(connection, _KICK, handle, reason) ; }
	SINT32 sendLeave(SINT32 connection)
		{ return _sendBasic(connection, _LEAVE) ; }
	SINT32 sendPromote(SINT32 connection, const ChatHandle& handle)
		{ return _sendBasic(connection, _PROMOTE, handle) ; }
	SINT32 sendDemote(SINT32 connection, const ChatHandle& handle)
		{ return _sendBasic(connection, _DEMOTE, handle) ; }
	SINT32 sendText(SINT32 connection, UINT32 field, const std::string& text = "")
		{
			m_key = field ;
			return _sendBasic(connection, _TEXT, 0ULL, text) ;
		}
	SINT32 sendResign(SINT32 connection, const ChatHandle& handle)
		{ return _sendBasic(connection, _RESIGN, handle) ; }
	SINT32 sendJoin(SINT32 connection, const ChatHandle& handle) // handle of the recruiter
		{ return _sendBasic(connection, _JOIN, handle) ; }
	// The integer system packs the int ID and the value into one 46-ibit handle
	SINT32 sendInteger(SINT32 connection, UINT32 key, UINT32 value)
		{
			m_key = key ;
			m_value = value ;
			return _sendBasic(connection, _INTEGER) ;
		}

public:
	SINT32 packer(LOGICAL mode, ACTION_OBJECT* a) ;
	SINT32 CREATE_packer(LOGICAL mode, ACTION_OBJECT* a) ;
	SINT32 handle_packer(LOGICAL mode, ACTION_OBJECT* a) ;
	SINT32 text_packer(LOGICAL mode, ACTION_OBJECT* a) ;
	SINT32 int_packer(LOGICAL mode, ACTION_OBJECT* a) ;

public:
	// Number of players required to create a squad
	static const size_t CREATE_PLAYERS_REQUIRED = 4 ;
	static const UINT32 MAX_TEXT_LEN = 400 ;

public:
	UINT32				m_action ;			// Action being executed

	// Handle is either the target player handle, the squad handle for
	// create or an ID field for the text string.
	// If m_action is _INTEGER then the handle is a 48-bit compressed
	// key/value pair.
	ChatHandle			m_handle ;
	std::string			m_text ;			// Full squad name for create, text for kick
	std::vector<ChatHandle>	m_players ;		// Players for create
	std::vector<UINT32>	m_playerIDs ;		// Populated by the host
	// For integer changes
	UINT32				m_key ;
	UINT32				m_value ;
} ;

class SquadControlResult
{
public:
	SquadControlResult(UINT32 result, UINT32 action=0, const ChatHandle& handle=0LL)
		: m_result(result), m_action(action), m_handle(handle) {}
	SquadControlResult(ACTION_OBJECT* const a) { a->offset = packer(TEUL_UNPACK, a) ; }
	~SquadControlResult() {}

public:
	SINT32 packer(LOGICAL mode, ACTION_OBJECT* a) ;
	SINT32 receive(SINT32 teulIndex) ;
	SINT32 send(SINT32 teulIndex) ;

public:
	UINT32				m_result ;			// The reason it failed
	UINT32				m_action ;			// What you tried to do
	ChatHandle			m_handle ;			// To whom (or 0LL)

public:
	const char*	printResult() const
	{
		static const char* results[] = {
				"invalid"
			,	"done"
			,	"created"
			,	"alreadyinsquad"
			,	"notinsquad"
			,	"squadexists"
			,	"invalidshortname"
			,	"invalidsquadfull"
			,	"nosuchplayer"
			,	"playernotonline"
			,	"playernotinsquad"
			,	"playerinothersquad"
			,	"playerinsquad"
			,	"ranktolow"
			,	"demotelimit"
			,	"promotelimit"
			,	"positiontaken"
			,	"playerdeclined"
			,	"playerconsidering"
			,	"playerconsideringother"
			,	"noinvite"
			,	"texttoolong"
			,	"texttooshort"
			,	"textinvalid"
			,	"notimplemented"
			,	"cantdotoself"
			,	"playeroutranks"
			,	"squadunavailable"
			,	"mustresign"
			,	"operationfailed"
			,	"positionoccupied"
			,	"createmembererror"
			,	"createexpired"
			,	"createinprogress"
			,	"createtoofewmembers"
			,	"createwaitingothers"
			,	"accepted"
			,	"declined"
			,	"havekicked"
			,	"left"
			,	"kicked"
			,	"recruitergone"
		} ;
		return results[m_result] ;
	}
} ;
