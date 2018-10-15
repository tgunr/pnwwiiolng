#include <syscfg.h> // Path is project specific.
// Date: Mon Jul 31 13:32:26 2000
//
// The code behind the messages the mmhost will be sending to the chat server
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson

#include "chat/chatBrigadeMove.h"
#include "chat/chatBuzzardSpawn.h"
#include "chat/chatCellTypes.h"
#include "chat/chatConstants.h"
#include "chat/chatName.h"
#include "chat/chatPersonaUpdate.h"
#include "chat/chatServerInfo.h"
#include "chat/chatTypes.h"
#include "map/mapPersonaUpdate.h"
#include "map/mapPlayer.h"
#include "miss/mission.h"
#include "miss/missionMobileSpawnWaypoint.h"
#include "miss/missLeaderChange.h"
#include "miss/missMobileSpawn.h"
#include "mm/mmMain.h"
#include "server/serverIndexes.h"
#include "strt/strtCPStruct.h"
#include "strt/strtData.h"
#include "strt/strtProtos.h"
#include "weap/weapList.h"
#include "wwii/wwiiCallbacks.h"
#include "wwii/wwiiConfig.h"
#include "wwii/wwiiHCUnits.h"

static TEUL_CALLBACK* _SEND_PERSONA_UPDATE;
static TEUL_CALLBACK* _SEND_SERVER_INFO;

extern void mmProcessBuzzardSpawn(const SINT32 teulIndex, const BuzzardSpawn& request);

//Function actually sends a personaUpdate
static SINT32 _sendPersonaUpdate(ACTION_OBJECT* const a)
{
	return ((Chat::PersonaUpdate*)(a->localRawData))->packer(TEUL_PACK, a);
}

//The actual send function for server info
static SINT32 _sendServerInfo(ACTION_OBJECT* const a)
{
	ChatServerInfo* info = (ChatServerInfo*)(a->localRawData);
	return info->packer(TEUL_PACK, a);
}

//recvs a smite user from the chat host
static SINT32 _recvSmiteUser(ACTION_OBJECT* const a)
{
	CHAT_SMITE_USER smiteUser;

	a->offset = teulPackVoid(TEUL_UNPACK, a->packed, a->offset, &smiteUser, sizeof(CHAT_SMITE_USER));

	dbugLog(_FLW,"received a smite user for customerID %u", smiteUser.customerID);

	//Smite the user
	extern void mapSmiteCustomer(const playerid_t playerID);
	mapSmiteCustomer(smiteUser.customerID);

	return a->offset;
}

static SINT32
_recvRefreshPlayer(ACTION_OBJECT* const a)
{
	UINT32 playerID = *(UINT32*)(a->packed);

	dbugLog(_FLD, "received player refresh request for p:%u", playerID);
	Player* const player = PlayerInterface::Lookup(playerID);
	if (player) {
		player->notifyChatOfCell();
		player->updateChat();
	}

	return a->offset + Teul::BytesToBits(sizeof(UINT32));
}

static SINT32
_recvMissionDeletionRequest(ACTION_OBJECT* const a)
{
	if ( a->m_client != nullptr || a->canRead<UINT32>() == false )
		return -EINVAL;

	auto& missionID = a->read<UINT32>();
	if ( missionID <= 0 || missionCancel(missionID) < 0 )
		return -EINVAL;

	return a->offset;
}

SINT32
Chat::PersonaUpdate::send()
{
	return mmSendToChat(_SEND_PERSONA_UPDATE, this);
}

//////////////////////////////////////////////////////////////////////
// Send a message directly to a single player
//This function sends a message to a client apprising it of the location of
//	the chat server.  To be used after chatSendPlayerInfo tells the chat
//	server about the client.
//	<teulIndex> is the index you want to send the information on
//	<OTP> is the otp to use
//	<ipAddress> is the address of the chat server
//	<port> is the port of the chat server
void chatSendServerInfo(SINT32 teulIndex, UINT32 otp, UINT32 ipAddress, UINT16 port)
{
	ChatServerInfo info(otp, ipAddress, port);
	dbugLog(_FLD ,"t:%d %s(%08x, %s:%u)", teulIndex, __FUNCTION__, otp, teulGetAddress(ipAddress), port);

	teulSend(teulIndex, _SEND_SERVER_INFO, &info);
}

//////////////////////////////////////////////////////////////////////
// Let the chat host know that a mission has gone away

SINT32
chatSendMissionDeletionNotice(const UINT32& missionID)
{
	Teul::Packet outgoing(Teul::PacketId(Chat::CallbackX, Chat::MISSION_DELETION_Y), missionID);
	return serverSendPacketToType(outgoing, CHAT_HOST) ? 0 : -ENOTCONN;
}

// Constructor for a Chat::PersonaUpdate

Chat::PersonaUpdate::PersonaUpdate(
		const ChatHandle handle, WWII_COUNTRY country, BRANCH_ENUM branch, RANK_NO rank
		, const Instance::Descriptor& instance, const UINT32 sortieID
		, UINT32 brigade, UINT32 mission, const ChatHandle leader
		, const FULL_OBJ_ID origin, const FULL_OBJ_ID target, const PermissionMask mask, const WEAPON_SPEC weapon
		, bool isSpawned, bool badChecksum
	)
	: m_instance(instance)
	, m_handleID(handle)
	, m_leader(leader)
	, m_permissionMask(mask)
	, m_origin(origin)
	, m_target(target)
	, m_weapon(weapon)
	, m_missionID(mission)
	, m_brigadeID(brigade)
	, m_sortieID(sortieID)
	, m_country(country)
	, m_branch(branch)
	, m_rank(rank)
	, m_isSpawned(isSpawned)
	, m_badChecksum(host_config.arena.checksums ? badChecksum : false)
{
	if ( brigade == 0 || mission == 0 )
	{
		m_origin = nullObjID;
		m_target = nullObjID;
		m_leader = 0LL;
	}
}

//////////////////////////////////////////////////////////////////////
// Receive a brigade move notice

template<>
SINT32
BRIGADE_MOVE_MSG::receive(SINT32 teulIndex, BRIGADE_MOVE_MSG& incoming)
{
	const BrigadeMove& data = incoming.data();
	HCUnit* unit = HCUnit::Lookup((UINT32)data.m_brigadeID);
	if ( unit == nullptr ) throw std::invalid_argument("move message for unknown brigade");

	if ( data.m_destination.id != 0 )
	{
		CPStruct* cp = strtGetCP(data.m_destination);
		if ( cp == nullptr ) throw std::invalid_argument("invalid destination in brigade move");
		dbugLog(_FLD, "t:%d moving %s (#%d) to %s by %s", teulIndex, unit->name(), unit->unitID(), cp->name, Chat::Name(incoming.key()).c_str());
		unit->setHomeCPID(data.m_destination);
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////
// Send a mission mobile spawn waypoint

SINT32
chatSendMissionMobileSpawnWaypoint(const MobileSpawnWaypoint& wp)
{
	ENDPOINT* const endpoint = serverPeerEndpointByType(CHAT_HOST);
	if (endpoint != nullptr) {
		MOBILE_SPAWN_WAYPOINT outbound(wp, MOBILE_SPAWN_WAYPOINT::EXCHANGE);
		return outbound.send(*endpoint);
	}
	return -ENOTCONN;
}

template<>
SINT32
MOBILE_SPAWN_WAYPOINT::receive(SINT32 teulIndex, MOBILE_SPAWN_WAYPOINT& incoming)
{
	MobileSpawnWaypoint& wp = incoming.data();

	if ( wp.m_deployed )
	{
        dbugLog(_FLD, "mswp add m:%u p:%u c:%s sto:%u stoSubType:%d", wp.m_missionID, wp.m_playerID, Chat::Name(wp.m_ownerCall).c_str(), wp.m_stoUID, wp.m_stoSubType);
		// Request for* where* we want the MSP deployed
		missAddMobileSpawn(wp.m_missionID, wp.m_playerID, wp.m_ownerCall, wp.m_x, wp.m_y, wp.m_z, wp.m_stoUID, wp.m_stoSubType);
	}
	else
	{
		dbugLog(_FLD, "mswp del m:%u sto:%u", wp.m_missionID, wp.m_stoUID);
		missMobileSpawnCancel(wp.m_missionID, wp.m_ownerCall, (SINT32)wp.m_x, (SINT32)wp.m_y);
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////
// Incoming re1uest for a change of mission leader from the chat host

static SINT32
_recvLeaderChange(ACTION_OBJECT* const a)
{
	if ( a->m_client != nullptr || a->canRead<LeaderChange>() == false )
	{
		dbugLog(_FLA, "t:%d invalid LeaderChange packet", a->teulIndex);
		return -EINVAL;
	}
	auto& incoming = a->read<LeaderChange>();
	if ( incoming.m_result == LeaderChange::_SET_BACKUP_LEADER )
		dbugLog(_FLI, "m:%u backup leader change to p:%u"
					, incoming.m_missionID
					, incoming.m_leaderID);
	else
		dbugLog(_FLI, "m:%u leader change to p:%u c:%s"
					, incoming.m_missionID
					, incoming.m_leaderID
					, Chat::Name(incoming.m_leaderHandle).c_str()
					);

	incoming.m_result = incoming.process();

	if ( incoming.m_result != LeaderChange::_SUCCESS )
	{
		// Send result back to the originating server.
		ENDPOINT* const endpoint = a->endpoint;
		if ( endpoint != nullptr )
		{
			Teul::Packet outgoing(Teul::PacketId(Chat::CallbackX, Chat::MISSION_LEADER_CHANGE_Y), incoming);
			outgoing.send(*endpoint);
		}
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////
// Incoming update to a map persona from the chat host

SINT32
_recvMapPersonaUpdate(ACTION_OBJECT* const a)
{
	if ( a->m_client != nullptr || a->canRead<MapPersonaUpdate>() == false )
	{
		dbugLog(_FLW, "t:%d invalid MapPersonaUpdate packet", a->teulIndex);
		return -EINVAL;
	}

	auto& update = a->read<MapPersonaUpdate>();

	Player* const player = PlayerInterface::Lookup(update.m_playerID);
	if ( player == nullptr )
	{
		dbugLog(_FLW, "p:%u persona update from chat but no such player", update.m_playerID);
		return -EINVAL;
	}

    player->log(_FLD, "mpu isSquad:%c[%u/%" PR64BitUT "/%s] isDecal:%c(%u)"
                        , (update.m_isSquadUpdate ? 'y':'n')
                        , update.m_squadID, update.m_squadHandle, Chat::Name(update.m_squadHandle).c_str()
                        , (update.m_isDecalUpdate ? 'y':'n')
                        , update.m_decal
                        );

	if ( update.m_isSquadUpdate )
	{
		player->SetSquad(update.m_squadID);
		player->SetSquadHandle(update.m_squadHandle);
	}

	if ( update.m_isDecalUpdate )
	{
		player->SetDecal(update.m_decal);
	}

	return a->offset;
}

//////////////////////////////////////////////////////////////////////
// Receive a buzzard spawn packet.

static SINT32
_recvBuzzardSpawn(ACTION_OBJECT* const a)
{
	if ( a->m_client || a->canRead<BuzzardSpawn>() == false )
	{
		dbugLog(_FLA, "t:%d rxed buzzard spawn from player", a->teulIndex);
		return -EINVAL;
	}
	auto& buz = a->read<BuzzardSpawn>();
	mmProcessBuzzardSpawn(a->teulIndex, buz);

	return a->offset;
}

//////////////////////////////////////////////////////////////////////
//This function installs all the host callbacks

SINT32
chatInstallSupportCallbacks()
{
	BEGIN_CALLBACK_REGISTRATION(Chat::CallbackX)
	{
		// Send only
		REGISTER_SENDER(Chat::PERSONA_UPDATE_Y, _sendPersonaUpdate, _SEND_PERSONA_UPDATE);
		REGISTER_SENDER(Chat::SERVER_INFO_Y, _sendServerInfo, _SEND_SERVER_INFO);

		// Receive only
		REGISTER_RECEIVER(Chat::SMITE_USER_Y, _recvSmiteUser);

		REGISTER_RECEIVER(Chat::MISSION_DELETION_Y, _recvMissionDeletionRequest);

		REGISTER_MSG_RECEIVER(Chat::BRIGADE_MOVE_Y, BRIGADE_MOVE_MSG);

		REGISTER_MSG(Chat::MOBILE_SPAWN_WAYPOINT_Y, MOBILE_SPAWN_WAYPOINT);
		REGISTER_RECEIVER(Chat::MISSION_LEADER_CHANGE_Y, _recvLeaderChange);

		REGISTER_RECEIVER(Chat::BUZZARD_SPAWN_Y, _recvBuzzardSpawn);

		REGISTER_RECEIVER(Chat::MAP_PERSONA_UPDATE_Y, _recvMapPersonaUpdate);
		REGISTER_RECEIVER(Chat::MAP_REFRESH_PLAYER_Y, _recvRefreshPlayer);
	}
	END_CALLBACK_REGISTRATION(Chat::CallbackX)
}

