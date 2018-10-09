#include "syscfg.h" // Path is project specific.
// This module provides access to the socket
// functions used by the teul API. The end-user developer
// should never need the functions provided here.
//
// Copyright 1999 Playnet Inc, All rights reserved
// Not to be distributed in API form except as part of a compiled
// library
//
// Originating Programmer: Martin Poulin

//#define DEBUG_SEND_PACKET

#include "teul/teulCallbackStruct.h"
#include "teul/teulEndpoint.h"
#include "teul/teulEndpointImpl.h"
#include "teul/teulKit.h"
#include "teul/teulModule.h"
#include "teul/teulPacket.h"
#include "teul/teulPacketHeader.h"
#include "wwii/wwiiNetPorts.h"
#include "teul/raknetConn.h"
#include "wwii/wwiiConfig.h"
#include "mem/memPool.h"

// Maximum send buffer of 4096Kb.
static Mem::Pool<Teul::Packet, 256 * 4096 / sizeof(Teul::Packet)> s_delayedPackets;

//////////////////////////////////////////////////////////////////////
// Pack the "outdata" for a callback and send it to the specified
// endpoint destination.

SINT32 teulModSendCALLBACKe(ENDPOINT& endpoint, TEUL_CALLBACK& callback, const UCHAR protocolPreference)
{
	if (endpoint.state() == IS_DEAD) {
		dbugLog(_FLA, "t:%d sock:" PRF_SOCKET " endpoint not open, state:%d callback " PRF_CALLBACK ""
					, endpoint.teulIndex(), PRA_SOCKET(endpoint.socket), endpoint.state(), PRA_CALLBACK(callback));
		return -ETEUL_CB_CONN_DEAD;
	}

	dbugLog(_FLD, "t:%d sending callback " PRF_CALLBACK "", endpoint.teulIndex(), PRA_CALLBACK(callback));
	if (!callback.m_sendAction) {
		dbugLog(_FLA, "no action for callback " PRF_CALLBACK "", PRA_CALLBACK(callback));
		return -ETEUL_CB_SEND_NULL;
	}

    Teul::Packet outgoing(Teul::PacketId(callback.callX(), callback.callY()));
    outgoing.reserve(Teul::MaxPacket);

	ACTION_OBJECT actionObj(outgoing.writePointer(), 0, -1);
	actionObj.teulIndex = endpoint.teulIndex();
	actionObj.endpoint = &endpoint;
	actionObj.actionObj = &callback;
	actionObj.localRawData = callback.outData;

	const SINT32 packedSizeBits = (callback.m_sendAction)(&actionObj);
    ABORT_IF_EX(packedSizeBits >= Teul::MaxPacket * 8, "Data was too big for packet, it's just luck that you didn't crash already!");
	if (packedSizeBits < 0) {
		// probable data corruption
		dbugLog(_FLA, "t:%d cb:" PRF_CALLBACK " pack failed:%d", endpoint.teulIndex(), PRA_CALLBACK(callback), packedSizeBits);
		return packedSizeBits;
	}

	outgoing.setPayloadSizeBytes(Teul::BitsToBytes(packedSizeBits));

	const SINT32 sendResult = outgoing.send(endpoint, protocolPreference);
	if (sendResult < 0) {
		dbugLog(_FLW, "t:%d callback " PRF_CALLBACK " send failed:%d", endpoint.teulIndex(), PRA_CALLBACK(callback), sendResult);
		return sendResult;
	}

	return packedSizeBits;
}

//////////////////////////////////////////////////////////////////////
// Actually write data to a socket.

SINT32 ENDPOINT::writeData(void* const data, const size_t sizeInBytes, const UINT32 address, const UINT16 port)
{
    ssize_t result = SOCKET_ERROR;
    struct sockaddr_in *realAddress = nullptr, possibleAddrStorage = destSockAddress;
    socklen_t realAddressLength = 0;
    
    // Passing NULL for destination is identical to calling send(), and sendto()
    // will scream EISCONN for anything but NULL on a TCP socket, so only set
    // the destination address for UDP endpoints.
	if (protocol != PROTOCOL_TCP && protocol != PROTOCOL_CTRL) {
        realAddress = &possibleAddrStorage;
        realAddressLength = sizeof(possibleAddrStorage);
        if (address) {
            realAddress->sin_family = AF_INET;
            realAddress->sin_addr.s_addr = htonl(address);
            realAddress->sin_port = htons(port ? port : WWII::Net::Ports::PrivateBcast);
        }
    }
    
    int err = 0;
    
    do {
	    result = ::sendto(socket, static_cast<char *>(data), sizeInBytes, 0, (struct sockaddr *)realAddress, realAddressLength);
    } while (result == SOCKET_ERROR && (err = dbugGetErrno()) == PLAT_EINTR);
    
	if (result == SOCKET_ERROR) {
		if (safe_isEAGAIN(err)) {
            dbugLog(_FLD, "::sendto returned code %d, probably no space in kernel bufs", err);
            return 0;
        } else {
			dbugLog(_FLW, "t:%d sock:" PRF_SOCKET " error on send %" PRSizeT " bytes to %s:%u, err:%d",
                    teulIndex(), PRA_SOCKET(socket), sizeInBytes, teulGetAddress(address), port, err);
			return -err;
		}
	}

	dbugLog(_FLD, "t:%d sock:" PRF_SOCKET " %" PRSsizeT " bytes sent %" PRSizeT " requested", teulIndex(), PRA_SOCKET(socket), result, sizeInBytes);
    if (result > 0) {
	    lastDataSent.makeCurrent();
    }
	return result;
}

//////////////////////////////////////////////////////////////////////
// Try to send or queue a packet to a socket.

SINT32 ENDPOINT::sendPacket(Teul::Packet& packet, const UCHAR protocolPreference)
{
	const UINT32 packetBytes = packet.packetSizeBytes();

	// First check to see if this should be sent via a RakNet backchannel
    //      - through configuration, make sure we even have it enabled.
    //      - application must be asking for UDP
    //      - channel must exist, be initialized
    //      - if a client, must also be connected
	if (protocolPreference == PROTOCOL_UDP &&
		m_pairdUdpConn != nullptr && m_pairdUdpConn->isInitialized() &&
		(m_pairdUdpConn->isServer() || m_pairdUdpConn->isConnected())
#if platBuild == BUILD_SERVER
        && host_config.netcode3Enabled
#endif
    ) {
        // No need to encrypt, RakNet has its own encryption
        if (m_pairdUdpConn->send(packet, *this)) {
            return static_cast<SINT32>(packetBytes);
        }
	}  // fall through if sending failed - transmit via normal path

	packet.header().m_tag = ++this->m_impl->sentTag;

	// Encrypt the data.
	if (packet.header().packetToken() != Teul::PacketToken::Unencrypted) {
#ifdef FIX_CRYPTO_MISALIGNMENT
        std::unique_ptr<char[]> tempData = std::make_unique<char[]>((packet.header().packetSizeBytes() + 3) & ~3);
    
        memcpy(tempData.get(), packet.encryptable(), packet.header().packetSizeBytes());
        encrypt(reinterpret_cast<unsigned char * const>(tempData.get()), packet.header().packetSizeBytes());
        memcpy(packet.encryptable(), tempData.get(), packet.header().packetSizeBytes());
#else
        encrypt(packet.encryptable(), packet.header().packetSizeBytes());
#endif
	}

    ssize_t result = 0;
    
    // If there's no send queue, try the send immediately.
	if (m_impl->m_sendQueue.empty()) {
		result = writeData(packet.packet(), packetBytes, packet.address(), packet.port());

		if (result < 0) {
            return result;
		}
        ABORT_IF_EX(static_cast<UINT32>(result) > packetBytes, "Sent %" PRSsizeT " bytes but only tried to send %u bytes!", result, packetBytes);
    }

    packet.addBytesSent(result);
    if (packet.sentBytes() < packetBytes) {
        // Entire packet not yet sent, queue it.
        Teul::Packet* const copy = s_delayedPackets.New(packet);
        copy->addBytesSent(packet.sentBytes());
        m_impl->m_sendQueue.push_back(copy);
        setWritesPending(true);
    }
	return static_cast<SINT32>(packetBytes); // always returns entire size even if send was partial?
}

//////////////////////////////////////////////////////////////////////
// Free the first packet in an endpoint's send queue.

void EndpointImpl::freeFirstPacketInSendQueue()
{
	Teul::Packet* packet = m_sendQueue.front();
	m_sendQueue.pop_front();
	s_delayedPackets.Delete(packet);
}

//////////////////////////////////////////////////////////////////////
// Try to drain the send queue of a socket.

SINT32 ENDPOINT::pushSendQueue()
{
	while (m_inUse && !m_closed && !m_impl->m_sendQueue.empty()) {
		Teul::Packet* packet = m_impl->m_sendQueue.front();
		size_t sentBytes = packet->sentBytes();
		size_t remainingBytes = packet->packetSizeBytes() - sentBytes;
		void* data = static_cast<char *>(packet->packet()) + sentBytes;

		ssize_t result = writeData(data, remainingBytes, packet->address(), packet->port());
		if (result < 0) {
			return result;
		}
        ABORT_IF_EX(static_cast<size_t>(result) > remainingBytes, "Sent %" PRSsizeT " bytes but only tried to send %" PRSizeT " bytes!", result, remainingBytes);

		packet->addBytesSent(result);
		if (packet->sentBytes() < packet->packetSizeBytes()) {
            // Hit flow control again. Break out for now.
            return 0;
        }

        // Sent the whole packet
        m_impl->freeFirstPacketInSendQueue();
	}

    // If we get down here, we successfully drained the entire queue.
    setWritesPending(false);
	return 0;
}

//////////////////////////////////////////////////////////////////////
//

SINT32 Teul::Packet::send(ENDPOINT& endpoint, const UCHAR protocolPreference)
{
    static_assert(sizeof(Teul::PacketHeader::tag_t) == sizeof(endpoint.m_impl->sentTag), "Endpoint and Packet must agree on tag size");

	if (endpoint.state() == IS_LISTENING || endpoint.state() == IS_DEAD) {
		dbugLog(_FLA, "t:%d tried to send to non-open endpoint [st:%d]", endpoint.teulIndex(), endpoint.state());
		return -ENOTCONN;
	}
	ABORT_IF_EX(header().m_tag != 0 && header().packetToken() != Teul::PacketToken::Unencrypted, "tried to re-send a Teul::Packet");

    teulSyncClock();
#if platBuild == BUILD_CLIENT
	header().m_tick = (Teul::PacketHeader::tick_t)teulClock64;
#else
	// Send the client back it's tick plus the amount of time elapsed
	// since that tick was received, by which the client can deduce
	// it's own ping time.
	header().m_tick = endpoint.m_impl->m_lastPacketTick + (Teul::PacketHeader::tick_t)(teulClock64 - endpoint.m_impl->m_lastTickRx);
#endif

	// The packet size, as far as Teul is concerned. Doesn't include the prefix.
	size_t psb = packetSizeBytes();
	size_t payloadBytes = psb - Teul::PacketPrefixBytes;
    
	header().setPacketSizeBytes(payloadBytes);
	ABORT_IF_EX(header().packetSizeBytes() != payloadBytes, "Teul has developed a reality distortion field");
	m_sent = 0;

	SINT32 result = endpoint.sendPacket(*this, protocolPreference);
    
	if (result < 0) {
        // Failed to send. Don't dbugGetErrno() here. We already did that in sendData().
        dbugLog(_FLW, "t:%d sock:" PRF_SOCKET " err:%d send failed ", endpoint.teulIndex(), PRA_SOCKET(endpoint.socket), -result);
        // This is a HORRIBLE hack to allow us to avoid recursion during initial connection of an endpoint.
        // This call to teulClose() is the only potential point of recursion into teulUpdateTransport() as
        // the code is currently written, and we need to avoid it during TCP connection. This should be
        // removed as soon as possible.
        if (header().m_callX != 0 || header().m_callY != 5) {
		    teulClose(endpoint);
        }
	}

	return result;
}

//////////////////////////////////////////////////////////////////////

SINT32 teulModSendCALLBACK(SINT32 teulIndex, TEUL_CALLBACK& object, const UCHAR protocol)
{
	ENDPOINT* const theEndpoint = teulGetEndpoint(teulIndex);
    
	if (!theEndpoint) {
		dbugLog(_FLA, "t:%d " PRF_CALLBACK " invalid endpoint in send callback", teulIndex, PRA_CALLBACK(object));
		return -EINVAL;
	}
	return teulModSendCALLBACKe(*theEndpoint, object, protocol);
}

//////////////////////////////////////////////////////////////////////

SINT32 teulModSendCallbackTo(SINT32 teulIndex, UINT32 address, UINT16 port, TEUL_CALLBACK& object)
{
	ENDPOINT* const theEndpoint = teulGetEndpoint(teulIndex);
    
	if (!theEndpoint) {
		dbugLog(_FLA, "t:%d " PRF_CALLBACK " invalid endpoint in send-to callback", teulIndex, PRA_CALLBACK(object));
		return -EINVAL;
	}

	// Temporarily save the dest address.
	sockaddr_in savedDestAddress = theEndpoint->destSockAddress;
    
	theEndpoint->destSockAddress.sin_addr.s_addr = htonl(address);
    theEndpoint->destSockAddress.sin_port = port ? htons(port) : theEndpoint->localSockAddress.sin_port;

	// Ensure the local network address components are populated if sending to a broadcast address.
	constexpr UINT32 broadcastNetMask = 0x00ffffff;
    
	if ((theEndpoint->destSockAddress.sin_addr.s_addr & broadcastNetMask) == 0) {
		theEndpoint->destSockAddress.sin_addr.s_addr |= (theEndpoint->localSockAddress.sin_addr.s_addr & broadcastNetMask);
    }
	if ((theEndpoint->destSockAddress.sin_addr.s_addr & ~broadcastNetMask) == 0) {
		theEndpoint->destSockAddress.sin_addr.s_addr |= (theEndpoint->localSockAddress.sin_addr.s_addr & ~broadcastNetMask);
    }

	SINT32 retval = teulModSendCALLBACKe(*theEndpoint, object);

	theEndpoint->destSockAddress = savedDestAddress;
	return retval;
}
