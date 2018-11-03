#pragma once
// Copyright (C) PlayNet Inc 2011
// Originating Programmer: Oliver "kfs1" Smith
//
// Teul::PacketHeader class.

namespace Teul
{
    //! 4-bit markers marker used to indicate start of a teul frame.
    enum class PacketToken: uint8_t {
        Encrypted   = 9, // 1001
        Unencrypted = 6, // 0110
    };

    constexpr const size_t PacketPrefixBytes = 3; // 4 bytes of type, 20 bits of size

#pragma pack(push, 1)
	//! @struct Teul::PacketHeader
	//! @brief Wire-format of the packet header.
	struct PacketHeader
	{
	public:
		using tag_t = unsigned char;		// Used for the tag.
		using tick_t = unsigned short;	    // Tick transport.
		using callxy_t = unsigned char;	    // For callback X/Y values.
    
        PacketHeader() = default;
        // Obvious precondition: prefixBuffer is at least PacketPrefixBytes byets long
        PacketHeader(char * const prefixBuffer) : PacketHeader() { ::memcpy(&m_prefix[0], prefixBuffer, sizeof(m_prefix)); }
    
		uint8_t packetType() const { return m_prefix[0] >> 4; }
		void setPacketType(uint8_t type_) { m_prefix[0] = (type_ << 4) | (m_prefix[0] & 0x0fU); }

        PacketToken packetToken() const { return static_cast<PacketToken>(packetType()); }
        void setPacketToken(PacketToken token_) { setPacketType(static_cast<uint8_t>(token_)); }

		UINT32 packetSizeBytes() const { return ((m_prefix[0] & 0x0fU) * 65536U) | (m_prefix[1] * 256U) | (m_prefix[2]); }
		void setPacketSizeBytes(UINT32 size_)
		{
			// preserve the upper 4 bits of [0] which represent the packet type,
			// but completely replace the lower four bits
			m_prefix[0] = (m_prefix[0] & 0xf0) | ((size_ >> 16) & 0x0f);
			m_prefix[1] = (size_ >> 8) & 0xff;
			m_prefix[2] = size_ & 0xff;
		}

		//// Unencrypted
		//! Packet type and size, 3 bytes
		uint8_t		m_prefix[PacketPrefixBytes];
		//// Encrypted
		//! ID of this packet.
		tag_t		m_tag;		// CAUTION: Do not move; teulModRecv depends on it.
		//! Lower 16-bits of tick time when packet was sent
		tick_t		m_tick;

		//! X value of the callback used.
		callxy_t	m_callX;
		//! Y value of the callback used.
		callxy_t	m_callY;
	};
#pragma pack(pop)
}

static_assert(sizeof(Teul::PacketHeader) == 8, "Teul::PacketHeader should be 8 bytes");
static_assert(std::is_standard_layout<Teul::PacketHeader>::value, "Teul::PacketHeader must be PoD");
