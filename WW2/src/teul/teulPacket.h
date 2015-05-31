#pragma once
// Copyright (C) PlayNet Inc 2011
// Originating Programmer: Oliver "kfs1" Smith
//
// Teul::Packet class.

namespace Teul
{

struct PacketHeader;

//! @class Teul::Packet
//! @brief Provides a minimal-copy packet buffer for header & payload data.
//! The Packet structure combines various TeulKit structures along with
//! payload capacity to provide a minimal-copy mechanism for combining
//! data into a send-ready format.
//!
//! Packet instances are constructed with enough space for a 64-byte packet
//! including TeulKit header overhead (about 51 bytes of payload). If more
//! space is required, a memory allocation is used instead.
//!
//! Users are intended to write their data directly into the payload()
//! area; in ideal conditions you should know in advance how much space
//! your payload will require.
//!
//! If the connection to which the data is sent requires encryption,
//! the data will be encrypted in-place immediately prior to sending,
//! again avoiding a copy operation.

struct PacketId
{
	uint8_t	m_x, m_y;

	template<typename T1, typename T2>
	PacketId(T1 callX, T2 callY) noexcept : m_x((uint8_t)callX), m_y((uint8_t)callY) {}
};

class Packet
{
public:
	//! Create a packet for a specific callback.
	Packet(PacketId id);
	//! Default/receive constructor.
	Packet();
#if defined(ALLOW_TEUL_PACKET_MOVE)
	//! Move constructor
	Packet(Packet&& moveRef);
#else
	Packet(Packet&& moveRef) = delete;
	Packet& operator = (Packet&& rhs) = delete;
#endif

	//! Construct and populate with a copy of an existing object.
	template<typename T>
	Packet(PacketId id, T src)
		: Packet(id)
	{
		new ((T*)allocateBytes(sizeof(T))) T(src);
	}

	//! Construct and populate from a pre-existing data source.
	Packet(PacketId id, const void* source, UINT32 size)
		: Packet(id)
	{
		append(source, size);
	}

	//! Copy constructor.
	Packet(const Packet& rhs);

	//! Copy operator.
	Packet& operator = (const Packet& rhs);

	//! Destructor.
	~Packet();

public:
	enum { DefaultLocalDataSize = 256 };
	enum { GrowthRate = 256 };
	enum { UnencryptedBytes = 3 };	// Start token and packet size unencrypted.

private:
	//! Data for this packet.
	UCHAR*			m_data;
	//! The size of the whole thing.
	UINT32			m_packetSizeBytes;
	//! Current capacity of the packet.
	UINT32			m_capacity;
	//! Byte sent.
	UINT32			m_sent;
	//! Destination address for UDP.
	UINT32			m_address;
	//! Destination port for UDP.
	UINT32			m_port;
	//! Default local buffer of X bytes.
	UCHAR			m_localData[DefaultLocalDataSize];

public:
	//! Is there any data in the packet?
	bool empty() const { return payloadSizeBytes() <= 0; }

	//! Clear out contents of packet for re-population/work.
	void reset();

	//! Encrypt and send data to given teulIndex.
	SINT32 send(const SINT32 teulIndex_);
	//! Pack, encrypt and send data to given endpoint.
	SINT32 send(ENDPOINT& endpoint_);

	//! Broadcast this packet to an array of endpoints.
	void send(const std::vector<ENDPOINT*>& endpoints);

	//! Encrypt and send data to specified host via given teulIndex.
	SINT32 sendTo(const SINT32 teulIndex, const UINT32 destAddress_, const UINT16 destPort_=0);
	//! Encrypt and send data to specified host over given endpoint.
	SINT32 sendTo(ENDPOINT& endpoint_, const UINT32 destAddress_, const UINT16 destPort_=0);

	//! Receive a packet, decrypt it, etc.
	SINT32 receive(ENDPOINT& endpoint_);

	//! Disable encryption on this packet (e.g. for broadcasting).
	void setUnencrypted();

	//! Determine whether this is encrypted or not.
	bool wantsEncryption();

	//! Increase the total packet allocation.
	//! @note We don't track the amount used.
	void setPacketSizeBytes(const UINT32 sizeBytes_);

	//! Increase the total packet allocation to accommodate the given payload size.
	//! @note We don't track the amount used.
	void setPayloadSizeBytes(const UINT32 sizeBytes_) { setPacketSizeBytes(sizeBytes_ + headerSizeBytes()); }

	//! Increase the PAYLOAD capacity. Has no effect if sizeBytes_ is less than current capacity).
	void reserve(const UINT32 payloadSizeBytes_) { setPayloadCapacityBytes(payloadSizeBytes_); }

	//! Increase the payload capacity size.
	void setPayloadCapacityBytes(const UINT32 payloadSizeBytes_) { setPacketCapacityBytes(payloadSizeBytes_ + headerSizeBytes()); }

	//! Increase the overall capacity.
	void setPacketCapacityBytes(const UINT32 packetSizeBytes_);

	//! Mark additional payload space as used.
	void increaseUsedPayloadBytes(const UINT32 bytes_) { m_packetSizeBytes += bytes_; }
	void increaseUsedPayloadBits(const UINT32 bits_) { increaseUsedPayloadBytes((bits_+7)/8); }

	//! Allocate bytes and return a pointer to the start of those bytes.
	//! @caution This pointer is potentially invalidated by any call
	//! that changes capacity and causes a reallocation. Use with care.
	public:
	void* allocateBytes(UINT32 growth_bytes)
	{
		const UINT32 oldSize_bytes = packetSizeBytes();
		reserve(oldSize_bytes + growth_bytes);	// May cause reallocation.
		m_packetSizeBytes += growth_bytes;		// Change the end-of-data position.
		return (char*)m_data + oldSize_bytes;	// Calculate pointer here because of potential realloc.
	}

	//! Allocate space for an instance of Type_ and return a pointer to the start of those bytes.
	//! @caution This pointer is potentially invalidated by any call
	//! that changes capacity and causes a reallocation. Use with care.
	template<typename Type_>
	Type_* allocatePtr(UINT32 numEntries=1) { return (Type_*)allocateBytes(sizeof(Type_) * numEntries); }

	//! Assign space for and return a pointer to a
	//! blank instance of a particular type.
	//! They are not initialized.
	//! @caution This reference is potentially invalidated by any call
	//! that changes capacity and causes a reallocation. Use with care.
	public:
	template<typename Type_>
	Type_& allocate() { return *allocatePtr<Type_>(); }

	template<typename Type_, typename... Args>
	Type_& construct(Args&&... args)
	{
		Type_* obj = allocatePtr<Type_>();
		new (obj) Type_(std::forward<Args>(args)...);
		return *obj;
	}

	// Please don't use pointers.
	template<typename _Type> void append(const _Type* data_) = delete;
	template<typename _Type> void append(_Type* data_) = delete;

	//! Add a piece of data to the end of the packet.
	public:
	template<typename _Type>
	void append(const _Type& data_)
	{
		append(&data_, sizeof(data_));
	}

	//! Add a given number of bytes from a given source.
	void append(const void* const src_, const UINT32 bytes_);

public:
	//! Retrieve a const type-cast pointer to the payload portion of the packet.
	//! @caution This pointer is potentially invalidated by any call
	//! that changes capacity and causes a reallocation. Use with care.
	const void* payload() const { assert(payloadSizeBytes() > 0); return (m_data + headerSizeBytes()); }
	//! Retrieve a type-cast pointer to the payload portion of the packet.
	//! @caution This pointer is potentially invalidated by any call
	//! that changes capacity and causes a reallocation. Use with care.
	void* payload() { assert(payloadSizeBytes() > 0); return (m_data + headerSizeBytes()); }

	//! Get the current write pointers (end of current payload).
	//! @caution This pointer is potentially invalidated by any call
	//! that changes capacity and causes a reallocation. Use with care.
	void* writePointer() { return m_data + packetSizeBytes(); }

	template<typename _Type>
	_Type& payloadRef(UINT32 offset_=0) { return *(_Type*)(m_data + headerSizeBytes() + offset_); }

	//! Retrieve a reference to the header portion of the packet.
	//! @caution This pointer is potentially invalidated by any call
	//! that changes capacity and causes a reallocation. Use with care.
	const PacketHeader& header() const { return *(const PacketHeader*)(m_data); }
	PacketHeader& header() { return *(PacketHeader*)(m_data); }

	//! Pointer to the whole packet.
	//! @caution This pointer is potentially invalidated by any call
	//! that changes capacity and causes a reallocation. Use with care.
	const void* packet() const { return m_data; }
	void* packet() { return m_data; }

	//! Pointer to the first part of the packet that should be encrypted.
	//! @note This points to a part of the header.
	//! @caution This pointer is potentially invalidated by any call
	//! that changes capacity and causes a reallocation. Use with care.
	UCHAR* encryptable() { return m_data + UnencryptedBytes; }

	//! Get the size of the envelope section of the header
	static UINT32 envelopeSizeBytes() { return headerSizeBytes() - UnencryptedBytes; }
	//! Get the size of the envelope section of the header
	static UINT32 envelopeSizeBits() { return envelopeSizeBytes() * 8; }

	//! Get the size of the entire packet header (in bytes).
	static UINT32 headerSizeBytes();
	//! Get the size of the payload portion of the packet (in bytes).
	UINT32 payloadSizeBytes() const { return packetSizeBytes() - headerSizeBytes(); }
	//! Get the size of the whole packet (in bytes).
	UINT32 packetSizeBytes() const { return m_packetSizeBytes; }

	//! Get the size of the header portion of the packet (in bits).
	//! @note We don't track the amount used.
	static UINT32 headerSizeBits() { return headerSizeBytes() * 8; }
	//! Get the size of the payload portion of the packet (in bits).
	//! @note We don't track the amount used.
	UINT32 payloadSizeBits() const { return payloadSizeBytes() * 8; }
	//! Get the size of the whole packet (in bits).
	//! @note We don't track the amount used.
	UINT32 packetSizeBits() const { return packetSizeBytes() * 8; }

	//! Get the current capacity.
	UINT32 packetCapacityBytes() const { return m_capacity; }

	//! Get the current payload capacity.
	UINT32 payloadCapacityBytes() const { return packetCapacityBytes() - headerSizeBytes(); }

	//! Address we are destined for (or zero for connected sockets).
	UINT32 address() const { return m_address; }
	//! Port we are destined for (or zero for connected sockets).
	UINT32 port() const { return m_port; }

	//! Bytes we have sent.
	UINT32 sentBytes() const { return m_sent; }
	//! Increment bytes sent.
	void sentBytes(const UINT32 bytes_) { m_sent += bytes_; }
};

}
