#pragma once
// Created: Thu Sep 15 11:31:52 CDT 2005
//
// Header file for packers
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "dbug/dbug.h"

using PackMode = UINT32;

//	The following routines pack and unpack the cross platform basic data types defined in teul.
//	Extended types and structures should be packed with user defined routines that use these to
// 	pack the individual elements.
//
//	[mode] specifies whether to pack the idata into the buffer or unpack the buffer into the idata.
//		TEUL_PACK puts idata in network byte order into buffer at bitoffset, returning the new
//			bitoffset.
//		TEUL_UNPACK extracts idata in local byte order from buffer at bitoffset, returning the
//			new bitoffset.
//	[significantbits] specifies the number of bits to pack from idata.  the default of 0 assumes all
//		 bits significant and are packed.data

//! Pack/unpack a signed value held in a 32-bit store.
SINT32 teulPackSINT32(PackMode mode, UCHAR* const buffer, SINT32 bitoffset, SINT32* const idata, SINT32 significantbits);

//! Pack/unpack a signed value held in a 16-bit store.
SINT32 teulPackSINT16(PackMode mode, UCHAR* const buffer, SINT32 bitoffset, SINT16* const idata, SINT32 significantbits);

//! Pack/unpack an unsigned value held in a 32-bit store.
SINT32 teulPackUINT32(PackMode mode, UCHAR* const buffer, SINT32 bitoffset, UINT32* const idata, SINT32 significantbits);

//! Pack/unpack an unsigned value held in a 16-bit store.
SINT32 teulPackUINT16(PackMode mode, UCHAR* const buffer, SINT32 bitoffset, UINT16* const idata, SINT32 significantbits);

//! Pack/unpack a signed value held in a 8-bit store.
SINT32 teulPackSCHAR(PackMode mode, UCHAR* const buffer, SINT32 bitoffset, SCHAR* const idata, SINT32 significantbits);

//! Pack/unpack an unsigned value held in a 8-bit store.
SINT32 teulPackUCHAR(PackMode mode, UCHAR* const buffer, SINT32 bitoffset, UCHAR* const idata, SINT32 significantbits);

//! Pack/unpack a value stored as a 32-bit float.
SINT32 teulPackFLOAT32(PackMode mode, UCHAR* const buffer, SINT32 bitoffset, FLOAT32* const idata, SINT32 significantbits);

//! Pack/unpack a value stored as a 64-bit float.
SINT32 teulPackFLOAT64(PackMode mode, UCHAR* const buffer, SINT32 bitoffset, FLOAT64* const idata, SINT32 significantbits);

//! Pack/unpack a signed value held in a 64-bit store.
SINT32 teulPackSINT64(PackMode mode, UCHAR* const buffer, SINT32 bitoffset, SINT64* const idata, SINT32 significantbits);

//! Pack/unpack an unsigned value held in a 64-bit store.
SINT32 teulPackUINT64(PackMode mode, UCHAR* const buffer, SINT32 bitoffset, UINT64* const idata, SINT32 significantbits);

// byte pack routine used only when you know the other side takes care of endian
// or the data is byte format (UCHAR but 'NOT' UNICODE).
// [size]  in bytes.
SINT32 teulPackVoid(PackMode mode, UCHAR* const buffer, SINT32 bitoffset, void* const anything, SINT32 size);

// converts bit offsets to actual size in bytes
namespace Teul
{
	static inline SINT32 BitsToBytes(const SINT32 numBits)
	{
		if ( numBits <= 1 )
			return numBits;

		return (numBits + 7) / 8;
	}

	static inline SINT32 BytesToBits(const SINT32 numBytes)
	{
		if ( numBytes <= 0 )
			return numBytes;

		return numBytes * 8;
	}
}

#define DEF_PACKER(_typename, _functor) \
	void packer(PackMode _mode, _typename _data, SINT32 bits=0)	\
	{\
		this->offset = _functor(_mode, this->packed, this->offset, _data, bits); \
	}

#define DEF_PACKREF(_typename, _functor) \
	void packer(PackMode _mode, _typename& _data, SINT32 bits=0) \
	{ \
		this->offset = _functor(_mode, this->packed, this->offset, &_data, bits); \
	} \
	void pack(const _typename& _data, SINT32 bits=0) { packer(TEUL_PACK, const_cast<_typename&>(_data), bits); } \
	void unpack(_typename& _data, SINT32 bits=0) { packer(TEUL_UNPACK, _data, bits); }

//! Helper macro for receive actions that want to verify a live connection.
#define REQUIRE_CLIENT_DATA()	\
	if ( PN_UNLIKELY(a->m_client == nullptr) )	\
		return -ETEUL_CB_REQ_DATA;

//! Helper macro for receive actions that require a player/client entity,
//! if one exists it is supplied as a reference.
//! e.g. REQUIRE_CLIENT_DATA_AS(Client, client) creates Client& client.
#define REQUIRE_CLIENT_DATA_AS(_CastAs_, _Name_)	\
	REQUIRE_CLIENT_DATA()	;	\
	_CastAs_& _Name_ = *(static_cast<_CastAs_*>(a->m_client));

// ACTION objects are passed into each send and receive routine containing
// the information needed to create or decode a message.
struct ACTION_OBJECT
{
	ACTION_OBJECT() = default;
	ACTION_OBJECT(const ACTION_OBJECT& rhs) = default;
	ACTION_OBJECT& operator = (const ACTION_OBJECT& rhs) = default;

	ACTION_OBJECT(void* const packed_, const SINT32 offset_bits_, const SINT32 size_bytes_=0)
		: teulIndex(-1), actionObj(nullptr), localRawData(nullptr), packed(static_cast<UCHAR*>(packed_))
		, offset(offset_bits_), m_size_bytes(size_bytes_)
		, fromAddress(0), fromPort(0)
		, endpoint(nullptr), m_client(nullptr)
	{}

	void packer(PackMode mode, bool& value, SINT32 sb = 1) { return (mode == TEUL_PACK) ? pack(value, sb) : unpack(value, sb); }
	void bytePacker(PackMode mode, void* byteStream, SINT32 numBytes) { this->offset = teulPackVoid(mode, this->packed, this->offset, byteStream, numBytes); }
	void packBytes(const void* byteStream, SINT32 numBytes) { this->offset = teulPackVoid(TEUL_PACK, this->packed, this->offset, const_cast<void*>(byteStream), numBytes); }
	void unpackBytes(void* byteStream, SINT32 numBytes) { this->offset = teulPackVoid(TEUL_UNPACK, this->packed, this->offset, byteStream, numBytes); }
	template<class T, size_t N>
	void arrayPacker(PackMode mode, T (&array)[N]) { this->offset = teulPackVoid(mode, this->packed, this->offset, array, sizeof(array[0]) * N); }

	//! @brief Check that the packed data stream has enough bytes to populate an object type.
	template<class Typename_>
	bool canRead() const
	{
		return offset >= 0 && (static_cast<size_t>(offset) + sizeof(Typename_) * 8) <= static_cast<size_t>(sizeInBits());
	}

	//! @brief Consume an object, in-place, from the packed data section.
	//! @note Does NOT check that there is space for such an object.
	template<class Typename_>
	Typename_& read()
	{
		ABORT_IF(offset < 0);
		void* ptr = reinterpret_cast<char*>(packed) + (static_cast<size_t>(offset) / 8);
		offset += static_cast<SINT32>(sizeof(Typename_) * 8);
		return *reinterpret_cast<Typename_*>(ptr);
	}

	template<class T>
	SINT32 packClass(PackMode mode)
	{
		return reinterpret_cast<T*>(localRawData)->packer(mode, this);
	}

	//! Pack data directly.
	/// NOT READY FOR USE.
	template<typename _Type>
	void copy(PackMode mode, _Type& instance_)
	{
		static const size_t bytes = sizeof(_Type);

		// Align to byte boundary so we don't mess about with bits.
		this->offset = (this->offset + 7) & (~7);
		// Take a pointer to this position in the byte stream.
		auto streamPtr = reinterpret_cast<char*>(this->packed) + static_cast<size_t>(Teul::BitsToBytes(this->offset));
		// Increment offset.
		this->offset += Teul::BytesToBits(bytes);

		if (mode == TEUL_PACK) {
			memcpy(streamPtr, &instance_, bytes);
		} else {
			memcpy(&instance_, streamPtr, bytes);
        }
	}

	void packSecondsDelta(PackMode mode, UINT32& time, SINT32 sizeBits=0);
	void packChatHandle(PackMode mode, ChatHandle* const handle) {
        this->offset = teulPackUINT64(mode, this->packed, this->offset, static_cast<UINT64*>(handle), GAME_HANDLE_BITS);
    }

	// Size is in bytes which discards 3 significant bits of data and means that
	// we can't accurately identify the case where there are less than 7 bits of
	// data in the current buffer.
	bool atEndOfBuffer() const { return offset < 0 || (offsetInBytes() >= sizeInBytes()); }

	//! @brief Get the current buffer-offset position in bits.
	//! @returns Positive number of bits, or a negative error value.
	SINT32 offsetInBits() const { return offset; }
	//! @brief Get the current buffer-offset position in bytes.
	//! @returns Positive number of bytes (rounded up), or a negative error value.
	SINT32 offsetInBytes() const { return (offset > 0) ? ((offset + 7) / 8) : offset; }

	//! @brief Get the current object size in bytes.
	SINT32 sizeInBytes() const { return m_size_bytes; }

	//! @brief Get the current object size in bits.
	SINT32 sizeInBits() const { return m_size_bytes * 8; }

	/////////
	// And now for the mundane stuff..

	SINT32			teulIndex;		// the connection that is either the origin or
									// destination of this index.
	struct TEUL_CALLBACK*	actionObj;	// a pointer to the originating CALLBACK object.
									// , always -1 for outgoing.

	void*			localRawData;	// where to get or place the data.
	UCHAR*			packed;			// the compressed stream of information
									// which must be unpacked by the callback.
	SINT32			offset;			// the beginning bit offset into the data.
	SINT32			m_size_bytes;	// the size in bytes of the incoming data.

	UINT32			fromAddress;	// address the callback originated from (UDP)
	UINT32			fromPort;		// port the callback originated from (UDP)

	struct ENDPOINT*	endpoint;	// The endpoint (or nullptr) this is on.
	void*			m_client;		//! Pointer to the local client info OR nullptr.

	DEF_PACKER(SINT16*, teulPackSINT16)
	DEF_PACKREF(SINT16, teulPackSINT16)
	DEF_PACKER(UINT16*, teulPackUINT16)
	DEF_PACKREF(UINT16, teulPackUINT16)
	DEF_PACKER(SINT32*, teulPackSINT32)
	DEF_PACKREF(SINT32, teulPackSINT32)
	DEF_PACKER(UINT32*, teulPackUINT32)
	DEF_PACKREF(UINT32, teulPackUINT32)
	DEF_PACKER(SCHAR*, teulPackSCHAR)
	DEF_PACKREF(SCHAR, teulPackSCHAR)
	DEF_PACKER(UCHAR*, teulPackUCHAR)
	DEF_PACKREF(UCHAR, teulPackUCHAR)
	DEF_PACKER(UINT64*, teulPackUINT64)
	DEF_PACKREF(UINT64, teulPackUINT64)
	DEF_PACKER(SINT64*, teulPackSINT64)
	DEF_PACKREF(SINT64, teulPackSINT64)
	DEF_PACKER(FLOAT32*, teulPackFLOAT32)
	DEF_PACKREF(FLOAT32, teulPackFLOAT32)
	DEF_PACKER(FLOAT64*, teulPackFLOAT64)
	DEF_PACKREF(FLOAT64, teulPackFLOAT64)

	DEF_PACKER(void*, teulPackVoid)

	void pack(const bool& value, SINT32 sb = 1)
	{
		assert(sb == 1 || sb == 0);
		UCHAR i(value ? 1 : 0);
		pack(i, 1);
	}
	void unpack(bool& value, SINT32 sb = 1)
	{
		assert(sb == 1 || sb == 0);
		UCHAR i(0);
		unpack(i, 1);
		value = (i != 0);
	}

	template<typename T>
	void pack(const T& _data, SINT32 bits = 0)
	{
		_data.pack(this, bits);
	}
	template<typename T>
	void unpack(T& _data, SINT32 bits = 0)
	{
		_data.unpack(this, bits);
	}
};

SINT32 teulPackSecondsDelta(PackMode mode, UCHAR* const buffer, SINT32 bitoffset, UINT32* timer, SINT32 size=0);

namespace Teul
{

//! @class Packer
//! @brief Store a value type and the appropriate pack/unpack function.
//! Implement the function 'packer'
template<typename _Type>
struct Packer
{
	using value_type = _Type;
	using self_type = Packer<value_type>;

	value_type m_value;

	operator value_type () const { return m_value; }
	const value_type& operator()() const { return m_value; }
	value_type& operator()() { return m_value; }

	//! Pack or unpack data.
	//! @param[in]	mode		TEUL_PACK / TEUL_UNPACK
	//! @param[in]	a			Pointer to action object describing data stream.
	//! @param[in]	bits		Maximum bits to pack (where honored).

	void packer(PackMode mode, ACTION_OBJECT* const a, SINT32 bits=0)
	{
		a->packer(mode, m_value, bits);
	}

	void pack(ACTION_OBJECT* const a, SINT32 bits = 0) const
	{
		a->pack(m_value, bits);
	}

	void unpack(ACTION_OBJECT* const a, SINT32 bits = 0)
	{
		a->unpack(m_value, bits);
	}
};

}
