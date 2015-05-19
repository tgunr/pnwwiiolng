#include <syscfg.h> // Path is project specific.
// This module provides high-level buffer stuffing
// utility functions used in the teul API.
//
// Copyright 1999 Playnet Inc, All rights reserved
// Not to be distributed in API form except as part of a compiled
// library
//
// Originating Programmer: Martin Poulin

#include "teul/teulConnectionInstruction.h"
#include "updt/loginTypes.h"
#include "weap/weapSerial.h"
#include "wwii/wwiiFullObjID.h"
#include "wwii/wwiiPack.h"
#include "wwii/wwiiPackString.h"

// pack significant bytes into buffer from left justified value in network byte order
static size_t _packBytes(size_t offset, UCHAR* const buffer, UCHAR* const value, const size_t numBytes, size_t significantBytes)
{
	if (significantBytes < 1)
	{
		if (significantBytes != 0) {
			dbugWrite(_FLA, "invalid significantBytes = %d\n", significantBytes);
			return (size_t)-EINVAL;
		}
		return offset;
	}

	// big-endian systems use the reverse of network/stream
	// ordering. this isn't about endian-favoring, it's
	// about the position of the significant values.

	// value should always come in left justified significant bytes
	size_t i = numBytes;
	const size_t cutoff = offset + significantBytes;
	do
	{
		buffer[offset++] = value[--i];
	} while ( offset < cutoff );

	return offset;
}

// pack significant bytes left justified into value from buffer in platform byte order
static size_t _unpackBytes(const size_t offset, UCHAR* const buffer, UCHAR* const value, const size_t numBytes, const size_t significantBytes )
{
	if (significantBytes<1)
	{
		dbugSetErrno(EINVAL);
		dbugLog(_FLA,"invalid significantBytes = %zd", significantBytes);
		return  (size_t)-EINVAL;
	}

	// set value to left justified significant bytes
	size_t i = numBytes - significantBytes;
	size_t j = significantBytes;
	do
	{
		value[i++] = buffer[offset + --j];
	} while ( j > 0 );

	return offset + significantBytes;
}

// Note: teulPackVoid caveat: last argument is size in BYTES not bits
SINT32 teulPackVoid(const LOGICAL mode, UCHAR* const buffer, const SINT32 bitOffset, void* const anything, SINT32 size)
{
	const SINT32 totalBits = bitOffset + (size<<3);

	if ( bitOffset < 0 )
	{
		dbugSetErrno(EINVAL);
		dbugLog(_FLA, "%s invalid bit offset = %d or totalBits = %d", ((mode==TEUL_PACK)?"PACK":"UNPACK"), bitOffset,totalBits);
		return -EINVAL;
	}

	// If we are even aligned (no bits below 0b1000),
	// we can just copy ourselves directly
	if ( (bitOffset & 7) == 0 )
	{
		if (mode == TEUL_PACK)
		{
			memcpy(&buffer[bitOffset >> 3], anything, size );
		}
		else
		{
			memcpy(anything, &buffer[bitOffset >> 3], size);
		}

		return totalBits;
	}

	for ( UINT32 i = 0; i < (UINT32)size; ++i )
	{
		teulPackUCHAR(mode, buffer, bitOffset + (i << 3), ((UCHAR*)anything) + i, 0);
	}

	return totalBits;
}

// generic pack for xINT32, xINT16, xCHAR, FLOAT32
static SINT32
_packBits(UCHAR* const buffer, SINT32 bitOffset, UINT32* const idata, SINT32 significantBits, SINT32 numBits)
{
	if ( bitOffset < 0 )
	{
		dbugSetErrno(EINVAL);
		dbugLog(_FLA, "invalid bit offset or significantBits = %d",significantBits);
		return -EINVAL;
	}


	if (buffer == nullptr || idata == nullptr)
	{
		dbugSetErrno(EINVAL);
		dbugLog(_FLA, "invalid buffer %p or data %p",(UINT32*) buffer, (UINT32*) idata);
		return -EINVAL;
	}

	const SINT32 totalBits = bitOffset + significantBits;

	SINT32 byteOffset = bitOffset / 8;
	bitOffset &= 0x7;		// Alignment offset, no-longer full offset.

	const SINT32 numBytes = numBits / 8;
	const SINT32 sigBytes = (significantBits+7) / 8;

	// left bit align, blow off insignificant bits
	UINT32 data = (*idata) << (numBits - significantBits);

	// if the buffer isn't byte aligned, bitwise-or the top bits into the buffer
	if (bitOffset != 0)
	{
		UCHAR buf[4];
		(void)_packBytes(0, buf, (UCHAR*) &data, numBytes, sigBytes);

		for (SINT32 i = 0; i < sigBytes; ++i)
		{
			// clear out the bits we are putting in
			buffer[byteOffset] &= 0xff << (8-bitOffset);
			// 'or' top most bits into the remainder of the buffer byte
			buffer[byteOffset] |=  (buf[i] >> bitOffset); // get the MSB's

			buffer[byteOffset+1] &= 0xff >> (bitOffset);
			buffer[byteOffset+1] |= ((UCHAR)buf[i] << (8-bitOffset) & 0xff);

			byteOffset++;
		}

	}
	else
		// don't compensate for numBytes - sigBytes
		(void)_packBytes(byteOffset, buffer, (UCHAR*) &data, numBytes, sigBytes);

	return (totalBits);
}

// generic unpack for xINT32, xINT16, xCHAR, FLOAT32
static SINT32
_unpackBits (UCHAR* const buffer, SINT32 bitOffset, UINT32* const idata, SINT32 significantBits, SINT32 numBits)
{
	UCHAR data[sizeof(UINT32)]= {0,0,0,0};
	SINT32 byteOffset, numBytes, sigBytes, totalBits = bitOffset;

	*idata = 0;
	totalBits += significantBits;

	byteOffset = bitOffset>>3;
	// bit offset no == the dif between byte and bit...
	bitOffset -= byteOffset<<3;

	numBytes = numBits>>3;
	sigBytes = (significantBits+7)>>3;

	// if the data is already byte aligned just copy it out
	if (bitOffset != 0)
	{
		const int offsetCompliment = 8 - bitOffset;
		// not byte aligned, align it into data...
		for (SINT32 i = 0; i < sigBytes; ++i)
		{
			// copy the bits from the bit offset position into data byte aligned
			data[i] = (buffer[byteOffset++] << bitOffset) & 0xff;
			data[i] |= (buffer[byteOffset] >> offsetCompliment) & 0xff;
		}

		_unpackBytes(0, data, (UCHAR*) idata, numBytes, sigBytes);
	}
	else
		_unpackBytes(byteOffset, buffer, (UCHAR*) idata, numBytes, sigBytes);

	// right align data into idata
	static const UINT32 UINT32_BITS = sizeof(UINT32) << 3;
	(*idata) >>= (UINT32_BITS - significantBits);

	return (totalBits);
}

SINT32
teulPackSINT32(LOGICAL mode, UCHAR* const buffer, SINT32 bitOffset, SINT32* const idata, SINT32 significantBits)
{
	UINT32 xdata;

	if (significantBits < 1 || significantBits > 32)
		significantBits = 32;

	if (mode == TEUL_PACK)
	{
		return _packBits (buffer, bitOffset, (UINT32*) idata, significantBits, 32);
	}
	else
	{
		SINT32 newOffset = _unpackBits (buffer, bitOffset, &xdata, significantBits, 32);

		if ( ((xdata) >> (significantBits - 1)) != 0 && significantBits != 32)
			xdata = xdata | (0xffffffff << significantBits);

		*idata = (SINT32) xdata;

		return newOffset;
	}
}

SINT32
teulPackSINT16(LOGICAL mode, UCHAR* const buffer, SINT32 bitOffset, SINT16* const idata, SINT32 significantBits)
{
	UINT32 xdata = (UINT32) (*idata);

	if (significantBits < 1 || significantBits > 16)
		significantBits = 16;

	if (mode == TEUL_PACK)
	{
		return _packBits (buffer, bitOffset, &xdata, significantBits, 32);
	}
	else
	{
		SINT32	newOffset;

		xdata = 0;

		newOffset = _unpackBits (buffer, bitOffset, &xdata, significantBits, 32);

		if ( ((xdata) >> (significantBits - 1)) != 0)
			xdata = xdata | (0xffffffff << significantBits);

		*idata = (UINT16) xdata;

		return newOffset;
	}
}

SINT32
teulPackUINT16 (LOGICAL mode, UCHAR* const buffer, SINT32 bitOffset, UINT16* const idata, SINT32 significantBits)
{
	UINT32 xdata = (UINT32) *idata;

	if (significantBits < 1 || significantBits > 16)
		significantBits = 16;

	if (mode == TEUL_PACK)
		return _packBits(buffer, bitOffset, &xdata, significantBits, 32);
	else
	{
		SINT32 newOffset = _unpackBits(buffer, bitOffset, &xdata, significantBits, 32);
		*idata = (UINT16) xdata;
		return newOffset;
	}
}

SINT32
teulPackFLOAT32 (LOGICAL mode, UCHAR* const buffer, SINT32 bitOffset, FLOAT32* const idata, SINT32 /* significantBits */)
{
	if (mode == TEUL_PACK)
		return _packBits(buffer, bitOffset, (UINT32*) idata, 32, 32);
	else
		return _unpackBits(buffer, bitOffset, (UINT32*) idata, 32, 32);
}


SINT32
teulPackUINT64(LOGICAL mode, UCHAR* const buffer, SINT32 bitOffset, UINT64* const idata, SINT32 significantBits)
{
	if (significantBits < 1 || significantBits > 64)
		significantBits = 64;
	SINT32 sigHighBits = (significantBits > 32) ? significantBits - 32 : 0;
	SINT32 sigLowBits = (significantBits >= 32) ? 32 : significantBits;

	UINT32 bits = 0;
	if ( mode == TEUL_PACK )
	{
		if ( sigHighBits > 0 )
		{
			bits = (UINT32)((*idata) >> 32);
			bitOffset = _packBits(buffer, bitOffset, &bits, sigHighBits, 32);
		}
		bits = (UINT32)((*idata) & 0xffffffff);
		bitOffset = _packBits(buffer, bitOffset, &bits, sigLowBits, 32);
	}
	else
	{
		*idata = 0ULL;
		if ( sigHighBits > 0 )
		{
			bitOffset = _unpackBits(buffer, bitOffset, &bits, sigHighBits, 32);
			*idata = bits;
			*idata <<= 32;
		}
		bitOffset = _unpackBits(buffer, bitOffset, &bits, sigLowBits, 32);
		*idata |= bits;
	}

	return bitOffset;
}

SINT32
teulPackSINT64(LOGICAL mode, UCHAR* const buffer, SINT32 bitOffset, SINT64* const idata, SINT32 significantBits)
{
	if (significantBits < 1 || significantBits > 64)
		significantBits = 64;
	SINT32 sigHighBits = (significantBits > 32) ? significantBits - 32 : 0;
	SINT32 sigLowBits = (significantBits >= 32) ? 32 : significantBits;

	UINT32 bits = 0;
	if ( mode == TEUL_PACK )
	{
		if ( sigHighBits > 0 )
		{
			bits = (UINT32)((*idata) >> 32);
			bitOffset = _packBits(buffer, bitOffset, &bits, sigHighBits, 32);
		}
		bits = (UINT32)(*idata);
		bitOffset = _packBits(buffer, bitOffset, &bits, sigLowBits, 32);
	}
	else
	{
		UINT64 incoming = 0ULL;
		if ( sigHighBits > 0 )
		{
			bitOffset = _unpackBits(buffer, bitOffset, &bits, sigHighBits, 32);
			incoming = bits;
			incoming <<= 32;
		}
		bitOffset = _unpackBits(buffer, bitOffset, &bits, sigLowBits, 32);
		incoming |= bits;

		if ( significantBits != 64 && (incoming >> (significantBits - 1)) != 0 )
			incoming |= (~0ULL << significantBits);

		*idata = incoming;
	}

	return bitOffset;
}

SINT32
teulPackUINT32(LOGICAL mode, UCHAR* const buffer, SINT32 bitOffset, UINT32* const idata, SINT32 significantBits)
{
	if (significantBits < 1 || significantBits > 32)
		significantBits = 32;

	if (mode == TEUL_PACK)
		return _packBits(buffer, bitOffset, idata, significantBits, 32);
	else
		return _unpackBits(buffer,bitOffset,idata,significantBits,32);
}


SINT32
teulPackFLOAT64(LOGICAL mode, UCHAR* const buffer, SINT32 bitOffset, FLOAT64* const idata, SINT32 significantBits)
{
#ifdef OMG_THIS_DOESNT_WORK_WHEN_BITOFFSET_IS_NON_ZERO
	static const SINT32 numbytes = sizeof(FLOAT64), numBits = numbytes << 3;
	SINT32 totalBits = bitOffset + numBits;

	SINT32 i;

	SINT32 byteOffset = bitOffset >> 3;
	bitOffset -= byteOffset << 3;

	if (mode == TEUL_PACK)
	{
		if (bitOffset == 0)
			_packBytes(byteOffset,buffer, (UCHAR*) idata, numbytes, numbytes);
		else
		{
			byteOffset++;

			_packBytes(byteOffset,buffer, (UCHAR*) idata, numbytes, numbytes);
			// get the MSB's
			buffer[byteOffset-1] |= buffer[byteOffset] >> bitOffset;

			for (i = byteOffset; i < (byteOffset+numbytes); ++i)
			{	// byte align the holding cell
				buffer[i] = (buffer[i] << (8-bitOffset)) | (buffer[i+1] >> bitOffset);
			}
		}

		return (totalBits);
	}
	else
	{
		UCHAR data[numbytes];

		if (bitOffset == 0)
			_unpackBytes(byteOffset, buffer, (UCHAR*) idata, numbytes, numbytes);
		else
		{
			const int offsetCompliment = 8 - bitOffset;
			for (i = 0; i < numbytes; ++i)
			{	// byte align the holding cell
				data[i] = (buffer[byteOffset++] << bitOffset);
				data[i] |= (buffer[++byteOffset] >> offsetCompliment);
			}

			_unpackBytes(0, data, (UCHAR*) idata, numbytes, numbytes);
		}

		return (totalBits);
	}
#else
	return teulPackUINT64(mode, buffer, bitOffset, (UINT64*)idata, significantBits);
#endif
}

SINT32
teulPackSCHAR(LOGICAL mode, UCHAR* const buffer, SINT32 bitOffset, SCHAR* const idata, SINT32 significantBits)
{
	if (significantBits < 1 || significantBits > 8)
		significantBits = 8;

	if (mode == TEUL_PACK)
		return teulPackUCHAR (mode, buffer, bitOffset, (UCHAR*) idata, significantBits);
	else
	{
		SINT32	newOffset = teulPackUCHAR (mode, buffer, bitOffset, (UCHAR*) idata, significantBits);
		if (((*idata) >> (significantBits -1)) != 0)
			*idata = *idata | (0xff << significantBits);

		return newOffset;
	}
}

SINT32
teulPackUCHAR(LOGICAL mode, UCHAR* const buffer, SINT32 bitOffset, UCHAR* const idata, SINT32 significantBits)
{
	SINT32 byteOffset;

	if (significantBits < 1 || significantBits > 8 )
		significantBits = 8;

	SINT32 totalBits = bitOffset + significantBits;

	if ( bitOffset < 0)
	{
		dbugSetErrno(EINVAL);
		dbugLog(_FLA,"invalid toatalbits = %d or bitOffset = %d", totalBits, bitOffset);
		return -EINVAL;
	}

	byteOffset = bitOffset>>3;
	bitOffset -= byteOffset<<3;

	if (mode == TEUL_PACK)
	{
		UCHAR data = *idata;
		// shift off insignificant bits
		data = data << (8 - significantBits);

		if (bitOffset == 0)
			buffer[byteOffset] = data;
		else
		{
			buffer[byteOffset] &= 0xff << (8-bitOffset);
			buffer[byteOffset]  |= data >> bitOffset;
			buffer[byteOffset+1] &= 0xff >> (bitOffset);
			buffer[byteOffset+1] |= data << (8-bitOffset);
		}

		return (totalBits);
	}
	else
	{
		if (bitOffset == 0)
			*idata = buffer[byteOffset];
		else	// byte align the holding cell
			*idata = (UCHAR)((buffer[byteOffset] << bitOffset) | (buffer[byteOffset+1] >> (8-bitOffset)));

		// byte align the holding cell
		*idata = *idata >> (8 - significantBits);

		return (totalBits);
	}
}

//////////////////////////////////////////////////////////////////////
// Pack a PLAYER_LOGIN structure.

SINT32
PLAYER_LOGIN::packer(LOGICAL mode, UCHAR* const message, SINT32 offset)
{
	UINT32 Len = AUTH_SEQ_SIZE;

	offset = teulPackUINT32(mode, message, offset, &this->OTP, 0);
	offset = teulPackUINT32(mode, message, offset, &this->appVersion, 0);
	offset = teulPackVoid(mode, message, offset, &this->CopyRightString, Len);
	offset = teulPackUINT32(mode, message, offset, &this->attemptID, 0);

	return offset;
}

//////////////////////////////////////////////////////////////////////
// Pack a time-in-seconds delta

void
ACTION_OBJECT::packSecondsDelta(LOGICAL mode, UINT32& time, SINT32 sizeBits /* = 0 */)
{
	UINT32 delta = 0;
	if ( mode == TEUL_PACK )
		delta = (time >= teulClockTime) ? (time - teulClockTime) + 1 : 0;
	this->packer(mode, &delta, sizeBits);
	if ( mode == TEUL_UNPACK )
		time = (delta > 0) ? (teulClockTime + delta - 1) : 0;
}

SINT32
teulPackSecondsDelta(LOGICAL mode, UCHAR* const buffer, SINT32 bitoffset, UINT32* const timer, SINT32 size)
{
	UINT32 delta = 0;
	if ( mode == TEUL_PACK )
		delta = (*timer >= teulClockTime) ? (*timer - teulClockTime) + 1 : 0;
	bitoffset = teulPackUINT32(mode, buffer, bitoffset, &delta, size);
	if ( mode == TEUL_UNPACK )
		*timer = (delta > 0) ? (teulClockTime + delta - 1) : 0;
	return bitoffset;
}

SINT32
wwiiPackText(LOGICAL mode, UCHAR* const data, SINT32 offset, char* const text, SINT32 maxLen)
{
	UINT32 len = 0;
	UINT32 lenBits;

	if ( offset < 0 )
		return -EINVAL;

	if ( mode == TEUL_PACK )
	{
		len = (UINT32)strlen(text);
		if ( len > (UINT32)maxLen )
			len = (UINT32)maxLen;
	}
	else if ( mode == TEUL_UNPACK )
		text[0] = 0;

	// Let MaxLen determine how many bits to use for packing the length
	if ( maxLen == 0 || maxLen >= 65536 ) lenBits = 0;
	else if ( maxLen > 255 ) lenBits = 16;
	else lenBits = 8;

	offset = teulPackUINT32(mode, data, offset, &len, lenBits);
	if ( offset < 0 || len > (UINT32)maxLen )
		return -EINVAL;

	offset = teulPackVoid(mode, data, offset, text, len);

	if ( mode == TEUL_UNPACK )
		text[len] = 0;

	return offset;
}

//////////////////////////////////////////////////////////////////////
// Pack a connection instruction.

Teul::ConnectionInstruction::ConnectionInstruction(ACTION_OBJECT* const a)
	: m_role(Unknown), m_hostIP(0), m_port(0)
{
	packer(TEUL_UNPACK, a);
}

SINT32
Teul::ConnectionInstruction::packer(LOGICAL mode, ACTION_OBJECT* const a)
{
	a->offset = teulPackUINT32(mode, a->packed, a->offset, (UINT32*)&m_role, 8);
	a->packer(mode, m_hostIP);
#if platBuild == TEUL_SERVER
	UINT32 seed = rand();
#else
	UINT32 seed = m_otp ^ 0xdeadbeef;
#endif
	a->packer(mode, seed);
	a->packer(mode, m_port);
	a->packer(mode, m_otp);
#if platBuild == TEUL_SERVER
	UINT64 key = (UINT64)teulClock64;
#else
	UINT64 key = 0ULL;
#endif
	a->packer(mode, key);

	return a->offset;
}

SINT32
wwiiPackWeaponSpec(LOGICAL mode, UCHAR* const data, SINT32 offset, struct WEAPON_SPEC* const object, SINT32 /*size*/)
{
	return teulPackUINT16(mode, data, offset, (UINT16*)object, 0);
}

SINT32
wwiiPackWeaponSerial(LOGICAL mode, UCHAR* const data, SINT32 offset, struct WEAPON_SERIAL* const object, SINT32 /*size*/)
{
	offset = teulPackUINT32(mode, data, offset, &(object->serial), 0);
	offset = wwiiPackFullObjID(mode, data, offset, &(object->source), 0);
	return wwiiPackWeaponSpec(mode, data, offset, &(object->spec), 0);
}

//////////////////////////////////////////////////////////////////////
// New style pack function which works off of an action object

void
WEAPON_SPEC::pack(ACTION_OBJECT* a, SINT32) const
{
	a->pack(asUINT16(), 0);
}

void
WEAPON_SPEC::unpack(ACTION_OBJECT* a, SINT32)
{
	UINT16 spec = 0;
	a->unpack(spec, 0);
	fromShort(spec);
}

//////////////////////////////////////////////////////////////////////
// Pack a std::string.

SINT32
wwiiPackString(LOGICAL mode, UCHAR* data, SINT32 offset, std::string& text, SINT32 maxLen)
{
	if ( offset < 0 || maxLen < 0 )
		return -EINVAL;

	static const SINT32 _BUF_SIZE = 512;
	static char _buffer[_BUF_SIZE + 1];
	UINT32 len = 0;
	UINT32 uMaxLen = (UINT32)maxLen;
	UINT32 lenBits;

	if ( mode == TEUL_PACK )
	{
		len = text.empty() ? 0 : (UINT32)text.length();
		if ( len > uMaxLen )
			len = uMaxLen;
	}
	else if ( mode == TEUL_UNPACK )
		text = "";

	// Let MaxLen determine how many bits to use for packing the length
	if ( uMaxLen == 0 || uMaxLen > 65536 ) lenBits = 0;
	else if ( uMaxLen > 255 ) lenBits = 16;
	else lenBits = 8;

	offset = teulPackUINT32(mode, data, offset, &len, lenBits);
	if ( offset < 0 || len > uMaxLen )
	{
		return -EILSEQ;
	}

	if ( len > 0 )
	{
		if ( mode == TEUL_PACK )
		{
			offset = teulPackVoid(mode, data, offset, const_cast<char*>(text.c_str()), len);
		}
		else
		{
			text.reserve(len);
			for ( UINT32 pos = 0; pos < len; pos += _BUF_SIZE )
			{
				SINT32 bytes = len - pos;
				if ( bytes > _BUF_SIZE ) bytes = _BUF_SIZE;
				offset = teulPackVoid(mode, data, offset, _buffer, bytes);
				_buffer[bytes] = 0;
				text.append(_buffer, bytes);
			}
		}
	}

	return offset;
}

SINT32
wwiiPackFullObjID(LOGICAL mode, UCHAR* const data, SINT32 offset, struct FULL_OBJ_ID* const object, SINT32 /*size*/)
{
	offset = teulPackUINT16(mode,data,offset,&(object->id),0);
	if ( object->id != 0 )
	{
		offset = teulPackSINT16(mode,data,offset,&(object->cell.ox),0);
		offset = teulPackSINT16(mode,data,offset,&(object->cell.oy),0);
	}
	else if ( mode == TEUL_UNPACK )
	{
		object->cell.ox = object->cell.oy = 0;
	}
	if ( mode == TEUL_UNPACK )
	{
		object->objClass = 0;
		object->objType = 0;
	}

	return offset;
}

void FULL_OBJ_ID::pack(ACTION_OBJECT* const a, SINT32) const
{
	a->pack(id);
	if (id) {
		a->pack(cell.ox);
		a->pack(cell.oy);
	}
}

void FULL_OBJ_ID::unpack(ACTION_OBJECT* const a, SINT32)
{
	a->unpack(id);
	cell.ox = 0;
	cell.oy = 0;
	objClass = 0;
	objType = 0;
	if (id) {
		a->unpack(cell.ox);
		a->unpack(cell.oy);
	}
}
