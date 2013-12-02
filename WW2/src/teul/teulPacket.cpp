#include <syscfg.h>	// Path is project dependent.

#include "teul/teulCallbackStruct.h"	// For TEUL_CALLBACK details.
#include "teul/teulEndpoint.h"			// For struct ENDPOINT.
#include "teul/teulPacket.h"		// Main header.
#include "teul/teulPacketHeader.h"		// For Teul::PacketHeader.

//////////////////////////////////////////////////////////////////////
// Constructor for a Teul::Packet

Teul::Packet::Packet(const size_t payloadCapacityBytes_, const UCHAR callX_, const UCHAR callY_)
	: m_data(m_localData)
	, m_packetSizeBytes(headerSizeBytes())
	, m_capacity(DefaultLocalDataSize)
	, m_address(0)
	, m_port(0)
{
	header().packetType(Teul::Tokens::Encrypted);
	header().packetSizeBytes(headerSizeBytes());
	header().m_tag = 0;
	header().m_callX = callX_;
	header().m_callY = callY_;

	// Now ensure data is big enough to hold the payload.
	reserve(payloadCapacityBytes_);
}

//////////////////////////////////////////////////////////////////////
// Copy constructor.

Teul::Packet::Packet(const Teul::Packet& rhs)
	: m_data(m_localData)
	, m_packetSizeBytes(0)
	, m_capacity(DefaultLocalDataSize)
	, m_address(rhs.m_address)
	, m_port(rhs.m_port)
{
	const size_t newSize = rhs.packetSizeBytes();
	setPacketSizeBytes(newSize);
	memcpy(m_data, rhs.m_data, newSize);
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
}

//////////////////////////////////////////////////////////////////////
// Grow packet to accommodate and copy additional bytes.

void Teul::Packet::append(const void* const src_, const size_t bytes_)
{
	setPacketCapacityBytes(packetSizeBytes() + bytes_);
	memcpy(m_data + packetSizeBytes(), src_, bytes_);
	m_packetSizeBytes += bytes_;
}

//////////////////////////////////////////////////////////////////////
// Return the size of the packet header.

size_t
Teul::Packet::headerSizeBytes()
{
	return sizeof(Teul::PacketHeader);
}

//////////////////////////////////////////////////////////////////////
// Copy operator.

Teul::Packet&
Teul::Packet::operator=(const Teul::Packet& rhs)
{
	// We can't just "borrow" the packet data, we have to duplicate it.
	const size_t dataSize = rhs.packetSizeBytes();
	this->setPacketSizeBytes(dataSize);
	memcpy(this->m_data, rhs.m_data, dataSize);
	m_address = rhs.m_address;
	m_port = rhs.m_port;
	m_sent = 0;

	return *this;
}

//////////////////////////////////////////////////////////////////////
// Adjust the current size of the packet (and resultingly the write
// position if we are writing).

void
Teul::Packet::setPacketSizeBytes(const size_t sizeBytes_)
{
	if ( sizeBytes_ > m_capacity )
		setPacketCapacityBytes(sizeBytes_);

	m_packetSizeBytes = sizeBytes_;
}

//////////////////////////////////////////////////////////////////////
// Adjust the current packet capacity.

void
Teul::Packet::setPacketCapacityBytes(const size_t newCapacityBytes_)
{
	if ( newCapacityBytes_ > Teul::MaxAllowedPacketSize_bytes )
		throw std::invalid_argument("Illegal packet size");

	// No growth required.
	if ( newCapacityBytes_ <= m_capacity )
		return;

	// Increment capacity in fixed-size chunks until we reach
	// a size that accommodates the desired packet size.
	while ( m_capacity < 4096 && m_capacity < newCapacityBytes_ )
		m_capacity += GrowthRate;
	while ( m_capacity < newCapacityBytes_ )
		m_capacity += 4096;

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
		// area of memory, sue realloc to get a larger space.
		UCHAR* newData = (UCHAR*)memRealloc(m_data, m_capacity);
		if ( newData != NULL )
			m_data = newData;
	}
}

//////////////////////////////////////////////////////////////////////
// Send ready-to-go data to a given teulIndex (looks up endpoint).

SINT32
Teul::Packet::send(const SINT32 teulIndex_)
{
	ENDPOINT* endpoint = teulGetEndpoint(teulIndex_);
	if ( endpoint == NULL )
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
	if ( endpoint == NULL )
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
	if ( header().packetType() != Teul::Tokens::Encrypted && header().packetType() != Teul::Tokens::Unencrypted )
	{
		dbugLog(_FLW, "t:%d invalid packet header [%u]", endpoint_.teulIndex(), header().packetType());
		return -EINVAL;
	}

	m_packetSizeBytes = header().packetSizeBytes();

	if ( packetSizeBytes() < headerSizeBytes() )
	{
		dbugLog(_FLW, "t:%d malformed packet header", endpoint_.teulIndex());
		return -EINVAL;
	}

	TEUL_CALLBACK* const callback = TEUL_CALLBACK::Lookup(header().m_callX, header().m_callY);
	if ( callback == NULL )
	{
		dbugLog(_FLM, "t:%d cbx:%u cby:%u no such callback", endpoint_.teulIndex(), header().m_callX, header().m_callY);
		return -EINVAL;
	}

	return packetSizeBytes();
}

//////////////////////////////////////////////////////////////////////
// Hide the "size" field of a stream.

#if defined TEUL_CRYPT_VERSION_4
size_t
Teul::PacketHeader::maskSizeValue(UINT32 cryptVersion, const UINT64& seed, size_t size)
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
// Steal the contents of a teul packet.

void
Teul::Packet::stealContentsFrom(Teul::Packet& rhs)
{
	Teul::Packet& lhs = *this;	// Convenience.

	// If the source is using it's local data buffer,
	// just copy the data over
	if ( rhs.m_data == rhs.m_localData )
	{
		// Data fits within the local data, so just copy that.
		lhs.m_data = lhs.m_localData;
		memcpy(lhs.m_data, rhs.m_localData, rhs.m_packetSizeBytes);
	}
	else
	{
		lhs.m_data = rhs.m_data;		// Take posession of the data,
		rhs.m_data = rhs.m_localData;	// point rhs back at its local data.
	}

	// Steal the size counts.
	lhs.m_packetSizeBytes = rhs.m_packetSizeBytes;
	lhs.m_capacity = rhs.m_capacity;
	lhs.m_sent = rhs.m_sent;
	lhs.m_address = rhs.m_address;
	lhs.m_port = rhs.m_port;

	rhs.m_packetSizeBytes = 0;
	rhs.m_capacity = 0;
	rhs.m_sent = 0;
}

//////////////////////////////////////////////////////////////////////
// Broadcast a packet to an array of endpoints.

typedef std::vector<ENDPOINT*> ENDPOINTS;

void
Teul::Packet::send(const ENDPOINTS& endpoints)
{
	if ( wantsEncryption() )
	{
		// Need to send copies.
		for ( ENDPOINTS::const_iterator it = endpoints.begin(); it != endpoints.end(); ++it )
		{
			ENDPOINT* const endpoint = *it;
			if ( endpoint != NULL )
			{
				Teul::Packet copy(*this);
				copy.send(*endpoint);
			}
		}
	}
	else
	{
		// Not encrypted, we can just broadcast.
		for ( ENDPOINTS::const_iterator it = endpoints.begin(); it != endpoints.end(); ++it )
		{
			ENDPOINT* const endpoint = *it;
			if ( endpoint != NULL )
				send(*endpoint);
		}
	}
}

