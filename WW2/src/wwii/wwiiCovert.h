#pragma once
// Created: Feb 10th 2004 23:15:37 by Oliver "kfsone" Smith
//
// Copyright (c) 2004 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Implementation of "Encrypted" variables. At instantiation each variable
// assigns itself a random XOR bit mask which we then use for 'encrypting'
// the value.

// We never store the value in the clear, it's debatable whether storing
// the variable in the clear to detect tampering would lead hackers to
// this code to tamper with, or whether not being able to detect tampering
// may allow them to frig with this mechanism

// We internalize the value as a raw stream of (upto) 64 bits, so that we
// can handle complex types such as floats or even fullObjIDs. This requires
// us to carefully control where the type-conversions occur in both
// setting and accessing the value (see 'get' and 'set').

// The UINT64 disguise requires a lot of compiler-appeasement. It's syntactic
// sugar. The get and set routines are actually pretty efficient. Infact,
// we may actually want to make sure they are* not* inlined to avoid literring
// the code with those operations.

// In get and set we could displace the bit-ordering, we could even generate
// a random displacement value between 4 and 60 per "set":
//
//	get
//		t = (UINT64)(t >> 51) | (UINT64)(t << 13);	// Restore bit ordering
//	set
//		t = (UINT64)(t >> 13) | (UINT64)(t << 51); // Displace the bits
// This starts to add a little expense to reading the data, and also starts
// to produce executable code with a footprint that hackers might more
// readily detect and read.

// Originating Programmer: Oliver "kfsone" Smith

//#define COMPLEX_COVERT

#ifdef MULTI_CLIENT
#define DONT_USE_COVERT
#else
//#define DONT_USE_COVERT
//#define COVERT_USE_CHAOTIC_NUMBERS
#endif

#ifdef COVERT_USE_CHAOTIC_NUMBERS
#include "math/mathnum.h"
#endif

#ifdef DONT_USE_COVERT
template<typename Tp>
class Covert
{
public:
	Tp value;
	Tp Set(const Tp& rhs) noexcept { value = rhs; return value;}
	Tp Get(void) const noexcept { return value; }

	Covert(const Tp& rhs) noexcept : value(rhs) {}
	Covert() noexcept : value(0) {}

	// Cast to own type operator: so we can be used in most contexts
	operator Tp () const noexcept { return Get(); }

	// Copy operator
	const Tp operator=(const Covert& rhs) noexcept { return Set(rhs.Get()); }

	// Assignment operators
	const Tp operator=(const Tp& rhs)	noexcept { return Set(rhs); }
	const Tp operator+=(const Tp& rhs)	noexcept { return Set(Get() + rhs); }
	const Tp operator-=(const Tp& rhs)	noexcept { return Set(Get() - rhs); }
	const Tp operator*=(const Tp& rhs)	noexcept { return Set(Get() * rhs); }
	const Tp operator/=(const Tp& rhs)	noexcept { return Set(Get() / rhs); }
	const Tp operator<<=(const int rhs) noexcept { return Set(Get() << rhs); }
	const Tp operator>>=(const int rhs) noexcept { return Set(Get() >> rhs); }
	const Tp operator++() noexcept { return Set(Get()+1); }
	const Tp operator--() noexcept { return Set(Get()-1); }
	const Tp operator++(int) noexcept { return Set(Get()+1); }
	const Tp operator--(int) noexcept { return Set(Get()-1); }
	const Tp operator&=(const Tp& rhs) noexcept { return Set(Get() & rhs); }
	const Tp operator|=(const Tp& rhs) noexcept { return Set(Get() | rhs); }
	const Tp operator^=(const Tp& rhs) noexcept { return Set(Get() ^ rhs); }
	const Tp operator!() noexcept { return !Get(); }
};
#else
extern UINT32 wwiiGetUpdateNo(bool increment);
extern bool wwiiIncrementHits();
#include "wwii/wwiiOhYeahCodes.h"
extern void wwiiOhYeah(const UCHAR);

// Covert type class:
//  - Obscure values of static data
//  - Avoid predictable locations of data
//
// Usage:
//  Covert<REALTYPE> variable;
//  Beyond that you can mostly use the variables as
//  you did before.
//
// Caveats:
//  - Only works for data upto 64 bits long
//  - Complex datatypes must be cast to reference elements

template<typename Tp>
class Covert
{
	UINT64	m_mask;				// Encryption Mask for this instance
#ifdef RELOCATING_COVERT
	UINT64*	m_data;				// Pointer to the value
			// use "Deref()" to get a reference to the value store.
#else
	UINT64	m_data;
#endif
#ifdef COMPLEX_COVERT
	UINT64	m_exposed;			// Seemingly exposed version of the value
		// To stop the player from simply clamping a value,
		// we add a modifier that slowly increments over time.
	UINT16	m_hits;				// Slowly incrementing modifier
#endif

public:
	typedef Tp value_type;
	typedef Covert<value_type> self_type;

#ifdef RELOCATING_COVERT
	UINT64& Deref() noexcept { return *m_data; }
	const UINT64& Deref() const noexcept { return *m_data; }
#else
	UINT64& Deref() noexcept { return m_data; }
	const UINT64& Deref() const noexcept { return m_data; }
#endif

	// Function for setting a covert variable
	value_type Set(const value_type& rhs) noexcept
	{
		UINT64 t = ((uint64_t)rand() << 32) | rand(); // Deliberately not initialized.

		// Bit-copy RHS into the top of 't', leaving the
		// pre-existing stack-derived "crap" in the lower bits.
		*(value_type*)&t = rhs;

		// XOR the value with the 2s-compliment of the mask
		// Why? So that set(0) does not produce* m_addr == m_mask
		Deref() = t ^ ~m_mask;

#ifdef COMPLEX_COVERT
		if ( wwiiIncrementHits() )
		{
			// Increment mangler must be prime and 1 lower than a power of 2
			static const unsigned short int INCREMENT_MANGLING_PRIME = 127;	// So increments aren't predictable
			static const unsigned short int UNSIGN_MASK = (unsigned short int)~0x8000;
			m_hits = ((m_hits + (rand() & INCREMENT_MANGLING_PRIME)) & UNSIGN_MASK);
		}

		m_exposed = _exposeValue(t);
#endif

		// Return the naked value
		return rhs;
	}

	// Function for fetching a "covert" variable
	value_type Get(void) const
	{
		// Decrypt the value.
		UINT64 t = (Deref() ^ ~m_mask);

#ifdef COMPLEX_COVERT
		if ( m_exposed != _exposeValue(t) && (rand() & 15) == 0 )
		{
			wwiiGetUpdateNo();		// Bump the increment number for inconvenience to them
			wwiiOhYeah(OH_YEAH_COVERT);
		}
#endif
		return *(value_type*)&t;
	}

private:
	bool Equals(const self_type& rhs) const
	{
		const UINT64 lhsValue = (Deref() ^ ~m_mask);
		const UINT64 rhsValue = (rhs->Deref() ^ ~ rhs->m_mask);
		return *(value_type*)&lhsValue == *(value_type*)&rhsValue;
	}

	bool Equals(value_type rhsValue) const
	{
		const UINT64 lhsValue = (Deref() ^ ~m_mask);
		return (*(value_type*)&lhsValue) == rhsValue;
	}

public:
	// Cast to own type operator: so we can be used in most contexts
	operator value_type () const { return Get(); }

	// Copy operator
	const value_type operator=(const Covert& rhs) { return Set(rhs); }

	// Assignment operators
	const self_type& operator=(const value_type& rhs)	{  Set(rhs); return *this; }
	const self_type& operator+=(const value_type& rhs)	{  Set(Get() + rhs); return *this; }
	const self_type& operator-=(const value_type& rhs)	{  Set(Get() - rhs); return *this; }
	const self_type& operator*=(const value_type& rhs)	{  Set(Get() * rhs); return *this; }
	const self_type& operator/=(const value_type& rhs)	{  Set(Get() / rhs); return *this; }
	const self_type& operator<<=(const int rhs) {  Set(Get() << rhs); return *this; }
	const self_type& operator>>=(const int rhs) {  Set(Get() >> rhs); return *this; }
	const self_type& operator++() {  Set(Get()+1); return *this; }
	const self_type& operator--() {  Set(Get()-1); return *this; }
	const self_type& operator++(int) {  Set(Get()+1); return *this; }
	const self_type& operator--(int) {  Set(Get()-1); return *this; }
	const self_type& operator&=(const value_type& rhs) {  Set(Get() & rhs); return *this; }
	const self_type& operator|=(const value_type& rhs) {  Set(Get() | rhs); return *this; }
	const self_type& operator^=(const value_type& rhs) {  Set(Get() ^ rhs); return *this; }
	const value_type operator!() { return !Get(); }

	// Constructor with assignment
	Covert(const value_type& rhs)
		: m_mask(0LL)
		, m_data(0ULL)
#ifdef COMPLEX_COVERT
		, m_exposed(0LL), m_hits(0)
#endif
	{
		_initialize();
		Set(rhs);
	}

	// Constructor with no assignment
	Covert()
		: m_mask(0LL)
		, m_data(0ULL)
#ifdef COMPLEX_COVERT
		, m_exposed(0ULL), m_hits(0)
#endif
	{
		_initialize();
		// Since we can't assume Tp is a simple type,
		// we need to initialize* m_addr with a uint64
		Deref() = 0ULL ^ ~m_mask;
	}

	// Destructor
	~Covert() noexcept
	{
#ifdef RELOCATING_COVERT
		delete m_data;
#endif
	}

	///////////////////////////////////////////////////////////
	// Private assistant functions below here
private:
#ifdef COMPLEX_COVERT
	UINT64 inline _exposeValue(UINT64& t) const
	{
		return (UINT64)((SINT64)t + (SINT64)m_hits);
	}
#endif

	UINT32 _randomNumber() const noexcept
	{
#ifdef COVERT_USE_CHAOTIC_NUMBERS
		return (UINT32)mathGetChaoticNumber() << (rand() % 16) + rand();
#else
		return (UINT32)rand();
#endif
	}

	void _generateMask() noexcept
	{
		// The casting is necessary to stop truncation to 32 bits during all this
		static const int BIT_SIZE = (int)sizeof(UINT64) << 3;
		for ( int i = 0; i < BIT_SIZE; i += 32 )
		{
			m_mask ^= ((UINT64)_randomNumber()) << i;
		}
	}

	void _initialize()
	{
#ifdef COVERT_USE_CHAOTIC_NUMBERS
		// Ensure random seed has been set
		mathSeedRandomNumberGenerator();
#endif

		// Scramble memory allocations so that coverts
		// aren't stored in the same order each time.
#ifdef RELOCATING_COVERT
 #ifdef COVERT_USE_CHAOTIC_NUMBERS
		bool allocateFeint = (mathGetChaoticNumber() & 1);
 #else
		bool allocateFeint = (rand() & 1);
 #endif

		UINT64* feint = nullptr;
		try
		{
			if ( allocateFeint == true )
				feint = new UINT64;

			m_addr = new UINT64;
		}
		catch (std::exception& e)
		{
			if ( allocateFeint == true )
				delete feint;
			throw;
		}

		if ( allocateFeint == true )
			delete feint;
#endif

		_generateMask();
	}
};
#endif
