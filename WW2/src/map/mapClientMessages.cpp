#include <syscfg.h> // Path is project specific.
// Date: Thu Jan 25 11:04:13 2001
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Eric "Meat" Hall

#include "client/client.h"
#include "map/mapArenaDescription.h"
#include "map/mapBrigadeSummary.h"
#include "map/mapClientMessages.h"
#include "map/mapConstants.h"
#include "map/mapHCUnitLocations.h"
#include "map/mapMissionList.h"
#include "map/mapPersonaSummary.h"
#include "map/mapSwingMeter.h"
#include "phui/Caption.h"
#include "phui/View.h"
#include "tut/tutGroupProgress.h"
#include "veh/vehdecal.h"
#include "ui/uiMain.h"
#include "wwii/wwiiCallbacks.h"
#include "wwii/wwiiHCUnits.h"
#include "wwii/wwiiOpinionPoll.h"
#include "wwii/wwiiPack.h"
#include "map/mapSupercellVislevelXfer.h"

static	TEUL_CALLBACK*	MAP_SEND_EXIT_WWIIOL;
static	TEUL_CALLBACK*	_XCHG_PERSONA_LIST;
static	TEUL_CALLBACK*	_XCHG_PERSONA_SELECT;
static	TEUL_CALLBACK*	_XCHG_BRIGADE_LIST;
static	TEUL_CALLBACK*	_XCHG_BRIGADE_SELECT;
static	TEUL_CALLBACK*	_XCHG_MISSION_LIST;
static	TEUL_CALLBACK*	_XCHG_TUT_PROGRESS;

#if platOS == OS_WIN32
#include <io.h>
#include <fcntl.h>

// Generate a 64-bit checksum of the file we were launched from. Don't hardcode
// the filename, since we don't want it to be editable, and we don't want the user
// to be able to point it at some file other than the one we are running.
//
// This function is named discretely incase symbol tables leak out; we don't want a
// function called "buildWW2exeCheckSum".

#define READ_BUF_SIZE 8192			// 8k
#define CHECKSUM_BIT_LEN 64
static UINT64
_check64(const char* filename)
{
	int		fd = -1;				// Level 2 file descriptor
	int		bytes = 0;				// Number of bytes successfully read
	int		i;
	UINT64	checksum = 0;

	char* memory = (char*)alloca(READ_BUF_SIZE);
	if ( memory == nullptr )
	{
		dbugWrite("No memory");
		return 0;
	}

	// Get a descriptor for reading the file in binary mode
	fd = _open(filename, _O_RDONLY | _O_BINARY, 0);
	if ( fd == -1 )
	{
		dbugWrite("File missing");
		return 0;
	}

	do
	{
		bytes = _read(fd, memory, READ_BUF_SIZE);
		if ( bytes < 0 )
		{
			dbugWrite("Read failure");
			checksum = 0;
			break;
		}

		// Its possible we might read 0 bytes
		if ( bytes == 0 )
			break;

		// Rotate 5 bits right and add the new value, this gives some weight to positional value
		// of each value.
		for ( i = 0; i < bytes; i++ )
		{
			checksum = ((checksum & 0x1f) << (CHECKSUM_BIT_LEN-5)) | (checksum >> 5);
			checksum += memory[i];
		}
	}
	while ( bytes > 0 );

	_close(fd);

	return checksum;
}
#endif

static SINT32 _sendExitWWIIOL(ACTION_OBJECT* const a)
{
	return a->offset;
}

static SINT32
_sendPersonaListRequest(ACTION_OBJECT* const a)
{
	PersonaSummaries* request = (PersonaSummaries*)(a->localRawData);
	return request->packer(TEUL_PACK, a);
}

static SINT32
_sendPersonaSelectRequest(ACTION_OBJECT* const a)
{
	PersonaSelection* request = (PersonaSelection*)(a->localRawData);
	return request->packer(TEUL_PACK, a);
}

static SINT32
_sendBrigadeListRequest(ACTION_OBJECT* const a)
{
	BrigadeSummaries* request = (BrigadeSummaries*)(a->localRawData);
	return request->packer(TEUL_PACK, a);
}

static SINT32
_sendBrigadeSelectRequest(ACTION_OBJECT* const a)
{
	BrigadeSelection* request = (BrigadeSelection*)(a->localRawData);
	return request->packer(TEUL_PACK, a);
}

static SINT32
_sendMissionListRequest(ACTION_OBJECT* const a)
{
	MissionList* request = (MissionList*)(a->localRawData);
	return request->packer(TEUL_PACK, a);
}

static SINT32 _sendTutProgress(ACTION_OBJECT* const a)
{
	((Tutorial::GroupProgress*)(a->localRawData))->packer(TEUL_PACK, a);
	return a->offset;
}

static SINT32
_recvPersonaListResponse(ACTION_OBJECT* const a)
{
	PersonaSummaries ps(a);
	ps.receive(a->teulIndex);
	return a->offset;
}

static SINT32
_recvPersonaSelectResponse(ACTION_OBJECT* const a)
{
	PersonaSelection ps(a);
	ps.receive(a->teulIndex);
	return a->offset;
}

//////////////////////////////////////////////////////////////////////
// Receive a HCUnitList which will contain one or more High Command
// Units (brigades, divisions, etc). Since these are static, server-
// side, the host sends them to us once on startup. The objects
// register themselves into <HCUNIT_INDEX hcUnits> so that when the
// host can simply send us IDs to reference these objects.
// Note:
// The host spits the HCUnits out one country at a time to keep
// the amount of data spam down.

SINT32
HCUnitList::receive(SINT32 teulIndex)
{
	static UINT32 _packets = 0;

	if ( m_listStart )
	{
		// m_listStart indicates the first packet in a sequence
		Write_Debug("Receiving new HCUnit List");
		_packets = 0;
	}
	else
	{
		// If m_listStart is false, this packet is a continuation
		Write_Debug("Receiving HCUnit list packet #%d", _packets);
	}

	// We contain several HCUnit objects in our m_units field.
	for ( UINT32 i = 0; i < m_count && i < MAX_HCUNITS_PER_LIST; ++i )
	{
		HCUnit& unit = m_units[i];

		Write_Debug("p#%d e#%d unit#%d type#%d rank#%d:: %s", _packets, i, unit.unitID(), unit.type(), unit.rank(), unit.name());

		// Constructed this way, the HCUnit will self-register itself into hcUnits.
		HCUnit* hcUnit = new HCUnit(unit.unitID(), unit.type(), unit.parentID(), unit.country(), unit.branch(), unit.movementType(), unit.unitClass(), unit.handle(), unit.name(), unit.shortName(), unit.rank(), unit.homeCPID(), unit.owner(), unit.ownerRank());

		// The client assigns decals to brigades, so we need to look the unit up and pass it to the decal
		// code /once/ per HC Unit
		if(hcUnit->isBrigade() == true)
		{
			vehdecalCreateUnitMap(hcUnit);
		}
	}

	++_packets;

	if ( m_listEnd )
	{
		// When m_listEnd is set, this is the last packet in a sequence
		Write_Debug("HCUnit list completed: %d units in %d packets", hcUnits.size(), _packets);
	}

	return 0;
}

static SINT32
_recvHCUnitList(ACTION_OBJECT* const a)
{
	HCUnitList hcul(a);
	hcul.receive(a->teulIndex);
	return a->offset;
}

static SINT32
_recvBrigadeListResponse(ACTION_OBJECT* const a)
{
	try
	{
		BrigadeSummaries* bs = new BrigadeSummaries(a);
		bs->receive(a->teulIndex);
		delete bs;
	}
	catch (std::exception& e)
	{
		(void)e;
		Write_Debug("BrigadeSummaries failed: %s", e.what());
	}
	return a->offset;
}

static SINT32
_recvBrigadeSelectResponse(ACTION_OBJECT* const a)
{
	BrigadeSelection bs(a);
	bs.receive(a->teulIndex);
	return a->offset;
}

static SINT32
_recvMissionListResponse(ACTION_OBJECT* const a)
{
	MissionList ml(a);
	ml.receive(a->teulIndex);
	return a->offset;
}

//////////////////////////////////////////////////////////////////////

static SINT32
_recvArenaDescription(ACTION_OBJECT* const a)
{
	g_arenaDescription.packer(TEUL_UNPACK, a);
	if(a->offset > 0)
		g_arenaDescription.receive(a->teulIndex);

#ifdef RA_AND_CAMPAIGN
	g_arenaDescription["play_mode"] = "0";
#endif


	return a->offset;
}

//////////////////////////////////////////////////////////////////////

static void showBuildInfo(std::string& display)
{
	phui::Caption* buildinfo = (phui::Caption*)uiFindView("buildinfo");
	if(!buildinfo) return;

	ARENA_VARS::iterator itr = g_arenaDescription.find("host_rev");

	if(Online && itr != g_arenaDescription.end())
	{
		display = "Host: ";
		display += itr->second.c_str();

		buildinfo->SetValue(display.c_str());
	}
	else if(Online)
	{
		display = "Host unknown";
		buildinfo->SetValue(display.c_str());
	}
	else
	{
		display = "Host offline";
		buildinfo->SetValue(display.c_str());
	}
}


SINT32
ArenaDescription::receive(SINT32 teulIndex)
{
#ifdef FANCY_ARENA_DESC_HANDLING
	ARENA_VARS::iterator it;
	for(it = begin(); it != end(); ++it)
	{
		// it->first = key, it->second = value
		// expose me to javascript
		// right now we receive this data nad do nothing with it
	}
#endif

	// Display host build info while loading
	std::string display;
	showBuildInfo(display);

	return 0;
}

//////////////////////////////////////////////////////////////////////
// Receive tutorial progress.

static SINT32
_recvTutProgressBatch(ACTION_OBJECT* const a)
{
	// The host stuffs as many GroupProgress objects into the
	// packet as will fit - it doesn't send a count, it just
	// trusts to the fact offset will reach size when we reach
	// the end of the package.
	while ( a->atEndOfBuffer() == false )
	{
		Tutorial::GroupProgress incoming(a);
		incoming.receive(nullptr);
	}
	return a->offset;
}

//////////////////////////////////////////////////////////////////////
// Process the "big list of vislevels".

extern void strtApplySuperCellVisLevels(const SupercellVislevelXfer& xfer);

static SINT32
_recvBuildingVisLevels(ACTION_OBJECT* const a)
{
	UCHAR* const src = a->packed;
	UCHAR* const endPtr = src + a->sizeInBytes();

	for ( UCHAR* ptr = src; src < endPtr; )
	{
		SupercellVislevelXfer& xfer = *(SupercellVislevelXfer*)ptr;
		const size_t size = xfer.size_bytes();
		if ( size == 0 )
		{
			ptr = endPtr;
			break;
		}

		ptr += size;

		strtApplySuperCellVisLevels(xfer);
	}

	return a->offset + a->sizeInBits();
}

//////////////////////////////////////////////////////////////////////

SINT32 mapSendExitWWIIOL(SINT32 hostIndex)
{
	return teulSend(hostIndex,MAP_SEND_EXIT_WWIIOL, nullptr);
}

//////////////////////////////////////////////////////////////////////

// Send a request for a list of personas
// Note: pass "hostIndex" as teulIndex
SINT32
PersonaSummaries::send(SINT32 teulIndex)
{
	// Client never sends persona summaries up to the host
	m_count = 0;
	return teulSend(teulIndex, _XCHG_PERSONA_LIST, this);
}

//////////////////////////////////////////////////////////////////////

// Ask the host to select a specific persona
// Pass "hostIndex" as teulIndex
SINT32
PersonaSelection::send(SINT32 teulIndex)
{
#if platOS == OS_WIN32
	char filename[256];

	// Checksum the current executable
	if ( GetModuleFileNameA(nullptr, filename, sizeof(filename)-1) )
	{
		m_checksum = _check64(filename);	// See function for description; its cryptic on purpose.
	}
#else
	m_checksum = 8173587580LL;
#endif

	return teulSend(teulIndex, _XCHG_PERSONA_SELECT, this);
}

//////////////////////////////////////////////////////////////////////

SINT32
BrigadeSummaries::send(SINT32 teulIndex)
{
	return teulSend(teulIndex, _XCHG_BRIGADE_LIST, this);
}

//////////////////////////////////////////////////////////////////////

SINT32
BrigadeSelection::send(SINT32 teulIndex)
{
	if ( m_status == BRIG_SELECT )
		Write_Debug("SELECTing brigade %d", m_detail.brigadeID());
	else if ( m_status == BRIG_DETAIL )
		Write_Debug("QUERYing brigade %d", m_detail.brigadeID());
	else
	{
		Write_Debug("Unknown selection request type.");
		return -EINVAL;
	}

	///TODO: Prevent player from spamming the host with selection requests,
	/// since this could be confusing in terms of the responses we get back.

	return teulSend(teulIndex, _XCHG_BRIGADE_SELECT, this);
}

//////////////////////////////////////////////////////////////////////

// Request a mission list
SINT32
MissionList::send(SINT32 teulIndex)
{
	// For now we just send a status
	m_status = MLR_OK;
	return teulSend(teulIndex, _XCHG_MISSION_LIST, this);
}

//////////////////////////////////////////////////////////////////////
// Send an opinion poll choice back to the host

void
mapSendOpinionPollResult(SINT32 teulIndex, UINT32 pollID, UINT32 entryID)
{
	OpinionChoice choice(pollID, entryID);
	OPINION_CHOICE_MSG outgoing(choice);
	outgoing.send(teulIndex);
}

//////////////////////////////////////////////////////////////////////
// Change the flags of a progress group on the host.
// CAVEAT: This current actually gets sent to the chat host
// because we don't have a persistent map connection yet.

void
Tutorial::GroupProgress::send(SINT32 chatIndex)
{
	teulSend(chatIndex, _XCHG_TUT_PROGRESS, this);
}

//////////////////////////////////////////////////////////////////////

SINT32 mapInstallClientCallbacks(void)
{
	BEGIN_CALLBACK_REGISTRATION(Map::CallbackX)

		REGISTER_SENDER(Map::EXIT_WWIIOL_Y, _sendExitWWIIOL, MAP_SEND_EXIT_WWIIOL);

		REGISTER_RECEIVER(Map::XCHG_HCUNIT_LIST_Y, _recvHCUnitList);
		REGISTER_RECEIVER(Map::ARENA_DESCRIPTION_Y, _recvArenaDescription);

		REGISTER_SENDRECV(Map::XCHG_PERSONA_LIST_Y, _sendPersonaListRequest,_recvPersonaListResponse, _XCHG_PERSONA_LIST);
		REGISTER_SENDRECV(Map::XCHG_PERSONA_SELECT_Y, _sendPersonaSelectRequest, _recvPersonaSelectResponse, _XCHG_PERSONA_SELECT);
		REGISTER_SENDRECV(Map::XCHG_BRIGADE_LIST_Y, _sendBrigadeListRequest, _recvBrigadeListResponse, _XCHG_BRIGADE_LIST);
		REGISTER_SENDRECV(Map::XCHG_BRIGADE_SELECT_Y, _sendBrigadeSelectRequest, _recvBrigadeSelectResponse, _XCHG_BRIGADE_SELECT);
		REGISTER_SENDRECV(Map::XCHG_MISSION_LIST_Y, _sendMissionListRequest, _recvMissionListResponse, _XCHG_MISSION_LIST);

		REGISTER_MSG_RECEIVER(Map::HC_UNIT_LOCATIONS_Y, HC_UNIT_LOCATIONS);
		REGISTER_MSG_RECEIVER(Map::SWING_METER_Y, SWING_METER_MSG);

		// Incoming description of an opinion poll
		REGISTER_MSG_RECEIVER(Map::OPINION_POLL_Y, OPINION_POLL_MSG);

		// Our response (if any) to a poll
		REGISTER_MSG_SENDER(Map::POLL_RESULT_Y, OPINION_CHOICE_MSG);

		REGISTER_RECVSEND(Map::TUT_PROGRESS_Y, _recvTutProgressBatch, _sendTutProgress, _XCHG_TUT_PROGRESS);

		// The big packet of strat data we get when we spawn.
		REGISTER_RECEIVER(Map::BUILDING_VISLEVELS_Y, _recvBuildingVisLevels);

	END_CALLBACK_REGISTRATION
}
