#include <syscfg.h> // Path is project specific.
// Created: Mon Jul 02 18:32:26 2001
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Marty "Vandal" Poulin

#include "server/serverInterface.h"
#include "server/serverMessages.h"
#include "server/serverPersonaExit.h"
#include "server/serverPersonaMirror.h"
#include "server/serverPersonaMirrorReply.h"
#include "server/serverPlayerStatusNode.h"
#include "server/serverPlayerStatusUpdate.h"
#include "server/serverServerInfo.h"
#include "teul/teulAuth.h"
#include "teul/teulCallbackStruct.h"
#include "teul/teulConstants.h"
#include "teul/teulEndpoint.h"
#include "teul/teulServer.h"
#include "wwii/wwiiCallbacks.h"
#include "wwii/wwiiConfig.h"

TEUL_CALLBACK*			SERVER_HEARTBEAT_CB;
TEUL_CALLBACK*			PERSONA_EXIT_CB;
TEUL_CALLBACK*			PLAYER_MIRROR_CB;
static TEUL_CALLBACK*	PLAYER_MIRROR_REPLY_CB;
TEUL_CALLBACK*			CLUSTER_SHUTDOWN_CB;

//	------------------------------	Shared Private Prototypes

// move players from one server list to another and update state
extern SINT32 serverUpdatePlayer(const Server::PlayerStatusUpdate* const update);

extern ERROR_FUNCTION	extErrorFn;

static SINT32
_recvPlayerUpdate(ACTION_OBJECT* const a)
{
	if ( Server::myServerInfo.flags & Server::SVRFLAG_IGNORE_PSTAT )
		return a->offset;

	if ( PN_UNLIKELY(a->canRead<Server::PlayerStatusUpdate>() == false) )
		return -EINVAL;

	Server::PlayerStatusUpdate& update = a->read<Server::PlayerStatusUpdate>();

	dbugLog(_FLD, "p:%u c:%s received update %d source %s from address %s status %d server %llu "
				,update.dispatch.m_customerID
				,update.dispatch.m_callsign
				,update.id
				,teulGetAddress(update.source)
				,teulGetAddress(a->fromAddress)
				,static_cast<int>(update.status)
                ,update.serverID);

	serverUpdatePlayer(&update);

	return a->offset;
}

static SINT32
_recvHeartBeat(ACTION_OBJECT* const a)
{
	Server::HeartBeat heartBeat;

	a->offset = teulPackVoid(TEUL_UNPACK, a->packed, a->offset, &heartBeat, sizeof(heartBeat));

	dbugWrite(_FLI,"received heartbeat from %s", teulGetAddress(heartBeat.address));
	heartBeat.process();

	return a->offset;
}

static SINT32
_recvPersonaExit(ACTION_OBJECT* const a)
{
	Server::PersonaExit persExit;
	persExit.packer(TEUL_UNPACK, a);

	dbugLog(_FLI, "t:%d p:%u c:%s otp:%u received personaExit ip:%s from %s"
				, a->teulIndex
				, persExit.dispatch().m_customerID
				, persExit.dispatch().m_callsign
				, persExit.dispatch().m_OTP
				, teulGetAddress(persExit.dispatch().m_clientIP)
				, teulGetAddress(a->fromAddress)
				);

	persExit.onReceive(a->teulIndex);

	return a->offset;
}

static SINT32
_recvClusterDown(ACTION_OBJECT* const a)
{
	dbugLog(_FLW, "t:%d received Cluster Down command", a->teulIndex);
	///TODO: Move this to the controller or signal myself to shutdown, anything
	///but invoking an external command!
	system(PN_PLAYNET_PATH "/bin/stophost");
	return -EINVAL;
}

static SINT32
_sendHeartBeat(ACTION_OBJECT* const a)
{
	Server::HeartBeat* stats = (Server::HeartBeat*)a->localRawData;
	SINT32 maxPlayers = 0;

	maxPlayers = stats->maxPlayers();
	if ( stats->status < Server::SERVER_ACCEPTING )
		stats->m_maxPlayers = 0;

	a->offset = teulPackVoid(TEUL_PACK, a->packed, a->offset, stats, sizeof(*stats));

	stats->m_maxPlayers = maxPlayers;

#ifdef HEAVY_DEBUG
	dbugLog(_FLD, "sending heartbeat status %d to connection %d",stats->status, a->teulIndex);
#endif

	return a->offset;
}

static void
_persXferError(SINT32 teulIndex, const UCHAR* const data, SINT32 size, const TEUL_CALLBACK* const object, SINT32 error)
{
	dbugLog(_FLA, "couldn't transfer persona!");

	if ( extErrorFn)
		(extErrorFn) (teulIndex, data, size, object, error);
}

static SINT32
_sendPersonaExit(ACTION_OBJECT* const a)
{
	Server::PersonaExit* persExit = (Server::PersonaExit*)a->localRawData;
	persExit->packer(TEUL_PACK, a);
	return a->offset;
}

static SINT32
_sendClusterDown(ACTION_OBJECT* const a)
{
	return a->offset;
}

static SINT32
_sendPlayerMirror(ACTION_OBJECT* const a)
{
	Server::PersonaMirror* const mirror = (Server::PersonaMirror*)a->localRawData;
	mirror->packer(TEUL_PACK, a);
	return a->offset;
}

static SINT32
_recvPlayerMirror(ACTION_OBJECT* const a)
{
	char auxBuffer[Teul::MaxPacket];
	Server::PersonaMirror mirror(auxBuffer);
	mirror.packer(TEUL_UNPACK, a);
	Server::PersonaMirrorReply reply(mirror);

	if ( g_logDebug )
	{
		char remoteServerAddress[64];
		teulGetAddress(a->fromAddress, remoteServerAddress, sizeof(remoteServerAddress));
		dbugLog(_FLI, "xfer:%llu(%s) p:%u c:%s otp:%u received player mirror for ip:%s"
					, mirror.xferID()
					, remoteServerAddress
					, mirror.dispatch().m_customerID
					, mirror.dispatch().m_callsign
					, mirror.dispatch().m_OTP
					, teulGetAddress(mirror.dispatch().m_clientIP)
					);
	}

	const SINT32 result = mirror.onReceive(a->teulIndex);

	reply.resultCode = (result >= 0) ? 0 : -EINVAL;

	teulSendTo(a->teulIndex, a->fromAddress, (UINT16)a->fromPort, PLAYER_MIRROR_REPLY_CB, &reply);

	return a->offset;
}

static SINT32
_sendPlayerMirrorReply(ACTION_OBJECT* const a)
{
	Server::PersonaMirrorReply* reply = (Server::PersonaMirrorReply*)(a->localRawData);
	reply->packer(TEUL_PACK, a);
	return a->offset;
}

static SINT32
_recvPlayerMirrorReply(ACTION_OBJECT* const a)
{
	Server::PersonaMirrorReply reply;
	reply.packer(TEUL_UNPACK, a);
	reply.onReceive(a->teulIndex);
	return a->offset;
}

static SINT32
_recvHostConfig(ACTION_OBJECT* const a)
{
	if ( PN_UNLIKELY(a->sizeInBytes() != sizeof(host_config)) )
	{
		dbugLog(_FLA, "broadcast host configuration size mismatch: rx:%" PRSizeT ", expected:%" PRSizeT "", (size_t)a->sizeInBytes(), sizeof(host_config));
		return -EINVAL;
	}

	auto& newConfig = a->read<WWII_CONFIG>();
	host_config = newConfig;

	dbugLog(_FLD, "received host configuration broadcast");

	return a->offset;
}

SINT32
serverInitCallbacks()
{
	BEGIN_CALLBACK_REGISTRATION(Teul::Serv::CallbackX)
	{
		REGISTER_SENDRECV(PLAYER_MIRROR_Y, _sendPlayerMirror, _recvPlayerMirror, PLAYER_MIRROR_CB);
		PLAYER_MIRROR_CB->error = _persXferError;

		REGISTER_SENDRECV(PLAYER_MIRROR_REPLY_Y, _sendPlayerMirrorReply, _recvPlayerMirrorReply, PLAYER_MIRROR_REPLY_CB);

		REGISTER_SENDRECV(SERV_HEARTBEAT_Y, _sendHeartBeat, _recvHeartBeat, SERVER_HEARTBEAT_CB);

		REGISTER_SENDRECV(CLUSTER_SHUTDOWN_Y, _sendClusterDown, _recvClusterDown, CLUSTER_SHUTDOWN_CB);

		REGISTER_SENDRECV(PLAYER_EXIT_Y, _sendPersonaExit, _recvPersonaExit, PERSONA_EXIT_CB);

		REGISTER_RECEIVER(PLAYER_UPDATE_Y, _recvPlayerUpdate);

		REGISTER_RECEIVER(HOST_CONFIG_Y, _recvHostConfig);
	}
	END_CALLBACK_REGISTRATION(Teul::Serv::CallbackX)
}

