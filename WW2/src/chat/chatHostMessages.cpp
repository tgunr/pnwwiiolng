#include <syscfg.h> // Path is project specific.
// Date: Wed Jul 26 14:57:05 2000
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson

#include "versionNo.h"

#include "chat/chatBrigadeControl.h"
#include "chat/chatBrigadeControlList.h"
#include "chat/chatBrigadeMove.h"
#include "chat/chatBroadcast.h"
#include "chat/chatBroadcastObject.h"
#include "chat/chatCellTypes.h"
#include "chat/chatChannelList.h"
#include "chat/chatConstants.h"
#include "chat/chatData.h"
#include "chat/chatDB.h"
#include "chat/chatDirectMessage.h"
#include "chat/chatDotCommand.h"
#include "chat/chatEditStringMsg.h"
#include "chat/chatGCIMessage.h"
#include "chat/chatGenericStringMsg.h"
#include "chat/chatGridTypes.h"
#include "chat/chatHoldFallVote.h"
#include "chat/chatHostBrigadeControl.h"
#include "chat/chatHost.h"
#include "chat/chatHostMessage.h"
#include "chat/chatHostMessages.h"
#include "chat/chatHostMotds.h"
#include "chat/chatLatLong.h"
#include "chat/chatList.h"
#include "chat/chatLocality.h"
#include "chat/chatLocalityUpdate.h"
#include "chat/chatLocalizedMessage.h"
#include "chat/chatMissionInviteMsg.h"
#include "chat/chatMissionMemberUpdates.h"
#include "chat/chatMissions.h"
#include "chat/chatMissionTypes.h"
#include "chat/chatModReport.h"
#include "chat/chatMotds.h"
#include "chat/chatName.h"
#include "chat/chatOrdnanceSTO.h"
#include "chat/chatPersonaUpdate.h"
#include "chat/chatPlayer.h"
#include "chat/chatSaveChannels.h"
#include "chat/chatServerTrackedObjects.h"
#include "chat/chatSideMessage.h"
#include "chat/chatSquadControl.h"
#include "chat/chatSquadList.h"
#include "chat/chatSquadListTypes.h"
#include "chat/chatSTOJoules.h"
#include "chat/chatTuneToChannel.h"
#include "chat/chatTypes.h"
#include "chat/chatUpdateChannel.h"
#include "chat/chatUserNLA.h"
#include "chat/chatWaypointExchg.h"
#include "chat/chatWaypoints.h"
#include "chat/chatWhoList.h"
#include "chat/chatClientMessages.h"
#include "host/hostConstants.h"
#include "host/hostTimeUpdate.h"
#include "inst/instHost.h"
#include "map/mapConstants.h"
#include "map/mapHCUnitLocations.h"
#include "map/mapPersonaUpdate.h"
#include "miss/missionMobileSpawnWaypoint.h"
#include "miss/missLeaderChange.h"
#include "score/scorePacket.h"
#include "score/scoreUMSKill.h"
#include "server/serverInterface.h"
#include "server/serverPlayerStatusNode.h"
#include "strt/strtAttackOrder.h"
#include "strt/strtChatTypes.h"
#include "strt/strtConstants.h"
#include "teul/teulEndpointDefs.h"
#include "teul/teulPacket.h"
#include "teul/teulServer.h"
#include "teul/teulTickSample.h"
#include "teul/teulTransmission.h"
#include "tut/tutGroupProgress.h"
#include "wwii/wwiiCallbacks.h"
#include "wwii/wwiiConfig.h"
#include "wwii/wwiiNetPorts.h"
#include "wwii/wwiiPack.h"
#include "wwii/wwiiServerTrackedObject.h"
#include "wwii/wwiiServerTypeEnums.h"
#include "wwii/wwiiSquadMsg.h"
#include "wwii/wwiiThreadTextQuery.h"

// Raknet related
#include "strt/strtUpdates.h"
#include "include/wthrTime.h"
#include "strt/strtCPFlags.h"
#include "chat/chatRecruiterList.h"
#include "chat/chatPreference.h"
#include "chat/chatCellXfer.h"

typedef teul_network_object<MobileSpawnWaypoint> MOBILE_SPAWN_WAYPOINT;

TEUL_CALLBACK*	CHAT_SEND_MESSAGE;
TEUL_CALLBACK*	CHAT_XCHG_SERVER_TRACKED_OBJS;

static TEUL_CALLBACK*	CHAT_SEND_NLA;

static TEUL_CALLBACK*	CHAT_SVR_TUNE_CHANNEL;

#ifdef SPAWN_RESTRICT
static TEUL_CALLBACK*	CHAT_SEND_SPAWN_RESTRICTION;
#endif //SPAWN_RESTRICT

// WWIIOL Specific.
TEUL_CALLBACK*	CHAT_SEND_TUNE_HANDLE_ACK;
TEUL_CALLBACK*	CHAT_SQUAD_CONTROL;
static TEUL_CALLBACK*	CHAT_MOTD_REPLY;

///TEMPORARY
#include "map/mapAWS.h"

static UINT32 g_messageID = 1;

//////////////////////////////////////////////////////////////////////
//

static SINT32
_sendMOTD_REPLY(ACTION_OBJECT* const a)
{
	MOTD_REPLY* object = (MOTD_REPLY*)(a->localRawData);
	return object->packer(TEUL_PACK, a);
}

//////////////////////////////////////////////////////////////////////
//

static SINT32
_sendSquadControlResult(ACTION_OBJECT* const a)
{
	SquadControlResult* message = (SquadControlResult*)(a->localRawData);
	return message->packer(TEUL_PACK, a);
}

//////////////////////////////////////////////////////////////////////
//

static SINT32
_sendBrigadeControl(ACTION_OBJECT* const a)
{
	BrigadeControlNotice* bcn = (BrigadeControlNotice*)(a->localRawData);
	return bcn->packer(TEUL_PACK, a);
}

//////////////////////////////////////////////////////////////////////
//

static SINT32
_sendAWS(ACTION_OBJECT* const a)
{
	AWS::Grid* grid = (AWS::Grid*)a->localRawData;
	return grid->packer(TEUL_PACK, a);
}

//////////////////////////////////////////////////////////////////////
//

static SINT32
_sendHoldFallInfo(ACTION_OBJECT* const a)
{
	a->offset = ((HoldFallVote*)(a->localRawData))->packer(TEUL_PACK, a);
	return a->offset;
}

//////////////////////////////////////////////////////////////////////
//

static SINT32
_sendChatMissionStatus(ACTION_OBJECT* const a)
{
	auto data = *(std::pair<Chat::MissionStatus*, TeulTimeMS>*)(a->localRawData);
	Chat::MissionStatus& cms = *(data.first);
	const TeulTimeMS& lastPack = data.second;
	cms.pack(a, lastPack);
	return a->offset;
}

//////////////////////////////////////////////////////////////////////
//

static SINT32
_recvGCIMessage(ACTION_OBJECT* const a)
{
	if ( a->m_client != nullptr || a->canRead<Chat::GCIMessage>() == false )
		return -EINVAL;
	auto& gciMessage = a->read<Chat::GCIMessage>();

	if ( g_logDebug )
	{
		if ( g_debugChat )
			dbugLog(_FLD, "gci:%u side:%u fac:" PRF_FULL_OBJ_ID " cp:" PRF_FULL_OBJ_ID " name:%s"
							, gciMessage.gci
							, gciMessage.side
							, PRA_FULL_OBJ_ID(gciMessage.facility)
							, PRA_FULL_OBJ_ID(gciMessage.where)
							, gciMessage.name
							);
	}

	chatRouteGCIMessage(gciMessage, g_messageID++);

	return a->offset;
}

//////////////////////////////////////////////////////////////////////
//

static SINT32
_recvSideMessage(ACTION_OBJECT* const a)
{
	if ( a->m_client != nullptr )
		return -EINVAL;

	Chat::SideMessage message(a);
	if ( a->offset < 0 )
	{
		dbugLog(_FLA, "received garbled side message from %d", a->teulIndex);
		return a->offset;
	}

	switch ( message.m_type )
	{
		case SM_SIDE:
			chatRouteSideMessage(message.m_side, message.m_prefix, message.m_message, message.m_objectiveID, message.m_arg1.c_str(), message.m_arg2.c_str());
		break;

		case SM_HC:
			chatHCMessage(message.m_side, message.m_message, message.m_arg1.c_str(), message.m_arg2.c_str());
		break;

		default:
			dbugLog(_FLA, "invalid side_message type %d", message.m_type);
			return -EINVAL;
		break;
	}

	return a->offset;
}

//////////////////////////////////////////////////////////////////////
//

static SINT32
_recvMOTD_REQ(ACTION_OBJECT* const a)
{
	REQUIRE_CLIENT_DATA();

	MOTD_REQUEST req(a);
	if ( req.type >= MOTD::_MAX_TYPES )
	{
		dbugLog(_FLA,"couldnt find motd for groupID %u", req.type);
		return -EINVAL;
	}

	MOTD_REPLY& reply = chatGetMotd(req.type);

	return teulSend(a->teulIndex, CHAT_MOTD_REPLY, &reply);
}

//////////////////////////////////////////////////////////////////////
//

static SINT32
_recvAWS(ACTION_OBJECT* const a)
{
	REQUIRE_CLIENT_DATA_AS(Player, player);
	AWS::Request request(a);

	if ( isValidSide(player.side()) == false )
	{
		dbugLog(_FLA, "player has invalid side for AWS request");
		return -EINVAL;
	}

	chatProcessAWSRequest(player, request);

	return a->offset;
}

//////////////////////////////////////////////////////////////////////
//

void
chatSendMotd(SINT32 teulIndex, MOTD_REPLY& reply)
{
	teulSend(teulIndex, CHAT_MOTD_REPLY, &reply);
}

//////////////////////////////////////////////////////////////////////
//

static SINT32
_recvLocality(ACTION_OBJECT* const a)
{
	REQUIRE_CLIENT_DATA_AS(Player, player);
	// We should only receive these now when the client is trying to tell us
	// that the player is abusing it.
	try
	{
		Chat::Locality loc(a);			// Use the ChatLocality self-unpacker
		if ( a->offset <= 0 )
			throw std::logic_error("Unable to unpack locality");

		player.updateLocality(loc.m_x, loc.m_y, loc.m_z, true, loc.m_nearCP, 0, nullObjID, 0, false, false, nullObjID, -1);
	}
	catch (std::exception& e)
	{
		dbugLog(_FLA, "t:%d _recvClientLocality: %s", a->teulIndex, e.what());
	}

	return a->offset;
}

//////////////////////////////////////////////////////////////////////
//

static SINT32
_recvSCMDResult(ACTION_OBJECT* const a)
{
	try
	{
		////FBK: Maybe the constructor should do the whole job. It would be cleaner.
		StratCommand scmd(a);		// Unpack constructor
		if ( a->offset <= 0 )
			throw std::logic_error("Unable to unpack StratCommand");

		scmd.process();
	}
	catch (std::exception& e)
	{
		dbugLog(_FLA, "t:%d _recvSCMDResult: %s", a->teulIndex, e.what());
		return -EINVAL;
	}

	return a->offset;
}

//////////////////////////////////////////////////////////////////////
// Mission host telling us a mission has been cancelled

static SINT32
_recvMissionDeleteNotice(ACTION_OBJECT* const a)
{
	if ( a->m_client != nullptr || a->canRead<UINT32>() == false )
	{
		return -EINVAL;
	}
	auto& missionID = a->read<UINT32>();
	if ( missionID > 0 )
		chatDeleteMission(missionID);

	return a->offset;
}

//////////////////////////////////////////////////////////////////////
//

static SINT32
_recvSquadControl(ACTION_OBJECT* const a)
{
	SquadControl incoming(a);

	if ( g_debugSquads )
		dbugLog(_FLD, "t:%d receiving squad control", a->teulIndex);

	if ( a->offset < 0 )
		return a->offset;

	if ( incoming.receive(a->teulIndex) < 0 )
		return -EINVAL;

	return a->offset;
}

//////////////////////////////////////////////////////////////////////
// Receive a brigade control from the player

static SINT32
_recvBrigadeControl(ACTION_OBJECT* const a)
{
	BrigadeControlRequest incoming(a);

	if ( g_debugPersonas )
		dbugLog(_FLD, "t:%d incoming brigade control action %u", a->teulIndex, incoming.m_action);

	incoming.processRequest(a->teulIndex);

	return a->offset;
}

//////////////////////////////////////////////////////////////////////
// Receive a request from a player for a list of recruiters

static SINT32
_recvRecruiterRequest(ACTION_OBJECT* const a)
{
	REQUIRE_CLIENT_DATA_AS(Player, player);

	UCHAR language = player.getPreference(CPR_PLAYER_LANGUAGE);
	a->offset = teulPackUCHAR(TEUL_UNPACK, a->packed, a->offset, &language, 0);

	chatRequestRecruiter(&player, language);

	return a->offset;
}

//////////////////////////////////////////////////////////////////////
// Send an out of band data message

void
OOBMessage::send()
{
	dbugLog(_FLD, "p:%u c:%s sending oob string message: %s", m_originatorID, Chat::Name(m_originatorID).c_str(), m_text);

	Teul::Packet outgoing(Teul::PacketId(Chat::CallbackX, Chat::OOB_MESSAGE_Y), *this);
	serverSendPacketToType(outgoing, STRAT_HOST);
}

bool
chatSendUMSKill(const Player* player, const UINT32 stoUID)
{
	Scoring::Actor actShooter(player);
	actShooter.setSortie(player->sortieID(), player->weapon(), false);
	
	Teul::Packet outgoing(Teul::PacketId(Scoring::CallbackX, Scoring::EVENT_Y));
	outgoing.construct<Scoring::UMSKill>(actShooter, stoUID);
	return serverSendPacketToType(outgoing, MAP_HOST);
}

//////////////////////////////////////////////////////////////////////
// Send function for strat command objects

SINT32
StratCommand::send()
{
	Teul::Packet outgoing(Teul::PacketId(Chat::CallbackX, Chat::EXCHANGE_SCMD_Y), *this);
	return serverSendPacketToType(outgoing, STRAT_HOST);
}

//////////////////////////////////////////////////////////////////////
//

bool
chatSendStratCommand(SINT32 playerID, int cmd, const FULL_OBJ_ID& cp, WWII_SIDE side)
{
	try
	{
		StratCommand scmd(playerID, (SCMD_ENUM)cmd, cp, side);
		scmd.send();
		dbugLog(_FLI, "%d send scmd %d for " PRF_FULL_OBJ_ID "", playerID, cmd, PRA_FULL_OBJ_ID(cp));
	}
	catch (std::exception& e)
	{
		dbugLog(_FLA, "StratCommand send error: %s", e.what());
		return false;
	}

	return true;
}

//////////////////////////////////////////////////////////////////////
// Announce the current server population to the strat host

SINT32
chatSendPopulation()
{
	Teul::Packet outgoing(Teul::PacketId(Strat::CallbackX, STRT_POPULATION_NOTICE_Y));
	outgoing.reserve(sizeof(UINT32)* MAX_SIDES);
	UINT32 sideCounts[MAX_SIDES];
	for ( UINT32 i = 0; i < MAX_SIDES; ++i )
	{
		sideCounts[i] = instGetCampaignSideCount(i);
		outgoing.append(sideCounts[i]);
	}

	dbugLog(_FLD, "sending population notice: %u:%u:%u:%u", sideCounts[0], sideCounts[1], sideCounts[2], sideCounts[3]);

	return serverSendPacketToType(outgoing, STRAT_HOST) ? 0 : -ENOTCONN;
}


//////////////////////////////////////////////////////////////////////
// Ask the mission host to cancel a particular mission

SINT32
chatSendMissionDeleteRequest(const UINT32& missionID)
{
	Teul::Packet outgoing(Teul::PacketId(Chat::CallbackX, Chat::MISSION_DELETION_Y), missionID);
	return serverSendPacketToType(outgoing, MAP_HOST) ? 0 : -ENOTCONN;
}

//////////////////////////////////////////////////////////////////////
// Send a localized message packet to this player

//////////////////////////////////////////////////////////////////////
// Asks the host to validate a brigade move; if you want it to execute
// the move too, set actionable to true.

void
chatSendBrigadeMove(const ChatHandle& byPlayer, bool isGM, UINT32 brigadeID, const FULL_OBJ_ID& fid, bool isActionable)
{
	ENDPOINT* stratEndpoint = serverPeerEndpointByType(STRAT_HOST);
	if (PN_LIKELY(stratEndpoint != nullptr)) {
		BRIGADE_MOVE_MSG outgoing(byPlayer, isGM, BRIGADE_MOVE_MSG::EXCHANGE);

		outgoing.data().m_action = BMR_APPROVED;
		outgoing.data().m_brigadeID = brigadeID;
		outgoing.data().m_destination = fid;
		outgoing.data().m_actionable = isActionable;

		outgoing.send(*stratEndpoint, BRIGADE_MOVE_MSG::EXCHANGE);
	}
}

//////////////////////////////////////////////////////////////////////
// And the comeback

template<>
SINT32
BRIGADE_MOVE_MSG::receive(SINT32 /*teulIndex*/, BRIGADE_MOVE_MSG& incoming)
{
	if ( incoming.action() != EXCHANGE ) // We should only get exchanges
	{
		dbugLog(_FLA, "Invalid brigade move msg");
		return -EINVAL;
	}

	incoming.data().process(incoming.key());

	return 0;
}

//////////////////////////////////////////////////////////////////////
// Request from the player for a squad info update

template<>
SINT32
SQUAD_MSG::receive(SINT32 teulIndex, SQUAD_MSG& incoming)
{
	Player* player = PlayerInterface::Lookup(teulIndex);
	if ( player == nullptr )
		return -EINVAL;

	try
	{
		if ( incoming.action() != STUB_ONLY )
			throw "Invalid action";
		player->sendSquadInfo();
		return 0;
	}
	catch (const char* e)
	{
		dbugLog(_FLA, "t:%d p:%u c:%s ac:%d %s", player->teulIndex(), player->playerID(), player->callsign(), incoming.action(), e);
	}
	catch (std::exception& e)
	{
		dbugLog(_FLA, "t:%d p:%u c:%s ac:%d %s", player->teulIndex(), player->playerID(), player->callsign(), incoming.action(), e.what());
	}
	return -EINVAL;
}

//////////////////////////////////////////////////////////////////////
// Receive a leadership change request message

static SINT32
_recvLeaderChange(ACTION_OBJECT* const a)
{
	if ( a->m_client != nullptr || a->canRead<LeaderChange>() == false )
	{
		dbugLog(_FLA, "t:%d invalid LeaderChange packet", a->teulIndex);
		return -EINVAL;
	}

	auto& incoming = a->read<LeaderChange>();
	if ( incoming.m_result == LeaderChange::_FAIL_IS_LEADER )
		return 0;

	ChatMission* const mission = chatFindMission(incoming.m_missionID);
	if ( !mission )
		return 0;
	if ( incoming.m_result == LeaderChange::_FAIL_NO_MISSION )
	{
		Player* const player = PlayerInterface::Lookup(incoming.m_leaderID);
		if ( player != nullptr )
		{
			mission->SetLeader(incoming.m_leaderHandle);
			return 0;
		}
	}

	Player* const player = PlayerInterface::Lookup(incoming.m_requester);
	if ( player != nullptr )
		player->message(LSTR_CAUTION, LSTR_LEADERCHANGE_FAIL);

	return 0;
}

//////////////////////////////////////////////////////////////////////
// Send a squad control result

SINT32
SquadControlResult::send(SINT32 teulIndex)
{
	dbugLog(_FLD, "t:%d res:%d[%s] sending squad control result", teulIndex, m_result, printResult());
	return teulSend(teulIndex, CHAT_SQUAD_CONTROL, this);
}

void
SquadList::sendMessage(int prefix, int message, const char* m1, const char* m2)
{
	chatDistributeLocMessage(begin(), end(), prefix, message, m1, m2);
}

//////////////////////////////////////////////////////////////////////
// Notify mmhost of a change of backup leader

void
chatSendBackupLeaderChange(UINT32 missionID, UINT32 playerID)
{
	Teul::Packet outgoing(Teul::PacketId(Chat::CallbackX, Chat::MISSION_LEADER_CHANGE_Y));
	outgoing.construct<LeaderChange>(missionID, playerID, 0, 0, LeaderChange::_SET_BACKUP_LEADER);
	serverSendPacketToType(outgoing, MAP_HOST);
}

//////////////////////////////////////////////////////////////////////
// Request a change of leader

void
chatSendLeaderChange(UINT32 missionID, UINT32 playerID, const ChatHandle& handle, const ChatHandle& requester)
{
	Teul::Packet outgoing(Teul::PacketId(Chat::CallbackX, Chat::MISSION_LEADER_CHANGE_Y));
	outgoing.construct<LeaderChange>(missionID, playerID, handle, requester, LeaderChange::_SUCCESS);
	serverSendPacketToType(outgoing, MAP_HOST);
}

//////////////////////////////////////////////////////////////////////
//

static void
_foundUser(Player& player, const CHAT_FOUND_USER& found, const PlayerStatusNode* psn)
{
	if ( psn == nullptr )
	{
		player.message(LSTR_SYSTEM, LSTR_GENERIC, "FIND: Internal error" );
		return;
	}

	Player* target = nullptr;
	target = (Player*)psn->localInfo;
	if ( target == nullptr )
	{
		player.message(LSTR_SYSTEM, LSTR_PINFO_LOGGING_INOUT, psn->dispatch().m_callsign);
		return;
	}

	if ( found.isFound == FALSE )
	{
		player.message(LSTR_CAUTION, LSTR_PINFO_NOT_ON_CELLS, target->callsign());
		return;
	}

	target->describeTo(player, found.position.rawX / 100, found.position.rawY / 100, false);
}

//////////////////////////////////////////////////////////////////////
//The send for the tune to Channel

static SINT32
_sendTuneToChannel(ACTION_OBJECT* const a)
{
	Chat::TuneToChannel* tune = (Chat::TuneToChannel*)(a->localRawData);
	return tune->packer(TEUL_PACK, a);
}

//////////////////////////////////////////////////////////////////////
//

static SINT32
_sendMessage(ACTION_OBJECT* const a)
{
	Chat::HostMessage* message = (Chat::HostMessage*)(a->localRawData);
	return message->packer(TEUL_PACK, a);
}

//////////////////////////////////////////////////////////////////////
//

static SINT32
_sendNLA(ACTION_OBJECT* const a)
{
	Chat::UserNLA* nla = (Chat::UserNLA*)(a->localRawData);
	return nla->packer(TEUL_PACK, a);
}

//////////////////////////////////////////////////////////////////////
// Send a server tracked object packet (may contain multiple STOs)

static SINT32
_sendSTO(ACTION_OBJECT* const a)
{
	STOBatch* batch = (STOBatch*)(a->localRawData);

	// 1. Send a count
	SINT32 countOffset = a->offset;
	UINT32 count = 0;
	a->offset = teulPackUINT32(TEUL_PACK, a->packed, a->offset, &count, 8);

	// 2. Send STOs until we're out or full
	UINT16 removal = ServerTrackedObject::_REMOVAL;
	ServerTrackedObject* sto;
	UINT32 removalID = 0;

	static SINT32 HEADER_SIZE = 1;
	static SINT32 REM_SIZE_LIMIT = Teul::TCPStdPayload - HEADER_SIZE - 5;	// 5 bytes for a REM
	static SINT32 ADD_SIZE_LIMIT = Teul::TCPStdPayload - HEADER_SIZE - ServerTrackedObject::_MAX_PACKED_SIZE;

	for (;; )
	{
		sto = batch->fetch();
		if ( sto == nullptr )
			break;

		// Use the type OR type REMOVAL
		if ( batch->isRemoval() )
		{
			if ( a->offset > REM_SIZE_LIMIT )
				break;

			removalID = sto->uid();
			a->offset = wwiiPackServerTrackedObjectType(TEUL_PACK, a->packed, a->offset, &removal, 0);
			a->offset = teulPackUINT32(TEUL_PACK, a->packed, a->offset, &removalID, 0);
		}
		else
		{
			if ( a->offset > ADD_SIZE_LIMIT )
				break;

			a->offset = sto->packer(TEUL_PACK, a);
		}

		++count;
		batch->next();
	}

	// Retro-fit the count
	teulPackUINT32(TEUL_PACK, a->packed, countOffset, &count, 8);

	return a->offset;
}

//////////////////////////////////////////////////////////////////////
//

SINT32
STOBatch::send(SINT32 teulIndex)
{
	m_sent = 0;

	while ( !completed() )
	{
		if ( teulSend(teulIndex, CHAT_XCHG_SERVER_TRACKED_OBJS, this) < 0 )
			break;
	}

	m_adds.clear();
	m_removes.clear();

	return 0;
}

//////////////////////////////////////////////////////////////////////
//Recv function for when the mmhost sends us player persona updates

static SINT32
_recvPersonaUpdate(ACTION_OBJECT* const a)
{
	Chat::PersonaUpdate update(a);
	if ( a->offset <= 0 )
	{
		dbugLog(_FLA, "t:%d failed unpacking persona update: %d", a->teulIndex, a->offset);
		return -EINVAL;
	}

	if ( update.m_handleID == 0 )
	{
		dbugLog(_FLA, "t:%d received personaUpdate with NULL handle", a->teulIndex);
		return -EINVAL;
	}

	Player* player = PlayerInterface::Lookup(update.m_handleID);
	if ( player == nullptr )
	{
		// This can occur when the player isn't done logging in yet.
#ifdef CLOSED_BETA
		dbugLog(_FLD, "t:%d Unable to find player %s:%lld", a->teulIndex, Chat::Name(update.m_handleID).c_str(), update.m_handleID);
#endif
		return a->offset;
	}

	if ( g_debugPersonas )
		dbugLog(_FLD, "p:%u c:%s received persona update with weapon spec %u.%u.%u.%u"
						, player->playerID(), player->callsign(), update.m_weapon.wCountry, update.m_weapon.wCategory, update.m_weapon.wClass, update.m_weapon.wType);

	player->updatePersona(update);

	return a->offset;
}

//////////////////////////////////////////////////////////////////////
// Assumption: Caller has pre-checked that player and player->localInfo
// are not null before calling us. That's why this has double underscores
// in its name. It's not only static-local its local to its parent.

static void
__describePlayer(const Player* player, char* msg, int size)
{
	snprintf(msg, size, "[%s->%s M#%d %d/%d]"
							, chatPropertyName(player->originCpID())
							, "?"
							, player->missionID()
							, player->playerID()
							, player->teulIndex()
							);
	msg[size - 1] = 0;
}

//////////////////////////////////////////////////////////////////////
//

static SINT32
_recvKickNotice(ACTION_OBJECT* const a)
{
	if ( a->m_client != nullptr || a->canRead<Chat::KickNotice>() == false )
	{
		dbugLog(_FLW, "t:%d invalid Chat::KickNotice", a->teulIndex);
		return -EINVAL;
	}

	auto& incoming = a->read<Chat::KickNotice>();
	Chat::Name name(incoming.m_handle);

	dbugLog(_FLI, "t:%d p:%u c:%s kick notice: %s"
					, a->teulIndex
					, incoming.m_playerID
					, name.c_str()
					, incoming.m_problem
					);

	SCHAR message[512] = "";	// To build the kick message in.
	Player* const player = (Player*)ServerInterface::LookupByPlayerID(incoming.m_playerID);
	if ( player )
	{
		__describePlayer(player, message, sizeof(message));
	}
	else
	{
		dbugLog(_FLW, "Kick notice for %s[%u]: player not connected to chat"
						, name.c_str()
						, incoming.m_playerID
						);
		strprintf(message, "[id: %u, not connected]", incoming.m_playerID);
	}

	chatdbRecordEvent("KICK", name.c_str(), "kicked", incoming.m_problem, message);

	if ( incoming.m_problem[0] != '*' )
	{
		strprintf(message, "KICKED %s: %s", name.c_str(), incoming.m_problem);
		chatDispatchGMMessage("HOST", message);
	}

	return a->offset;
}

//////////////////////////////////////////////////////////////////////
//

static SINT32
_recvFoundUser(ACTION_OBJECT* const a)
{
	CHAT_FOUND_USER found;

	a->offset = teulPackVoid(TEUL_UNPACK, a->packed, a->offset, &found, sizeof(CHAT_FOUND_USER));

	if ( a->offset <= 0 )
	{
		dbugLog(_FLA, "unable to unpack found user");
		return a->offset;
	}

	///TODO Send the playerID not the teulIndex
	Player* player = PlayerInterface::Lookup(found.locUser.reqConnection);
	if ( player != nullptr && player->playerID() == found.locUser.reqPlayer )
		_foundUser(*player, found, ServerInterface::PlayerStatusNodeByPlayerID(found.locUser.playerID));
	else
	{
		dbugLog(_FLW, "Requestor went away [%u req by %d on %d]"
						, found.locUser.playerID
						, found.locUser.reqPlayer, found.locUser.reqConnection
						);
	}

	return a->offset;
}

//////////////////////////////////////////////////////////////////////
//

static SINT32
_recvMessage(ACTION_OBJECT* const a)
{
	REQUIRE_CLIENT_DATA_AS(Player, player);

	Chat::ClientMessage message;
	a->offset = message.packer(TEUL_UNPACK, a);
	if ( a->offset <= 0 )
	{
		dbugLog(_FLA, "could not unpack incoming message");
		return a->offset;
	}

	player.SetUITarget(message.m_uiTarget);

	// Values not included in the transmission
	message.m_id = g_messageID++;
	message.m_country = player.country();
	message.m_handle = player.chatHandle();

	// Pull out client header information
	char *p = message.m_message;
	// Don't write control characters to the debug log file
	//dbugLog(_FLD, "Processing received chat msg: %s - size %d", p, strlen(p));
	if (*p == 0x01)
	{
		// message looks like this:
		// 0     1           13    14
		// {0x01}AJFK3dej30fh{0x01}<chat text>
		// mark start, find end
		p = strchr(p, '\x01');
		if (!p) {
			dbugLog(_FLA, "Client chat packet received with no terminating null");
			return a->offset;
		} else if (*p != nullptr) {
			// null terminate the client info string, overwrite the 0x01 header terminator
			*p++ = 0;
			char *endStartMsg = p;
			dbugLog(_FLD, "Encoded b64 client info: %s", start);

			// decode it
			message.clientInfo.decode(start);

			// remove it from the message - source is null terminated, limit to max IRC length
			strlcpy(message.m_message, endStartMsg, MAX_IRC_LEN);
			message.m_length -= (endStartMsg - message.m_message);
			dbugLog(_FLD, "Message after client header removed: %s length - %d", start, message.m_length);
		}
		else
		{
			dbugLog(_FLA, "Didn't find terminating control character, aborting");
			return a->offset;
		}
	}
	else
	{
		// Not a command that has control information
		//dbugLog(_FLW, "No client information control header found, ignoring!!");
	}

	// Prevent them typing carriage returns
	for ( p = message.m_message; *p != 0; ++p )
	{
		if ( *p == '\n' || *p == '\r' )
			*p = ' ';
	}

	chatRouteMessage(&player, &message, message.m_id, true);

	player.SetUITarget(nullObjID);

	return a->offset;
}

//////////////////////////////////////////////////////////////////////
//

static SINT32
_recvTuneToChannel(ACTION_OBJECT* const a)
{
	REQUIRE_CLIENT_DATA_AS(Player, player);

	Chat::TuneToChannel tune;

	a->offset = tune.packer(TEUL_UNPACK, a);

	if ( g_debugRadios )
		dbugLog(_FLD, "t:%d p:%u c:%s radio:%d tune to %d"
						, a->teulIndex, player.playerID(), player.callsign()
						, tune.m_radioIndex, tune.m_type);

	player.tuneRadio(tune.m_radioIndex, tune.m_type, tune.m_handle);
	chatSaveChannels(player);

	return a->offset;
}

//////////////////////////////////////////////////////////////////////
//

static SINT32
_recvUpdateChannel(ACTION_OBJECT* const a)
{
	REQUIRE_CLIENT_DATA_AS(Player, player);
	Chat::UpdateChannel update;

	update.packer(TEUL_UNPACK, a);
	if ( a->offset <= 0 )
		return a->offset;

	player.updateChannelStatus(update);

	return a->offset;
}

//////////////////////////////////////////////////////////////////////
// Incoming response from the cell host to our bandwidth change request

static SINT32
_recvBandwidthNotice(ACTION_OBJECT* const a)
{
	BANDWIDTH_NOTICE notice;

	try
	{
		a->offset = teulPackVoid(TEUL_UNPACK, a->packed, a->offset, &notice, sizeof(notice));
		notice.response[sizeof(notice.response)-1] = 0;

		if ( a->offset < 0 )
			throw std::logic_error("Unable to unpack bandwidth notice");

		if ( notice.playerID <= 1 )
			throw std::logic_error("Bandwidth response for invalid player ID");

		Player* player = PlayerInterface::Lookup(notice.playerID);
		if ( player == nullptr )
			throw std::logic_error("Player not connected");

		// For future reference, remember the update mode
		dbugLog(_FLI, "t:%d p:%u c:%s entered updatemode %d %s", player->teulIndex(), player->playerID(), player->callsign(), notice.mode, notice.response);
		player->SetPreference(CPR_UPDATE_MODE, notice.mode, false);
		if ( notice.response[0] && (notice.display || player->isGM()) )
			player->message(LSTR_NOTE, LSTR_GENERIC, notice.response);
		if ( player->isSpawned() == false )
			player->SetSpawned(true);
	}
	catch (std::exception& e)
	{
		dbugLog(_FLA, "t:%d %s: %s", a->teulIndex, __FUNCTION__, e.what());
	}

	return a->offset;
}

//////////////////////////////////////////////////////////////////////
// Receive a server tracked object request

static SINT32
_recvSTO(ACTION_OBJECT* const a)
{
	// Read-ahead the STO type without
	UINT16 type = 0;
	wwiiPackServerTrackedObjectType(TEUL_UNPACK, a->packed, a->offset, &type, 0);

	chatReceiveSTO(type, a);

	return a->offset;
}

//////////////////////////////////////////////////////////////////////
// Receive a tutorial progress update from a client

static SINT32
_recvTutProgress(ACTION_OBJECT* const a)
{
	REQUIRE_CLIENT_DATA_AS(Player, player);
	if ( g_debugPersonas )
		player.log(_FLD, "t:%d p:%u c:%s receiving tutorial progress", a->teulIndex, player.playerID(), player.callsign());

	while ( a->atEndOfBuffer() == false )
	{
		Tutorial::GroupProgress incoming(a);
		incoming.receive(&player);
	}

	return a->offset;
}

//////////////////////////////////////////////////////////////////////
// Instance the ThreadTextQuery class for aggregating

const char* const _PROGRESS_UPDATE_PREFIX = "INSERT INTO wwii.training_progress(player_id,group_name,progress)VALUES";
const char* const _PROGRESS_UPDATE_SUFFIX = "ON DUPLICATE KEY UPDATE player_id=values(player_id),group_name=values(group_name),progress=values(progress)";

static ThreadTextQuery _progressUpdates(DBThread::Default, _PROGRESS_UPDATE_PREFIX, _PROGRESS_UPDATE_SUFFIX);

SINT32
Tutorial::GroupProgress::receive(Player* player) const
{
	Chat::Name name(m_groupHandle);
#ifdef BETA_RELEASE
	player->log(_FLD, "group progress update %s=%llu", name.c_str(), m_flags);
#endif
	char entry[88];
	strprintf(entry, "(%u,\"%s\",%llu)", player->playerID(), name.c_str(), m_flags);
	_progressUpdates.addEntry(entry);
	return 0;
}

bool
Chat::CommitProgressUpdates()
{
	_progressUpdates.commit(1000);
	return true;
}

// Ask the cell host the player is currently on to change their
// bandwidth mode. Returns 'false' if we can't find the specified
// player ID or if the player is not on a cell host.

bool
chatSendBandwidthNotice(const Player& player, UINT32 mode)
{
	auto host = player.getCellHost();
	if (host == 0) {
		dbugLog(_FLD, "p:%u c:%s %s: not on a cell host", player.playerID(), player.callsign(), __FUNCTION__);
		return false;
	}
	teulindex_t peerIdx = serverGetPeerIndex(host, CELL_HOST);
	if (peerIdx < 0) {
		dbugLog(_FLD, "p:%u c:%s %s: not connected to cell %08x", player.playerID(), player.callsign(), __FUNCTION__, host);
		return false;
	}

	if (g_debugPersonas) {
		char hostAddr[64];
		teulGetAddress(host, hostAddr, sizeof(hostAddr));
		dbugLog(_FLD, "p:%u c:%s Requesting host %s give mode %u after spawning"
			, player.playerID(), player.callsign(), hostAddr, mode);
	}

	BANDWIDTH_CHANGE change{ player.playerID(), mode };
	Teul::Packet outgoing(Teul::PacketId(Chat::CallbackX, Chat::BANDWIDTH_NOTICE_Y), change);
	return outgoing.send(peerIdx);
}

bool
Chat::TuneToChannel::send(SINT32 connection)
{
	if ( g_debugRadios )
		dbugLog(_FLD, "t:%d sending tune radio #%u to type %u [%u/%u]"
						, connection
						, m_radioIndex
						, m_type
						, m_tx
						, m_rx
						);
	return teulSend(connection, CHAT_SVR_TUNE_CHANNEL, this) >= 0;
}

SINT32
chatSendUserNLA(SINT32 connection, UCHAR radioIndex, const ChatHandle& handleID)
{
	Chat::UserNLA userNLA;

	userNLA.radioIndex = radioIndex;
	userNLA.handle = handleID;

	if ( g_debugRadios )
		dbugLog(_FLD, "t:%d sending NLA on index %d", connection, radioIndex);

	return teulSend(connection, CHAT_SEND_NLA, &userNLA);
}

UINT32
nextMessageID(void)
{
	return g_messageID++;
}

//////////////////////////////////////////////////////////////////////
// Send a localized message to an entire channel

bool
chatMessageChannel(const ChannelID& channelID, LSTRING_ID prefix, LSTRING_ID message, const char* arg1, const char* arg2)
{
	Chat::Channel* channel = chatFindChannel(channelID);

	// No such channel currently
	if ( channel == nullptr )
	{
		if ( g_debugChat )
			dbugLog(_FLD, "cmc: ch:%llu.%u.%u.%llu has NULL channel", channelID.InstID(), channelID.Type(), channelID.Side(), channelID.SubID());
		return false;
	}

	Chat::LocalizedMessage lmsg(prefix, message, arg1, arg2);

	chatBroadcastLocMessage(channel->subscribers().begin(), channel->subscribers().end(), lmsg);

	return true;
}

//////////////////////////////////////////////////////////////////////
//Tells an mmhost to smite the user
SINT32
chatSendSmiteUser(UINT32 customerID, const char* handle)
{
	if ( customerID == 0 && handle[0] == 0 )
		return 0;

	CHAT_SMITE_USER smite { customerID, chatGetHandle(handle) };
	Teul::Packet outgoing(Teul::PacketId(Chat::CallbackX, Chat::SMITE_USER_Y), smite);
	return serverSendPacketToPeers(outgoing);
}

//////////////////////////////////////////////////////////////////////
// Tell the cell hosts to move a player's position

namespace Chat
{

bool
SendPlayerMove(UINT32 cellHost, Chat::PlayerMove& move)
{
	dbugLog(_FLD, "sending p:%u moving:%d/%d/%d to cell %u", move.m_playerID, move.m_x, move.m_y, move.m_z, cellHost);
	Teul::Packet outgoing(Teul::PacketId(Host::CallbackX, Host::Callbacks::MovePlayer_Y), move);
	return outgoing.send(serverGetPeerIndex(cellHost, CELL_HOST));
}

}

//////////////////////////////////////////////////////////////////////
// Query the cell host broadcast network for the whereabouts of a
// player in the game world.

void
chatSendLocateUser(Player& requester, const playerid_t playerID)
{
	CHAT_LOC_USER locUser;
	CHAT_FOUND_USER found;

	// who is asking for the info
	locUser.reqPlayer = requester.playerID();
	locUser.reqConnection = requester.teulIndex();
	locUser.playerID = playerID;			// who info is being requested of

	const PlayerStatusNode* psn = ServerInterface::PlayerStatusNodeByPlayerID(playerID);
	const Player* player = psn ? static_cast<const Player*>(psn->localInfo) : nullptr;
	auto cellHost = player ? player->getCellHost() : 0;
	if (cellHost)
	{
		dbugLog(_FLI, "Sending chatSendLocateUser for %d to %s", playerID, teulGetAddress(psn->m_curServer.address));
		Teul::Packet outgoing(Teul::PacketId(Chat::CallbackX, Chat::LOCATE_USER_Y), locUser);
		SINT32 cellIndex = serverGetPeerIndex(cellHost, CELL_HOST);
		outgoing.send(cellIndex);
	}
	else
	{
		found.locUser = locUser;

		if (psn)
		{
			found.serverType = psn->m_curServer.type;
			found.serverAddress = psn->m_curServer.address;
			found.isFound = TRUE;
		}
		else
			found.isFound = FALSE;

		_foundUser(requester, found, psn);
	}

	return;
}

//////////////////////////////////////////////////////////////////////
// Tell a cell-host to despawn a player

bool
chatSendForcedDespawn(Player* player, LSTRING_ID messageID, const char* reason)
{
	if ( false == player->isSpawned() )
		return false;

	const UINT32 cell = player->getCellHost();
	if ( 0 == cell )
		return false;
	const SINT32 cellIndex = serverGetPeerIndex(cell, CELL_HOST);
	if ( cellIndex < 0 )
		return false;

	// Tell them
	player->message(LSTR_WARNING, messageID, reason);

	Teul::Packet outgoing(Teul::PacketId(Host::CallbackX, Host::Callbacks::ForceDespawn_Y), player->playerID());
	outgoing.setUnencrypted();
	outgoing.send(cellIndex);

	return true;
}

//////////////////////////////////////////////////////////////////////
// Receive a ModReport

SINT32
_recvModReport(ACTION_OBJECT* const a)
{
	Chat::ModReport& report = a->read<Chat::ModReport>();
	ChatHandle& handle = a->read<ChatHandle>();
	Chat::SendModReport(handle, report);
	return a->offset;
}

//////////////////////////////////////////////////////////////////////
// Receive an edit-string message

template<>
SINT32
EDIT_STRING_MSG::receive(SINT32 teulIndex, EDIT_STRING_MSG& incoming)
{
	if ( incoming.action() != EXCHANGE )
	{
		dbugLog(_FLA, "t:%d f:%s invalid action %d", teulIndex, __FUNCTION__, incoming.action());
		return -EINVAL;
	}

	SINT32 result = incoming.data().receive(teulIndex);
	if ( result < 0 )
	{
		dbugLog(_FLA, "t:%d f:%s got %d from attempting to edit string", teulIndex, __FUNCTION__, result);
		return -EINVAL;
	}

	incoming.data().setError((UINT32)result);
	return incoming.send(teulIndex, EXCHANGE);
}

//////////////////////////////////////////////////////////////////////
// Receive an incoming generic string request

template<>
SINT32
GENERIC_STRING_MSG::receive(SINT32 teulIndex, GENERIC_STRING_MSG& incoming)
{
	if ( incoming.action() != EXCHANGE )
	{
		dbugLog(_FLA, "t:%d f:%s invalid action %d", teulIndex, __FUNCTION__, incoming.action());
		return -EINVAL;
	}

	return incoming.data().receive(teulIndex);
}

//////////////////////////////////////////////////////////////////////
// Receive a DIRECTMESSAGE structure from one of the hosts

SINT32
_recvDirectMessage(ACTION_OBJECT* const a)
{
	if ( a->m_client != nullptr )
	{
		dbugLog(_FLW, "t:%d client sent direct message", a->teulIndex);
		return -EINVAL;
	}

	auto& directMessage = *(Chat::DirectMessage*)(a->packed);
	// Forward the message to the player requesting it
	Player* const player = PlayerInterface::Lookup(directMessage.m_playerID);
	if ( player == nullptr )
	{
		///TODO: Lower to FLD, this is probably a player who has recently disconnected.
		dbugLog(_FLW, "t:%d sent direct message for p:%u [not found]", a->teulIndex, directMessage.m_playerID);
		return -EINVAL;
	}
	if ( player->isConnected() == false )
		return 0;
	directMessage.SetPointers();
	player->message(directMessage);
	return a->offset;
}

//////////////////////////////////////////////////////////////////////
// Receive a Chat::SystemMessage structure from one of the hosts

static SINT32
_recvBroadcastMessage(ACTION_OBJECT* const a)
{
	if ( a->m_client != nullptr )
		return -EINVAL;
	auto& broadcast = *(Chat::Broadcast*)(a->packed);
	// Route the system message it contains
	broadcast.SetPointers();
	broadcast.route();
	return a->offset;
}

void
chatSendMapPersonaUpdate(playerid_t playerID, bool isSquadUpdate, UINT32 squadID, const ChatHandle& squadHandle, bool isDecalUpdate, UINT32 decal)
{
	Teul::Packet outgoing(Teul::PacketId(Chat::CallbackX, Chat::MAP_PERSONA_UPDATE_Y));
	outgoing.construct<MapPersonaUpdate>(squadHandle, isSquadUpdate, isDecalUpdate, playerID, squadID, decal);
	serverSendPacketToType(outgoing, MAP_HOST);
}

//////////////////////////////////////////////////////////////////////
// Receive user preferences

SINT32
_recvChatPreferences(ACTION_OBJECT* const a)
{
	REQUIRE_CLIENT_DATA_AS(Player, player);
	while ( a->atEndOfBuffer() == false )
	{
		auto& pref = a->read<Chat::Preference>();
		player.SetPreference(pref.m_prefID, pref.m_value, true);
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////
// Receive a Teul::TickSample speed-hack report.

extern void chatHandleTickSample(const UINT32 source, Player& player, const Teul::TickSample& sample);

static SINT32
_recvTickSample(ACTION_OBJECT* const a)
{
	if ( a->m_client != nullptr )
		return -EINVAL;
	if ( a->canRead<Teul::TickSample>() == false )
		return -EINVAL;
	auto& sample = a->read<Teul::TickSample>();

	Player* const player = PlayerInterface::Lookup(sample.playerID);
	if ( player != nullptr )
		chatHandleTickSample(sample.source, *player, sample);

	return a->offset;
}

//////////////////////////////////////////////////////////////////////
// Receive a response to an attack order

static SINT32
_recvAttackOrder(ACTION_OBJECT* const a)
{
	if ( a->m_client != nullptr )
		return -EINVAL;
	if ( a->canRead<AttackOrder>() == false )
		return -EINVAL;
	dbugLog(_FLD, "got attack order response from strat host");
	auto& incoming = a->read<AttackOrder>();
	chatBrigadeAttackOrderResponse(incoming);
	return a->offset;
}

//////////////////////////////////////////////////////////////////////
// Recevive a game time update notification

SINT32
_recvTimeUpdate(ACTION_OBJECT* const a)
{
	if ( a->m_client != nullptr || a->canRead<Host::TimeUpdate>() == false )
		return -EINVAL;

	auto& incoming = a->read<Host::TimeUpdate>();

	wthrSetGameTime(incoming.gametime);
	wthrSetWeatherPattern(incoming.weather);
	wthrSetFogPattern(incoming.rain);

	dbugLog(_FLI, "Game Time: %u[%u]. Weather: %u/%u", incoming.gametime, incoming.factor, incoming.weather, incoming.rain);

	return a->offset;
}

//////////////////////////////////////////////////////////////////////
// Receive a list of locality updates from the cell hosts

static SINT32
_recvLocalityUpdates(ACTION_OBJECT* const a)
{
	if ( a->m_client != nullptr )
	{
		dbugLog(_FLA, "t:%d player sending us locality updates? naughty naughty", a->teulIndex);
		return -EINVAL;
	}

	UINT32 updates = 0;
	while ( a->canRead<LocalityUpdate>() )
	{
		auto& update = a->read<LocalityUpdate>();
		++updates;

		Player* player = PlayerInterface::Lookup(update.m_playerID);
		if ( player == nullptr )
			continue;

		player->updateLocality(update.m_x, update.m_y, update.m_z
								, update.m_cpChanged, update.m_nearCP
								, update.m_riding, nullObjID
								, update.m_lastPing
								, update.m_onAWS
								, update.m_inCaptureBldg
								, update.m_captureFac
								, update.m_progress
								);
	}

	dbugLog(_FLD, "received %u locality updates", updates);

	return a->offset;
}

//////////////////////////////////////////////////////////////////////

static SINT32
_recvCellHostUpdate(ACTION_OBJECT* const a, Chat::CellHostUpdate& msg)
{
	if (a->m_client != nullptr)	// host only
		return -EINVAL;
	Player* player = PlayerInterface::Lookup(msg.playerID);
	if (player == nullptr) {
		dbugLog(_FLD, "t:%d ignoring cell host update for playerid %u", a->teulIndex, msg.playerID);
		return -EINVAL;
	}
	player->SetCellHost(msg.cellHost);

	return a->offset;
}

//////////////////////////////////////////////////////////////////////
// Receive updates on CPs/facilities

static SINT32
_recvStratUpdates(ACTION_OBJECT* const a)
{
	if ( a->m_client != nullptr )
		return -EINVAL;

	UINT32 updates = 0;

	while ( a->canRead<StratUpdates>() )
	{
		auto& entry = a->read<StratUpdates>();
		if ( entry.m_fullObjID.objClass == CLASS_BUILDING )
			continue;

		Property* property = chatFindProperty(entry.m_fullObjID);
		if ( property == nullptr )
		{
			dbugLog(_FLA, "%hd.%hd:%hu unrecognized full obj id", entry.m_fullObjID.cell.ox, entry.m_fullObjID.cell.oy, entry.m_fullObjID.id);
			continue;
		}

		property->m_flags = entry.m_flags;
		if ( property->m_owner != entry.m_owner )
			dbugLog(_FLD, "%s changed owner from %u to %u", property->m_name.c_str(), property->m_owner, entry.m_owner);
		property->m_owner = (WWII_COUNTRY)entry.m_owner;
		if ( property->IsCP() )
			((PropCP*)property)->m_controller = (WWII_COUNTRY)entry.u_extra.m_controller;

		++updates;
	}

	dbugLog(_FLD, "received %u strat updates", updates);

	return a->offset;
}

//////////////////////////////////////////////////////////////////////
//This function installs our callbacks

static SINT32
_registerMMCallbacks()
{
	BEGIN_CALLBACK_REGISTRATION(Map::CallbackX)
		REGISTER_MSG_SENDER(Map::HC_UNIT_LOCATIONS_Y, HC_UNIT_LOCATIONS);
		REGISTER_RECEIVER(Map::TUT_PROGRESS_Y, _recvTutProgress);
	END_CALLBACK_REGISTRATION
}

static SINT32
_registerStratCallbacks()
{
	BEGIN_CALLBACK_REGISTRATION(Strat::CallbackX)
		REGISTER_RECEIVER(STRT_BROADCAST_TIME_UPDATE_Y, _recvTimeUpdate);
		REGISTER_RECEIVER(STRT_UPDATE_Y, _recvStratUpdates);
	END_CALLBACK_REGISTRATION
}

SINT32
chatInstallHostCallbacks()
{
	_registerMMCallbacks();
	_registerStratCallbacks();

	wwiiRegisterPlayerMessageCallback();

	BEGIN_CALLBACK_REGISTRATION(Chat::CallbackX)

		REGISTER_RECEIVER(Chat::FOUND_USER_Y, _recvFoundUser);
		REGISTER_RECEIVER(Chat::CLIENT_MESSAGE_Y, _recvMessage);
		REGISTER_RECEIVER(Chat::UPDATE_CHANNEL_Y, _recvUpdateChannel);
		REGISTER_RECEIVER(Chat::PERSONA_UPDATE_Y, _recvPersonaUpdate);
		REGISTER_RECEIVER(Chat::FE_TUNE_TO_CHANNEL_Y, _recvTuneToChannel);
		REGISTER_RECEIVER(Chat::KICK_NOTICE_Y, _recvKickNotice);

		REGISTER_SENDER(Chat::SERVER_MESSAGE_Y, _sendMessage, CHAT_SEND_MESSAGE);
		REGISTER_SENDER(Chat::SVR_TUNE_TO_CHANNEL_Y, _sendTuneToChannel, CHAT_SVR_TUNE_CHANNEL);
		REGISTER_SENDER(Chat::USER_NLA_Y, _sendNLA, CHAT_SEND_NLA);

#ifdef SPAWN_RESTRICT
		REGISTER_SENDER(Chat::SPAWN_RESTRICTION_Y, _sendSpawnRestriction, CHAT_SEND_SPAWN_RESTRICTION);
#endif //SPAWN_RESTRICT

		//Send/Recv a strat command response
		REGISTER_RECEIVER(Chat::BANDWIDTH_NOTICE_Y, _recvBandwidthNotice);

		REGISTER_MSG_SENDER(Chat::WHO_LIST_Y, CHAT_WHO_LIST);
		REGISTER_RECEIVER(Chat::MOD_REPORT_Y, _recvModReport);
		REGISTER_MSG(Chat::WAYPOINT_Y, WAYPOINT_XFER);
		REGISTER_MSG(Chat::WAYPOINT_CTRL_Y, WAYPOINT_CONTROL);
		REGISTER_MSG_SENDER(Chat::WAYPOINT_LIST_Y, WAYPOINT_XFER_ARRAY);
		REGISTER_MSG_SENDER(Chat::MISSION_MEMBER_UPDATE_Y, MISSION_MEMBER_UPDATES);

		REGISTER_MSG(Chat::EDIT_STRING_Y, EDIT_STRING_MSG);
		REGISTER_MSG(Chat::GENERIC_STRING_Y, GENERIC_STRING_MSG);

		REGISTER_MSG_RECEIVER(Chat::STO_JOULES_Y, STO_JOULES);

		REGISTER_SENDRECV(Chat::SERVER_TRACKED_OBJECTS_Y, _sendSTO, _recvSTO, CHAT_XCHG_SERVER_TRACKED_OBJS);

		REGISTER_RECEIVER(Chat::PREFERENCE_Y, _recvChatPreferences);

		REGISTER_RECEIVER(Chat::LOCALIZED_MESSAGE_Y, _recvDirectMessage);
		REGISTER_RECEIVER(Chat::BROADCAST_MESSAGE_Y, _recvBroadcastMessage);
		REGISTER_RECEIVER(Chat::TICK_SAMPLE_Y, _recvTickSample);

		// WWIIOL Specific
		REGISTER_RECEIVER(Chat::MOTD_REQ_Y, _recvMOTD_REQ);
		REGISTER_RECEIVER(Chat::CLIENT_LOCALITY_Y, _recvLocality);
		REGISTER_RECEIVER(Chat::SIDE_MESSAGE_Y, _recvSideMessage);
		REGISTER_SENDER(Chat::MOTD_REPLY_Y, _sendMOTD_REPLY, CHAT_MOTD_REPLY);
		REGISTER_SENDRECV(Chat::SQUAD_CONTROL_Y, _sendSquadControlResult, _recvSquadControl, CHAT_SQUAD_CONTROL);
		REGISTER_RECEIVER(Chat::EXCHANGE_SCMD_Y, _recvSCMDResult);
		REGISTER_RECEIVER(Chat::MISSION_DELETION_Y, _recvMissionDeleteNotice);
		REGISTER_MSG(Chat::BRIGADE_MOVE_Y, BRIGADE_MOVE_MSG);
		REGISTER_MSG(Chat::SQUAD_Y, SQUAD_MSG);
		REGISTER_RECEIVER(Chat::MISSION_LEADER_CHANGE_Y, _recvLeaderChange);
		REGISTER_SENDRECV(Chat::BRIGADE_CONTROL_Y, _sendBrigadeControl, _recvBrigadeControl, BrigadeControl::s_callback);
		REGISTER_RECEIVER(Chat::GAME_GCI_MESSAGE_Y, _recvGCIMessage);
		REGISTER_MSG_SENDER(Chat::MISSION_INVITE_Y, MISSION_INVITE_MSG);
		REGISTER_SENDRECV(Chat::AWS_Y, _sendAWS, _recvAWS, AWS::callback);
		REGISTER_SENDER(Chat::HOLD_FALL_INFO_Y, _sendHoldFallInfo, HoldFallVote::s_callback);
		REGISTER_SENDER(Chat::CHAT_MISS_STATUS_Y, _sendChatMissionStatus, Chat::MissionStatus::s_callback);
		REGISTER_MSG_SENDER(Chat::RECRUITER_LIST_Y, RECRUITER_LIST);
		REGISTER_RECEIVER(Chat::RECRUITER_REQUEST_Y, _recvRecruiterRequest);
		REGISTER_MSG(Chat::MOBILE_SPAWN_WAYPOINT_Y, MOBILE_SPAWN_WAYPOINT);
		REGISTER_RECEIVER(Chat::ATTACK_ORDER_Y, _recvAttackOrder);
		REGISTER_RECEIVER(Chat::HOST_LOCALITY_UPDATES_Y, _recvLocalityUpdates);
		REGISTER_OMG_RECEIVER(Chat::CellHostUpdate, _recvCellHostUpdate);

	END_CALLBACK_REGISTRATION
}
