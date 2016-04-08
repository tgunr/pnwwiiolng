#include <syscfg.h> // Path is project specific.
// Date: Sun Feb 15 2004
//
// Routines for handling the incoming chat->dbse messages
//
// Copyright (c) 2004 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "versionNo.h"

#include "chat/chatdbHostMessages.h"
#include "chat/chatdbParser.h"
#include "chat/chatdbSquadXchg.h"
#include "chat/chatHandle.h"
#include "chat/chatSquadControl.h"
#include "chat/chatSquadControlResults.h"
#include "chat/chatSquadMemberUpdates.h"
#include "chat/chatTypes.h"
#include "dbse/dbseConnection.h"
#include "dbse/dbseConstants.h"
#include "dbse/dbseWrapper.h"
#include "include/mydbQuery.h"
#include "teul/teulEndpointDefs.h"
#include "teul/teulKit.h"
#include "teul/teulPackers.h"

extern DBHANDLE dbh;
extern DBCONNECTION dbConnection;

//////////////////////////////////////////////////////////////////////
// Read a squad's details

void
chatdbSquadInfo(SINT32 teulIndex, UINT32 squadID, bool /*sendMembers*/)
{
	if ( squadID == 0 )
		return;

	try
	{
		DBQuery q(dbh,
					"SELECT s.short_name, s.name"
							", cr.callsign, unix_timestamp(s.created)"
							", s.flags, s.voting, s.recruiting"
							", s.rookie_days, s.last_joined, s.last_joiner"
							", s.description, s.motd, s.homepage"
							", s.squad_night"
							", s.pri_language"
							", s.slogan"
						" FROM wwii_squads s"
							" LEFT JOIN wwii_player cr ON (s.creatorID = cr.playerID)"
						" WHERE s.squadID = %u"
							" LIMIT 1"
							, squadID);

		if ( q.HasRows() == false || q.FetchRow() == false )
			throw std::logic_error("no rows for squad");

		auto& row = q.Row();
		CHATDB_SQUAD_XCHG squadXchg;
		Squad& squad = squadXchg.data();

		squad.m_squadID = squadID;
		squad.m_shortHandle = chatGetHandle(row.NextString());
		squad.m_name = row.NextString();

		squad.m_creatorHandle = chatGetHandle(row.NextString());
		squad.m_created = row.NextUInt();

		squad.m_flags = row.NextUInt();
		squad.m_votingEnabled = (row.NextUInt() != 0);
		squad.m_recruiting = (row.NextUInt() != 0);

		squad.m_rookieDays = row.NextUInt();
		squad.m_lastJoined = row.NextUInt();
		squad.m_lastJoiner = chatGetHandle(row.NextString());

		squad.m_description = row.NextString();
		squad.m_motd = row.NextString();
		squad.m_homepage = row.NextString();

		squad.setSquadNight(row.NextString());
		squad.m_language = row.NextUInt();

		squad.m_slogan = row.NextString();

		betaLog(_FLI, "sq:%u sending info to chat host", squad.m_squadID);
		if ( squadXchg.send(teulIndex) < 0 )
			dbugLog(_FLA, "squadXchg send failed");
	}
	catch ( std::exception& e)
	{
		dbugLog(_FLA, "%s(%u): %s", __FUNCTION__, squadID, e.what());
	}
}

//////////////////////////////////////////////////////////////////////
// Simple error class for throwing errors in the SquadUpdate stuff

class SquadError : public std::logic_error
{
public:
	explicit SquadError(const char* message, UINT32 error=SQR_OPERATION_FAILED) : std::logic_error(message), m_errorCode(error) {}
	const UINT32& errorCode() const { return m_errorCode; }
protected:
	UINT32 m_errorCode;
};

//////////////////////////////////////////////////////////////////////
// Check if a member's rank merrits a leadership update on the squad

static void
_checkLeadership(UINT32 squadID, RANK_NO oldRank, RANK_NO newRank, SINT32 teulIndex)
{
	if ( oldRank == Squad::CO || newRank == Squad::CO )
		chatdbSquadInfo(teulIndex, squadID, false);
}

//////////////////////////////////////////////////////////////////////
// Check the availability of a given squad name

static void
_checkExistingSquad(const Chat::Name& gameName)
{
	DBQuery q(dbh, "SELECT squadID FROM wwii_squads WHERE LOWER(short_name) = LOWER('%s') LIMIT 1", gameName.c_str());
	if ( q.HasRows() )
		throw SquadError("Squad already exists", SQR_SQUAD_EXISTS);
}

static void
_checkReservedSquad(const Chat::Name& gameName)
{
	DBQuery q(dbh, "SELECT short_name FROM wwii_reserved_squadnames WHERE lower(short_name) = LOWER('%s') LIMIT 1", gameName.c_str());
	if ( q.HasRows() )
		throw SquadError("Squad name reserved", SQR_INVALID_SQUAD_SHORT);
}

static UINT32
_createSquad(UINT32 ownerID, UINT32* members, const Chat::Name& shortName, const char* fullName, SINT32 teulIndex)
{
	DBQuery q(dbh, "INSERT INTO wwii_squads (creatorid, short_name, name, password, description, active, recruiting, voting, disbanded, created)"
					" VALUES (%u, '%s', '%s', '', '', 1, 1, 1, 0, now())"
					, ownerID, shortName.c_str(), fullName);
	UINT32 squadID = q.LastInsertID();
	if ( squadID == 0 )
		throw SquadError("No last insert ID from squad insertion");

	char playerIDs[(14 * (SquadControl::CREATE_PLAYERS_REQUIRED+1)) + 1];
	char* p = playerIDs;
	sprintf(playerIDs, "%u", ownerID);
	for ( UINT32 i = 0; i < SquadControl::CREATE_PLAYERS_REQUIRED; ++i )
	{
		while ( *p )
			++p;
		sprintf(p, ",%u", members[i]);
	}

	// Promote the CO and XO
	DBSimpleQuery(dbh, "UPDATE wwii_player"
							" SET squadID=%u"
								", squad_rank=IF(playerID=%u,%u,IF(playerID=%u,%u,%u))"
								", squad_joined=NOW()"
								", squad_time_played=0"
							" WHERE playerID in (%s)"
						, squadID, ownerID, Squad::CO, members[0], Squad::XO, Squad::MEMBER, playerIDs
						);

	// Send the squad's information back to the chat host
	chatdbSquadInfo(teulIndex, squadID, true);

	return squadID;
}

//////////////////////////////////////////////////////////////////////
// Process a squad member update

void
chatdbSquadMemberUpdate(SINT32 teulIndex, SquadMemberUpdate& update)
{
	Chat::Name gameName(update.m_handle), orig(update.m_originator);
	char safeText[SquadMemberUpdate::MAX_TEXT*2 + 1];
	safeText[0] = 0;
	escapeString(update.m_text, safeText, SquadMemberUpdate::MAX_TEXT*2);

	try
	{
		switch (update.m_action)
		{
		case SquadMemberUpdate::_ADD:
			LogAbort("SMU::ADD is deprecated");
		break;
		case SquadMemberUpdate::_PROMOTE:
		{
			if ( update.m_handle < 0x40LL ) throw SquadError("invalid handle", SQR_INVALID);
			if ( update.m_rank <= Squad::NOT_IN_SQUAD || update.m_rank >= Squad::MAX_RANK )
				throw SquadError("Invalid rank for promotion");
			DBQuery q(dbh, "UPDATE wwii_player SET squad_rank = %u WHERE squadID = %u AND callsign = '%s' AND squad_rank = %u"
							, update.m_rank, update.m_squadID, gameName.c_str(), update.m_prev);
			if ( q.HasRows() == false )
				throw SquadError("Unable to promote player");

			_checkLeadership(update.m_squadID, update.m_prev, update.m_rank, teulIndex);
		}
		break;
		case SquadMemberUpdate::_DEMOTE:
		{
			if ( update.m_handle < 0x40LL ) throw SquadError("invalid handle", SQR_INVALID);
			if ( update.m_rank <= Squad::NOT_IN_SQUAD || update.m_rank >= Squad::MAX_RANK )
				throw SquadError("Invalid rank for demotion");
			DBQuery q(dbh, "UPDATE wwii_player SET squad_rank = %u WHERE squadID = %u AND callsign = '%s' AND squad_rank = %u"
							, update.m_rank, update.m_squadID, gameName.c_str(), update.m_prev);
			if ( q.HasRows() == false )
				throw SquadError("Unable to demote player");

			_checkLeadership(update.m_squadID, update.m_prev, update.m_rank, teulIndex);
		}
		break;
		case SquadMemberUpdate::_APPOINT:
		{
			if ( update.m_handle < 0x40LL ) throw SquadError("invalid handle", SQR_INVALID);
			if ( update.m_rank <= Squad::NOT_IN_SQUAD || update.m_rank >= Squad::MAX_RANK )
				throw SquadError("Invalid rank for appointment");
			DBQuery q(dbh, "UPDATE wwii_player"
								" SET squad_rank=%u"
									", squad_joined=IF(squad_joined,squad_joined,NOW())"
								" WHERE squadID = %u AND callsign = '%s' AND squad_rank < %u"
							, update.m_rank, update.m_squadID, gameName.c_str(), update.m_rank);
			if ( q.HasRows() == false )
				throw SquadError("Unable to appoint player");

			// If we've become CO remove the prior member
			if ( update.m_rank == Squad::CO )
			{
				DBSimpleQuery(dbh, "UPDATE wwii_player SET squad_rank=%u WHERE squadID=%u AND squad_rank=%u AND callsign != '%s'"
								, Squad::XO, update.m_squadID, update.m_rank, gameName.c_str());
			}

			_checkLeadership(update.m_squadID, update.m_prev, update.m_rank, teulIndex);
		}
		break;
		case SquadMemberUpdate::_REMOVE:
		{
			if ( update.m_handle < 0x40LL ) throw SquadError("invalid handle", SQR_INVALID);
			DBQuery q(dbh, "UPDATE wwii_player"
								" SET squad_rank=0, squadID=0, squad_joined=NOW()"
								" WHERE squadID = %u AND callsign = '%s'"
							, update.m_squadID, gameName.c_str());
			if ( q.HasRows() == false )
				throw SquadError("Player not in squad", SQR_PLAYER_NOT_IN_SQUAD);

			_checkLeadership(update.m_squadID, update.m_prev, update.m_rank, teulIndex);
		}
		break;
		case SquadMemberUpdate::_CREATE_CHECK:
		{
			_checkExistingSquad(gameName);
			_checkReservedSquad(gameName);
		}
		break;
		case SquadMemberUpdate::_CREATE:
		{
			_checkExistingSquad(gameName);
			_checkReservedSquad(gameName);

			update.m_squadID = _createSquad(update.m_coID, update.m_members, gameName, safeText, teulIndex);
		}
		break;
		default:
		case SquadMemberUpdate::_INVALID:
			throw SquadError("Invalid/not implemented yet", SQR_NOT_IMPLEMENTED);
		break;
		}

		// Record this event
		DBSimpleQuery(dbh, "INSERT INTO wwii_squad_audit (squadID, action, occured, originator, target, newval, prev, coID, textmessage)"
							" VALUES (%u, %u, now(), '%s', '%s', %u, %u, %u, '%s')"
							, update.m_squadID, update.m_action, orig.c_str(), gameName.c_str(), update.m_rank, update.m_prev, update.m_coID, safeText);

		update.m_result = SQR_DONE;
	}
	catch ( SquadError& e )
	{
		dbugLog(_FLA, "sq:%d act:%d targ:%s orig:%s nr:%d failed: %s", update.m_squadID, update.m_action, gameName.c_str(), orig.c_str(), update.m_rank, e.what());
		update.m_result = e.errorCode();
	}
	catch ( std::exception& e )
	{
		dbugLog(_FLA, "sq:%d act:%d targ:%s orig:%s nr:%d failed: %s", update.m_squadID, update.m_action, gameName.c_str(), orig.c_str(), update.m_rank, e.what());
		update.m_result = SQR_OPERATION_FAILED;
	}

	if ( chatdbSendSquadMemberUpdateResult(teulIndex, update) < 0 )
		dbugLog(_FLA, "failed sending squad member update result");
}
