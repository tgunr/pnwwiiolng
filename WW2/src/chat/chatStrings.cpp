#include <syscfg.h> // Path is project specific.
// Date: Thu Oct 13 14:06:08 CDT 2005
//
// Handling of Edit- and Generic- Strings
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "chat/chatEditString.h"
#include "chat/chatGenericString.h"
#include "chat/chatPlayer.h"
#include "wwii/wwiiHCUnits.h"
#include "chat/chatHost.h"
#include "wwii/wwiiThreads.h"
#include "wwii/wwiiThreadText.h"
#include "chat/chatMotds.h"
#include "chat/chatHostMotds.h"
#include "chat/chatHostMessages.h"

static UINT32
_editSideMotd(Player* player, UINT64 id, std::string& text)
{
	if ( text.length() > MOTD_REPLY::_MAX_LENGTH )
		return EditString::_TOO_LONG;
	if ( id == MOTD::_CRS )
	{
		if ( !player->isPlaynet() )
			return EditString::_PERMISSION_DENIED;
	}
	else
	{
		if ( player->effectiveCommandSide() != (WWII_SIDE)id )
			return EditString::_PERMISSION_DENIED;
		if ( !player->isGM() )
		{
			if ( player->side() == ALLIED )
			{
				if ( !player->permissions(ALLIED_MOTD) )
					return EditString::_PERMISSION_DENIED;
			}
			else if ( player->side() == AXIS )
			{
				if ( !player->permissions(AXIS_MOTD) )
					return EditString::_PERMISSION_DENIED;
			}
		}
	}

	if ( !chatSetMotd((UINT32)id, player->chatHandle(), text) )
		return EditString::_INVALID;

	DBThread::EscapeString(text);

	static const char QRY_UPDATE_MOTD[] =
		"INSERT INTO wwii_motd (type, added, added_by, motd) VALUES (%u,now(),%u,'%s')";
	DBThread::Do(DBThread::Default, QRY_UPDATE_MOTD, (UINT32)id, player->playerID(), text.c_str());

	chatSendMotd(player->teulIndex(), chatGetMotd((UINT32)id));

	return EditString::_ACCEPTED;
}

static UINT32
_editHCUnitMotd(Player* player, UINT32 id, std::string& text)
{
	HCUnit* unit = HCUnit::Lookup(id);
	if ( unit == nullptr )
	{
		dbugLog(_FLA, "t:%d p:%u c:%s tried to edit MOTD for invalid hcUnit %u"
						, player->teulIndex(), player->playerID(), player->callsign()
						, id);
		return EditString::_INVALID;
	}

	if ( player->effectiveCommandSide() != unit->side() )
	{
		dbugLog(_FLA, "t:%d p:%u c:%s tried to edit MOTD for hcUnit %s country %d player acs: %d"
						, player->teulIndex(), player->playerID(), player->callsign()
						, unit->name()
						, unit->country()
						, player->activeCommandSide());
		return EditString::_PERMISSION_DENIED;
	}

	DBThread::EscapeString(text);

	// Trim "crap: " off the beginning of the existing MOTD
	size_t n = text.find(": ");
	if ( n < text.length() && n < 12 )
		text = text.substr(n + 2);

	static const char QRY_UPDATE_HCUNIT_MOTD[] =
		"UPDATE strat_hc_units SET motd = '%s: %s', modified = now() WHERE unit_id = %hu";
	DBThread::Do(DBThread::Default, QRY_UPDATE_HCUNIT_MOTD, player->callsign(), text.c_str(), id);

	return EditString::_ACCEPTED;
}

//////////////////////////////////////////////////////////////////////
// Receive a request to edit a string

SINT32
EditString::receive(SINT32 teulIndex)
{
	Player* player = PlayerInterface::Lookup(teulIndex);
	if ( player == nullptr )
	{
		dbugLog(_FLA, "t:%d f:%s no player registered on this index", teulIndex, __FUNCTION__);
		return -EINVAL;
	}

	dbugLog(_FLD, "t:%d p:%u c:%s received edit string %u:%llu:%s"
					, teulIndex, player->playerID(), player->callsign()
					, category(), id(), text().c_str());

	UINT32 result;
	switch ( category() )
	{
	case _SIDE_MOTD:
		result = _editSideMotd(player, (UINT16)id(), text());
	break;
	case _HCUNIT_MOTD:
		result = _editHCUnitMotd(player, (UINT16)id(), text());
	break;
	case _UNDEFINED:
	default:
		dbugLog(_FLA, "t:%d p:%u c:%s invalid edit string category %d", teulIndex, player->playerID(), player->callsign(), category());
		return -EINVAL;
	}

	return result;
}

SINT32
GenericString::receive(SINT32 teulIndex)
{
	Player* player = PlayerInterface::Lookup(teulIndex);
	if ( player == nullptr )
	{
		dbugLog(_FLA, "t:%d f:%s no player registered on this index", teulIndex, __FUNCTION__);
		return -EINVAL;
	}

	return 0;
}

