#include <syscfg.h> // Path is project specific.
#include "client/ww2Client.h"
#include "map/mapPersonaSummary.h"
#include "map/mapMissionList.h"
#include "client/client.h"
#include "miss/missionDetail.h"
#include "chat/chatWhoList.h"
#include "chat/chatWaypointExchg.h"
#include "wwii/wwiiSquadMsg.h"
#include "chat/chatSquadControl.h"
#include "updt/updtTypes.h"
#include "chat/chatMissionInvite.h"
#include "client/clientResupplyIssue.h"
#include "fp/fpFPS.h"
#include "tut/tutGroupProgress.h"
#include "wwii/wwiiOpinionPoll.h"
#include "fio/fio.h"

#include "client_revision.h"

// WWIIOL-specific.
#include "wwii/wwiiHCUnits.h"
#include "map/mapBrigadeSummary.h"
#include "miss/missionListUpdate.h"
#include "chat/chatMissionMemberUpdates.h"
#include "wwii/wwiiHCUnits.h"
#include "map/mapHCUnitLocations.h"
#include "map/mapSwingMeter.h"
#include "chat/chatRecruiterList.h"
#include "chat/chatMissionInviteMsg.h"

const UINT32 WWII::svnRevisionNo = SVN_REVISION_NO;
const char* const WWII::svnRevisionStr = SVN_REVISION_STR;

namespace Chat
{
	struct MissionStatus;
};

void clientForcedCellExit(SINT32 teulIndex, UINT32 exitCode)
{
	ww2::Client* client = ww2::Client::FromTeulIndex(teulIndex);
	if(client)
	{
		clientErrorLog("t:%d ec:%u fce", teulIndex, exitCode);
		client->ProcessForcedCellExit(exitCode);
	}
	else
	{
		clientErrorLog("t:%d ec:%u fce/no client", teulIndex, exitCode);
	}
}

void clientActivateCell(SINT32 teulIndex, LOGICAL on, UINT32 host, UINT32 port)
{
	ww2::Client* client = ww2::Client::FromTeulIndex(teulIndex);
	if(client)
	{
		clientErrorLog("t:%d on:%u h:%u p:%u aCell", teulIndex, on, host, port);
		client->ActivateCell();
	}
	else
	{
		clientErrorLog("t:%d aCell/no client", teulIndex);
	}
}

void clientWorldUpdate(SINT32 teulIndex, struct WORLD_UPDATE* wu)
{
	ww2::Client* client = ww2::Client::FromTeulIndex(teulIndex);
	if(client)
		client->ProcessWorldUpdate(*wu);
}

void clientActivateMap(SINT32 teulIndex, LOGICAL on, UINT32 host, UINT32 port)
{
	ww2::Client* client = ww2::Client::FromTeulIndex(teulIndex);
	if(client)
	{
		clientErrorLog("t:%d on:%u h:%u p:%u aMap", teulIndex, on, host, port);
		client->ActivateMap();
	}
	else
	{
		clientErrorLog("t:%d aMap/no client", teulIndex);
	}
}

void clientMissionPostReply(SINT32 teulIndex, struct MISSION_POST_REPLY* reply)
{
	ww2::Client* client = ww2::Client::FromTeulIndex(teulIndex);
	if(client)
		client->ProcessMissionPostReply(*reply);
}

void clientMissionRoomListReply(SINT32 teulIndex,  RoomList* roomlist)
{
	ww2::Client* client = ww2::Client::FromTeulIndex(teulIndex);
	if(client)
		client->ProcessMissionRoomListReply(*roomlist);
}

void clientMissionLaunchUpdate(SINT32 teulIndex, struct MISS_LAUNCH_UPDATE* launchupdate)
{
	ww2::Client* client = ww2::Client::FromTeulIndex(teulIndex);
	if(client)
		client->ProcessMissionLaunchUpdate(*launchupdate);
}

void clientMissionResultsReply(SINT32 teulIndex, struct MISSION_RESULTS* results)
{
	ww2::Client* client = ww2::Client::FromTeulIndex(teulIndex);
	if(client)
		client->ProcessMissionResults(*results);
}

void clientProcessWeapList(SINT32 teulIndex, struct ACTION_OBJECT* a)
{
	ww2::Client* client = ww2::Client::FromTeulIndex(teulIndex);
	if(client)
		client->ProcessWeapList(a);
}

void clientMissionDetailReply(SINT32 teulIndex, MissionDetail& result)
{
	ww2::Client* client = ww2::Client::FromTeulIndex(teulIndex);
	if(client)
		client->ProcessMissionDetail(result);

}

void clientProcessPostAccept(SINT32 teulIndex, struct FE_POST_ACCEPT* postAccept)
{
	ww2::Client* client = ww2::Client::FromTeulIndex(teulIndex);
	if(client)
	{
		clientErrorLog("t:%d pAcc", teulIndex);
		client->ProcessPostAccept(*postAccept);
	}
	else
	{
		clientErrorLog("t:%d pAcc/no client", teulIndex);
	}
}

void clientProcessPostDeny(SINT32 teulIndex, struct FE_POST_DENY* postDeny)
{
	ww2::Client* client = ww2::Client::FromTeulIndex(teulIndex);
	if(client)
	{
		clientErrorLog("t:%d pDen", teulIndex);
		client->ProcessPostDeny(*postDeny);
	}
	else
	{
		clientErrorLog("t:%d pDen/no client", teulIndex);
	}
}

void clientProcessPostExpired(SINT32 teulIndex, struct FE_POST_EXPIRED* postExpired)
{
	ww2::Client* client = ww2::Client::FromTeulIndex(teulIndex);
	if(client)
		client->ProcessCrewPostExpired(*postExpired);

}

void clientProcessPostList(SINT32 teulIndex, struct FE_POST_LIST_RESPONSE* postListResponse)
{
	ww2::Client* client = ww2::Client::FromTeulIndex(teulIndex);
	if(client)
		client->ProcessPostList(*postListResponse);
}

void clientProcessJoinDeny(SINT32 teulIndex, struct FE_JOIN_DENY* joinDeny)
{
	ww2::Client* client = ww2::Client::FromTeulIndex(teulIndex);
	if(client)
		client->ProcessCrewJoinDeny(*joinDeny);
}

void clientProcessJoinPerm(SINT32 teulIndex, struct FE_JOIN_PERM_REQUEST* joinPermRequest)
{
	ww2::Client* client = ww2::Client::FromTeulIndex(teulIndex);
	if(client)
		client->ProcessCrewJoinPerm(*joinPermRequest);
}

void clientProcessJoinAccept(SINT32 teulIndex, struct FE_JOIN_ACCEPT* joinAccept)
{
	ww2::Client* client = ww2::Client::FromTeulIndex(teulIndex);
	if(client)
		client->ProcessCrewJoinAccept(*joinAccept);
}

void clientProcessSpawnNotify(SINT32 teulIndex, class FESpawnNotify const& spawnNotify)
{
	ww2::Client* client = ww2::Client::FromTeulIndex(teulIndex);
	if(client)
	{
		clientErrorLog("t:%d sNot", teulIndex);
		client->ProcessSpawnNotify(spawnNotify);
	}
	else
	{
		clientErrorLog("t:%d sNot/no client", teulIndex);
	}
}

void clientProcessCrewBailed(SINT32 teulIndex, struct FE_CREW_BAILED* crewBailed)
{
	ww2::Client* client = ww2::Client::FromTeulIndex(teulIndex);
	if(client)
		client->ProcessCrewBailed(*crewBailed);
}

void clientChatProcessMotdReply(SINT32 teulIndex, const MOTD_REPLY& reply)
{
	ww2::Client* client = ww2::Client::FromTeulIndex(teulIndex);
	if(client)
		client->ProcessChatMotdReply(reply);
}

void clientChatProcessHostMessage(SINT32 teulIndex, const Chat::HostMessage* message)
{
	ww2::Client* client = ww2::Client::FromTeulIndex(teulIndex);
	if(client)
		client->ProcessChatHostMessage(*message);
}

void clientChatTuneToChannel(SINT32 teulIndex, const Chat::TuneToChannel* tuneToChannel)
{
	ww2::Client* client = ww2::Client::FromTeulIndex(teulIndex);
	if(client)
		client->ProcessChatTuneToChannel(*tuneToChannel);
}

void clientChatReceiveNLA(SINT32 teulIndex, const Chat::UserNLA* userNLA)
{
	ww2::Client* client = ww2::Client::FromTeulIndex(teulIndex);
	if(client)
		client->ProcessChatNLA(*userNLA);
}

void clientChatProcessLocalizedMessage(SINT32 teulIndex, const Chat::LocalizedMessage* lmsg)
{
	ww2::Client* client = ww2::Client::FromTeulIndex(teulIndex);
	if(client)
		client->ProcessChatLocalizedMessage(*lmsg);
}

void clientChatProcessGCIMessage(SINT32 teulIndex, const Chat::GCIMessage* pgciMsg)
{
	ww2::Client* client = ww2::Client::FromTeulIndex(teulIndex);
	if(client)
		client->ProcessChatGCIMessage(*pgciMsg);
}

void clientChatProcessServerInfo(SINT32 teulIndex, const ChatServerInfo* serverInfo)
{
	ww2::Client* client = ww2::Client::FromTeulIndex(teulIndex);
	if(client)
	{
		clientErrorLog("t:%d srvInf", teulIndex);
		client->ProcessChatServerInfo(*serverInfo);
	}
	else
	{
		clientErrorLog("t:%d srvInf/no client", teulIndex);
	}
}

void clientProcessChatMissionStatus(int teulIndex, const Chat::MissionStatus* ms)
{
	ww2::Client* client = ww2::Client::FromTeulIndex(teulIndex);
	if(client)
		client->ProcessMissionStatus(ms);
}

SINT32 PersonaSummaries::receive(SINT32 teulIndex)
{
	ww2::Client* client = ww2::Client::FromTeulIndex(teulIndex);
	if(client)
		client->ProcessPersonaSummaries(*this);

	return 0;
}

SINT32 PersonaSelection::receive(SINT32 teulIndex)
{
	ww2::Client* client = ww2::Client::FromTeulIndex(teulIndex);
	if(client)
		client->ProcessPersonaSelection(*this);

	return 0;
}

SINT32 BrigadeSummaries::receive(SINT32 teulIndex)
{
	ww2::Client* client = ww2::Client::FromTeulIndex(teulIndex);
	if(client)
		client->ProcessBrigadeSummaries(*this);

	return 0;
}

SINT32 BrigadeSelection::receive(SINT32 teulIndex)
{
	ww2::Client* client = ww2::Client::FromTeulIndex(teulIndex);
	if(client)
		client->ProcessBrigadeSelection(*this);

	return 0;
}

SINT32 MissionList::receive(SINT32 teulIndex)
{
	ww2::Client* client = ww2::Client::FromTeulIndex(teulIndex);
	if(client)
		client->ProcessMissionList(*this);

	return 0;
}

void ResupplyIssue::process()
{
	// call fps stuff here
	fpFPSResupplySelf(chatGetWideCallsign(m_fromHandle));
}

template <>
SINT32 MISSION_LIST_UPDATE::receive(SINT32 teulIndex, MISSION_LIST_UPDATE& incoming)
{
	UINT32 brigadeID = (UINT32)incoming.key();

	ww2::Client* client = ww2::Client::FromTeulIndex(teulIndex);
	if(client)
		client->ProcessMissionListUpdate(brigadeID);


	return 0;
}

//////////////////////////////////////////////////////////////////////
// Receive a map HCUnit Locations message

template<>
SINT32 HC_UNIT_LOCATIONS::receive(SINT32 teulIndex, HC_UNIT_LOCATIONS& incoming)
{
	ww2::Client* client = ww2::Client::FromTeulIndex(teulIndex);
	if(!client)
		return 0;

	Write_Debug("Receiving %u incoming HC Unit Locations", incoming.size());

	for ( UINT32 i = 0; i < incoming.size(); ++i )
		client->ProcessHCUnitLocation(incoming.data(i));

	incoming.clear();

	return 0;
}

//////////////////////////////////////////////////////////////////////
// Receive a CHAT_WHO_LIST

template <>
SINT32 CHAT_WHO_LIST::receive(SINT32 teulIndex, CHAT_WHO_LIST& incoming)
{
	///NOTES
	// rev 1 - key and flags are unused, eventually they will
	// help distinguish which who list we are receiving.

	ww2::Client* client = ww2::Client::FromTeulIndex(teulIndex);
	if(!client)
		return 0;

	// The owner field of the incoming who list will tell us
	// what kind of ChatWhoList this is.
	// - CWT_SQUAD is an update of squad members not-intended for display
	// - CWT_WHO = generic list
	// - CWT_TRAINERS = list of trainers online
	// - CWT_FRIENDS = friends list
	// - CWT_IGNORE = ignore list
	// - CWT_HC = high command list
	int type = (int)incoming.owner();

	if ( incoming.empty() )
	{
		client->ProcessEmptyChatWhoList(type);
		return 0;
	}

	for ( UINT32 i = 0; i < incoming.size(); ++i )
	{
		client->ProcessChatWho(type, incoming.data(i));
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////
// Receive a squad RECRUITER_LIST

template <>
SINT32 RECRUITER_LIST::receive(SINT32 teulIndex, RECRUITER_LIST& incoming)
{
	///NOTES
	// rev 1 - key and flags are unused, eventually they will
	// help distinguish which who list we are receiving.

	ww2::Client* client = ww2::Client::FromTeulIndex(teulIndex);
	if(!client)
		return 0;

	if(!incoming.empty())
	{
		for ( UINT32 i = 0; i < incoming.size(); ++i )
			client->ProcessRecruiter(incoming.data(i), (i == 0));
	}
	else
	{
		client->ProcessRecruiter(true);
	}

	return 0;
}

// Receive a single waypoint from the host
template<>
SINT32 WAYPOINT_XFER::receive(SINT32 teulIndex, WAYPOINT_XFER& incoming)
{
	ww2::Client* client = ww2::Client::FromTeulIndex(teulIndex);
	if(client)
		client->ProcessWaypoint(incoming.data(), (int)incoming.key());

	return 0;
}

// Receive a waypoint [request] control message
template<>
SINT32 WAYPOINT_CONTROL::receive(SINT32 teulIndex, WAYPOINT_CONTROL& incoming)
{
	// A control message updating the status of a waypoint or a request for a waypoint
	ww2::Client* client = ww2::Client::FromTeulIndex(teulIndex);
	if(client)
		client->ProcessWaypointControl(incoming.data(), (int)incoming.flags(), incoming.key());

	return 0;
}

// Receive an array of waypoint posts
template<>
SINT32 WAYPOINT_XFER_ARRAY::receive(SINT32 teulIndex, WAYPOINT_XFER_ARRAY& incoming)
{
	ww2::Client* client = ww2::Client::FromTeulIndex(teulIndex);
	if(!client) return 0;

	// we were clearing waypoints out here, but this was clearing the mission origin
	// and target waypoints that get set on mission join. This only ever gets called
	// when the player joins a mission that already has multiple waypoints specified
	// leading to inconsistent waypoint clearing behavior

	for ( UINT32 i = 0; i < incoming.size(); ++i )
		client->ProcessWaypoint(incoming.data(i), WPXT_POST);

	return 0;
}

template <>
SINT32 MISSION_MEMBER_UPDATES::receive(SINT32 teulIndex, MISSION_MEMBER_UPDATES& incoming)
{
	ww2::Client* client = ww2::Client::FromTeulIndex(teulIndex);
	if(!client)
		return 0;

	for ( UINT32 i = 0; i < incoming.size(); ++i )
	{
		client->ProcessMissionMemberUpdate(incoming.data(i));
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////
// Receive a SQUAD_MSG

template <>
SINT32 SQUAD_MSG::receive(SINT32 teulIndex, SQUAD_MSG& incoming)
{
	ww2::Client* client = ww2::Client::FromTeulIndex(teulIndex);
	if(!client)
		return 0;

	if(incoming.data().isInvite())
		client->ProcessSquadInvite(incoming.data());
	else
		client->ProcessSquad(incoming.data());

	return 0;
}

//////////////////////////////////////////////////////////////////////
// Receive a SquadControlResult message

SINT32 SquadControlResult::receive(SINT32 teulIndex)
{
	ww2::Client* client = ww2::Client::FromTeulIndex(teulIndex);
	if(!client) return 0;

	client->ProcessSquadControl(*this);

	return 0;
}

//////////////////////////////////////////////////////////////////////
// Receive an incoming SwingMeter message

template <>
SINT32 SWING_METER_MSG::receive(SINT32 teulIndex, SWING_METER_MSG& incoming)
{
	const SwingMeter& meter = incoming.data();

	// Values available:
	//  meter.meter() = the swing value. -3 = Allied favor, +3 = axis favor
	//  meter.inEffect() = true if side balancing is in effect

	ww2::Client* client = ww2::Client::FromTeulIndex(teulIndex);

	if(client)
		client->ProcessSwingMeter(meter);

	return 0;
}

//////////////////////////////////////////////////////////////////////
// Receive an incoming opinion poll description

template<>
SINT32 OPINION_POLL_MSG::receive(SINT32 teulIndex, OPINION_POLL_MSG& incoming)
{
	OpinionPoll& poll = incoming.data();
	// Check if this is a valid poll or an empty stub
	if ( poll.pollID() <= 0 || poll.size() < 2 )
		return 0;

	ww2::Client* client = ww2::Client::FromTeulIndex(teulIndex);
	if(client)
	{
		client->ProcessOpinionPoll(poll);
	}

	return 0;
}

template <> SINT32
MISSION_INVITE_MSG::receive(SINT32 teulIndex, MISSION_INVITE_MSG& incoming)
{
	ww2::Client* client = ww2::Client::FromTeulIndex(teulIndex);
	if(client)
		client->ProcessMissionInvite(incoming.data());

	return 0;
}

void clientRemoveVehicle(SINT32 teulIndex, UINT32 index)
{
	ww2::Client* client = ww2::Client::FromTeulIndex(teulIndex);
	if(client)
		client->ProcessRemoveVehicle(index);
}

void clientProcessPositionUpdateMsg(SINT32 teulIndex, VEHICLE_UPDATE_HEADER* VehicleHeader)
{
	ww2::Client* client = ww2::Client::FromTeulIndex(teulIndex);
	if(client)
		client->ProcessPositionUpdateMsg(*VehicleHeader);
}

void clientProcessVehicleRequestResponse(SINT32 teulIndex, VEHICLE_REPLY* Reply)
{
	ww2::Client* client = ww2::Client::FromTeulIndex(teulIndex);
	if(client)
		client->ProcessVehicleRequest(*Reply);
}

void clientProcessVehicleHit(int teulIndex, Vehicle::Hit const& hit)
{
	ww2::Client* client = ww2::Client::FromTeulIndex(teulIndex);
	if(client)
		client->ProcessVehicleHit(hit);
}

bool clientReceiveServerTrackedObject(int teulIndex, int type, ACTION_OBJECT* a)
{
	ww2::Client* client = ww2::Client::FromTeulIndex(teulIndex);
	if(!client) return false;

	return client->ReceiveServerTrackedObject(type, a);
}

#include "client/GameClient.h"	// RAMPSTER!
SINT32 Tutorial::GroupProgress::receive(Player* player) const
{
	PlayerClient()->ProcessTutorialFlags(m_groupHandle, m_flags);
	return 0;
}
