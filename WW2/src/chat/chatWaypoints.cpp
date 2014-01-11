#include <syscfg.h> // Path is project specific.
// Date: Mon Mar 21 19:27:49 CST 2005
//
// Functionality for manipulating chat waypoints
//
// Copyright (C) 2005 PlayNet, Inc. All Rights Reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "chat/chatList.h"
#include "chat/chatMissionTypes.h"
#include "chat/chatMissions.h"
#include "chat/chatName.h"
#include "chat/chatPlayer.h"
#include "chat/chatWaypoint.h"
#include "chat/chatWaypointExchg.h"

#include "wwii/wwiiSerialNumber.h"		// For preserving contact report serial number of a campaign.

bool
Waypoint::operator==(const Waypoint& rhs) const
{
	if ( handle() != rhs.handle() ) return false;
	if ( playerHandle() != rhs.playerHandle() ) return false;
	if ( type() != rhs.type() ) return false;
	if ( subType() != rhs.subType() ) return false;
	if ( x() != rhs.x() ) return false;
	if ( y() != rhs.y() ) return false;
	return ( strncmp(notes(), rhs.notes(), MAX_WP_NOTE_LENGTH) == 0 );
}

static ChatHandle
_getWaypointHandle()
{
	static SerialNumber waypointSerial("waypoint", 0, 100, ~0);
	Chat::Name wpName;
	wpName.format("-%05X", (UINT32)(++waypointSerial) & 0x0fffff);
	return wpName.Handle();
}

void
Waypoint::processIncomingFrom(class Player* const player)
{
	try
	{
		// Must be on a mission
		if ( !player->missionID() )
		{
			dbugLog(_FLI, "t:%d p:%u c:%s non-mission member sent waypoint", player->teulIndex(), player->playerID(), player->callsign());
			throw WPCTRL_NO_MISSION;
		}

		// Client can't name a waypoint with "-"
		if ( type() == WPT_ENEMY )
			m_handle = _getWaypointHandle();
		else
		{
			const wchar_t prefix = chatGetWideCallsign(handle())[0];
			if ( prefix == L'-' )
			{
				dbugLog(_FLM, "t:%d p:%u c:%s waypoint handle prefixed with reserved character (%lc)", player->teulIndex(), player->playerID(), player->callsign(), prefix);
				throw WPCTRL_INVALID;
			}
		}

		if ( m_playerHandle == 0LL )
			m_playerHandle = player->chatHandle();

		ChatMission& mission = chatGetMission(player->missionID());

		if ( player->isMissionLeader() == true || leaderOnlyPosting() == false )
		{
			if ( handle() == 0 )
			{
				dbugLog(_FLM, "t:%d p:%u c:%s waypoint with handle 0", player->teulIndex(), player->playerID(), player->callsign());
				throw WPCTRL_INVALID;
			}

			if ( chatShareWaypoint(*this, &mission, player->side()) == false )
			{
				dbugLog(_FLM, "t:%d p:%u c:%s waypoint share failed", player->teulIndex(), player->playerID(), player->callsign());
				throw WPCTRL_DENIED;
			}

			if ( player->isMissionLeader() == true )
				mission.lockLeader();

			return;
		}

		if ( m_handle == 0LL )
		{
			m_handle = _getWaypointHandle();
		}

		Player* const leader = PlayerInterface::Lookup(mission.matchLeader());

		if ( leader != nullptr && leader->teulIndex() >= 0 )
		{
			betaLog(_FLI, "t:%d p:%u c:%s m:%u forwarding waypoint to mission leader %s", player->teulIndex(), player->playerID(), player->callsign(), mission.missionID(), leader->callsign());
			WAYPOINT_XFER forward(*this, WPXT_REQUEST);
			forward.send(leader->teulIndex());
		}
	}
	catch ( WPCTRL_TYPE& e )
	{
		WAYPOINT_CONTROL denied(handle(), e);
		denied.send(player->teulIndex());
	}
}

template<>
SINT32
WAYPOINT_XFER::receive(SINT32 teulIndex, WAYPOINT_XFER& incoming)
{
	// Mission leader approving/removing a waypoint
	Player* player = PlayerInterface::Lookup(teulIndex);
	if ( player == nullptr )
		return -EINVAL;							// No data on this player

	dbugLog(_FLI, "t:%d p:%u c:%s received waypoint %llu %s x:%d y:%d posted by %s"
					, player->teulIndex(), player->playerID(), player->callsign()
					, incoming.data().handle(), Chat::Name(incoming.data().handle()).c_str()
					, incoming.data().x(), incoming.data().y(), Chat::Name(incoming.data().playerHandle()).c_str()
					);

	incoming.data().processIncomingFrom(player);

	return 0;
}

template<>
SINT32
WAYPOINT_CONTROL::receive(SINT32 teulIndex, WAYPOINT_CONTROL& incoming)
{
	Player* const player = PlayerInterface::Lookup(teulIndex);
	if ( player == nullptr )
		return -EINVAL;							// No data on this player

	dbugLog(_FLD, "t:%d p:%u c:%s received waypoint control %llu[%s] %u"
					, player->teulIndex(), player->playerID(), player->callsign()
					, incoming.key(), Chat::Name(incoming.key()).c_str(), incoming.flags());

	ChatMission* const mission = chatFindMission(player->missionID());
	if ( mission == nullptr )
		return -EINVAL;

	if ( !player->isMissionLeader() )
	{
		betaLog(_FLA, "t:%d p:%u c:%s non-leader requesting waypoint %llu[%s] %u"
						, player->teulIndex(), player->playerID(), player->callsign()
						, incoming.key(), Chat::Name(incoming.key()).c_str(), incoming.flags());
		return -EACCES;
	}

	// Mission leader approving/removing a waypoint
	ChatHandle handle = incoming.key();
	WPCTRL_TYPE type = (WPCTRL_TYPE)incoming.flags();

	switch (type)
	{
	case WPCTRL_APPROVE:
		dbugLog(_FLA, "Don't understand wpctrl approve - remove it moron");
	break;

	case WPCTRL_REMOVE:
		mission->removeWaypoint(handle, true);
	break;

	default:
		dbugLog(_FLA, "Don't understand wpctrl %d", type);
	break;
	}

	return 0;
}

