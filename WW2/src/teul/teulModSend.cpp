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

#if platOS == OS_LINUX
# define SOCK_SEND_FLAGS (MSG_DONTWAIT|MSG_NOSIGNAL)
# include <sys/socket.h>
#else
# define SOCK_SEND_FLAGS 0
#endif

// Maximum send buffer of 256Kb.
static const SINT32 MaxSendBufferSize = 256 * 1024 ;	// 256kb

//////////////////////////////////////////////////////////////////////

SINT32
teulInitSend(void)
{
	totalPacketsSent = 0;
	totalPacketsRecv = 0;
	totalPacketsXfer = 0;

	return 0;
}

//////////////////////////////////////////////////////////////////////
// Pack the "outdata" for a callback and send it to the specified
// endpoint destination.

SINT32
teulModSendCALLBACKe(ENDPOINT& endpoint, TEUL_CALLBACK& callback)
{
	if( endpoint.state() == IS_DEAD )
	{
		dbugLog(_FLA,"endpoint not open state = %d, index %d callback %d:%d ", endpoint.state(), endpoint.teulIndex(), callback.callX(), callback.callY()) ;
		return -ETEUL_CB_CONN_DEAD;
	}
	if ( callback.m_sendAction == nullptr)
	{
		dbugLog(_FLA, "cant execute NULL callback x:%d y:%d", callback.callX(), callback.callY());
		return -ETEUL_CB_SEND_NULL;
	}

	// All packers need an ACTION_OBJECT, and we
	ACTION_OBJECT actionObj;
	actionObj.teulIndex = endpoint.teulIndex();
	actionObj.endpoint = &endpoint;
	actionObj.actionObj = &callback;
	actionObj.localRawData = callback.outData;

	Teul::Packet outgoing(Teul::MaxPacket, callback.callX(), callback.callY()) ;

	actionObj.packed = (UCHAR*)outgoing.writePointer();
	actionObj.offset = 0;
	actionObj.m_size_bytes = -1 ;

	const SINT32 packedSizeBits = (callback.m_sendAction)(&actionObj) ;
	if ( packedSizeBits < 0 )
	{
		dbugLog(_FLA, "t:%d cb:%u,%u pack failed:%d probable data corruption", endpoint.teulIndex(), callback.callX(), callback.callY(), packedSizeBits);
		return packedSizeBits ;
	}

	outgoing.setPayloadSizeBytes(Teul::BitsToBytes(packedSizeBits)) ;

	const SINT32 sendResult = outgoing.send(endpoint) ;
	if ( sendResult < 0 )
	{
		dbugLog(_FLW, "t:%d cb:%u,%u send failed:%d", endpoint.teulIndex(), callback.callX(), callback.callY(), sendResult);
		return sendResult ;
	}

	return packedSizeBits ;
}

//////////////////////////////////////////////////////////////////////
// Actually write data to a socket.

SINT32
ENDPOINT::writeData(void* const data, const size_t sizeInBytes, const UINT32 address, const UINT32 port)
{
	if ( protocol == PROTOCOL_TCP || protocol == PROTOCOL_CTRL )
	{
		return ::send(socket, (const char*)data, sizeInBytes, SOCK_SEND_FLAGS) ;
	}

	// Apply the default port.
	const UINT16 targetPort = port ? (UINT16) port : (UINT16) WWII::Net::Ports::PrivateBcast ;

	sockaddr_in inetAddress = destSockAddress ;
	if ( address != 0 )
	{
		inetAddress.sin_family = AF_INET;
		inetAddress.sin_addr.s_addr = htonl( address );
		inetAddress.sin_port = htons( targetPort ) ;
	}

	const SINT32 result = ::sendto(socket, (const char*)data, sizeInBytes, 0, (struct sockaddr*)&inetAddress, sizeof(inetAddress)) ;
	if ( result <= 0 )
	{
		// Socket closed.
		if ( result == 0 )
		{
			dbugLog(_FLW, "t:%d sock:%u socket closed on sendto for %" PRSizeT " bytes to %u,%hu [%u,%hu] attempt", teulIndex(), socket, sizeInBytes, address, port, inetAddress.sin_addr.s_addr, inetAddress.sin_port) ;
			teulClose(*this, Teul::Closing::Closed) ;
			dbugSetErrno(Teul::Closing::Closed) ;
			return -Teul::Closing::Closed ;
		}

		// Check for an interrupted send attempt.
		const int err = dbugGetErrno() ;
		if ( (err != EAGAIN) & (err != EINTR) & (err != EWOULDBLOCK) )
		{
			dbugLog(_FLW, "t:%d sock:%u send error on send %" PRSizeT " bytes to %u,%hu [%u,%hu] attempt, err:%d", teulIndex(), socket, sizeInBytes, address, port, inetAddress.sin_addr.s_addr, inetAddress.sin_port, err) ;
			return -ETEUL_ENOSPC_SEND_SOCKET ;
		}

		return 0 ;
	}

	lastDataSent.makeCurrent() ;

	return result ;
}

//////////////////////////////////////////////////////////////////////
// Try to send or queue a packet to a socket.

SINT32
ENDPOINT::sendPacket(Teul::Packet& packet)
{
	const size_t packetBytes = packet.packetSizeBytes() ;

	// If there's no send queue, try and send the data directly.
	if ( m_impl->m_sendQueue.empty() == true )
	{
		const SINT32 result = writeData(packet.packet(), packetBytes, packet.address(), packet.port()) ;
		// Anything but a partial send, the sender handles.
		if ( result < 0 )
		{
			// If the error was the result of an interruption, then lets just queue the packet
			// and give it a retry later on.
			const SINT32 err = dbugGetErrno() ;
			if ( (err != EAGAIN) & (err != EINTR) & (err != EWOULDBLOCK) )
			{
				return result ;
			}
		}
		if ( result > 0 )
		{
			const size_t sent = (size_t) result ;
			packet.sentBytes(sent) ;
			if ( packet.sentBytes() >= packetBytes )
				return packetBytes ;
		}
		// Partial or blocked send falls thru and queues the remainder.
	}

	Teul::Packet* const copy = new Teul::Packet(0, 0, 0) ;
	copy->stealContentsFrom(packet) ;

	m_impl->m_sendQueue.push_back(copy) ;

	setWritesPending(true) ;

	return packetBytes ;
}

//////////////////////////////////////////////////////////////////////
// Try to drain the send queue of a socket.

SINT32
ENDPOINT::pushSendQueue()
{
	if ( m_writesPending == true )
		return 0 ;

	while ( m_inUse == true && m_closed == false && m_impl->m_sendQueue.empty() == false )
	{
		Teul::Packet* packet = m_impl->m_sendQueue.front() ;
		const size_t sentBytes = packet->sentBytes() ;
		const size_t remainingBytes = packet->packetSizeBytes() - sentBytes ;
		void* data = (char*)packet->packet() + sentBytes ;
		const SINT32 result = writeData(data, remainingBytes, packet->address(), packet->port()) ;
		// Result will be less than sendbytes if it is an error (negative),
		// close (0) or smaller number of bytes.
		if ( result <= 0 )
		{
			setWritesPending(true) ;
			return result ;
		}
		packet->sentBytes((size_t)result) ;
		if ( packet->sentBytes() >= packet->packetSizeBytes() )
		{
			// We appear to have sent the data.
			delete packet ;
			m_impl->m_sendQueue.erase(m_impl->m_sendQueue.begin()) ;
		}
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////
//

SINT32
Teul::Packet::send(ENDPOINT& endpoint)
{
	if ( endpoint.state() == IS_LISTENING || endpoint.state() >= IS_CLOSING )
	{
		dbugLog(_FLA, "t:%d tried to send to non-open endpoint [st:%d]", endpoint.teulIndex(), endpoint.state()) ;
		return -ENOTCONN ;
	}

	ABORT_IF_EX(header().m_tag != 0 && header().packetType() != Teul::Tokens::Unencrypted
					, "tried to re-send a Teul::Packet");

#if !defined(__APPLE__)
	static_assert( sizeof(header().m_tag) == sizeof(endpoint.m_impl->sentTag), "Endpoint and Packet disagree on tag size" ) ;
#endif

	header().m_tag = ++endpoint.m_impl->sentTag ;

#if !defined(AS_SERVER)
	teulSyncClock() ;
	header().m_tick = (Teul::PacketHeader::tick_t)teulClock64 ;
#else
	// Send the client back it's tick plus the amount of time elapsed
	// since that tick was received, by which the client can deduce
	// it's own ping time.
	header().m_tick = endpoint.m_impl->m_lastPacketTick + (Teul::PacketHeader::tick_t)(teulClock64 - endpoint.m_impl->m_lastTickRx) ;
#endif

	// The packet size, as far as* teulkit* is concerned ...
	// doesn't include the prefix.
	const size_t psb = packetSizeBytes() ;
	header().packetSizeBytes(psb - UnencryptedBytes) ;
	m_sent = 0 ;

	// Encrypt the data.
	if ( header().packetType() != Teul::Tokens::Unencrypted )
	{
		endpoint.encrypt(encryptable(), header().packetSizeBytes()) ;
	}

	const SINT32 result = endpoint.sendPacket(*this) ;
	if ( result <= 0 )
	{
		int err = 0 ;
		// Connection closed.
		if ( result == 0 )
		{
			dbugLog(_FLM, "t:%d sock:%d closed during send err:%d", endpoint.teulIndex(), endpoint.socket, dbugGetErrno()) ;
		}
		else
		{
			err = dbugGetErrno() ;
			// Failed to send.
			dbugLog(_FLW, "t:%d sock:%u err:%d(%d) send failed ", endpoint.teulIndex(), endpoint.socket, result, err) ;
		}
		teulClose(endpoint, Teul::Closing::SendFail) ;
		return err ;
	}

	return result ;
}

//////////////////////////////////////////////////////////////////////

SINT32
teulModSendCALLBACK(SINT32 teulIndex, TEUL_CALLBACK& object)
{
	ENDPOINT* const theEndpoint = teulGetEndpoint(teulIndex) ;
	if ( theEndpoint == NULL)
	{
		dbugLog(_FLA, "t:%d x:%d y:%d invalid endpoint in send callback", teulIndex, object.callX(), object.callY()) ;
		return -EINVAL;
	}

	return teulModSendCALLBACKe(*theEndpoint, object);
}

//////////////////////////////////////////////////////////////////////

SINT32
teulModSendCallbackTo(SINT32 teulIndex, UINT32 address, UINT16 port, TEUL_CALLBACK& object)
{
	ENDPOINT* const theEndpoint = teulGetEndpoint(teulIndex) ;
	if ( theEndpoint == NULL )
	{
		dbugLog(_FLA, "t:%d null endpoint", teulIndex);
		return -EINVAL;
	}

	// Temporarily save the dest address.
	sockaddr_in savedDestAddress = theEndpoint->destSockAddress ;
	theEndpoint->destSockAddress.sin_addr.s_addr = htonl( address );
	if ( port == 0 )
		theEndpoint->destSockAddress.sin_port = theEndpoint->localSockAddress.sin_port ;
	else
		theEndpoint->destSockAddress.sin_port = htons(port);

	// Make sure we populate the local network address components if
	// we appear to be sending to a broadcast address.
	static const UINT32 netMask = 0x00ffffff ;
	if ( (theEndpoint->destSockAddress.sin_addr.s_addr & netMask) == 0 )
		theEndpoint->destSockAddress.sin_addr.s_addr |= (theEndpoint->localSockAddress.sin_addr.s_addr & netMask) ;
	if ( (theEndpoint->destSockAddress.sin_addr.s_addr & ~netMask) == 0)
		theEndpoint->destSockAddress.sin_addr.s_addr |= (theEndpoint->localSockAddress.sin_addr.s_addr & ~netMask);

	const SINT32 retval = teulModSendCALLBACKe(*theEndpoint, object);

	theEndpoint->destSockAddress = savedDestAddress ;

	return retval;
}

//////////////////////////////////////////////////////////////////////

SINT32
teulModResendPACKET(ENDPOINT* const /* inEndpoint */, PACKET_HEAD* const /* packet */)
{
	LogAbort("retransmit does not work.");
}

