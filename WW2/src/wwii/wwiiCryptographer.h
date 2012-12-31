#pragma once
// Class that provides in-place, seeded cryptography of data.
// Copyright (C) PlayNet Inc, 2011.
// Originating Programmer: Oliver "kfs1" Smith

#if platOS == OS_WIN32
#pragma warning(disable:4127)	// Conditional expression is constant.
#endif

namespace WWII
{
//! @class WWII::Cryptographer
//! @brief Provides a proprietary encryption system.

template<bool Encrypting>
class Cryptographer
{
public:
	typedef unsigned long long SeedType ;
	typedef unsigned char ByteType ;

public:
	//! Default constructor.
	Cryptographer()
		: m_seed((SeedType)0)
		{
			newSeed() ;
		}

	//! Constructor: Requires a seed.
	Cryptographer(const SeedType& seed_)
		: m_seed(seed_)
		{
		}

	//! Get current seed value.
	const SeedType& seed() const { return m_seed ; }

	//! Set the seed.
	void setSeed(const SeedType& rhs_) { m_seed = rhs_ ; }

	//! Generate a new seed.
	void newSeed()
	{
		m_seed = teulClock64 + (((TeulTimeMS)rand()) << 32) + ((TeulTimeMS)rand()) ;
	}

private:
	//! Copy-operations disabled.
	Cryptographer& operator = (const Cryptographer&) ;

private:
	SeedType	m_seed ;

private:
	//! Rotate the seed some number of bits leftwards with
	//! wrapping so that the left-most bits wind up in the
	//! right most positions after the operation has been
	//! completed.
	//!
	//! We use the lower 3 bits of the value to vary the
	//! amount of adjustment.
	//!
	//! Finally, we xor the not of the adjustment into
	//! the seed.
	void
	_addValueToSeed(SeedType value)
	{
		const size_t bitsToMoveLeft = ((m_seed & 3) ^ (value & 3)) + 1 ;
		const size_t bitsToMoveRight = (sizeof(value) * 8) - bitsToMoveLeft ;
		value ^= 1ULL << (m_seed & 7) ;
		m_seed = ((m_seed << bitsToMoveLeft) | (m_seed >> bitsToMoveRight)) ^ (~value) ;
	}

	//! Xor the given value with some bits taken from seed,
	//! essentially either encoding or decoding it.
	ByteType
	_translateValue(ByteType source)
	{
		return source ^= (ByteType)(m_seed >> (m_seed & 7)) ;
	}

	//! Non-destructive processing of data to generate checksum.
	//! If Encrypting is true, will proceed as though the data
	//! is unencrypted, otherwise will proceed as though the
	//! data is already encrypted.
	void
	checksum(const ByteType* source, const size_t bytes)
	{
		_addValueToSeed((SeedType)bytes) ;

		for ( size_t i = 0 ; i < bytes ; ++i )
		{
			const ByteType sourceValue = *(source++) ;
			_addValueToSeed(Encrypting ? sourceValue : (_translateValue(sourceValue) ^ (ByteType)i)) ;
		}
	}

public:
	void
	translate(ByteType* const stream, const size_t bytes)
	{
		// Find the remainder of bytes modulo sizeof(SeedType)
		// and process those bytes individually.
		const size_t boundaryOffset = (bytes & (sizeof(SeedType) - 1)) ;
		if ( boundaryOffset > 0 )
		{
			ByteType* const tail = stream + bytes - boundaryOffset ;
			for ( size_t i = 0 ; i < boundaryOffset ; ++i )
			{
				const ByteType sourceValue = tail[i] ;
				const ByteType resultValue = sourceValue ^ ((ByteType)m_seed) ;
				tail[i] = resultValue ;
				_addValueToSeed(Encrypting ? sourceValue : resultValue) ;
			}
		}

		// Now encrypt the remainder of the message in big units
		// (units of SeedType bytes at a time).
		const size_t numUINT32s = (bytes - boundaryOffset) / sizeof(UINT32) ;
		if ( numUINT32s <= 0 )
			return ;

		// Incorporate the byte count into the seed.
		_addValueToSeed((SeedType)bytes) ;
		UINT32* const base = (UINT32*)stream ;
		UINT32 base0, base1 ;
		if ( Encrypting )
			base0 = ((SeedType)(base[0])) ;
		else
			base1 = ((SeedType)(base[numUINT32s - 1])) ;
		for ( size_t i = 0 ; i < numUINT32s ; i += sizeof(UINT32) )
		{
			base[i] ^= ((m_seed + i) >> (i & 31)) ;
		}
		if ( !Encrypting )
			base0 = ((SeedType)(base[0])) ;
		else
			base1 = ((SeedType)(base[numUINT32s - 1])) ;
		_addValueToSeed(base0) ;
		if ( base1 != base0 )
			_addValueToSeed(base1) ;
	}

} ;

}

#if platOS == OS_WIN32
#pragma warning(default:4127)
#endif
