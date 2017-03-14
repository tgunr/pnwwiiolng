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
#include <execinfo.h>

#include "wwii/wwiiHash.h"
using SYNCHED_VEHICLES = std::unordered_map<UINT32, bool>;
static SYNCHED_VEHICLES g_synched;

static Teul::Packet vehicleBroadcasts(Teul::PacketId(Comm::CallbackX, BCAST_VEH_UPDATE_Y));
static UINT32 numberOfVehiclesPending = 0;
static SINT32 maxSize = 0;

extern SINT32 _userConnections;

bool g_clustered = false;

//////////////////////////////////////////////////////////////////////
// Receive a remove list from a remote cell host

extern SINT32
cellVehicleRemoval(ACTION_OBJECT* const a)
{
	const UINT32 numUpdates = a->sizeInBytes() / (sizeof(UINT32)+sizeof(UCHAR));
	UCHAR* data = reinterpret_cast<UCHAR*>(a->packed);

	for ( UINT32 i = 0; i < numUpdates; ++i )
	{
		const UINT32 vehIndex = *(reinterpret_cast<UINT32 *>(data));
		data += sizeof(UINT32);
		const PLAYER_EXIT_CODE exitCode = static_cast<PLAYER_EXIT_CODE>(*data);
		data ++;
		hostRemoveRemoteVehicle(vehIndex, exitCode);
	}

	dbugLog(_FLD, "cell:%s removed %u remote vehicles"
					, teulGetAddress(a->fromAddress), numUpdates);

	return a->offset;
}

//////////////////////////////////////////////////////////////////////
// Receive a vehicle update packet and pass it on to the comm system

UINT32
hostReceiveVehicleUpdates(unsigned char* const input, const UINT32 bytes, const UINT32 source)
{

#ifdef DEV_BUILD
	char temp_buff[bytes * 3 + 1];
	for (UINT32 i=0; i < bytes; i++)
	{
		sprintf(temp_buff+i*3, "%02X ", input[i]);
	}
	temp_buff[bytes*3] = 0;
	dbugLog(_FLD, "MultiCell: Incoming packet: %s", temp_buff);
#endif

	const unsigned char* const end = input + bytes;
	UINT32 count = 0;
	SINT32 offset = 0;
	SINT32 size = 0;
	while ( (input + (offset/8)) < end  && offset >= 0 )
	{
		// Since we are padding each block and they aren't being accounted for in the PACK/UNPACK we need to
		// reset the offset manually
		SINT32 nextOffset = offset;
	        offset = teulPackSINT32(TEUL_UNPACK, input, offset, &size, 0);
		nextOffset += size * 8;

#ifdef DEV_BUILD
                dbugLog(_FLD, "hostReceiveVehicleUpdates extracting update of size %d", size);
#endif
		commRecvVehicleUpdate(source, input, offset);

		offset = nextOffset;
		++count;
	}

	return count;
}

//////////////////////////////////////////////////////////////////////
// Receive a vehicle list resync request packet 

UINT32
hostReceiveResyncVehicles(unsigned char* const input, const UINT32 bytes, const UINT32 source)
{
	// vehicles present in this list are presumed to be synched, by clearing the list we are telling our host
	// that they need to resend the full information to other host
	g_synched.clear();	

	dbugLog(_FLI, "Received resync request from %s", teulGetAddress(source));
	void *frames[64]; int n = backtrace(frames, 64); char **names = backtrace_symbols(frames, n); for (int i = 0; i < n; ++i) { dbugLog(_FLD, "Frame %d: %s", i, names[i]); } free(names);

	return 0;
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
	{
		return;
	}

	if ( !g_clustered )
	{
		if ( vehicleBroadcasts.empty() == false )
			vehicleBroadcasts.reset();
		numberOfVehiclesPending = 0;
		return;
	}

	vehicleBroadcasts.setUnencrypted();
	const bool sent = vehicleBroadcasts.send(updateBcastSend) > 0;
	if ( sent == false )
	{
		dbugLog(_FLA, "Unable to broadcast update of size %d to cellnet", vehicleBroadcasts.packetSizeBytes());
	}
	else
	{
#ifdef DEV_BUILD
		dbugLog(_FLD, "pushed %u vbs in %u bytes. maxBits=%d, avgBits=%u", numberOfVehiclesPending, vehicleBroadcasts.packetSizeBytes(), maxSize * 8, vehicleBroadcasts.packetSizeBits() / numberOfVehiclesPending);
#endif
	}
	vehicleBroadcasts.reset();
	numberOfVehiclesPending = 0;
}

//////////////////////////////////////////////////////////////////////
// Send a vehicle broadcast to the other cellhosts

bool
hostSendVehicleBroadcast(UCHAR* buffer, SINT32 sizeBits)
{
	SINT32 sizeBytes = 0;

	if ( buffer == nullptr )
	{
		LogAbort("called hostSendVehicleBroadcast with NULL");
	}
	if ( sizeBits == 0 )
	{
		LogAbort("called hostSendVehicleBroadcast with 0 size");
	}

	if ( !g_clustered )
		return false;

	// Convert bits to bytes and round size up to a multiple of 32.
	sizeBytes = ((sizeBits & ~((sizeof(SINT32)*8)-1))) / 8 + sizeof(SINT32);

	if ( sizeBytes > maxSize )
		maxSize = sizeBytes;

	// store in the final size
	teulPackSINT32(TEUL_PACK, buffer, 0, &sizeBytes, 0);

	vehicleBroadcasts.append(buffer, sizeBytes);
	++numberOfVehiclesPending;

#ifdef DEV_BUILD
	dbugLog(_FLD, "hostSendVehicleBroadcast: original bits = %d(bytes=%d), queuing size = %d, packet allocated length = %d, packet size = %d, pendingPackets = %d", 
		sizeBits,
		sizeBits/8,
		sizeBytes,
		vehicleBroadcasts.packetCapacityBytes(),
		vehicleBroadcasts.packetSizeBytes(),
		numberOfVehiclesPending);
#endif

	// disable packaging of multiple vehicles for now and just send them out as we get them
	// we want to force updates based on how many connections we have.  We might want to put a cap on it too.
	// 1400 is just under the max MTU size but 35 is about the size of your average veh update
    UINT32 limitPacketSize = std::min( static_cast<UINT32>(1400),
		 (static_cast<UINT32>(_userConnections) * 35));
	if ( vehicleBroadcasts.packetSizeBytes() >= limitPacketSize )
	{
		hostPushVehicleBroadcasts();
	}

	return true;
}

//////////////////////////////////////////////////////////////////////
// Force FIRST_TIME updates for all
// This should be called when a cell hosts starts
void
hostForceFirstTimeUpdates()
{
	if ( !g_clustered )
		return;

	g_synched.clear();

	dbugLog(_FLI, "Host initiating request to RESYNC vehicles to all listening cell hosts");

	Teul::Packet outgoing(Teul::PacketId(Comm::CallbackX, RESYNC_VEHICLES_Y));
	outgoing.setUnencrypted();
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

		dbugLog(_FLD, "Adding vehicle to remove broadcast %u, exit code %u", vehIndex, exitCode);

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

