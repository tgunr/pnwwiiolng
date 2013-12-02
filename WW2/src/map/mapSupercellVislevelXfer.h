#pragma once
// Created 2011/9/20 13:24:01 CDT
// Copyright (C) PlayNet Inc 2011 -- All rights reserved.
// Not for redistribution except as part of an API.
//
// Originating Programmer: Oliver "kfsone" Smith

#pragma pack(push, 1)
//! @class SupercellVislevelXfer
//! @brief Describes the vis levels for all buildings in a supercell.
class SupercellVislevelXfer
{
	public: typedef UCHAR vis_t;					//!< What size value we group vislevel bits in.
	public:	static const size_t BitsPerObject = 4;	// How many bits each object needs.

			//! Size of this entry (scx/scy) in bytes.
	public: size_t size_bytes() const { return m_size_bytes; }
			//! Supercell x-coordinate for this entry.
	public: SINT16 scx() const { return m_scx; }
			//! Supercell y-coordinate for this entry.
	public: SINT16 scy() const { return m_scy; }

			//! Extract the vis level for a building from our data.
	public:	vis_t getBuildingLevel(const UINT32 bid) const
			{
				const size_t shift = getBitPositionInFlag(bid);
				const vis_t maskValue = BitMask << shift;
				const vis_t* const ptr = getBuildingFlagPtr(bid);
				vis_t visValue = (*ptr & maskValue) >> shift;
				// Handle sign.
				if ( visValue & (1 << (BitsPerObject - 1)) )
					visValue |= (~0U << BitsPerObject);
				return visValue;
			}

	private:	void* operator new(size_t);	// Do not use "new" on this object.

	//////////////////////////////////////////////////////////////////////
	// Member variables.

	protected:	UINT16		m_size_bytes;	//!< Number of bytes this segment is.
	protected:	SINT16		m_scx;			//!< X of the supercell.
	protected:	SINT16		m_scy;			//!< Y of the supercell.
	protected:	vis_t		m_data;

	//////////////////////////////////////////////////////////////////////
	// Internal constants.

	public: static const size_t SizeBits = sizeof(UINT16) * 8;
	public:	static const size_t SCXYBits = 2 * sizeof(SINT16) * 8;	//	Size of scx and scy.
	public: static const size_t HeaderBits = SizeBits + SCXYBits;
	public:	static const size_t FlagBits = sizeof(vis_t) * 8;
	public:	static const vis_t BitMask = (vis_t)((1 << BitsPerObject) - 1);

	//////////////////////////////////////////////////////////////////////
	// Internal functions, they are public purely so that the Unit Tests
	// are able to access them directly.

			//! Calculate the index of the vis_t a given building is in.
			//! i.e. vis_t container = m_data[getBuildingBastOffset(buildingId)];
	public:	static size_t getBuildingBaseOffset(const UINT32 bid)
			{
				return bid * BitsPerObject / FlagBits;
			}

			//! Return a const pointer to the vis_t containing a given building.
	public:	const vis_t* getBuildingFlagPtr(const UINT32 bid) const
			{
				return (&m_data) + getBuildingBaseOffset(bid);
			}

			//! Return a pointer to the vis_t containing a given building.
	public:	vis_t* getBuildingFlagPtr(const UINT32 bid)
			{
				return (&m_data) + getBuildingBaseOffset(bid);
			}

			//! Calculate the start bit-position within a vis_t that a given building occupies.
	public:	static size_t getBitPositionInFlag(const UINT32 bid)
			{
				return (bid * BitsPerObject) % FlagBits;
			}

};
#pragma pack(pop)

