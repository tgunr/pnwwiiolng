#include <syscfg.h> // Path is project specific.
// Created: Fri Nov  3 13:52:34 2000
//
// These are the messages that the host side sends and receives.  This file
// contains all the setups and such nonsense for this bit of fun.
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson

#include <queue>

#include "chat/chatConstants.h"
#include "chat/chatName.h"
#include "chat/chatStratSupportMessages.h"
#include "garrisons/garrisonCountryChangeRequest.h"
#include "host/hostConstants.h"
#include "host/hostTimeUpdate.h"
#include "include/configVars.h"
#include "include/luaClient.h"
#include "include/luaGlue.h"
#include "miss/missionDamageEvent.h"
#include "miss/missionPack.h"
#include "mm/mmSerialNumberRequest.h"
#include "server/server.h"
#include "server/serverPeers.h"
#include "strt/strtAttackOrder.h"
#include "strt/strtCP.h"
#include "strt/strtCPFlags.h"
#include "strt/strtCPStruct.h"
#include "strt/strtCaptureContribution.h"
#include "strt/strtCaptureScore.h"
#include "strt/strtCaptureState.h"
#include "strt/strtConstants.h"
#include "strt/strtDamageQueueEntry.h"
#include "strt/strtData.h"
#include "strt/strtEnemyReport.h"
#include "strt/strtFacilityStruct.h"
#include "strt/strtFacilityWeaponList.h"
#include "strt/strtHCUnits.h"
#include "strt/strtHostMessages.h"
#include "strt/strtIO.h"
#include "strt/strtJoinList.h"
#include "strt/strtMain.h"
#include "strt/strtMission.h"
#include "strt/strtObjectStruct.h"
#include "strt/strtObjectiveStats.h"
#include "strt/strtPack.h"
#include "strt/strtServerTypes.h"
#include "strt/strtSpawn.h"
#include "strt/strtSupercell.h"
#include "strt/strtSupply.h"
#include "strt/strtSyncData.h"
#include "strt/strtTypes.h"
#include "strt/strtUpdaterPack.h"
#include "strt/strtUpdates.h"
#include "strt/strtWeaponListRequest.h"
#include "strt/strtWeaponsReturn.h"
#include "teul/teulEndpoint.h"
#include "teul/teulPacket.h"
#include "teul/teulServer.h"
#include "teul/teulUtils.h"
#include "weap/weapWeapon.h"
#include "wwii/wwiiCallbacks.h"
#include "wwii/wwiiConfig.h"
#include "wwii/wwiiExitCodes.h"
#include "wwii/wwiiServerTypeEnums.h"

#undef DEBUG_SYNC

extern UINT32 strtGetNextSerial();
extern UINT32 g_chatPopulations[];

TEUL_CALLBACK *STRT_SEND_MAP_CONNECTION_REQ;

static TEUL_CALLBACK *_SEND_MM_POST_ACCEPT;
static TEUL_CALLBACK *_SEND_MM_POST_DENY;
static TEUL_CALLBACK *_SEND_MM_JOIN_ACCEPT;
static TEUL_CALLBACK *_SEND_MM_JOIN_DENY;
static TEUL_CALLBACK *_XCHG_MM_SPAWN_GRANT;
static TEUL_CALLBACK *_XCHG_MM_CAPACITY;
static TEUL_CALLBACK *_XCHG_MM_GARRISON_COUNTRY_CHANGE;

static TEUL_CALLBACK *_SEND_SYNC_CPS;
static TEUL_CALLBACK *_SEND_SYNC_FACILITIES;
static TEUL_CALLBACK *_SEND_SYNC_BUILDINGS;

static bool _allowSyncing = false;

template <class Type>
class SyncQueue : public std::vector<Type *>
{
public:
	bool queue(Type *t)
	{
		if (t == nullptr || !_allowSyncing || t->m_needsSync)
			return false;
		this->push_back(t);
		t->m_needsSync = true;
		return true;
	}

	Type *next()
	{
		if (this->empty())
			return nullptr;
		Type *t = this->front();
		this->erase(this->begin());
		if (!t->m_needsSync)
			return nullptr;
		t->m_needsSync = false;
		return (Type *)t;
	}
};

static SyncQueue<CPStruct> _cpQueue;
static SyncQueue<FacilityStruct> _facilityQueue;
static SyncQueue<ObjectStruct> _objectQueue;


void
strtAllowSyncing()
{
	_allowSyncing = true;
}

// When facility state data changes, we need to queue up the facility
// for dispatch to the other hosts -- to avoid congestion.
// The list is FIFO not FILO.
void
strtQueueVisLevelUpdate(ObjectStruct* object)
{
	_objectQueue.queue(object);
}

// When facility state data changes, we need to queue up the facility
// for dispatch to the other hosts -- to avoid congestion.
// The list is FIFO not FILO.
void
strtQueueFacilityStatus(FacilityStruct* facility)
{
	_facilityQueue.queue(facility);
}

// When facility state data changes, we need to queue up the facility
// for dispatch to the other hosts -- to avoid congestion.
// The list is FIFO not FILO.
void
strtQueueCPStatus(CPStruct* cp)
{
	_cpQueue.queue(cp);
}

static void _populateEntry(Teul::Packet &outgoing, const FacilityStruct *fac)
{
	auto &entry = outgoing.allocate<StratUpdates>();

	entry.m_fullObjID = fac->fullObjID;
	entry.m_owner = (UCHAR)fac->country;
	// Copy the capture status
	entry.u_extra.m_capture.m_stage = fac->m_captureStage;
	entry.u_extra.m_capture.m_counter = fac->m_captureCounter;
	entry.u_extra.m_capture.m_lastChange = teulTime() - fac->m_captureStateChanged;
	entry.m_flags = (UCHAR)fac->flags;
	if (fac->isOpen())
		entry.m_flags |= CP_STAT_OPEN;
	else
		entry.m_flags &= ~CP_STAT_OPEN;
}

static void _populateEntry(Teul::Packet &outgoing, const CPStruct *cp)
{
	auto &entry = outgoing.allocate<StratUpdates>();

	assert(cp->fullObjID.id != 0);
	entry.m_fullObjID = cp->fullObjID;
	entry.m_owner = (UCHAR)cp->country;
	entry.u_extra.m_controller = (UCHAR)cp->controllingCountry;
	entry.m_flags = cp->flags;
}

//////////////////////////////////////////////////////////////////////
// Send the chat host all of the current strat states

void
strtSendChatInitialUpdates()
{
	ENDPOINT *endpoint = serverPeerEndpointByType(CHAT_HOST);
	if (endpoint == nullptr)
		return;

	Teul::Packet outgoing(Teul::PacketId(Strat::CallbackX, STRT_UPDATE_Y));
	UINT32 entries = 0;

	CPStruct *cp = strtGetCPList();
	while (cp != nullptr)
	{
		// Bridges have totally BS data in them
		if (!cp->isBridge())
		{
			// Write the facilities
			FacilityStruct *fac = cp->facilityList;
			while (fac != nullptr)
			{
				_populateEntry(outgoing, fac);
				++entries;
				fac = fac->next;
			}
		}

		// Write the CP afterwards
		_populateEntry(outgoing, cp);

		++entries;
		cp = cp->next;
	}

	if (outgoing.empty() == false)
		outgoing.send(*endpoint);

	dbugLog(_FLI, "synced %u strat objects for chat host", entries);
}

void
strtBroadcastStratUpdates()
{
	Teul::Packet outgoing(Teul::PacketId(Strat::CallbackX, STRT_UPDATE_Y));

	FacilityStruct *fac = nullptr;
	while ((fac = _facilityQueue.next()) != nullptr)
	{
		_populateEntry(outgoing, fac);
	}

	CPStruct *cp = nullptr;
	while ((cp = _cpQueue.next()) != nullptr)
	{
		_populateEntry(outgoing, cp);
	}

	ObjectStruct *obj = nullptr;
	while ((obj = _objectQueue.next()) != nullptr)
	{
		auto &entry = outgoing.allocate<StratUpdates>();
		entry.m_fullObjID = obj->fullObjID;
		entry.m_flags = obj->effectiveVisLevel();
		entry.u_extra.m_buildingDamagedBy = obj->m_damagedBy;
	}

	if (outgoing.empty() == false)
	{
		serverSendPacketToPeers(outgoing);
	}
}

SINT32
strtSendPostExpired(UINT32 customerID, const WEAPON_SERIAL &weaponSerial, bool cancelled)
{
	MM_POST_EXPIRED expired{customerID, weaponSerial, cancelled};
	Teul::Packet outgoing(Teul::PacketId(Strat::CallbackX, STRT_MM_POST_EXPIRED_Y), expired);
	return serverSendPacketToType(outgoing, MAP_HOST) ? 0 : -EINVAL;
}

SINT32
strtBroadcastTimeUpdate(UINT32 gameTime, UINT32 timeFactor, UINT32 weatherPattern, UINT32 fogPattern)
{
	Teul::Packet outgoing(Teul::PacketId(Strat::CallbackX, STRT_BROADCAST_TIME_UPDATE_Y));
	outgoing.construct<Host::TimeUpdate>(gameTime, timeFactor, weatherPattern, fogPattern, host_config.arena.theme);
	return serverSendPacketToPeers(outgoing);
}

SINT32 strtBroadcastConfigUpdate()
{
	for (auto i = 0; i < MAX_COUNTRIES; ++i)
	{
		host_config.routDestination[i] = (routDestination[i] ? routDestination[i]->fullObjID.Key() : 0ULL);
	}
	Teul::Packet outgoing(Teul::PacketId(Strat::CallbackX, STRT_BROADCAST_CONFIG_UPDATE_Y), host_config);
	return serverSendPacketToPeers(outgoing);
}

SINT32 strtSendMMCrewBailed(UINT32 commanderID, UINT32 crewID)
{
	MM_CREW_BAILED bailed{commanderID, crewID};
	Teul::Packet outgoing(Teul::PacketId(Strat::CallbackX, STRT_MM_CREW_BAILED_Y), bailed);
	return serverSendPacketToType(outgoing, MAP_HOST) ? 0 : -EINVAL;
}

SINT32 strtSendMMJoinDeny(UINT32 crewID, UINT32 reasonCode)
{
	MM_JOIN_DENY deny{crewID, reasonCode};
	Teul::Packet outgoing(Teul::PacketId(Strat::CallbackX, STRT_MM_JOIN_DENY_Y), deny);
	return serverSendPacketToType(outgoing, MAP_HOST) ? 0 : -EINVAL;
}

SINT32 strtSendMMJoinListResponse(UINT32 commandID, UINT32 crewID, ChatHandle crewHandle)
{
	MM_JOIN_LIST_RESPONSE response{commandID, crewID, crewHandle};
	Teul::Packet outgoing(Teul::PacketId(Strat::CallbackX, STRT_MM_JOIN_LIST_Y), response);
	return serverSendPacketToType(outgoing, MAP_HOST) ? 0 : -EINVAL;
}

// A lot smarter than the rest of the send functions, this function actually
// breaks the CP information inside this supercell into chunks that can
// be sent easily
SINT32
strtSyncCPs(UINT32 address, UINT16 port, Supercell *sc)
{
	// Return an error if handed a NULL
	if (sc == nullptr)
		return -1;

	// Technically, a no-op is a success. :)
	if (sc->numCPs == 0)
		return 0;

	UINT32 i, f;
	UINT32 startID = 0;
	STRT_SYNC_CPS &syncCPs = strtGetSyncCPs();
	SINT32 fretval = 0, retval = 0, numLeft = 0;

	syncCPs.sx = sc->x;
	syncCPs.sy = sc->y;

	while (startID < sc->numCPs)
	{
		numLeft = sc->numCPs - startID;
		// Figure out how many cps to send
		if (numLeft > STRT_CPS_PER_SYNC)
			syncCPs.numEntries = STRT_CPS_PER_SYNC;
		else
			syncCPs.numEntries = numLeft;

		// Set our startID
		syncCPs.startID = startID;

#ifdef DEBUG_SYNC
		dbugLog(_FLD,"syncing server %s cps in supercell %d:%d, sending %d cps, from %d to %d"
				, teulGetAddress(address), syncCPs.sx, syncCPs.sy, syncCPs.numEntries, syncCPs.startID, syncCPs.startID + syncCPs.numEntries - 1);
#endif
		// Make the message
		for (i = 0, f = startID; i < syncCPs.numEntries; ++i, ++f)
		{
			CPStruct *cp = &sc->cps[f];
			syncCPs.info[i].uOID = cp->uOID;
			syncCPs.info[i].owner = (UCHAR)cp->country;
			syncCPs.info[i].control = (UCHAR)cp->controllingCountry;
			syncCPs.info[i].flags = cp->flags;

#ifdef DEBUG_SYNC
			dbugLog(_FLD,"sending CP txID %u  owner %d control %d flags %d"
						, f, syncCPs.info[i].owner, syncCPs.info[i].control, syncCPs.info[i].flags);
#endif
		}

		// Send the message
		retval = teulSendTo(Server::broadcastIndex, address, port, _SEND_SYNC_CPS, &syncCPs);

		if (retval < 0)
		{
			dbugLog(_FLA, "Error sending syncCPs to stratUpdater!");
			fretval = retval;
		}

		// Set the next startID
		startID += syncCPs.numEntries;
	}

	return fretval;
}

// A lot smarter than the rest of the send functions, this function actually
// breaks the facility information inside this supercell into chunks that can
// be sent easily
SINT32 strtSyncFacilities(UINT32 address, UINT16 port, Supercell *sc)
{
	// Return an error if handed a NULL
	if (sc == nullptr)
		return -1;

	// Technically, a no-op is a success. :)
	if (sc->numFacilities == 0)
		return 0;

	UINT32 i, f;
	UINT32 startID = 0;
	STRT_SYNC_FACILITIES &syncFacilities = strtGetSyncFacilities();
	SINT32 fretval = 0, retval = 0, numLeft = 0;

	syncFacilities.sx = sc->x;
	syncFacilities.sy = sc->y;

	const char *destination = teulGetAddress(address);

	while (startID < sc->numFacilities)
	{
		numLeft = sc->numFacilities - startID;
		// Figure out how many facilities to send
		if (numLeft > STRT_FACILITIES_PER_SYNC)
			syncFacilities.numEntries = STRT_FACILITIES_PER_SYNC;
		else
			syncFacilities.numEntries = numLeft;

		// Set our startID
		syncFacilities.startID = startID;

		dbugLog(_FLD,"syncing server %s facilities in supercell %d:%d, sending %d facilities, from %d to %d"
					, destination, syncFacilities.sx, syncFacilities.sy, syncFacilities.numEntries, syncFacilities.startID, syncFacilities.startID + syncFacilities.numEntries - 1);

		// Make the message
		for (i = 0, f = startID; i < syncFacilities.numEntries; ++i, ++f)
		{
			syncFacilities.uOID[i] = sc->facilities[f].uOID;
			syncFacilities.country[i] = (UCHAR)(sc->facilities[f].country | ((sc->facilities[f].isOpen()) ? FAC_OPEN_FLAG : 0));
			syncFacilities.captureStage[i] = (UINT8)(sc->facilities[f].m_captureStage);
			syncFacilities.captureCounter[i] = (sc->facilities[f].m_captureCounter);
			syncFacilities.captureStateChanged[i] = (UINT32)(teulTime() - sc->facilities[f].m_captureStateChanged);
#ifdef DEBUG_SYNC
			dbugLog(_FLD,"facility %d => %u open %d sending %x",f, sc->facilities[f].uOID, sc->facilities[f].isOpen(), syncFacilities.country[f]);
#endif
		}

		// Send the message
		retval = teulSendTo(Server::broadcastIndex, address, port, _SEND_SYNC_FACILITIES, &syncFacilities);

		if (retval < 0)
		{
			dbugLog(_FLA, "Error sending syncFacilities to stratUpdater!");
			fretval = retval;
		}

		// Set the next startID
		startID += syncFacilities.numEntries;
	}

	return fretval;
}

// A lot smarter than the rest of the send functions, this function actually
// breaks the object information inside this supercell into chunks that can
// be sent easily
SINT32 strtSyncObjects(UINT32 address, UINT16 port, Supercell *sc)
{
	// Return an error if handed a NULL
	if (sc == nullptr)
		return -1;

	// Technically, a no-op is a success. :)
	if (sc->numObjects == 0)
		return 0;

	UINT32 i, f;
	UINT32 startID = 0;
	STRT_SYNC_BUILDINGS &syncBuildings = strtGetSyncBuildings();
	SINT32 fretval = 0, retval = 0, numLeft = 0;

	syncBuildings.sx = sc->x;
	syncBuildings.sy = sc->y;

	const char *destination = teulGetAddress(address);

	while (startID < sc->numObjects)
	{
		numLeft = sc->numObjects - startID;
		// Figure out how many objects to send
		if (numLeft > STRT_BUILDINGS_PER_SYNC)
			syncBuildings.numEntries = STRT_BUILDINGS_PER_SYNC;
		else
			syncBuildings.numEntries = numLeft;

		// Set our startID
		syncBuildings.startID = startID;

		//#ifdef DEBUG_SYNC
		dbugLog(_FLD,"syncing server %s objects in supercell %d:%d, sending %u objects, from %u to %u"
				, destination, syncBuildings.sx, syncBuildings.sy, syncBuildings.numEntries, syncBuildings.startID, syncBuildings.startID + syncBuildings.numEntries - 1);
		//#endif
		// Make the message
		for (i = 0, f = startID; i < syncBuildings.numEntries; ++i, ++f)
		{
			syncBuildings.vislevel[i] = (UCHAR)sc->objects[f].effectiveVisLevel();
		}

		// Send the message
		retval = teulSendTo(Server::broadcastIndex, address, port, _SEND_SYNC_BUILDINGS, &syncBuildings);

		if (retval < 0)
		{
			dbugLog(_FLA, "Error sending syncBuildings to stratUpdater!");
			fretval = retval;
		}

		// Set the next startID
		startID += syncBuildings.numEntries;
	}

	return fretval;
}

bool
strtReplyToCapacityRequest(SINT32 teulIndex, const MM_CAPACITY_REQUEST& request, FULL_OBJ_ID destFacilityID, bool vehicleDelivered)
{
	MM_CAPACITY_RESPONSE response;

	if (strtProcessCapacityRequest(request, response, destFacilityID, vehicleDelivered) < 0)
	{
		dbugLog(_FLA, "recieved unprocessible capacity request for facility " PRF_FULL_OBJ_ID "", PRA_FULL_OBJ_ID(request.facility));
	}

	// Send the response
	if ((teulSend(teulIndex, _XCHG_MM_CAPACITY, &response)) < 0)
	{
		dbugLog(_FLA, "error sending capacity response");
		return false;
	}

	return true;
}

static SINT32 _recvEnemyReport(ACTION_OBJECT *const a)
{
	STRAT_ENEMY_REPORT report;

	a->offset = teulPackVoid(TEUL_UNPACK, a->packed, a->offset, &report, sizeof(STRAT_ENEMY_REPORT));

	if (a->offset > 0)
	{
		dbugLog(_FLI, "received enemy report for vehicle " PRF_WEAPON_SERIAL "", PRA_WEAPON_SERIAL(report.enemyReported));

		if (!strtAddEnemyReport(report))
			return -EINVAL;
	}

	return a->offset;
}

static SINT32 _recvMMPostRequest(ACTION_OBJECT *const a)
{
	MM_POST_REQUEST postRequest;
	MM_POST_ACCEPT postAccept;
	MM_POST_DENY postDeny;

	a->offset = teulPackVoid(TEUL_UNPACK, a->packed, a->offset, &postRequest, sizeof(MM_POST_REQUEST));

#ifdef DEBUG
	dbugLog(_FLD,"postRequest: %d %d:%d:%d:%d:%d %d:%d:%d:%d %d"
			, postRequest.customerID
			, postRequest.facility.cell.ox, postRequest.facility.cell.oy
			, postRequest.facility.id, postRequest.facility.objClass, postRequest.facility.objType
			, postRequest.weapon.wCountry, postRequest.weapon.wCategory
			, postRequest.weapon.wClass, postRequest.weapon.wType
			, postRequest.immediateSpawn);
#endif

	if (strtProcessPostRequest(a->teulIndex, postRequest, postAccept, postDeny))
	{
		if (teulSend(a->teulIndex, _SEND_MM_POST_ACCEPT, &postAccept) < 0)
			dbugLog(_FLA, "Error sending mm post accept");
	}
	else
	{
		if (teulSend(a->teulIndex, _SEND_MM_POST_DENY, &postDeny) < 0)
			dbugLog(_FLA, "Error sending mm post deny");
	}

	return a->offset;
}

static SINT32 _sendMMPostAccept(ACTION_OBJECT *const a)
{
	return teulPackVoid(TEUL_PACK, a->packed, a->offset, a->localRawData, sizeof(MM_POST_ACCEPT));
}

static SINT32 _sendMMPostDeny(ACTION_OBJECT *const a)
{
	return teulPackVoid(TEUL_PACK, a->packed, a->offset, a->localRawData, sizeof(MM_POST_DENY));
}

static SINT32
_recvMMPostListRequest(ACTION_OBJECT* const a)
{
	MM_POST_LIST_REQUEST postListRequest;

	a->offset = teulPackVoid(TEUL_UNPACK, a->packed, a->offset, &postListRequest, sizeof(MM_POST_LIST_REQUEST));

#ifdef DEBUG
	dbugLog(_FLD,"postListReq: %d %d:%d:%d:%d:%d", postListRequest.crewID, postListRequest.facility.cell.ox, postListRequest.facility.cell.oy, postListRequest.facility.id, postListRequest.facility.objClass, postListRequest.facility.objType);
#endif

	strtProcessPostListRequest(a->teulIndex, postListRequest);

	return a->offset;
}

static SINT32 _recvMMJoinRequest(ACTION_OBJECT *const a)
{
	MM_JOIN_REQUEST joinRequest;
	MM_JOIN_ACCEPT joinAccept;
	MM_JOIN_DENY joinDeny;

	a->offset = teulPackVoid(TEUL_UNPACK, a->packed, a->offset, &joinRequest, sizeof(MM_JOIN_REQUEST));

#ifdef DEBUG

	dbugLog(_FLD,"joinRequest: %d %d:%d:%d:%d:%d %d:%d:%d:%d", joinRequest.customerID, joinRequest.weaponSerial.source.cell.ox, joinRequest.weaponSerial.source.cell.oy, joinRequest.weaponSerial.source.id, joinRequest.weaponSerial.source.objClass, joinRequest.weaponSerial.source.objType, joinRequest.weaponSerial.spec.wCountry, joinRequest.weaponSerial.spec.wCategory, joinRequest.weaponSerial.spec.wClass, joinRequest.weaponSerial.spec.wType);

#endif

	if (strtProcessJoinRequest(joinRequest, joinAccept, joinDeny))
	{
		if (teulSend(a->teulIndex, _SEND_MM_JOIN_ACCEPT, &joinAccept) < 0)
			dbugLog(_FLA, "Error sending mm join accept");
	}
	else
	{
		if (teulSend(a->teulIndex, _SEND_MM_JOIN_DENY, &joinDeny) < 0)
			dbugLog(_FLA, "Error sending mm join deny");
	}
	return a->offset;
}

static SINT32 _sendMMJoinAccept(ACTION_OBJECT *const a)
{
	return teulPackVoid(TEUL_PACK, a->packed, a->offset, a->localRawData, sizeof(MM_JOIN_ACCEPT));
}

static SINT32 _sendMMJoinDeny(ACTION_OBJECT *const a)
{
	return teulPackVoid(TEUL_PACK, a->packed, a->offset, a->localRawData, sizeof(MM_JOIN_DENY));
}

static SINT32 _recvMMSpawnGrantRequest(ACTION_OBJECT *const a)
{
	MM_SPAWN_GRANT_REQUEST spawnGrantRequest;
	MM_SPAWN_GRANT_RESPONSE spawnGrantResponse;

	a->offset = teulPackVoid(TEUL_UNPACK, a->packed, a->offset, &spawnGrantRequest, sizeof(MM_SPAWN_GRANT_REQUEST));

	dbugLog(_FLD, "spawnGrantRequest: %d %d:%d:%d:%d:%d %d:%d:%d:%d", spawnGrantRequest.weaponSerial.serial, spawnGrantRequest.weaponSerial.source.cell.ox, spawnGrantRequest.weaponSerial.source.cell.oy, spawnGrantRequest.weaponSerial.source.id, spawnGrantRequest.weaponSerial.source.objClass, spawnGrantRequest.weaponSerial.source.objType, spawnGrantRequest.weaponSerial.spec.wCountry, spawnGrantRequest.weaponSerial.spec.wCategory, spawnGrantRequest.weaponSerial.spec.wClass, spawnGrantRequest.weaponSerial.spec.wType);

	const bool success = strtProcessSpawnGrantRequest(spawnGrantRequest, spawnGrantResponse);
	if (success)
	{
		// serious hack here!!!
		MM_CAPACITY_REQUEST mmCapacityReq;
		MM_CAPACITY_RESPONSE mmCapacityResponse;

		mmCapacityReq.facility = spawnGrantRequest.facility;
		if (strtProcessCapacityRequest(mmCapacityReq, mmCapacityResponse, nullObjID, false) < 0)
		{
			// cry
			dbugLog(_FLA, "couldn't process process capacity request for x:%d y:%d id:%d"
						,spawnGrantRequest.facility.cell.ox
						,spawnGrantRequest.facility.cell.oy
						,spawnGrantRequest.facility.id);
		}
		spawnGrantResponse.capacity = mmCapacityResponse.capacity;
	}

	if ((teulSend(a->teulIndex, _XCHG_MM_SPAWN_GRANT, &spawnGrantResponse)) < 0)
		dbugLog(_FLA, "Error sending spawn grant response");

	return a->offset;
}

static SINT32 _sendMMSpawnGrantResponse(ACTION_OBJECT *const a)
{
	return teulPackVoid(TEUL_PACK, a->packed, a->offset, a->localRawData, sizeof(MM_SPAWN_GRANT_RESPONSE));
}

static SINT32
_recvMMJoinListRequest(ACTION_OBJECT* const a)
{
	MM_POST_INFO_REQUEST request;

	a->offset = teulPackVoid(TEUL_UNPACK, a->packed, a->offset, &request, sizeof(MM_POST_INFO_REQUEST));

	dbugLog(_FLD, "p:%u joinListRequest " PRF_WEAPON_SERIAL " %s %u", request.customerID, PRA_WEAPON_SERIAL(request.weaponSerial), Chat::Name(request.handle).c_str(), request.join);

	if (!strtProcessJoinListRequest(a->teulIndex, request))
	{
		dbugLog(_FLD, "p:%u joinListRequest failed, sending deny", request.customerID);
		strtSendMMJoinDeny(request.customerID, MM_JOIN_FAIL_CANT_FIND_POSTED_WEAPON);
	}

	return a->offset;
}


static SINT32 _recvMMPostInfoUpdate(ACTION_OBJECT *const a)
{
	MM_POST_INFO_UPDATE update;

	a->offset = teulPackVoid(TEUL_UNPACK, a->packed, a->offset, &update, sizeof(MM_POST_INFO_UPDATE));

	dbugLog(_FLD, "p:%u postInfoUpdate " PRF_WEAPON_SERIAL " %u", update.customerID, PRA_WEAPON_SERIAL(update.weaponSerial), update.multicrew);

	strtProcessPostInfoUpdate(&update);

	return a->offset;
}


static SINT32 _recvMMPostCancel(ACTION_OBJECT *const a)
{
	MM_POST_CANCEL postCancel;

	a->offset = teulPackVoid(TEUL_UNPACK, a->packed, a->offset, &postCancel, sizeof(MM_POST_CANCEL));

	LogAbort("postCancel: " PRF_WEAPON_SERIAL " %s"
					, PRA_WEAPON_SERIAL(postCancel.weaponSerial)
					, weapPrintName(postCancel.weaponSerial.spec)
					);
}

static SINT32 _recvWeaponsReturn(ACTION_OBJECT *const a)
{
	WeaponsReturn &incoming = *(WeaponsReturn *)(a->packed);

	dbugLog(_FLD,"_recvWeaponsReturn: %u (%s/%s%s)"
			, incoming.m_sortieID
			, (incoming.m_isVehicleDead) ? "DEAD" : "alive"
			, (incoming.m_exitCode == PEXIT_RTB) ? "RTB" : "non-rtb"
			, (incoming.m_rtbPromoted) ? "*" : ""
			);

	incoming.process();

	return a->offset + Teul::BytesToBits(sizeof(incoming));
}

static SINT32
_recvMMSpawnCancel(ACTION_OBJECT* const a)
{
	MM_SPAWN_CANCEL spawnCancel;

	a->offset = teulPackVoid(TEUL_UNPACK, a->packed, a->offset, &spawnCancel, sizeof(MM_SPAWN_CANCEL));

    dbugLog(_FLD, "p:%u v:%u spawnCancel: %d:%d:%d:%d:%d %d:%d:%d:%d"
                    , spawnCancel.customerID, spawnCancel.weaponSerial.serial
                    , spawnCancel.weaponSerial.source.cell.ox, spawnCancel.weaponSerial.source.cell.oy, spawnCancel.weaponSerial.source.id, spawnCancel.weaponSerial.source.objClass, spawnCancel.weaponSerial.source.objType
                    , spawnCancel.weaponSerial.spec.wCountry, spawnCancel.weaponSerial.spec.wCategory, spawnCancel.weaponSerial.spec.wClass, spawnCancel.weaponSerial.spec.wType);

	strtProcessSpawnCancel(spawnCancel);

	return a->offset;
}

static SINT32
_recvSyncDamageList(ACTION_OBJECT* const a)
{
#ifdef DEBUG_HEAVY
	dbugLog(_FLD, "Recieved %u damage messages from stratUpdater %s", numItems, teulGetAddress(a->fromAddress));
#endif

	// The cell sends a raw stream of DAMAGE_QUEUE entries.
	const DAMAGE_QUEUE_ENTRY *const entries = (DAMAGE_QUEUE_ENTRY *)(a->packed);
	const size_t numEntries = a->sizeInBytes() / sizeof(*entries);
	FULL_OBJ_ID fullObjID;
	for (size_t i = 0; i < numEntries; ++i)
	{
		const DAMAGE_QUEUE_ENTRY &entry = entries[i];
		fullObjID.FromKey(entry.objectKey);
		strtDamage(fullObjID, entry.damageJoules, entry.weapon, entry.playerID);
	}

	return a->offset + a->sizeInBits();
}

bool
_requestRequiresWeaponReturn(const MM_CAPACITY_REQUEST& request, SINT32 teulIndex)
{
	if (request.rtb != RTB_SUCCESS)
		return false;

	// Can we find valid data for this user climbing into a vehicle?
	WeapJoinRequestNode *const joinNode = WeapJoinRequestNode::Lookup(request.customerID);
	if (joinNode == nullptr || joinNode->m_joinList == nullptr || joinNode->m_joinList->m_weapon == nullptr)
		return false;

	dbugLog(_FLD, "queueing capacity request for %u to ti:%d", request.customerID, teulIndex);
	joinNode->m_capacityTeulIndex = teulIndex;
	joinNode->m_capacityRequest = request;

	return true;
}

static SINT32
_recvMMCapacityRequest(ACTION_OBJECT* const a)
{
	if ((size_t)a->sizeInBytes() < sizeof(MM_CAPACITY_REQUEST))
	{
		dbugLog(_FLA, "illegal capacity request");
		return -EINVAL;
	}

	MM_CAPACITY_REQUEST request;
	a->offset = teulPackVoid(TEUL_UNPACK, a->packed, a->offset, &request, sizeof(request));
	dbugLog(_FLD, "capacityRequest: %u:%u:%u @ " PRF_FULL_OBJ_ID " code:%u"
				, request.tag, request.missionID
				, request.customerID
				, PRA_FULL_OBJ_ID(request.facility)
				, request.rtb
				);

	// If this is an RTB_SUCCESS, then there is a chance we will reserve the vehicle,
	// so we need to attach this request to the join node for that vehicle
	if (_requestRequiresWeaponReturn(request, a->teulIndex))
		return a->offset;

	strtReplyToCapacityRequest(a->teulIndex, request, nullObjID, false);

	return a->offset;
}

static SINT32
_sendMMCapacityResponse(ACTION_OBJECT* const a)
{
	return teulPackVoid(TEUL_PACK, a->packed, a->offset, a->localRawData, sizeof(MM_CAPACITY_RESPONSE));
}

static SINT32 _sendSyncCPs(ACTION_OBJECT *const a)
{
	return strtPackSyncCPs(TEUL_PACK, a->packed, a->offset, (STRT_SYNC_CPS *)a->localRawData, 0);
}

static SINT32 _sendSyncFacilities(ACTION_OBJECT *const a)
{
	return strtPackSyncFacilities(TEUL_PACK, a->packed, a->offset, (STRT_SYNC_FACILITIES *)a->localRawData, 0);
}

static SINT32 _sendSyncBuildings(ACTION_OBJECT *const a)
{
	return strtPackSyncBuildings(TEUL_PACK, a->packed, a->offset, (STRT_SYNC_BUILDINGS *)a->localRawData, 0);
}

template <>
SINT32
SERIAL_NUMBER_REQUEST::receive(SINT32 teulIndex, SERIAL_NUMBER_REQUEST& incoming)
{
	incoming.data().serial = strtGetNextSerial();
	dbugLog(_FLI, "p:%u requested serial number, giving %u", incoming.data().playerID, incoming.data().serial);
	return incoming.send(teulIndex, EXCHANGE);
}

//////////////////////////////////////////////////////////////////////
// Execute a Lua script.

static SINT32
_recvLua(ACTION_OBJECT* const a)
{
	const UINT32 bytes = a->sizeInBytes();
	if (bytes <= 1) // null byte is included
		return a->offset;

	const char *const input = (char *)(a->packed);
	if (input[bytes - 1] != '\0')
	{
		char sender[81];
		if (a->endpoint)
			teulGetAddress(a->endpoint->fromAddress(), sender, sizeof(sender));
		else
			strsafecpy(sender, "?.?.?.?");
		dbugLog(_FLW, "t:%d ip:%s ignoring non-nul-terminated input", a->teulIndex, sender);
		return -EINVAL;
	}

	dbugLog(_FLI, "luaSrc: received command: %s", input);

	if (input[0] == ';')
	{
		std::string object(cv_luaPath.GetString());
		object += input + 1;
		object += ".lua";
		Lua::RunScript(object.c_str());
	}
	else
	{
		char sender[81];
		strprintf(sender, "t:%d ip:%s", a->teulIndex, (a->endpoint ? teulGetAddress(a->endpoint->fromAddress()) : "?.?.?.?"));
		std::string object(input);
		Lua::RunObject(sender, object);
	}

	Lua::SCRIPT_RESULTS &results = Lua::Results();
	dbugLog(_FLI, "luaSrv: returning %" PRSizeT " lines of results", results.size());
	Lua::SCRIPT_RESULTS::iterator it;
	std::string reply = "";
	for (it = results.begin(); it != results.end(); ++it)
	{
		reply += *it;
	}

	dbugLog(_FLI, "reply:[%s]", reply.c_str());
	reply += LUA_END_SEQ;

	const size_t lenWithNullByte = reply.length() + 1;
	Teul::Packet outgoing(Teul::PacketId(Host::CallbackX, Host::Callbacks::LuaPacket_Y), reply.c_str(), lenWithNullByte);
	outgoing.setUnencrypted();
	outgoing.send(a->teulIndex);

	results.clear();

	return a->offset;
}

//////////////////////////////////////////////////////////////////////
// Receive a capture contribution event

static SINT32
_recvCaptureContrib(ACTION_OBJECT* const a)
{
	// dbugLog(_FLN, "Strat receive capture contrib.");

	if (a->canRead<Strat::CaptureContributionEvent>() == false)
		return -EINVAL;

	auto &event = a->read<Strat::CaptureContributionEvent>();
	dbugLog(_FLD, "p:%u c:%s v:%u m:%u f:%u hcu:%u status:%u"
					, event.m_playerID, Chat::Name(event.m_handle).c_str(), event.m_sortieID
					, event.m_missionID, event.m_facilityID, event.m_unitID
					, event.m_status
					);

	FacilityStruct *forFacility = strtGetFacilityByOID(event.m_facilityID);
	if (nullptr == forFacility)
	{
		dbugLog(_FLA, "p:%u c:%s %s: invalid facilityID %u", event.m_playerID, Chat::Name(event.m_handle).c_str(), __FUNCTION__, event.m_facilityID);
		return -EINVAL;
	}
	HCUnit *forUnit = HCUnit::Lookup(event.m_unitID);
	if (nullptr == forUnit)
	{
		dbugLog(_FLA, "p:%u c:%s %s: invalid unitID %u", event.m_playerID, Chat::Name(event.m_handle).c_str(), __FUNCTION__, event.m_unitID);
		return -EINVAL;
	}

	Strat::CaptureContribution::Queue(event.m_status, *forFacility, event.m_handle, event.m_playerID, event.m_sortieID, event.m_missionID, *forUnit);

	return a->offset;
}

//////////////////////////////////////////////////////////////////////
// Chat host updating us as to current campaign populations.

static SINT32
_recvPopulationNotice(ACTION_OBJECT* const a)
{
	for (UINT32 i = 0; i < MAX_SIDES; ++i)
	{
		g_chatPopulations[i] = a->read<UINT32>();
	}

	dbugLog(_FLI, "Unpacked chat population notice; Allies: %u, Axis: %u", g_chatPopulations[ALLIED], g_chatPopulations[AXIS]);

	// Set total players
	g_chatPopulations[WWII_NO_SIDE] = g_chatPopulations[ALLIED] + g_chatPopulations[AXIS];

	// Recalculate the objective limits
	g_objectiveStats.calculate();

	return a->offset;
}

//////////////////////////////////////////////////////////////////////
// Chat requesting an attack order.

static SINT32
_recvAttackOrder(ACTION_OBJECT* const a)
{
	AttackOrder &order = a->read<AttackOrder>();
	order.process();
	return a->offset;
}

void
AttackOrder::process()
{
	m_result = SCMD_NO_EFFECT;
	CPStruct *cp = strtGetCPorBridge(m_target);
	HCUnit *unit = HCUnit::Lookup((UINT32)m_brigadeID);

	if (cp == nullptr)
	{
		dbugLog(_FLA, "AttackOrder(" PRF_FULL_OBJ_ID ") invalid target", PRA_FULL_OBJ_ID(m_target));
	}
	else if (unit == nullptr)
	{
		dbugLog(_FLA, "AttackOrder(%u) invalid unit", m_brigadeID);
		return;
	}
	else
	{
		switch (m_orderType)
		{
			case ORD_ATTACK:
				dbugLog(_FLD, "received ORD_ATTACK order for %s from %s", cp->name, Chat::Name(m_requester).c_str());
				m_result = cp->requestAttack(m_execute, m_brigadeID);
				break;
			case ORD_DEFEND:
				dbugLog(_FLD, "received ORD_DEFEND order for %s from %s", cp->name, Chat::Name(m_requester).c_str());
				m_result = cp->requestDefend(unit->side(), m_execute);
				break;
			case ORD_CLEAR:
				dbugLog(_FLD, "received ORD_CLEAR order for %s from %s", cp->name, Chat::Name(m_requester).c_str());
				m_result = cp->requestClear(unit->side(), m_delay, m_execute);
				break;
			default:
				LogAbort("Unrecognized attack order packet from %s", Chat::Name(m_requester).c_str());
		}
	}
	dbugLog(_FLD, " result: %u", m_result);

	// Send back the response
	Teul::Packet outgoing(Teul::PacketId(Chat::CallbackX, Chat::ATTACK_ORDER_Y), *this);
	serverSendPacketToType(outgoing, CHAT_HOST);
}

bool
CPStruct::placeAttackObjective(const StratHCUnit* byUnit)
{
	static const ChatHandle systemID = chatGetHandle("SYSTEM");
	if (byUnit == nullptr)
		return false;
	AttackOrder order(systemID, byUnit->unitID(), fullObjID, ORD_ATTACK, true);
	order.process();
	return true;
}

//////////////////////////////////////////////////////////////////////
// Receive and process a Weapon List Request

static SINT32
_recvWeaponListRequest(ACTION_OBJECT* const a)
{
	if (a->canRead<WeaponListRequest>() == false)
		return -EINVAL;
	auto &request = a->read<WeaponListRequest>();

	Teul::Packet outgoing(Teul::PacketId(Strat::CallbackX, STRT_FACILITY_WEAP_LIST_Y));
	auto &reply = outgoing.construct<FacilityWeaponList>(request.m_playerID, request.m_fullObjID);
	try
	{
		FacilityStruct *facility = strtGetFacility(request.m_fullObjID);
		if (facility == nullptr)
		{
			dbugLog(_FLI, "p:%u hc:%u requested unknown facility " PRF_FULL_OBJ_ID ""
							, request.m_playerID, request.m_brigadeID
							, PRA_FULL_OBJ_ID(request.m_fullObjID)
							);
			throw FacilityWeaponList::_INVALID;
		}

		UINT32 supplyingBrigade = strtGetSupplyingUnit(*facility, request.m_brigadeID);
		if (supplyingBrigade == 0)
			supplyingBrigade = request.m_brigadeID;

		StratHCUnit *unit = StratHCUnit::Lookup(supplyingBrigade);
		if (unit == nullptr)
		{
			dbugLog(_FLI, "p:%u requested FWL for unknown brigade %u"
							, request.m_playerID, request.m_brigadeID
							);
			throw FacilityWeaponList::_INVALID;
		}

		if (!reply.populate(*unit, *facility, request.m_playerID, request.m_permMask))
			throw FacilityWeaponList::_INVALID;
	}
	catch (FacilityWeaponList::REASON_CODE &reason)
	{
		reply.m_status = reason;
		// Don't send any weapons, and make the client wait before auto-refreshing.
		reply.m_numWeapons = 0;
		reply.m_refreshSeconds = 5;
	}

	outgoing.send(*a->endpoint);

	return a->offset;
}

//////////////////////////////////////////////////////////////////////
// Receive a GarrisonCountryChangeReq from the map host

static SINT32 _recvMMGarrisonCountryChangeRequest(ACTION_OBJECT *const a)
{
	GarrisonCountryChangeReq request(a);
    request.strtProcessRequest();
    //dbugLog(_FLD, "GarrisonCountryChangeReq received on strat host from %d to change CP %u to country %u ", request.m_clientsTeulIndex, request.m_cp.id, request.m_country);
    return teulSend(a->teulIndex, _XCHG_MM_GARRISON_COUNTRY_CHANGE, &request);
}

//////////////////////////////////////////////////////////////////////
// Process a GarrisonCountryChangeReq

void GarrisonCountryChangeReq::strtProcessRequest()
{
	if (m_result == GCR_REQUESTING)
	{
		CPStruct *cp = strtGetCP(m_cp);
		if (cp == nullptr)
		{
			dbugLog(_FLI, "GarrisonCountryChangeReq(" PRF_FULL_OBJ_ID ") invalid target", PRA_FULL_OBJ_ID(m_cp));
		}
		else
		{
			if (cp->isObjective() || cp->islinkedToAnAO())
			{
				m_result = GCR_FAILED_HAS_OBJECTIVE;
                dbugLog(_FLI, "GarrisonCountryChangeReq failed because the CP has an Objective or is linked to one.");
			}
			else if (wwiiSide(m_country) != wwiiSide(cp->country))
			{
				m_result = GCR_FAILED_WRONG_OWNER;
                dbugLog(_FLI, "GarrisonCountryChangeReq failed because the CP is owned by the other side.");
			}
            else if (m_country == cp->country)
            {
                m_result = GCR_FAILED_OWNER_ALREADY;
                dbugLog(_FLI, "GarrisonCountryChangeReq failed because the CP is owned by the requested country already.");
            }
            // TODO Add some checks here for percentages of UK, French and US towns already on the front line.

            if (m_result == GCR_REQUESTING)
            {
                if (cp->changeOwnership(m_country, true, true))
                {
                    cp->changeAllGarrisonCountries(m_country, true);
                    m_result = GCR_APPROVED;
                    dbugLog(_FLI, "Approved GarrisonCountryChangeReq received from %d to change CP %u to country %u ", m_clientsTeulIndex, m_cp.id, m_country);
                }
                else
                {
                    dbugLog(_FLI, "GarrisonCountryChangeReq cp->changeOwnership failed for %s whilst changing from country %u to %u",
                            cp->name, cp->country, m_country);
                    m_result = GCR_FAILED;
                }
            }
		}
	}
}


SINT32 GarrisonCountryChangeReq::sendToMap(SINT32 teulIndex) 
{ 
    return teulSend(teulIndex, _XCHG_MM_GARRISON_COUNTRY_CHANGE, this); 
}

//////////////////////////////////////////////////////////////////////
// Used by strat to respond to a GarrisonCountryChangeReq from the map host.

static SINT32 _sendMMGarrisonCountryChangeResponse(ACTION_OBJECT *const a)
{
	GarrisonCountryChangeReq *request = (GarrisonCountryChangeReq *)(a->localRawData);
    //dbugLog(_FLD, "GarrisonCountryChangeResponse is being sent to the map host from strat with a client teulindex of %d to change CP %u to country %u ", request->m_clientsTeulIndex, request->m_cp.id, request->m_country);
    return request->packer(TEUL_PACK, a);
}

//////////////////////////////////////////////////////////////////////
// Host-host and luaclient messages.

static SINT32 
_registerInterHostCallbacks(){
	BEGIN_CALLBACK_REGISTRATION(Host::CallbackX)
    {
        REGISTER_RECEIVER((UCHAR)Host::Callbacks::LuaPacket_Y, _recvLua);
    }
    END_CALLBACK_REGISTRATION(Host::CallbackX)
}

//////////////////////////////////////////////////////////////////////
// Public functions

// Installs our callbacks
SINT32
strtHostInstallCallbacks()
{
	_registerInterHostCallbacks();

	BEGIN_CALLBACK_REGISTRATION(Strat::CallbackX)
	{
		REGISTER_RECEIVER(STRT_MM_POST_LIST_Y, _recvMMPostListRequest);
		REGISTER_RECVSEND(STRT_MM_SPAWN_GRANT_Y, _recvMMSpawnGrantRequest, _sendMMSpawnGrantResponse, _XCHG_MM_SPAWN_GRANT);
		REGISTER_RECEIVER(STRT_MM_JOIN_LIST_Y, _recvMMJoinListRequest);
		REGISTER_RECVSEND(STRT_MM_CAPACITY_Y, _recvMMCapacityRequest, _sendMMCapacityResponse, _XCHG_MM_CAPACITY);
		REGISTER_RECVSEND(STRT_MM_GARRISON_COUNTRY_CHANGE_Y, _recvMMGarrisonCountryChangeRequest, _sendMMGarrisonCountryChangeResponse, _XCHG_MM_GARRISON_COUNTRY_CHANGE);

		REGISTER_RECEIVER(STRT_SYNC_DAMAGE_LIST_Y, _recvSyncDamageList);
		REGISTER_RECEIVER(STRT_MM_POST_REQ_Y, _recvMMPostRequest);
		REGISTER_RECEIVER(STRT_MM_JOIN_REQ_Y, _recvMMJoinRequest);
		REGISTER_RECEIVER(STRT_MM_POST_INFO_UPDATE_Y, _recvMMPostInfoUpdate);
		REGISTER_RECEIVER(STRT_MM_POST_CANCEL_Y, _recvMMPostCancel);
		REGISTER_RECEIVER(STRT_SYNC_WEAPONS_RETURN_Y, _recvWeaponsReturn);
		REGISTER_RECEIVER(STRT_MM_SPAWN_CANCEL_Y, _recvMMSpawnCancel);
		REGISTER_RECEIVER(STRT_ENEMY_REPORT_Y, _recvEnemyReport);

		REGISTER_SENDER(STRT_MM_POST_ACCEPT_Y, _sendMMPostAccept, _SEND_MM_POST_ACCEPT);
		REGISTER_SENDER(STRT_MM_POST_DENY_Y, _sendMMPostDeny, _SEND_MM_POST_DENY);
		REGISTER_SENDER(STRT_MM_JOIN_ACCEPT_Y, _sendMMJoinAccept, _SEND_MM_JOIN_ACCEPT);
		REGISTER_SENDER(STRT_MM_JOIN_DENY_Y, _sendMMJoinDeny, _SEND_MM_JOIN_DENY);
		REGISTER_SENDER(STRT_SYNC_CPS_Y, _sendSyncCPs, _SEND_SYNC_CPS);
		REGISTER_SENDER(STRT_SYNC_FACILITIES_Y, _sendSyncFacilities, _SEND_SYNC_FACILITIES);
		REGISTER_SENDER(STRT_SYNC_BUILDINGS_Y, _sendSyncBuildings, _SEND_SYNC_BUILDINGS);

		REGISTER_MSG(STRT_MM_SERIAL_REQUEST_Y, SERIAL_NUMBER_REQUEST);

		REGISTER_RECEIVER(STRT_CAPTURE_CONTRIB_Y, _recvCaptureContrib);
		REGISTER_RECEIVER(STRT_POPULATION_NOTICE_Y, _recvPopulationNotice);
		REGISTER_RECEIVER(STRT_ATTACK_ORDER_Y, _recvAttackOrder);

		REGISTER_RECEIVER(STRT_FACILITY_WEAP_LIST_Y, _recvWeaponListRequest);
	}
	END_CALLBACK_REGISTRATION(Strat::CallbackX)
}

//////////////////////////////////////////////////////////////////////
// Send a capture scoring notice to the cell hosts

void
strtSendCaptureScoreNotice(const FULL_OBJ_ID fid, WWII_SIDE forSide)
{
	// dbugLog(_FLN, "Strat strtSendCaptureScoreNotice()");

	dbugLog(_FLI, "capscore: bld:%d,%d:%u side:%u", fid.cell.ox, fid.cell.oy, fid.id, forSide);
	Teul::Packet outgoing(Teul::PacketId(Host::CallbackX, Host::Callbacks::CaptureScore_Y));
	outgoing.construct<CaptureScore>(fid, forSide);
	serverSendPacketToType(outgoing, CELL_HOST);
}

//////////////////////////////////////////////////////////////////////
// Send a capture state message

void
strtSendCaptureState(const FacilityStruct* facility, WWII_SIDE forSide, SINT32 rate, bool spawnable)
{
	// dbugLog(_FLN, "Strat send capture state.");

	Teul::Packet outgoing(Teul::PacketId(Host::CallbackX, Host::Callbacks::CaptureState_Y));
	auto &state = outgoing.allocate<CaptureState>();
	state.populate(facility, forSide, rate, spawnable);

	serverSendPacketToType(outgoing, CELL_HOST);
}
