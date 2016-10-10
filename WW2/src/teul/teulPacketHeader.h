#pragma once
// Copyright (C) PlayNet Inc 2011
// Originating Programmer: Oliver "kfs1" Smith
//
// Teul::PacketHeader class.

namespace Teul
{
#pragma pack(push, 1)
	//! @struct Teul::PacketHeader
	//! @brief Wire-format of the packet header.
	struct PacketHeader
	{
	public:
		using tag_t = unsigned char;		// Used for the tag.
		using tick_t = unsigned short;	    // Tick transport.
		using callxy_t = unsigned char;	    // For callback X/Y values.
	public:
		UINT32 packetType() const { return m_prefix[0] >> 4; }
		void packetType(UINT32 type_) { m_prefix[0] = ((UCHAR)type_ << 4) | (m_prefix[0] & 0x0f); }
		UINT32 packetSizeBytes() const { return ((m_prefix[0] & 0x0f) << 16) | (m_prefix[1] << 8) | (m_prefix[2]); }
		void packetSizeBytes(UINT32 size_)
		{
			// preserve the upper 4 bits of [0] which represent the packet type,
			// but completely replace the lower four bits
			m_prefix[0] = (m_prefix[0] & 0xf0) | ((size_ >> 16) & 0x0f);
			m_prefix[1] = (size_ >> 8) & 0xff;
			m_prefix[2] = size_ & 0xff;
		}

		//// Unencrypted
		//! Packet type and size, 3 bytes
		UCHAR		m_prefix[3];
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
