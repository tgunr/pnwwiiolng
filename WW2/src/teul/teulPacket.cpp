#include <syscfg.h>	// Path is project dependent.

#include "mem/mem.h"
#include "teul/teulCallbackStruct.h"	// For TEUL_CALLBACK details.
#include "teul/teulEndpoint.h"			// For struct ENDPOINT.
#include "teul/teulPacket.h"			// Main header.
#include "teul/teulPacketHeader.h"		// For Teul::PacketHeader.

//////////////////////////////////////////////////////////////////////
// Constructor for a Teul::Packet

Teul::Packet::Packet(PacketId id)
	: m_data(m_localData)
	, m_packetSizeBytes(headerSizeBytes())
	, m_capacity(DefaultLocalDataSize)
	, m_sent(0)
	, m_address(0)
	, m_port(0)
{
	header().packetType(Teul::Tokens::Encrypted);
	header().packetSizeBytes(headerSizeBytes());
	header().m_tag = 0;
	header().m_callX = id.m_x;
	header().m_callY = id.m_y;
}

//////////////////////////////////////////////////////////////////////
// Copy constructor.

Teul::Packet::Packet(const Teul::Packet& rhs)
	: m_data(m_localData)
	, m_packetSizeBytes(0)
	, m_capacity(DefaultLocalDataSize)
	, m_sent(0)
	, m_address(rhs.m_address)
	, m_port(rhs.m_port)
{
	const UINT32 newSize = rhs.packetSizeBytes();
	setPacketSizeBytes(newSize);
	memcpy(m_data, rhs.m_data, newSize);
}

//////////////////////////////////////////////////////////////////////
// Move constructor

/*
	The downside to having a move constructor is that we can't resend
	a packet multiple times. Until we have a mechanism that allows us
	to specify whether a packet is copyable or not, I guess we'll have
	to pay the hit.
*/
#ifdef ALLOW_TEUL_PACKET_MOVE
Teul::Packet::Packet(Teul::Packet&& rhs)
{
	if (PN_UNLIKELY(&rhs == this))
		return;

	this->m_data = rhs.m_data;		// Take possession of the data,
	rhs.m_data = rhs.m_localData;	// point rhs back at its local data.

	// If the source is using it's local data buffer, just copy the data over
	if (this->m_data == rhs.m_localData) {
		// Data fits within the local data, so just copy that.
		this->m_data = this->m_localData;
		memcpy(this->m_localData, rhs.m_localData, rhs.m_packetSizeBytes);
	}

	// Steal the size counts.
	this->m_packetSizeBytes = rhs.m_packetSizeBytes;
	this->m_capacity = rhs.m_capacity;
	this->m_sent = rhs.m_sent;
	this->m_address = rhs.m_address;
	this->m_port = rhs.m_port;

	rhs.m_packetSizeBytes = 0;
	rhs.m_capacity = 0;
	rhs.m_sent = 0;
}
#endif

//////////////////////////////////////////////////////////////////////
// Copy operator.

Teul::Packet&
Teul::Packet::operator=(const Teul::Packet& rhs)
{
	// We can't just "borrow" the packet data, we have to duplicate it.
	const UINT32 dataSize = rhs.packetSizeBytes();
	this->setPacketSizeBytes(dataSize);
	memcpy(this->m_data, rhs.m_data, dataSize);
	m_address = rhs.m_address;
	m_port = rhs.m_port;
	m_sent = 0;

	return *this;
}

//////////////////////////////////////////////////////////////////////
// Destructor for Teul::Packet.

Teul::Packet::~Packet()
{
	// Check if the data needs freeing.
	if ( m_data != m_localData )
		memFree(m_data);
}

//////////////////////////////////////////////////////////////////////
// Clear out contents of a packet.

void
Teul::Packet::reset()
{
	header().m_tag = 0;
	m_packetSizeBytes = headerSizeBytes();
	m_address = 0;
	m_port = 0;
	m_sent = 0;
}

//////////////////////////////////////////////////////////////////////
// Grow packet to accommodate and copy additional bytes.

void Teul::Packet::append(const void* const src_, const UINT32 bytes_)
{
	setPacketCapacityBytes(packetSizeBytes() + bytes_);
	memcpy(m_data + packetSizeBytes(), src_, bytes_);
	m_packetSizeBytes += bytes_;
}

//////////////////////////////////////////////////////////////////////
// Return the size of the packet header.

UINT32
Teul::Packet::headerSizeBytes()
{
	return sizeof(Teul::PacketHeader);
}

//////////////////////////////////////////////////////////////////////
// Adjust the current size of the packet (and so the write position
// if we are writing).

void
Teul::Packet::setPacketSizeBytes(const UINT32 sizeBytes_)
{
	if ( sizeBytes_ > m_capacity )
		setPacketCapacityBytes(sizeBytes_);

	m_packetSizeBytes = sizeBytes_;
}

//////////////////////////////////////////////////////////////////////
// Adjust the current packet capacity.

void
Teul::Packet::setPacketCapacityBytes(const UINT32 newCapacityBytes_)
{
	if ( PN_UNLIKELY(newCapacityBytes_ > Teul::MaxAllowedPacketSize_bytes) )
		throw std::invalid_argument("Illegal packet size");

	// No growth required.
	if ( newCapacityBytes_ <= m_capacity )
		return;

	// Increment capacity in fixed-size chunks until we reach
	// a size that accommodates the desired packet size.
	const UINT32 BigGrowthRate = 4096;
	if ( newCapacityBytes_ <= BigGrowthRate )
		m_capacity = ((newCapacityBytes_ - 1) & ~(GrowthRate - 1)) + GrowthRate;
	else
		m_capacity = ((newCapacityBytes_ - 1) & ~(BigGrowthRate - 1)) + BigGrowthRate;

	// Initially, we use the local data buffer for data.
	if ( m_data == m_localData )
	{
		// we've grown too big, alloc some memory.
		m_data = (UCHAR*)memCalloc(1, m_capacity);
		if ( packetSizeBytes() > 0 )
			memcpy(m_data, m_localData, packetSizeBytes());
	}
	else
	{
		// changing the size of an alloc()d
		// area of memory, use realloc to get a larger space.
		UCHAR* newData = (UCHAR*)memRealloc(m_data, m_capacity);
		if ( PN_LIKELY(newData != nullptr) )
			m_data = newData;
	}
}

//////////////////////////////////////////////////////////////////////
// Send ready-to-go data to a given teulIndex (looks up endpoint).

SINT32
Teul::Packet::send(const SINT32 teulIndex_)
{
	ENDPOINT* endpoint = teulGetEndpoint(teulIndex_);
	if ( PN_UNLIKELY(endpoint == nullptr) )
		return -EINVAL;

	return send(*endpoint);
}

//////////////////////////////////////////////////////////////////////
// Broadcast ready-to-go data over a given teulIndex to a specified
// destination IP and port.

SINT32
Teul::Packet::sendTo(const SINT32 teulIndex_, const UINT32 destAddress_, const UINT16 destPort_)
{
	ENDPOINT* const endpoint = teulGetEndpoint(teulIndex_);
	if ( PN_UNLIKELY(endpoint == nullptr) )
		return -EINVAL;

	return sendTo(*endpoint, destAddress_, destPort_);
}

//////////////////////////////////////////////////////////////////////
// Broadcast ready-to-go data over a given endpoint to a specified
// destination IP and port.

SINT32
Teul::Packet::sendTo(ENDPOINT& endpoint_, const UINT32 destAddress_, const UINT16 destPort_)
{
	const UINT32 localAddress = ntohl(endpoint_.localSockAddress.sin_addr.s_addr);
	m_address = destAddress_;
	if ( (destAddress_ & 0xffffff00) == 0 )
		m_address |= (localAddress & 0xffffff00);
	if ( (destAddress_ & 0x000000ff) == 0 )
		m_address |= (localAddress & 0x000000ff);
	m_port = destPort_;

	setUnencrypted();

	return send(endpoint_);
}

//////////////////////////////////////////////////////////////////////
// Populate a packet from a given endpoint.
// NOTE: This function is definitely not ready for prime time.

SINT32
Teul::Packet::receive(ENDPOINT& endpoint_)
{
	if ( PN_UNLIKELY(header().packetType() != Teul::Tokens::Encrypted && header().packetType() != Teul::Tokens::Unencrypted) )
	{
		dbugLog(_FLW, "t:%d invalid packet header [%u]", endpoint_.teulIndex(), header().packetType());
		return -EINVAL;
	}

	m_packetSizeBytes = header().packetSizeBytes();

	if ( PN_UNLIKELY(packetSizeBytes() < headerSizeBytes()) )
	{
		dbugLog(_FLW, "t:%d malformed packet header", endpoint_.teulIndex());
		return -EINVAL;
	}

	TEUL_CALLBACK* const callback = TEUL_CALLBACK::Lookup(header().m_callX, header().m_callY);
	if ( PN_UNLIKELY(callback == nullptr) )
	{
		dbugLog(_FLM, "t:%d cbx:%u cby:%u no such callback", endpoint_.teulIndex(), header().m_callX, header().m_callY);
		return -EINVAL;
	}

	return packetSizeBytes();
}

//////////////////////////////////////////////////////////////////////
// Hide the "size" field of a stream.

#if defined TEUL_CRYPT_VERSION_4
UINT32
Teul::PacketHeader::maskSizeValue(UINT32 cryptVersion, const UINT64& seed, UINT32 size)
{
	if ( cryptVersion >= Teul::CryptVersion3 )
	{
		const UINT16 seedMod = (UINT16)(seed >> (UINT32)(seed & 31));
		size ^= seedMod;
	}

	return size;
}
#endif

//////////////////////////////////////////////////////////////////////
// Disable encryption on a packet.

void
Teul::Packet::setUnencrypted()
{
	header().packetType(Teul::Tokens::Unencrypted);
}

//////////////////////////////////////////////////////////////////////
// Tell me if this packet will be using encryption or not.

bool
Teul::Packet::wantsEncryption()
{
	return (header().packetType() != Teul::Tokens::Unencrypted);
}

//////////////////////////////////////////////////////////////////////
// Broadcast a packet to an array of endpoints.

void
Teul::Packet::send(const std::vector<ENDPOINT*>& endpoints)
{
	if ( wantsEncryption() )
	{
		// Need to send copies.
		std::for_each(endpoints.begin(), endpoints.end(), [=](ENDPOINT* endpoint) { Teul::Packet copy(*this); copy.send(*endpoint); });
	}
	else
	{
		// Not encrypted, we can just broadcast.
		std::for_each(endpoints.begin(), endpoints.end(), [=](ENDPOINT* endpoint) { send(*endpoint); });
	}
}

