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
#include "teul/teulPackEnums.h"

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
extern SINT32 teulPackSINT32(LOGICAL mode, UCHAR* const buffer, SINT32 bitoffset, SINT32* const idata, SINT32 significantbits);

//! Pack/unpack a signed value held in a 16-bit store.
extern SINT32 teulPackSINT16(LOGICAL mode, UCHAR* const buffer, SINT32 bitoffset, SINT16* const idata, SINT32 significantbits);

//! Pack/unpack an unsigned value held in a 32-bit store.
extern SINT32 teulPackUINT32(LOGICAL mode, UCHAR* const buffer, SINT32 bitoffset, UINT32* const idata, SINT32 significantbits);

//! Pack/unpack an unsigned value held in a 16-bit store.
extern SINT32 teulPackUINT16(LOGICAL mode, UCHAR* const buffer, SINT32 bitoffset, UINT16* const idata, SINT32 significantbits);

//! Pack/unpack a signed value held in a 8-bit store.
extern SINT32 teulPackSCHAR(LOGICAL mode, UCHAR* const buffer, SINT32 bitoffset, SCHAR* const idata, SINT32 significantbits);

//! Pack/unpack an unsigned value held in a 8-bit store.
extern SINT32 teulPackUCHAR(LOGICAL mode, UCHAR* const buffer, SINT32 bitoffset, UCHAR* const idata, SINT32 significantbits);

//! Pack/unpack a value stored as a 32-bit float.
extern SINT32 teulPackFLOAT32(LOGICAL mode, UCHAR* const buffer, SINT32 bitoffset, FLOAT32* const idata, SINT32 significantbits);

//! Pack/unpack a value stored as a 64-bit float.
extern SINT32 teulPackFLOAT64(LOGICAL mode, UCHAR* const buffer, SINT32 bitoffset, FLOAT64* const idata, SINT32 significantbits);

//! Pack/unpack a signed value held in a 64-bit store.
extern SINT32 teulPackSINT64(LOGICAL mode, UCHAR* const buffer, SINT32 bitoffset, SINT64* const idata, SINT32 significantbits);

//! Pack/unpack an unsigned value held in a 64-bit store.
extern SINT32 teulPackUINT64(LOGICAL mode, UCHAR* const buffer, SINT32 bitoffset, UINT64* const idata, SINT32 significantbits);

// byte pack routine used only when you know the other side takes care of endian
// or the data is byte format (UCHAR but 'NOT' UNICODE).
// [size]  in bytes.
extern SINT32 teulPackVoid(LOGICAL mode , UCHAR* const buffer, SINT32 bitoffset, void* const anything, SINT32 size);

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

#define DEF_PACKER(_typename, _functor, _defSize) \
	void packer(LOGICAL _mode, _typename _data, SINT32 bits=_defSize)	\
	{\
		this->offset = _functor(_mode, this->packed, this->offset, (_typename)_data, bits); \
	}

#define DEF_PACKREF(_typename, _functor, _defSize) \
	void packer(LOGICAL _mode, _typename& _data, SINT32 bits=_defSize) \
	{\
		this->offset = _functor(_mode, this->packed, this->offset, &_data, bits); \
	}

#define CAST_PACKREF(_typename, _functor, _castAs, _defSize) \
	void packer(LOGICAL _mode, _typename& _data, SINT32 bits=_defSize) \
	{ \
		this->offset = _functor(_mode, this->packed, this->offset, &static_cast<_castAs>(_data), bits); \
	}

//! Helper macro for receive actions that want to verify a live connection.
#define REQUIRE_CLIENT_DATA()	\
	if ( PN_UNLIKELY(a->m_client == NULL) )	\
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
#if __cplusplus <= 199711L
	ACTION_OBJECT()
		: teulIndex(-1)
		, actionObj(NULL), localRawData(NULL), packed(NULL), offset(0), m_size_bytes(0)
		, fromAddress(0), fromPort(0)
		, endpoint(NULL), m_client(NULL)
	{}
#else
	// Required to turn ACTION_OBJECT back into POD now that
	// a constructor exists.
	ACTION_OBJECT() = default;
	ACTION_OBJECT(const ACTION_OBJECT& rhs) = default;
	ACTION_OBJECT& operator = (const ACTION_OBJECT& rhs) = default;
#endif

	ACTION_OBJECT(void* const packed_, const SINT32 offset_bits_, const SINT32 size_bytes_=0)
		: teulIndex(-1), actionObj(NULL), localRawData(NULL), packed((UCHAR*)packed_)
		, offset(offset_bits_), m_size_bytes(size_bytes_)
		, fromAddress(0), fromPort(0)
		, endpoint(NULL), m_client(NULL)
	{}

	void packer(LOGICAL mode, bool& value, SINT32 size_bytes_=0);
	void packBytes(LOGICAL mode, void* byteStream, SINT32 numBytes) { this->offset = teulPackVoid(mode, this->packed, this->offset, byteStream, numBytes); }

	//! @brief Check that the packed data stream has enough bytes to populate an object type.
	template<class Typename_>
	bool canRead() const
	{
		return (offset >= 0) ? ((SINT32)(static_cast<size_t>(offset) + sizeof(Typename_) * 8) <= (SINT32)sizeInBits()) : false;
	}

	//! @brief Consume an object, in-place, from the packed data section.
	//! @note Does NOT check that there is space for such an object.
	template<class Typename_>
	Typename_& read()
	{
		ABORT_IF(offset < 0);
		void* ptr = ((char*)(packed)) + (static_cast<size_t>(offset) / 8);
		offset += static_cast<SINT32>(sizeof(Typename_) * 8);
		return *(Typename_*)(ptr);
	}

	template<class _Class>
	SINT32 packClass(LOGICAL mode)
	{
		_Class* classInstance = (_Class*) this->localRawData;	 // Type cast readability
		return classInstance->packer(mode, this);
	}

	//! Pack data directly.
	/// NOT READY FOR USE.
	template<typename _Type>
	void copy(LOGICAL mode, _Type& instance_)
	{
		static const size_t bytes = sizeof(_Type);

		// Align to byte boundary so we don't mess about with bits.
		this->offset = (this->offset + 7) & (~7);
		// Take a pointer to this position in the byte stream.
		char* streamPtr = (char*)(this->packed) + static_cast<size_t>(Teul::BitsToBytes(this->offset));
		// Increment offset.
		this->offset += Teul::BytesToBits(bytes);

		if ( mode == TEUL_PACK )
			memcpy(streamPtr, &instance_, bytes);
		else
			memcpy(&instance_, streamPtr, bytes);
	}

	void packSecondsDelta(LOGICAL mode, UINT32& time, SINT32 sizeBits=0);
	void packChatHandle(LOGICAL mode, ChatHandle* const handle) { this->offset = teulPackUINT64(mode, this->packed, this->offset, (UINT64*)handle, GAME_HANDLE_BITS); }

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

	struct ENDPOINT*	endpoint;	// The endpoint (or NULL) this is on.
	void*			m_client;		//! Pointer to the local client info OR NULL.

	DEF_PACKER(SINT32*, teulPackSINT32, 0)
	DEF_PACKREF(SINT32, teulPackSINT32, 0)
	DEF_PACKER(UINT32*, teulPackUINT32, 0)
	DEF_PACKREF(UINT32, teulPackUINT32, 0)
	DEF_PACKER(SINT16*, teulPackSINT16, 0)
	DEF_PACKREF(SINT16, teulPackSINT16, 0)
	DEF_PACKER(UINT16*, teulPackUINT16, 0)
	DEF_PACKREF(UINT16, teulPackUINT16, 0)
	DEF_PACKER(SCHAR*, teulPackSCHAR, 0)
	DEF_PACKREF(SCHAR, teulPackSCHAR, 0)
	DEF_PACKER(UCHAR*, teulPackUCHAR, 0)
	DEF_PACKREF(UCHAR, teulPackUCHAR, 0)
	DEF_PACKER(UINT64*, teulPackUINT64, 0)
	DEF_PACKREF(UINT64, teulPackUINT64, 0)
	DEF_PACKER(SINT64*, teulPackSINT64, 0)
	DEF_PACKREF(SINT64, teulPackSINT64, 0)
	DEF_PACKER(FLOAT32*, teulPackFLOAT32, 0)
	DEF_PACKREF(FLOAT32, teulPackFLOAT32, 0)
	DEF_PACKER(FLOAT64*, teulPackFLOAT64, 0)
	DEF_PACKREF(FLOAT64, teulPackFLOAT64, 0)

	DEF_PACKER(void*, teulPackVoid, 0);
};

extern SINT32 teulPackSecondsDelta(LOGICAL mode, UCHAR* const buffer, SINT32 bitoffset, UINT32* timer, SINT32 size=0);

namespace Teul
{

//! @class Packer
//! @brief Store a value type and the appropriate pack/unpack function.
//! Implement the function 'packer'
template<typename _Type>
struct Packer
{
	typedef	_Type valuetype;
	typedef Packer<valuetype> selftype;

	valuetype m_value;

	operator valuetype () const { return m_value; }
	const valuetype& operator()() const { return m_value; }
	valuetype& operator()() { return m_value; }

	//! Pack or unpack data.
	//! @param[in]	mode		TEUL_PACK / TEUL_UNPACK
	//! @param[in]	a			Pointer to action object describing data stream.
	//! @param[in]	bits		Maximum bits to pack (where honored).

	void packer(LOGICAL mode, ACTION_OBJECT* const a, SINT32 bits=0)
	{
		a->packer(mode, m_value, bits);
	}
};

//! @brief Count the highest bit number needed to describe the given value.
//! E.g. BitCount<0>::count has a value of zero (bit 0 is the highest used).
template<UINT32 _value>
struct BitCount
{
	enum { count = (_value & 1) + Teul::BitCount<(_value >> 1)>::count };
};

template<>
struct BitCount<0>
{
	enum { count = 0 };
};

//! @brief Get the number of bits required to describe the given value.
//! E.g. BitRank<0>::rank has a value of one (one bit is required).
template<UINT32 _value>
struct BitRank
{
	enum { rank = (_value & 1) ? 1 : Teul::BitRank<(_value >> 1)>::rank + 1 };
};
template<>
struct BitRank<0>
{
	enum { rank = 0 };
};

//! @brief Test if an unsigned value fits into a given number of bits.
template<typename _Type>
bool FitUnsignedBitValue(const _Type& value, size_t bits)
{
	return ((UINT64)value >> bits) == 0ULL;
}

//! @brief Test if a signed value fits into a given number of bits,
//! allowing for a sign bit.
template<typename _Type>
bool FitSignedBitValue(const _Type& value, size_t bits)
{
	// Populate all the bits in "sign" with the sign bit
	// (i.e. sign will be either all zeros or all ones).
	const UINT64 sign = ((SINT64)value >> 63);
	// Create a mask for examining all the upper bits, from
	// 63 to the sign bit within the size of "bits".
	const UINT64 signMask = (~0ULL) << (bits - 1);
	// All of the bits in "value" and "sign" should be the
	// same; i.e. 1s for a negative value, 0s for a positive.
	// If not, then the value is too big.
	return ((UINT64)value & signMask) == (sign & signMask);
}

}
