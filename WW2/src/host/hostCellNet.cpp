#include <syscfg.h> // Path is project specific.
// Created: Sat Dec  9 11:14:14 CST 2006
//
// Inter-cell peering code using the cell-to-cell backplane.
//
// Copyright (C) PlayNet, Inc 1999-2006 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "include/clustercfg.h"

#if platOS != OS_WIN32
#include <arpa/inet.h>						// For ntohl
#else
#include <winsock2.h>
#endif

#include "chat/chatSupportRPCMessages.h"
#include "comm/commConstants.h"
#include "comm/commHostMessages.h"
#include "host/hostCellNet.h"
#include "host/host.h"
#include "host/hostResupplyIssue.h"
#include "host/hostVehicle.h"
#include "teul/teulBasic.h"
#include "teul/teulPacket.h"
#include "wwii/wwiiThreads.h"

#include "wwii/wwiiHash.h"
using SYNCHED_VEHICLES = std::unordered_map<UINT32, bool>;
static SYNCHED_VEHICLES g_synched;

static Teul::Packet vehicleBroadcasts(Teul::PacketId(Comm::CallbackX, BCAST_VEH_UPDATE_Y));
static UINT32 numberOfVehiclesPending = 0;
static UINT32 maxSize = 0;

bool g_clustered = false;

//////////////////////////////////////////////////////////////////////
// Receive a remove list from a remote cell host

extern SINT32
cellVehicleRemoval(ACTION_OBJECT* const a)
{
	const UINT32 numUpdates = a->sizeInBytes() / sizeof(UINT32);
	UINT32* data = (UINT32*)a->localRawData;

	for ( UINT32 i = 0; i < numUpdates; ++i )
	{
		const UINT32 vehIndex = *(data++);
		const UINT32 exitCode = *(data++);
		hostRemoveRemoteVehicle(vehIndex, (PLAYER_EXIT_CODE)exitCode);
	}

	dbugLog(_FLD, "cell:%s removed %u remote vehicles"
					, teulGetAddress(a->fromAddress), numUpdates);

	return a->offset;
}

//////////////////////////////////////////////////////////////////////
// Receive a vehicle update packet and pass it on to the comm system

UINT32
hostReceiveVehicleUpdates(const unsigned char* const input, const UINT32 bytes, const UINT32 source)
{
	const unsigned char* cur = input;
	const unsigned char* const end = cur + bytes;
	UINT32 count = 0;
	while ( cur < end )
	{
		SINT32 size = *((const SINT32*)cur);
		cur = cur + sizeof(SINT32);
		commRecvVehicleUpdate(source, cur);
		cur += size;
		++count;
	}

	return count;
}

//////////////////////////////////////////////////////////////////////
// Determine whether a vehicle is in the synched list or not.

bool
hostNeedVehicleFirstTime(UINT32 vehicleID)
{
	bool& current = g_synched[vehicleID];
	const bool result = !current;
	current = true;
	return result;
}

void
hostPushVehicleBroadcasts()
{
	if ( vehicleBroadcasts.empty() == true )
		return;

	if ( !g_clustered )
	{
		if ( vehicleBroadcasts.empty() == false )
			vehicleBroadcasts.reset();
		numberOfVehiclesPending = 0;
		return;
	}

	const bool sent = vehicleBroadcasts.send(updateBcastSend) > 0;
	if ( sent == false )
	{
		threadLog(_FLA, "Unable to broadcast update of size %d to cellnet", vehicleBroadcasts.packetSizeBytes());
	}
	else
	{
#ifdef DEV_BUILD
		threadLog(_FLD, "pushed %u vbs in %u bytes. maxBits=%u, avgBits=%u", numberOfVehiclesPending, vehicleBroadcasts.packetSizeBytes(), maxSize * 8, vehicleBroadcasts.packetSizeBits() / numberOfVehiclesPending);
#endif
	}
	vehicleBroadcasts.reset();
	numberOfVehiclesPending = 0;
}

//////////////////////////////////////////////////////////////////////
// Send a vehicle broadcast to the other cellhosts

bool
hostSendVehicleBroadcast(UCHAR* buffer, SINT32 size)
{
	if ( buffer == nullptr )
	{
		LogAbort("called hostSendVehicleBroadcast with NULL");
	}
	if ( size == 0 )
	{
		LogAbort("called hostSendVehicleBroadcast with 0 size");
	}

	if ( !g_clustered )
		return false;

	// Round size up to a multiple of 32.
	size = ((size & ~((sizeof(size)*8)-1)) + sizeof(size)) / 8;

	if ( (UINT32)size > maxSize )
		maxSize = size;

	if ( vehicleBroadcasts.packetSizeBytes() + size >= 4000 )
		hostPushVehicleBroadcasts();

	vehicleBroadcasts.append(buffer, size);
	++numberOfVehiclesPending;

	return true;
}

//////////////////////////////////////////////////////////////////////
// Force FIRST_TIME updates for all

void
hostForceFirstTimeUpdates()
{
	if ( !g_clustered )
		return;

	g_synched.clear();
	Teul::Packet outgoing(Teul::PacketId(Comm::CallbackX, RESYNC_VEHICLES_Y));
	outgoing.send(updateBcastSend);
}

//////////////////////////////////////////////////////////////////////
// Queue/send a vehicle to the remote cell hosts
// Special case: Vehicle ID of 0 can be used to send any queued data.

void
hostQueueVehicleRemove(UINT32 vehIndex, UCHAR exitCode)
{
	if ( !g_clustered )
		return;

	static Teul::Packet* outgoing = nullptr;
	static UINT32 vehiclesQueued = 0;

	if ( vehIndex == 0 && vehiclesQueued == 0 )
		return;

	if ( vehIndex != 0 )
	{
		if ( outgoing == nullptr )
		{
			outgoing = new Teul::Packet(Teul::PacketId(Comm::CallbackX, BCAST_VEH_REMOVE_Y));
			outgoing->setUnencrypted();
		}

		outgoing->append(vehIndex);
		outgoing->append(exitCode);

		g_synched.erase(vehIndex);
		++vehiclesQueued;

		return;
	}

	dbugLog(_FLD, "cell sending %u vehicle removals", vehiclesQueued);

	outgoing->send(updateBcastSend);

	delete outgoing;
	outgoing = nullptr;

	vehiclesQueued = 0;
}

