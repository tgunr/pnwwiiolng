#include <syscfg.h> // Path is project specific.
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
#include "teul/teulClosingEnums.h"
#include "teul/teulEndpoint.h"
#include "teul/teulEndpointImpl.h"
#include "teul/teulKit.h"
#include "teul/teulModule.h"
#include "teul/teulPacket.h"
#include "teul/teulPacketHeader.h"
#include "wwii/wwiiNetPorts.h"

UINT32 totalPacketsSent;
UINT32 totalPacketsRecv;
UINT32 totalPacketsXfer;

#if platOS == OS_LINUX || platOS == OS_MACOS
# define SOCK_SEND_FLAGS (MSG_DONTWAIT)
# include <sys/socket.h>
#else
# define SOCK_SEND_FLAGS 0
#endif

// Maximum send buffer of 256Kb.

#include "mem/memPool.h"

static Mem::Pool<Teul::Packet, 16 * 4096 / sizeof(Teul::Packet)> s_delayedPackets;

//////////////////////////////////////////////////////////////////////

void teulInitSend()
{
	totalPacketsSent = 0;
	totalPacketsRecv = 0;
	totalPacketsXfer = 0;
}

void teulCleanupSend()
{
	s_delayedPackets.Cleanup();
}

//////////////////////////////////////////////////////////////////////
// Pack the "outdata" for a callback and send it to the specified
// endpoint destination.

SINT32
teulModSendCALLBACKe(ENDPOINT& endpoint, TEUL_CALLBACK& callback)
{
	if( PN_UNLIKELY(endpoint.state() == IS_DEAD) )
	{
		WriteLog(LOG_TEUL, LOG_ABORT, "t:%d sock:" PRF_SOCKET " endpoint not open, state:%d callback " PRF_CALLBACK ""
					, endpoint.teulIndex(), PRA_SOCKET(endpoint.socket), endpoint.state(), PRA_CALLBACK(callback));
		return -ETEUL_CB_CONN_DEAD;
	}

	WriteLog(LOG_TEUL, LOG_DEBUG, "t:%d sending callback " PRF_CALLBACK "", endpoint.teulIndex(), PRA_CALLBACK(callback));

	if ( PN_UNLIKELY(callback.m_sendAction == nullptr) )
	{
		WriteLog(LOG_TEUL, LOG_ABORT, "no action for callback " PRF_CALLBACK "", PRA_CALLBACK(callback));
		return -ETEUL_CB_SEND_NULL;
	}

	// All packers need an ACTION_OBJECT, and we
	ACTION_OBJECT actionObj;
	actionObj.teulIndex = endpoint.teulIndex();
	actionObj.endpoint = &endpoint;
	actionObj.actionObj = &callback;
	actionObj.localRawData = callback.outData;

	Teul::Packet outgoing(Teul::PacketId(callback.callX(), callback.callY()));
	outgoing.reserve(Teul::MaxPacket);

	actionObj.packed = (UCHAR*)outgoing.writePointer();
	actionObj.offset = 0;
	actionObj.m_size_bytes = -1;

	const SINT32 packedSizeBits = (callback.m_sendAction)(&actionObj);
	if ( PN_UNLIKELY(packedSizeBits < 0) )
	{
		// probable data corruption
		WriteLog(LOG_TEUL, LOG_ABORT, "t:%d cb:" PRF_CALLBACK " pack failed:%d", endpoint.teulIndex(), PRA_CALLBACK(callback), packedSizeBits);
		return packedSizeBits;
	}

	outgoing.setPayloadSizeBytes(Teul::BitsToBytes(packedSizeBits));

	const SINT32 sendResult = outgoing.send(endpoint);
	if ( PN_UNLIKELY(sendResult < 0) )
	{
		WriteLog(LOG_TEUL, LOG_WARN, "t:%d callback " PRF_CALLBACK " send failed:%d", endpoint.teulIndex(), PRA_CALLBACK(callback), sendResult);
		return sendResult;
	}

	return packedSizeBits;
}

#if platBuild == BUILD_SERVER
SINT32
ENDPOINT::writeRaw(const void* const data, const size_t sizeInBytes) noexcept
{
	ABORT_IF_EX(protocol != PROTOCOL_CTRL, "ENDPOINT::sendRaw only allowed on CTRL interfaces");
	return ::send(socket, (const char*)data, sizeInBytes, SOCK_SEND_FLAGS);
}
#endif

//////////////////////////////////////////////////////////////////////
// Actually write data to a socket.

SINT32
ENDPOINT::writeData(void* const data, const size_t sizeInBytes, const UINT32 address, const UINT32 port)
{
	if ( protocol == PROTOCOL_TCP || protocol == PROTOCOL_CTRL )
	{
		// WriteLog(LOG_TEUL, LOG_DEBUG, "about to send sub-packet");
	 	SINT32 retval = ::send(socket, (const char*)data, sizeInBytes, SOCK_SEND_FLAGS);
		// WriteLog(LOG_TEUL, LOG_DEBUG, "Write result %d", retval);
		if (retval > 0)
			dbugOutputPacket( (*this), false, data, retval );
		return retval;
	}

	// Apply the default port.
	const UINT16 targetPort = port ? (UINT16) port : (UINT16) WWII::Net::Ports::PrivateBcast;

	sockaddr_in inetAddress = destSockAddress;
	if ( address != 0 )
	{
		inetAddress.sin_family = AF_INET;
		inetAddress.sin_addr.s_addr = htonl( address );
		inetAddress.sin_port = htons( targetPort );
	}

	const ssize_t result = ::sendto(socket, (const char*)data, sizeInBytes, 0, (struct sockaddr*)&inetAddress, sizeof(inetAddress));
	if ( PN_UNLIKELY(result <= 0) )
	{
		// Socket closed.
		if ( result == 0 )
		{
			WriteLog(LOG_TEUL, LOG_WARN, "t:%d sock:" PRF_SOCKET " socket closed on sendto for %" PRSizeT " bytes to %u,%u [%u,%hu] attempt"
						, teulIndex(), PRA_SOCKET(socket)
						, sizeInBytes
						, address, port, inetAddress.sin_addr.s_addr, inetAddress.sin_port
						);
			teulClose(*this, Teul::Closing::Closed);
			dbugSetErrno(Teul::Closing::Closed);
			return -Teul::Closing::Closed;
		}

		// Check for an interrupted send attempt.
		const int err = dbugGetErrno();
		if ( (err != PLAT_EAGAIN) & (err != PLAT_EINTR) & (err != PLAT_EWOULDBLOCK) )
		{
			WriteLog(LOG_TEUL, LOG_WARN, "t:%d sock:" PRF_SOCKET " send error on send %" PRSizeT " bytes to %u,%u [%u,%hu] attempt, err:%d"
						, teulIndex(), PRA_SOCKET(socket)
						, sizeInBytes
						, address, port, inetAddress.sin_addr.s_addr, inetAddress.sin_port
						, err
						);
			return -ETEUL_ENOSPC_SEND_SOCKET;
		}
		else
		{
			WriteLog(LOG_TEUL, LOG_DEBUG, "::sendto returned code %d, try again return", err);
		}

		return 0;
	}

	WriteLog(LOG_TEUL, LOG_DEBUG, "%" PRSsizeT " bytes sent %" PRSizeT " requested", result, sizeInBytes);

	lastDataSent.makeCurrent();

	return result;
}

//////////////////////////////////////////////////////////////////////
// Try to send or queue a packet to a socket.

SINT32
ENDPOINT::sendPacket(Teul::Packet& packet)
{
	const UINT32 packetBytes = packet.packetSizeBytes();

	// If there's no send queue, try and send the data directly.
	if ( m_impl->m_sendQueue.empty() == true )
	{
		// WriteLog(LOG_TEUL, LOG_DEBUG, "writing direct 1st time: bytesToSend = %d ", packetBytes);
		// WriteLog(LOG_TEUL, LOG_DEBUG, "start of packet address = %p", packet.packet());
		// WriteLog(LOG_TEUL, LOG_DEBUG, "address of packet obj = %p", &packet);

		const SINT32 result = writeData(packet.packet(), packetBytes, packet.address(), packet.port());
		// Anything but a partial send, the sender handles.
		if ( PN_UNLIKELY(result < 0) )
		{
			// If the error was the result of an interruption, then lets just queue the packet
			// and give it a retry later on.
			const SINT32 err = dbugGetErrno();
			if ( (err != PLAT_EAGAIN) && (err != PLAT_EINTR) && (err != PLAT_EWOULDBLOCK) )
			{
				return result;
			}
		}
		if ( result > 0 )
		{
			const size_t sent = (size_t) result;
#ifdef DEBUG_SEND_PACKET
			WriteLog(LOG_TEUL, LOG_DEBUG, "t:%d SendPacketInit [%u:%u] : %d/%u bytes",
			        teulIndex(), packet.header().m_callX, packet.header().m_callY,
			        result, packet.packetSizeBytes());
#endif
			packet.addBytesSent(sent);
			if ( packet.sentBytes() >= packetBytes )
			{
				return packetBytes;
			}
		}
		// Partial or blocked send falls thru and queues the remainder.
	}

#if defined(ALLOW_TEUL_PACKET_MOVE)
	Teul::Packet* const copy = s_delayedPackets.New(std::move(packet));
#else
	Teul::Packet* const copy = s_delayedPackets.New(packet);
	copy->addBytesSent(packet.sentBytes());
#endif

	m_impl->m_sendQueue.push_back(copy);

	setWritesPending(true);

	return packetBytes;
}

//////////////////////////////////////////////////////////////////////
// Free the first packet in an endpoint's send queue.

void
EndpointImpl::FreeFirstPacketInSendQueue()
{
	Teul::Packet* packet = m_sendQueue.front();
	m_sendQueue.pop_front();
	s_delayedPackets.Delete(packet);
}

//////////////////////////////////////////////////////////////////////
// Try to drain the send queue of a socket.

SINT32
ENDPOINT::pushSendQueue()
{
	if ( m_writesPending == true )
		return 0;

	while ( m_inUse == true && m_closed == false && m_impl->m_sendQueue.empty() == false )
	{
		Teul::Packet* packet = m_impl->m_sendQueue.front();
		const size_t sentBytes = packet->sentBytes();
		const size_t remainingBytes = packet->packetSizeBytes() - sentBytes;
		void* data = (char*)packet->packet() + sentBytes;

		// WriteLog(LOG_TEUL, LOG_DEBUG, "writing off of queue: sentBytes = %d, packet->packetSizeBytes = %d, remaining = %d", sentBytes, packet->packetSizeBytes(), remainingBytes);
		// WriteLog(LOG_TEUL, LOG_DEBUG, "start of packet address = %p", packet->packet());
		// WriteLog(LOG_TEUL, LOG_DEBUG, "address of packet obj = %p", packet);
		// WriteLog(LOG_TEUL, LOG_DEBUG, "address for write to start from = %p", data);

		const SINT32 result = writeData(data, remainingBytes, packet->address(), packet->port());
		// Result will be less than sendbytes if it is an error (negative),
		// close (0) or smaller number of bytes.
		if ( result <= 0 )
		{
			setWritesPending(true);
			return result;
		}
#ifdef DEBUG_SEND_PACKET
		WriteLog(LOG_TEUL, LOG_DEBUG, "t:%d SendPacketCont [%u:%u] : %d/%u bytes",
		        teulIndex(), packet->header().m_callX, packet->header().m_callY,
		        result, remainingBytes);
#endif
		packet->addBytesSent((size_t)result);
		if ( packet->sentBytes() >= packet->packetSizeBytes() )
		{
			// We appear to have sent the data.
			m_impl->FreeFirstPacketInSendQueue();
		}
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////
//

SINT32
Teul::Packet::send(ENDPOINT& endpoint)
{
	if ( PN_UNLIKELY(endpoint.state() == IS_LISTENING || endpoint.state() >= IS_CLOSING) )
	{
		WriteLog(LOG_TEUL, LOG_ABORT, "t:%d tried to send to non-open endpoint [st:%d]", endpoint.teulIndex(), endpoint.state());
		return -ENOTCONN;
	}

	ABORT_IF_EX(header().m_tag != 0 && header().packetType() != Teul::Tokens::Unencrypted
					, "tried to re-send a Teul::Packet");

	static_assert( sizeof(header().m_tag) == sizeof(endpoint.m_impl->sentTag), "Endpoint and Packet disagree on tag size" );

	header().m_tag = ++endpoint.m_impl->sentTag;

#if platBuild == BUILD_CLIENT
	teulSyncClock();
	header().m_tick = (Teul::PacketHeader::tick_t)teulClock64;
#else
	// Send the client back it's tick plus the amount of time elapsed
	// since that tick was received, by which the client can deduce
	// it's own ping time.
	header().m_tick = endpoint.m_impl->m_lastPacketTick + (Teul::PacketHeader::tick_t)(teulClock64 - endpoint.m_impl->m_lastTickRx);
#endif

	// The packet size, as far as* teulkit* is concerned ...
	// doesn't include the prefix.
	const size_t psb = packetSizeBytes();
	const size_t payloadBytes = psb - UnencryptedBytes;
	header().packetSizeBytes(payloadBytes);
	ABORT_IF(header().packetSizeBytes() != payloadBytes);
	m_sent = 0;

	// Encrypt the data.
	if ( header().packetType() != Teul::Tokens::Unencrypted )
	{
		endpoint.encrypt(encryptable(), header().packetSizeBytes());
	}

	const SINT32 result = endpoint.sendPacket(*this);
	if ( PN_UNLIKELY(result <= 0) )
	{
		int err = 0;
		// Connection closed.
		if ( result == 0 )
		{
			WriteLog(LOG_TEUL, LOG_RECOVER, "t:%d sock:" PRF_SOCKET " closed during send err:%d", endpoint.teulIndex(), PRA_SOCKET(endpoint.socket), dbugGetErrno());
		}
		else
		{
			err = dbugGetErrno();
			// Failed to send.
			WriteLog(LOG_TEUL, LOG_WARN, "t:%d sock:" PRF_SOCKET " err:%d(%d) send failed ", endpoint.teulIndex(), PRA_SOCKET(endpoint.socket), result, err);
		}
		teulClose(endpoint, Teul::Closing::SendFail);
		return err;
	}

	return result;
}

//////////////////////////////////////////////////////////////////////

SINT32
teulModSendCALLBACK(SINT32 teulIndex, TEUL_CALLBACK& object)
{
	ENDPOINT* const theEndpoint = teulGetEndpoint(teulIndex);
	if (PN_UNLIKELY(theEndpoint == nullptr))
	{
		WriteLog(LOG_TEUL, LOG_ABORT, "t:%d " PRF_CALLBACK " invalid endpoint in send callback", teulIndex, PRA_CALLBACK(object));
		return -EINVAL;
	}

	return teulModSendCALLBACKe(*theEndpoint, object);
}

//////////////////////////////////////////////////////////////////////

SINT32
teulModSendCallbackTo(SINT32 teulIndex, UINT32 address, UINT16 port, TEUL_CALLBACK& object)
{
	ENDPOINT* const theEndpoint = teulGetEndpoint(teulIndex);
	if ( PN_UNLIKELY(theEndpoint == nullptr) )
	{
		WriteLog(LOG_TEUL, LOG_ABORT, "t:%d null endpoint", teulIndex);
		return -EINVAL;
	}

	// Temporarily save the dest address.
	sockaddr_in savedDestAddress = theEndpoint->destSockAddress;
	theEndpoint->destSockAddress.sin_addr.s_addr = htonl( address );
	if ( port == 0 )
		theEndpoint->destSockAddress.sin_port = theEndpoint->localSockAddress.sin_port;
	else
		theEndpoint->destSockAddress.sin_port = htons(port);

	// Make sure we populate the local network address components if
	// we appear to be sending to a broadcast address.
	static const UINT32 netMask = 0x00ffffff;
	if ( (theEndpoint->destSockAddress.sin_addr.s_addr & netMask) == 0 )
		theEndpoint->destSockAddress.sin_addr.s_addr |= (theEndpoint->localSockAddress.sin_addr.s_addr & netMask);
	if ( (theEndpoint->destSockAddress.sin_addr.s_addr & ~netMask) == 0)
		theEndpoint->destSockAddress.sin_addr.s_addr |= (theEndpoint->localSockAddress.sin_addr.s_addr & ~netMask);

	const SINT32 retval = teulModSendCALLBACKe(*theEndpoint, object);

	theEndpoint->destSockAddress = savedDestAddress;

	return retval;
}
