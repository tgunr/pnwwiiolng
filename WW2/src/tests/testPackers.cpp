#include <syscfg.h> // Path is project specific.
// Created: Wed Oct  1 23:44:02 CDT 2008
//
// Test harness for the stock teul packers
//
// Copyright (C) PlayNet, Inc 1999-2008 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "tests/testUnits.h"
#include "teul/teulPackers.h"
#include "teul/teulNetTimeMSPacker.h"
#include "updt/updtVehicleHit.h"		// For Vehicle::Hit
#include <sstream>

class TestPackers : public TestUnit
{
public:	TestPackers() : TestUnit("TestPackers") {}

	template<typename _Type>
	bool TYPE_TEST_FUNC(PackMode mode, ACTION_OBJECT& a, _Type initialValue)
	{
		_Type  value = (mode == TEUL_PACK) ? initialValue : 0;
		a.packer(mode, &value, 0);
		assert( value == initialValue );
		CRITICAL( a.offset == sizeof(_Type) * 8 );
		return true;
	}

	SINT32
	multiplePackerTestFunction(PackMode mode, UCHAR* data, SINT32 offset)
	{
		ACTION_OBJECT a;

		a.offset = offset;
		a.packed  = data;

		{
			TYPE_TEST_FUNC<SINT32>(mode, a, 0x87654321);
			TYPE_TEST_FUNC<UCHAR>(mode, a, 'f');
			TYPE_TEST_FUNC<SINT16>(mode, a, (SINT16)0x06f47);
			TYPE_TEST_FUNC<SINT16>(mode, a, (SINT16)(0x0ae93U));
			TYPE_TEST_FUNC<UINT16>(mode, a, (UINT16)0x0fa49U);
			TYPE_TEST_FUNC<UINT32>(mode, a, (UINT32)0x0af0e7bc5U);
		}

		{
			TYPE_TEST_FUNC<FLOAT32>(mode, a, -0.0001f);
			TYPE_TEST_FUNC<FLOAT32>(mode, a, 0.0001f);
			TYPE_TEST_FUNC<FLOAT32>(mode, a, -40394234.0f);
			TYPE_TEST_FUNC<FLOAT32>(mode, a, 19234.3245f);
			TYPE_TEST_FUNC<FLOAT64>(mode, a, -0.0000000001);
			TYPE_TEST_FUNC<FLOAT64>(mode, a, 0.0000000001);
			TYPE_TEST_FUNC<FLOAT64>(mode, a, -4023492348912.0);
			TYPE_TEST_FUNC<FLOAT64>(mode, a, 4023492348912.0);
			TYPE_TEST_FUNC<FLOAT64>(mode, a, 2340923.23423498);
		}

		{
			TYPE_TEST_FUNC<UINT64>(mode, a, 1ULL);
			TYPE_TEST_FUNC<UINT64>(mode, a, 1024ULL);
			TYPE_TEST_FUNC<UINT64>(mode, a, 0xf8a1c03372839405ULL);
			TYPE_TEST_FUNC<SINT64>(mode, a, 0);
			TYPE_TEST_FUNC<SINT64>(mode, a, 1);
			TYPE_TEST_FUNC<SINT64>(mode, a, -1);
			TYPE_TEST_FUNC<SINT64>(mode, a, 0x0012345678890123LL);
			TYPE_TEST_FUNC<SINT64>(mode, a, -0x0012345678890123LL);
		}

		return a.offset;
	}

	bool
	TeulNetTimeTest(ACTION_OBJECT& a, const TeulTimeMS testValue, const TeulTimeMS offset, SINT32 expectOffset)
	{
		START_SUB_STAGE( "TeulTimeMS::packer" )

			ResetClock();
			AdvanceClock_ms(offset);

			for ( SINT32 i = 0; i < 65; ++i )
			{
				TestFocus("Value %" PR64BitUT "ms, Clock %" PR64BitUT "ms, Offset %d", testValue, teulClock64, i);

				Teul::Packer<Teul::NetTimeMS> pack, unpack;
				memset(&pack.m_value, 0x7A, sizeof(pack.m_value));
				memset(&unpack.m_value, 0xA7, sizeof(unpack.m_value));

				a.offset = i;
				pack.m_value = testValue;
				pack.packer(TEUL_PACK, &a);
				const SINT32 packOffset = a.offset;

				CRITICAL( ExpectEqual( pack.m_value, testValue ) );
				CRITICAL( ExpectEqual( packOffset, expectOffset + i ) );

				a.offset = i;
				unpack.m_value = 0;
				unpack.packer(TEUL_UNPACK, &a);
				TeulTimeMS unpackValue = unpack.m_value;
				const SINT32 unpackOffset = a.offset;

				CRITICAL( ExpectEqual( unpackOffset, packOffset ) );
				CRITICAL( ExpectEqual( unpackValue, testValue ) );
			}

		END_SUB_STAGE( "TeulTimeMS::packer" )

		return true;
	}

	template<typename _Type>
	bool TestPacker(const char* name, _Type value, SINT32 packBits, SINT32 expectBits=0)
	{
		constexpr size_t MaxBits = 256;
		constexpr size_t SizeBits = sizeof(_Type) * 8;
		static_assert( SizeBits < MaxBits, "Type too large for buffer." );
		assert( (size_t)packBits < SizeBits && "Too many bits for type" );
		if (!expectBits)
			expectBits = packBits ? packBits : sizeof(_Type) * 8;

		ACTION_OBJECT a;
		a.packed  = (UCHAR*)alloca(MaxBits / 8);

		std::stringstream stageName;
		if (sizeof(_Type) > 1)
			stageName << name << '(' << value << ')';
		else
			stageName << name << '(' << int(value) << ')';
		START_SUB_STAGE( stageName.str().c_str() )
		for ( SINT32 offset = 0; offset <= 64; ++offset )
		{
			char focus[128];
			sprintf(focus, "offset %d bits %d=>%d", offset, packBits, expectBits);
			TestFocus(focus);

			memset(a.packed, 0xa5, MaxBits / 8);

			_Type out = value;
			_Type in  = (_Type)0;

			a.offset = offset;
			a.packer(TEUL_PACK, out, packBits);
			CRITICAL( ExpectEqual( a.offset, offset + expectBits ) );
			const SINT32 outOffset = a.offset;

			a.offset = offset;
			a.packer(TEUL_UNPACK, in, packBits);
			CRITICAL( ExpectEqual( a.offset, offset + expectBits ) );
			const SINT32 inOffset = a.offset;

			ExpectEqual( outOffset, inOffset );
			CRITICAL( ExpectEqual( out, in ) );
		}
		END_SUB_STAGE( stageName.str().c_str())
		return true;
	}

	template<typename T, std::enable_if_t<std::is_integral<T>::value, T> = 0>
	void TestBitRanges(const char* name, T value)
	{
		SINT32 min = std::is_signed<T>::value ? 2 : 1;
		for (SINT32 bits = min; bits <= SINT32(sizeof(T) / 8); ++bits) {
			TestPacker(name, value & ~(~0U << bits), bits);
		}
	}

public: virtual bool Run() override
	{
		START_TEST_STAGE( "Multiple Packers / Single Buffer" )
			UCHAR buffer[4096];
			memset(buffer, 0xA3, sizeof(buffer));

			SINT32 outOffset = 0, inOffset = 0;

			START_SUB_STAGE( "Packing to buffer" )
				outOffset = multiplePackerTestFunction(TEUL_PACK, buffer, 0);
				CRITICAL( ExpectTrue( outOffset > 0 ) );
			END_SUB_STAGE( "Unpacking to buffer" )

			START_SUB_STAGE( "Unpacking from buffer" )
				inOffset = multiplePackerTestFunction(TEUL_UNPACK, buffer, 0);
				CRITICAL( ExpectTrue( inOffset > 0 ) );
			END_SUB_STAGE( "Unpacking from buffer" )

			START_SUB_STAGE( "Match in/out offset" )
				CRITICAL( ExpectEqual( outOffset, inOffset ) );
			END_SUB_STAGE( "Match in/out offset" )

			START_SUB_STAGE( "SINT64 test" )
				SINT64 outValue = -167772180LL;
				SINT64 inValue = 0LL;
				teulPackSINT64(TEUL_PACK, buffer, 1, &outValue, 39);
				teulPackSINT64(TEUL_UNPACK, buffer, 1, &inValue, 39);
				CRITICAL( ExpectEqual( outValue, inValue ) );
			END_SUB_STAGE( "SINT64 test" )

		END_TEST_STAGE( "Multiple Packers / Single Buffer" )

		START_TEST_STAGE( "Teul Packers / Varying offsets" )

			// bool
			TestPacker<bool>("bool", false, 0, 1);
			TestPacker<bool>("bool", false, 1, 1);

			TestPacker<bool>("bool", true, 0, 1);
			TestPacker<bool>("bool", true, 1, 1);

			// unsigned char
			TestPacker<UCHAR>("UCHAR", 0U, 0);
			TestPacker<UCHAR>("UCHAR", 0x7fU, 0);
			TestPacker<UCHAR>("UCHAR", 0x80U, 0);
			TestPacker<UCHAR>("UCHAR", 0x81U, 0);
			TestPacker<UCHAR>("UCHAR", 0xffU, 0);

			TestPacker<UCHAR>("UCHAR", 0x0U, 1);
			TestPacker<UCHAR>("UCHAR", 0x1U, 1);
			TestPacker<UCHAR>("UCHAR", 0x5U, 5);

			// signed char
			TestPacker<SCHAR>("SCHAR", 0, 0);
			TestPacker<SCHAR>("SCHAR", 127, 0);
			TestPacker<SCHAR>("SCHAR", -1, 0);
			TestPacker<SCHAR>("SCHAR", -126, 0);

			TestPacker<SCHAR>("SCHAR", 0x0, 2);
			TestPacker<SCHAR>("SCHAR", 0x1, 2);
			TestPacker<SCHAR>("SCHAR", -0x1, 2);

			// unsigned 16-bit int
			TestPacker<UINT16>("UINT16", 0U, 0);
			TestPacker<UINT16>("UINT16", 0xffU, 0);
			TestPacker<UINT16>("UINT16", 0x100U, 0);
			TestPacker<UINT16>("UINT16", 0x101U, 0);
			TestPacker<UINT16>("UINT16", 0x7fffU, 0);
			TestPacker<UINT16>("UINT16", 0x8000U, 0);
			TestPacker<UINT16>("UINT16", 0x8003U, 0);
			TestPacker<UINT16>("UINT16", 0xf0f0U, 0);
			TestPacker<UINT16>("UINT16", 0xffffU, 0);
			TestPacker<UINT16>("UINT16", 0U, 1);
			TestPacker<UINT16>("UINT16", 0U, 3);
			TestPacker<UINT16>("UINT16", 1U, 3);
			TestPacker<UINT16>("UINT16", 29U, 7);

			TestBitRanges<UINT16>("UINT16", 0x1234);

			// signed 16-bit int
			TestPacker<SINT16>("SINT16", 0, 0);
			TestPacker<SINT16>("SINT16", 0xff, 0);
			TestPacker<SINT16>("SINT16", 0x100, 0);
			TestPacker<SINT16>("SINT16", 0x101, 0);
			TestPacker<SINT16>("SINT16", 0x7fff, 0);

			TestPacker<SINT16>("SINT16", 0, 2);
			TestPacker<SINT16>("SINT16", 1, 2);
			TestPacker<SINT16>("SINT16", -1, 2);
			TestPacker<SINT16>("SINT16", 0x3fff, 15);
			TestPacker<SINT16>("SINT16", -0x3fff, 15);

			TestBitRanges<SINT16>("SINT16", 0x1234);
			TestBitRanges<SINT16>("SINT16", -0x1234);

			// unsigned 32-bit int
			TestPacker<UINT32>("UINT32", 0U, 0);
			TestPacker<UINT32>("UINT32", 0xffU, 0);
			TestPacker<UINT32>("UINT32", 0x100U, 0);
			TestPacker<UINT32>("UINT32", 0x101U, 0);
			TestPacker<UINT32>("UINT32", 0x7fffU, 0);
			TestPacker<UINT32>("UINT32", 0x8000U, 0);
			TestPacker<UINT32>("UINT32", 0x8003U, 0);
			TestPacker<UINT32>("UINT32", 0xf0f0U, 0);
			TestPacker<UINT32>("UINT32", 0xffffU, 0);
			TestPacker<UINT32>("UINT32", 0x10000U, 0);
			TestPacker<UINT32>("UINT32", 0x100000U, 0);
			TestPacker<UINT32>("UINT32", 0x1000000U, 0);
			TestPacker<UINT32>("UINT32", 0x10000000U, 0);
			TestPacker<UINT32>("UINT32", 0x7fffffffU, 0);
			TestPacker<UINT32>("UINT32", 0x80000000U, 0);
			TestPacker<UINT32>("UINT32", 0x81234567U, 0);
			TestPacker<UINT32>("UINT32", 0xf0f0f0f0U, 0);
			TestPacker<UINT32>("UINT32", 0xffffffffU, 0);
			TestPacker<UINT32>("UINT32", 0xdeadbeefU, 0);

			TestPacker<UINT32>("UINT32", 0U, 1);
			TestPacker<UINT32>("UINT32", 1U, 1);
			TestPacker<UINT32>("UINT32", 2U, 3);
			TestPacker<UINT32>("UINT32", 0x0345678U, 25);

			TestBitRanges<UINT32>("UINT32", 0x12345678);

			// signed 32-bit int
			TestPacker<SINT32>("SINT32", 0, 0);
			TestPacker<SINT32>("SINT32", 0xff, 0);
			TestPacker<SINT32>("SINT32", 0x101, 0);
			TestPacker<SINT32>("SINT32", 0x7fff, 0);
			TestPacker<SINT32>("SINT32", 0x8000, 0);
			TestPacker<SINT32>("SINT32", 0x8003, 0);
			TestPacker<SINT32>("SINT32", 0xf0f0, 0);
			TestPacker<SINT32>("SINT32", 0xffff, 0);
			TestPacker<SINT32>("SINT32", 0x7fffffff, 0);
			TestPacker<SINT32>("SINT32", 0x80000000, 0);
			TestPacker<SINT32>("SINT32", 0x81234567, 0);
			TestPacker<SINT32>("SINT32", 0xf0f0f0f0, 0);
			TestPacker<SINT32>("SINT32", 0xffffffff, 0);
			TestPacker<SINT32>("SINT32", 0xdeadbeef, 0);

			TestPacker<SINT32>("SINT32", 0, 2);
			TestPacker<SINT32>("SINT32", 0, 2);
			TestPacker<SINT32>("SINT32", 1, 2);
			TestPacker<SINT32>("SINT32", -1, 2);
			TestPacker<SINT32>("SINT32", 0xffff, 17);

			TestBitRanges<SINT32>("SINT32", 0x12345678);
			TestBitRanges<SINT32>("SINT32", -0x12345678);

			// unsigned 64-bit int
			TestPacker<UINT64>("UINT64", 0ULL, 0);
			TestPacker<UINT64>("UINT64", 0x1ULL, 1);
			TestPacker<UINT64>("UINT64", 0xfffffffULL, 28);
			TestPacker<UINT64>("UINT64", 0ULL, 0);
			TestPacker<UINT64>("UINT64", 0xffULL, 0);
			TestPacker<UINT64>("UINT64", 0x101ULL, 0);
			TestPacker<UINT64>("UINT64", 0x7fffULL, 0);
			TestPacker<UINT64>("UINT64", 0x8000ULL, 0);
			TestPacker<UINT64>("UINT64", 0x8003ULL, 0);
			TestPacker<UINT64>("UINT64", 0xf0f0ULL, 0);
			TestPacker<UINT64>("UINT64", 0xffffULL, 0);
			TestPacker<UINT64>("UINT64", 0x7fffffffULL, 0);
			TestPacker<UINT64>("UINT64", 0x80000000ULL, 0);
			TestPacker<UINT64>("UINT64", 0x81234567ULL, 0);
			TestPacker<UINT64>("UINT64", 0xf0f0f0f0ULL, 0);
			TestPacker<UINT64>("UINT64", 0xffffffffULL, 0);
			TestPacker<UINT64>("UINT64", 0xdeadbeefULL, 0);
			TestPacker<UINT64>("UINT64", 0xf00dbeeffeedULL, 0);
			TestPacker<UINT64>("UINT64", 0xfeeddeadbeeff00dULL, 0);

			TestBitRanges<UINT64>("UINT64", 0x1234567890abcdefULL);
			TestBitRanges<UINT64>("UINT64", 0x1f2e3d4c5b6a7089ULL);

			// signed 64-bit int
			TestPacker<SINT64>("SINT64", 0, 0);
			TestPacker<SINT64>("SINT64", 0xffLL, 0);
			TestPacker<SINT64>("SINT64", -0xffLL, 0);
			TestPacker<SINT64>("SINT64", 0x100LL, 0);
			TestPacker<SINT64>("SINT64", -0x100LL, 0);
			TestPacker<SINT64>("SINT64", 0x101LL, 0);
			TestPacker<SINT64>("SINT64", -0x101LL, 0);
			TestPacker<SINT64>("SINT64", 0x7fffLL, 0);
			TestPacker<SINT64>("SINT64", -0x7fffLL, 0);
			TestPacker<SINT64>("SINT64", 0x8000LL, 0);
			TestPacker<SINT64>("SINT64", -0x8000LL, 0);
			TestPacker<SINT64>("SINT64", 0x8003LL, 0);
			TestPacker<SINT64>("SINT64", -0x8003LL, 0);
			TestPacker<SINT64>("SINT64", 0xf0f0LL, 0);
			TestPacker<SINT64>("SINT64", -0xf0f0LL, 0);
			TestPacker<SINT64>("SINT64", 0xffffLL, 0);
			TestPacker<SINT64>("SINT64", -0xffffLL, 0);
			TestPacker<SINT64>("SINT64", 0x10000LL, 0);
			TestPacker<SINT64>("SINT64", -0x10000LL, 0);
			TestPacker<SINT64>("SINT64", 0x100000LL, 0);
			TestPacker<SINT64>("SINT64", -0x100000LL, 0);
			TestPacker<SINT64>("SINT64", 0x1000000LL, 0);
			TestPacker<SINT64>("SINT64", -0x1000000LL, 0);
			TestPacker<SINT64>("SINT64", 0x10000000LL, 0);
			TestPacker<SINT64>("SINT64", -0x10000000LL, 0);
			TestPacker<SINT64>("SINT64", 0x7fffffffLL, 0);
			TestPacker<SINT64>("SINT64", -0x7fffffffLL, 0);
			TestPacker<SINT64>("SINT64", 0x80000000LL, 0);
			TestPacker<SINT64>("SINT64", -0x80000000LL, 0);
			TestPacker<SINT64>("SINT64", 0x81234567LL, 0);
			TestPacker<SINT64>("SINT64", -0x81234567LL, 0);
			TestPacker<SINT64>("SINT64", 0xf0f0f0f0LL, 0);
			TestPacker<SINT64>("SINT64", -0xf0f0f0f0LL, 0);
			TestPacker<SINT64>("SINT64", 0xffffffffLL, 0);
			TestPacker<SINT64>("SINT64", -0xffffffffLL, 0);
			TestPacker<SINT64>("SINT64", 0xdeadbeefLL, 0);
			TestPacker<SINT64>("SINT64", -0xdeadbeefLL, 0);
			TestPacker<SINT64>("SINT64", 0xf00dbeeffeedLL, 0);
			TestPacker<SINT64>("SINT64", -0xf00dbeeffeedLL, 0);
			TestPacker<SINT64>("SINT64", 0xfeeddeadbeeff00dLL, 0);
			TestPacker<SINT64>("SINT64", -0xfeeddeadbeeff00dLL, 0);

			// can't pack a signed value in 1 bit.
			TestPacker<SINT64>("SINT64", 0, 2);
			TestPacker<SINT64>("SINT64", 0, 31);
			TestPacker<SINT64>("SINT64", 0, 32);
			TestPacker<SINT64>("SINT64", 0, 33);
			TestPacker<SINT64>("SINT64", 0, 63);

			TestPacker<SINT64>("SINT64", 1, 2);
			TestPacker<SINT64>("SINT64", 1, 31);
			TestPacker<SINT64>("SINT64", 1, 32);
			TestPacker<SINT64>("SINT64", 1, 33);
			TestPacker<SINT64>("SINT64", 1, 63);

			TestPacker<SINT64>("SINT64", -1, 2);
			TestPacker<SINT64>("SINT64", -1, 31);
			TestPacker<SINT64>("SINT64", -1, 32);
			TestPacker<SINT64>("SINT64", -1, 33);
			TestPacker<SINT64>("SINT64", -1, 63);

			TestPacker<SINT64>("SINT64", -2, 2);
			TestPacker<SINT64>("SINT64", -2, 31);
			TestPacker<SINT64>("SINT64", -2, 32);
			TestPacker<SINT64>("SINT64", -2, 33);
			TestPacker<SINT64>("SINT64", -2, 63);

			TestBitRanges<SINT64>("SINT64", 0x1234567890abcdefLL);
			TestBitRanges<SINT64>("SINT64", 0x1f2e3d4c5b6a7089LL);

			TestBitRanges<SINT64>("SINT64", -0x1234567890abcdefLL);
			TestBitRanges<SINT64>("SINT64", -0x1f2e3d4c5b6a7089LL);

			// 32-bit float
			TestPacker<FLOAT32>("FLOAT32", 0.0f, 0);
			TestPacker<FLOAT32>("FLOAT32", 1.0f, 0);
			TestPacker<FLOAT32>("FLOAT32", -1.0f, 0);
			TestPacker<FLOAT32>("FLOAT32", 0.0001f, 0);
			TestPacker<FLOAT32>("FLOAT32", -0.0001f, 0);
			TestPacker<FLOAT32>("FLOAT32", 1000.0f, 0);
			TestPacker<FLOAT32>("FLOAT32", -1000.0f, 0);
			TestPacker<FLOAT32>("FLOAT32", 1000.0001f, 0);
			TestPacker<FLOAT32>("FLOAT32", -1000.0001f, 0);
			TestPacker<FLOAT32>("FLOAT32", 65535.65535f, 0);
			TestPacker<FLOAT32>("FLOAT32", -65535.65535f, 0);
			TestPacker<FLOAT32>("FLOAT32", float(0xdeadbeef), 0);

			// 64-bit float
			TestPacker<FLOAT64>("FLOAT64", 0., 0);
			TestPacker<FLOAT64>("FLOAT64", 1., 0);
			TestPacker<FLOAT64>("FLOAT64", -1., 0);
			TestPacker<FLOAT64>("FLOAT64", 0.0001, 0);
			TestPacker<FLOAT64>("FLOAT64", -0.0001, 0);
			TestPacker<FLOAT64>("FLOAT64", 1000., 0);
			TestPacker<FLOAT64>("FLOAT64", -1000., 0);
			TestPacker<FLOAT64>("FLOAT64", 1000.0001, 0);
			TestPacker<FLOAT64>("FLOAT64", -1000.0001, 0);
			TestPacker<FLOAT64>("FLOAT64", 65535.65535, 0);
			TestPacker<FLOAT64>("FLOAT64", -65535.65535, 0);
			TestPacker<FLOAT64>("FLOAT64", 0xdeadbeef, 0);
			TestPacker<FLOAT64>("FLOAT64", (1LL << 36) - 1, 0);

		END_TEST_STAGE( "Teul Packers / Varying offsets" )

		START_TEST_STAGE( "Teul::Packer Tests" )

			static const TeulTimeMS SmallValue = 2020ULL;
			static const TeulTimeMS Boundary = (1ULL << (Teul::NetTimeMS::SmallDataBits() - 1ULL)) * Teul::NetTimeMS::Granularity();
			static const TeulTimeMS LargeValue = SmallValue + Boundary + Teul::NetTimeMS::Granularity();

			ACTION_OBJECT a;
			UCHAR msg[800];
			a.packed = msg;
			TestFocus("Small Value");
			CRITICAL( TeulNetTimeTest(a, SmallValue, 0, Teul::NetTimeMS::SmallDataBits()) );
			CRITICAL( TeulNetTimeTest(a, SmallValue, 20, Teul::NetTimeMS::SmallDataBits()) );
			CRITICAL( TeulNetTimeTest(a, SmallValue, LargeValue, Teul::NetTimeMS::LargeDataBits()) );

			TestFocus("Large Value");
			CRITICAL( TeulNetTimeTest(a, LargeValue, 0, Teul::NetTimeMS::LargeDataBits()) );
			CRITICAL( TeulNetTimeTest(a, LargeValue, 20, Teul::NetTimeMS::LargeDataBits()) );

			TeulNetTimeTest(a, SmallValue + SmallValue, SmallValue, Teul::NetTimeMS::SmallDataBits());
			TeulNetTimeTest(a, SmallValue + LargeValue, SmallValue, Teul::NetTimeMS::LargeDataBits());
			TeulNetTimeTest(a, SmallValue + LargeValue, LargeValue, Teul::NetTimeMS::SmallDataBits());
			TeulNetTimeTest(a, LargeValue + LargeValue, SmallValue, Teul::NetTimeMS::LargeDataBits());
			TeulNetTimeTest(a, LargeValue + LargeValue, LargeValue, Teul::NetTimeMS::LargeDataBits());

		END_TEST_STAGE( "Teul::Packer Tests" )

		START_TEST_STAGE( "Vehicle::Hit Packers" )

			static const UINT32 shooterID = 1122303;
			static const UINT32 victimID  = 3204;

			UCHAR msg[1500];

			Vehicle::Hit hit(victimID);

			CRITICAL( ExpectEqual( hit.m_timestamp, teulClock64 ) );
			CRITICAL( ExpectEqual( hit.m_shooter, 0 ) );
			CRITICAL( ExpectEqual( hit.m_target, victimID ) );
			CRITICAL( ExpectEqual( hit.m_components.size(), 0 ) );
			CRITICAL( ExpectEqual( hit.m_components.capacity(), (size_t)VEH_MAX_OBJ_HIT ) );

			SINT32 offset;

			START_SUB_STAGE( "Empty hit pack" )

				ACTION_OBJECT out;
				out.packed = msg;
				out.offset = 0;
				out.localRawData = nullptr;

				offset = hit.packer(TEUL_PACK, &out);

				CRITICAL( ExpectEqual( offset, 72 ) );

			END_SUB_STAGE( "Empty hit pack" )

			START_SUB_STAGE( "Empty hit unpack" )

				ACTION_OBJECT in(msg, 0, offset);
				CRITICAL( ExpectEqual( in.packed, msg ) );
				CRITICAL( ExpectEqual( in.offset, 0 ) );
				CRITICAL( ExpectEqual( in.sizeInBytes(), offset ) );

				Vehicle::Hit rhit(&in);

				CRITICAL( ExpectEqual( rhit.m_timestamp, teulClock64 ) );
				CRITICAL( ExpectEqual( rhit.m_shooter, hit.m_shooter ) );
				CRITICAL( ExpectEqual( rhit.m_target, hit.m_target ) );
				CRITICAL( ExpectEqual( rhit.m_components.size(), hit.m_components.size() ) );
				CRITICAL( ExpectEqual( rhit.m_components.capacity(), hit.m_components.capacity() ) );

			END_SUB_STAGE( "Empty hit unpack" )

			Vehicle::Hit::Component comp1(1, true, 1, 1ULL);
			Vehicle::Hit::Component comp2(2, false, 2, 0xfffffULL);
			Vehicle::Hit::Component comp3(4, true, 6, 0xfeab1234ca341010ULL);

			START_SUB_STAGE( "Pack with hits" )

				hit.m_components.push_back(comp1);
				CRITICAL( ExpectEqual( hit.m_components.size(), 1 ) );

				hit.m_components.push_back(comp2);
				hit.m_components.push_back(comp3);
				CRITICAL( ExpectEqual( hit.m_components.size(), 3 ) );

				hit.m_shooter = shooterID;

				ACTION_OBJECT out;
				out.packed = msg;
				out.offset = 0;
				out.localRawData = nullptr;

				offset = hit.packer(TEUL_PACK, &out);

				CRITICAL( ExpectEqual( offset, 72 + ( 7 + 1 + 6 + 2 + 16) + ( 7 + 1 + 6 + 2 + 32) + ( 7 + 1 + 6 + 2 + 64 ) ) );

			END_SUB_STAGE( "Pack with hits" )

			START_SUB_STAGE( "Unpack with hits" )

				ACTION_OBJECT in(msg, 0, offset);
				Vehicle::Hit rhit(&in);

				CRITICAL( ExpectEqual( rhit.m_timestamp, teulClock64 ) );
				CRITICAL( ExpectEqual( rhit.m_shooter, hit.m_shooter ) );
				CRITICAL( ExpectEqual( rhit.m_target, hit.m_target ) );
				CRITICAL( ExpectEqual( rhit.m_components.size(), hit.m_components.size() ) );
				CRITICAL( ExpectEqual( rhit.m_components.capacity(), hit.m_components.capacity() ) );

				CRITICAL( ExpectTrue( rhit.m_components[0] == hit.m_components[0] ) );
				CRITICAL( ExpectTrue( rhit.m_components[1] == hit.m_components[1] ) );
				CRITICAL( ExpectTrue( rhit.m_components[2] == hit.m_components[2] ) );

				CRITICAL( ExpectTrue( rhit.m_components[0] == comp1 ) );
				CRITICAL( ExpectTrue( rhit.m_components[1] == comp2 ) );
				CRITICAL( ExpectTrue( rhit.m_components[2] == comp3 ) );

			END_SUB_STAGE( "Unpack with hits" )

		END_TEST_STAGE( "Vehicle::Hit Packers" )

		return true;
	}
};

static TestPackers test;

