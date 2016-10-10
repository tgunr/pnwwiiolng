#include <syscfg.h> // Path is project specific.
// Date: Thu Jul 12 10:37:50 2001
//
// The code the updater uses to talk to the chat server
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson

#include "include/clustercfg.h"

#include "chat/chatCellHostSupportMessages.h"
#include "chat/chatCellTypes.h"
#include "chat/chatConstants.h"
#include "chat/chatModReport.h"
#include "chat/chatServerTypes.h"
#include "chat/chatTypes.h"
#include "host/hostFraming.h"
#include "host/host.h"
#include "host/hostPlayer.h"
#include "host/hostVehicle.h"
#include "server/serverIndexes.h"
#include "server/serverInterface.h"
#include "server/serverPeers.h"
#include "server/serverPlayerStatusNode.h"
#include "server/serverServerInfo.h"
#include "teul/teulPacket.h"
#include "teul/teulServer.h"
#include "wwii/wwiiCallbacks.h"

static TEUL_CALLBACK* _XCHG_BANDWIDTH_NOTICE;

extern void hostSmiteCustomer(playerid_t playerID);

//The actual send function for system messages

static HostVehicle*
_findVehicleByPlayerID(const playerid_t playerID)
{
	// Since they* might* be on another cell host, we have to look them up this way
	///TODO: Don't do it this way :)
	PlayerStatusNode* psn = ServerInterface::PlayerStatusNodeByPlayerID(playerID);
	if ( psn == nullptr || psn->m_curServer.id != Server::myServerInfo.stats.id )
		return nullptr;

	const Player* player = (Player*)(psn->localInfo);
	if ( player == nullptr )
		return nullptr;

	return player->vehicle;
}

//recvs a smite user from the chat host
static SINT32
_recvLocateUser(ACTION_OBJECT* a, CHAT_LOC_USER& locUser_)
{
	Teul::Packet outgoing(Teul::PacketId(Chat::CallbackX, Chat::FOUND_USER_Y));
	auto& found = outgoing.allocate<CHAT_FOUND_USER>();
	found.locUser = locUser_;
	found.isFound = FALSE;
	found.serverAddress = Server::myServerInfo.stats.address;
	found.serverType = Server::myServerInfo.stats.type;

	HostVehicle* vref = _findVehicleByPlayerID(locUser_.playerID);
	if ( vref != nullptr )
	{
		found.isFound = TRUE;
		found.vehicleID = vref->index;
		found.position.rawX = vref->x_cm();
		found.position.rawY = vref->y_cm();
		found.position.rawZ = vref->z_cm();
		found.weapon = vref->spec();
	}

	outgoing.send(*a->endpoint);

	return a->offset;
}

//recvs a smite user from the chat host
static SINT32
_recvSmiteUser(ACTION_OBJECT* const a)
{
	CHAT_SMITE_USER smiteUser;

	smiteUser.m_handle = 0LL;
	a->offset = teulPackVoid(TEUL_UNPACK, a->packed, a->offset, &smiteUser, sizeof(CHAT_SMITE_USER));

	dbugLog(_FLN, "received a smite user for customerID %u [%s]", smiteUser.customerID, Chat::Name(smiteUser.m_handle).c_str());

	//Smite the user
	hostSmiteCustomer(smiteUser.customerID);

	return a->offset;
}

static SINT32
_sendBandwidthNotice(ACTION_OBJECT* const a)
{
	return teulPackVoid(TEUL_PACK, a->packed, a->offset, a->localRawData, sizeof(BANDWIDTH_NOTICE));
}

// Receive a bandwidth change request from the chat host
// If the request is just a query, mode will be set to 255
static SINT32
_recvBandwidthNotice(ACTION_OBJECT* const a)
{
	BANDWIDTH_CHANGE request;

	a->offset = teulPackVoid(TEUL_UNPACK, a->packed, a->offset, &request, sizeof(request));

	if ( a->offset < 0 )
	{
		dbugLog(_FLW, "invalid bandwidth notice");
		return a->offset;
	}

	Player* player = (Player*)ServerInterface::LookupByPlayerID(request.playerID);
	if ( player == nullptr )
	{
		dbugLog(_FLA, "p:%u invalid player holder", request.playerID);
		return -EINVAL;
	}

	// The request is valid and the player is local to this server
	// Execute any non-255 mode request and then either way
	// send back a description of the current mode
	BANDWIDTH_NOTICE reply;
	memset(&reply, 0, sizeof(reply));

	reply.playerID = request.playerID;

	if ( request.mode != 255 )
	{
		dbugLog(_FLD, "t:%d p:%u c:%s v:%u requested framing mode %d"
						, player->teulIndex()
						, player->playerID()
						, player->callsign()
						, (player->vehicle ? player->vehicle->index : 0)
						, request.mode
						);
		player->setFramingMode(request.mode);
	}
	else
	{
		dbugLog(_FLI, "t:%d p:%u c:%s v:%u querying framing mode"
						, player->teulIndex()
						, player->playerID()
						, player->callsign()
						, (player->vehicle ? player->vehicle->index : 0)
						);
	}

	reply.mode = player->getCurrentFramingMode();
	reply.display = (request.mode == 255);
	strsafecpy(reply.response, framings[reply.mode].description);

	teulSendTo(a->teulIndex, a->fromAddress & DIRECT_MASK, a->fromPort, _XCHG_BANDWIDTH_NOTICE, &reply);

	return a->offset;
}

//This function installs all the host callbacks
SINT32
chatInstallCellHostSupportCallbacks()
{
	BEGIN_CALLBACK_REGISTRATION(Chat::CallbackX)
	{
		//		REGISTER_SENDER(Chat::GAME_DIRECT_MESSAGE_Y, _sendDirectMessage, _SEND_DIRECT_MESSAGE);

		REGISTER_RECEIVER(Chat::SMITE_USER_Y, _recvSmiteUser);
		REGISTER_RECEIVEOBJ(Chat::LOCATE_USER_Y, CHAT_LOC_USER, _recvLocateUser);

		// Send/receive bandwidth notices
		REGISTER_SENDRECV(Chat::BANDWIDTH_NOTICE_Y, _sendBandwidthNotice, _recvBandwidthNotice, _XCHG_BANDWIDTH_NOTICE);
	}
	END_CALLBACK_REGISTRATION(Chat::CallbackX)
}

bool
chatSendKickNotice(const playerid_t playerID, const SCHAR* callsign, const SCHAR* problem)
{
	dbugLog(_FLI, "KICK notice for %s:%u %s"
					, callsign
					, playerID
					, problem
					);

	Teul::Packet outgoing(Teul::PacketId(Chat::CallbackX, Chat::KICK_NOTICE_Y));
	outgoing.construct<Chat::KickNotice>(playerID, chatGetHandle(callsign), problem);
	return serverSendPacketToType(outgoing, CHAT_HOST);
}

void
Chat::SendModReport(const ChatHandle handle, const Chat::ModReport report)
{
	Teul::Packet outgoing(Teul::PacketId(Chat::CallbackX, Chat::MOD_REPORT_Y));
	outgoing.append(report);
	outgoing.append(handle);
	serverSendPacketToType(outgoing, CHAT_HOST);
}

