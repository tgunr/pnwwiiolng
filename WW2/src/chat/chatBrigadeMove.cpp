#include <syscfg.h> // Path is project specific.
// Date: Fri Apr 22 20:14:47 CDT 2005
//
// Routines for chatBrigadeMove
//
// Copyright (c) 2005 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "teul/teulKit.h"
#include "wwii/wwiiPack.h"
#include "chat/chatBrigadeMove.h"
#include "chat/chatPlayer.h"
#include "chat/chatList.h"
#include "wwii/wwiiHCUnits.h"
#include "chat/chatTimeInterval.h"
#include "chat/chatData.h"
#include "chat/chatDB.h"
#include "chat/chatHost.h"
#include "map/mapHCUnitLocations.h"
#include "chat/chatHostBrigadeControl.h"
#include "wwii/wwiiConfig.h"

using LAST_MOVEMENTS = std::map<UINT32,UINT32>;
static LAST_MOVEMENTS lastMovements;
bool g_newLocations = false;

//////////////////////////////////////////////////////////////////////
// Broadcast the brigade movements from the last few seconds

static void
_unitLocationsKeyframe(HC_UNIT_LOCATIONS& hculUpdate)
{
	HCUnitLocation entry;

	// We're interested in updates from the last 90 seconds
	const UINT32 frameWindow = teulTime() - 90;

	hculUpdate.clear();

	for ( auto it = hcUnits.begin(), end = hcUnits.end(); it != end; ++it )
	{
		HCUnit* unit = it->second;

		if ( !unit->isBrigade() && !unit->isDivision() )
			continue;
		if ( lastMovements[unit->unitID()] < frameWindow )
			continue;

		entry.m_unitID = unit->unitID();
		entry.m_homeCPID = unit->homeCPID();
		hculUpdate.add(entry);
	}

	if ( hculUpdate.empty() == false )
	{
		dbugLog(_FLD, "%" PRSizeT " updates in hcul keyframe", hculUpdate.size());
	}
}

//////////////////////////////////////////////////////////////////////
// Send all of the current HCUnit Locations

bool
chatUpdateHCUnitLocations()
{
	static UINT32 nextUpdate = teulTime();

	if ( !g_newLocations && nextUpdate > teulTime() )
		return false;
	nextUpdate = teulTime() + 15;

	HC_UNIT_LOCATIONS hculUpdate;

	_unitLocationsKeyframe(hculUpdate);

	if ( hculUpdate.empty() )
		return true;

	PLAYER_INDEX::iterator it = playerIndex.begin();
	while ( it != playerIndex.end() )
	{
		Player* player = it->second;
		++it;

		if ( player == nullptr || player->teulIndex() < 0 || !player->active() || !player->isSpawned() )
			continue;
		if ( hculUpdate.send(player->teulIndex()) < 0 )
		{
			dbugLog(_FLI, "<FAIL");
		}
	}

	g_newLocations = false;

	return true;
}

//////////////////////////////////////////////////////////////////////

static void
_changeHome(const ChatHandle handle, const UINT32 brigadeID, const FULL_OBJ_ID destinationID)
{
	HCUnit* const unit = HCUnit::Lookup(brigadeID);
	if ( unit == nullptr )
		return;

	if ( destinationID.id != 0 && destinationID != unit->homeCPID() )
		chatAnnounceBrigadeMove(*unit, unit->homeCPID(), destinationID, handle);

	unit->setHomeCPID(destinationID);
}

void
HCUnit::setHomeCPID(const FULL_OBJ_ID destination)
{
	m_homeCPID = destination;

	if ( isBrigade() || isDivision() )
	{
		lastMovements[unitID()] = teulTime();
		g_newLocations = true;
	}
}

//////////////////////////////////////////////////////////////////////
// Process an incoming brigade move message
void
BrigadeMove::process(const ChatHandle byHandle) const
{
	Player* player = PlayerInterface::Lookup(byHandle);
	Property* destination = chatFindProperty(m_destination);

	LSTRING_ID response = LSTR_NONE;
	const char* text = nullptr;
	char temp[16];

	switch (m_action)
	{
	case BMR_APPROVED:
		if ( m_actionable )
			_changeHome(byHandle, m_brigadeID, m_destination);
		else
			chatBrigadeMoveProposal(m_brigadeID, m_destination, byHandle);
	break;
	case BMR_FAILED:
		response = LSTR_UNIT_MOVE_FAILED;
	break;
	case BMR_FAILED_TARGET:
		response = LSTR_INVALID_TARGET;
		text = (destination) ? destination->Name() : "<unknown>";
	break;
	case BMR_FAILED_OWNER:
		response = LSTR_ENEMY_OWNS;
		text = (destination) ? destination->Name() : "target";
	break;
	case BMR_FAILED_CONTROL:
		response = LSTR_MUST_OAC;
		text = (destination) ? destination->Name() : "target";
	break;
	case BMR_FAILED_PERMISSION:
		response = LSTR_NOT_ACTIVE_HC;
	break;
	case BMR_FAILED_TIME:
		response = LSTR_UNIT_MOVE_DELAY;
		text = chatTimeInterval(m_delay);
	break;
	case BMR_FAILED_LINK:
		response = LSTR_HQU_ONE_LINK;
	break;
	case BMR_FAILED_CONTACT:
		response = LSTR_HQU_FAILED_CONTACT;
		text = (destination) ? destination->Name() : "target";
	break;
	case BMR_FAILED_ABS:
		response = LSTR_HQU_FAILED_ABS;
		text = (destination) ? destination->Name() : "target";
	break;
	case BMR_FAILED_NAVAL:
		response = LSTR_HQU_FAILED_NAVAL;
		text = (destination) ? destination->Name() : "target";
	break;
	case BMR_FAILED_AIR:
		response = LSTR_HQU_FAILED_AIR;
		text = (destination) ? destination->Name() : "target";
	break;
	case BMR_FAILED_FRONTLINE:
		response = LSTR_HQU_FAILED_FRONTLINE;
		text = (destination) ? destination->Name() : "target";
	break;
	case BMR_FAILED_NFL:
		response = LSTR_HQU_FAILED_NFL;
	break;
	case BMR_FAILED_OFFMAP:
		response = LSTR_HQU_FAILED_OFFMAP;
	break;
	case BMR_FAILED_OFFMAP_NP:
		response = LSTR_HQU_FAILED_OFFMAP_NP;
	break;
	case BMR_FAILED_PREM_SURDR:
			response = LSTR_HQU_FAILED_PREM_SURDR;
			strprintf(temp, "%14u", host_config.supply.surrenderInf);
			text = temp;
	break;
	case BMR_FAILED_ENEMY_DEPOT:
		response = LSTR_HQU_FAILED_ENEMY_DEPOT;
	break;
	case BMR_FAILED_ENEMY_FB:
		response = LSTR_HQU_FAILED_ENEMY_FB;
	break;
#ifdef PUBLIC_RELEASE
	default:
		response = LSTR_INTERNAL_ERROR;
#endif
	}

	if ( player )
	{
		if ( response != LSTR_NONE )
			player->message(LSTR_CAUTION, response, text);
		else if ( text )
			player->message(LSTR_CAUTION, LSTR_GENERIC, text);
	}
}
