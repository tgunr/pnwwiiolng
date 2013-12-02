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

class TestPackers : public TestUnit
{
public:	TestPackers() : TestUnit("TestPackers") {}

	template<typename _Type>
	bool TYPE_TEST_FUNC(LOGICAL mode, ACTION_OBJECT& a, _Type initialValue)
	{
		_Type  value = (mode == TEUL_PACK) ? initialValue : 0;
		a.packer(mode, &value, 0);
		assert( value == initialValue );
		CRITICAL( a.offset == sizeof(_Type) * 8 );
		return true;
	}

	SINT32
	multiplePackerTestFunction(LOGICAL mode, UCHAR* data, SINT32 offset)
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
				TestFocus("Value %llums, Clock %llums, Offset %d", testValue, teulClock64, i);

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
	bool
	TEST_FUNC(const char name[], std::vector<std::pair<_Type, size_t>>& tests)
	{
		static const size_t MaxBits = 256;
		static_assert( (sizeof(_Type) * 8) + 64 < MaxBits, "Type too large for buffer." );

		START_TEST_STAGE( name )
			ACTION_OBJECT a;
			a.packed  = (UCHAR*)alloca(MaxBits / 8);

			size_t testNo = 1;
			for ( auto it = tests.begin(); it != tests.end(); ++it, ++testNo )
			{
				for ( SINT32 i = 0; i <= 64; ++i )
				{
					const _Type& value = it->first;
					const SINT32 bits  = it->second;

					char focus[128];
					sprintf(focus, "test #%u off %d", testNo, bits);
					TestFocus(focus);

					memset(a.packed, 0xa5, MaxBits / 8);

					_Type out = value;
					_Type in  = (_Type)0;

					a.offset = i;
					a.packer(TEUL_PACK, out, bits);
					CRITICAL( ExpectEqual( a.offset, i + bits ) );
					const SINT32 outOffset = a.offset;

					a.offset = i;
					a.packer(TEUL_UNPACK, in, bits);
					CRITICAL( ExpectEqual( a.offset, i + bits ) );
					const SINT32 inOffset = a.offset;

					ExpectEqual( outOffset, inOffset );
					CRITICAL( ExpectEqual( out, in ) );
				}
			}
		END_TEST_STAGE( name )

		return true;
	}

public: virtual bool Run()
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

#ifdef GCC_4_5
			TEST_FUNC<UCHAR>("UCHAR", {
					{ 0, 0 }
				,	{ 0x7f, 0 }
				,	{ 0x80, 0 }
				,	{ 0x81, 0 }
				,	{ 0xff, 0 }

				,	{ 0x0, 1 }
				,	{ 0x1, 1 }
				,	{ 0x5, 5 }
			});

			TEST_FUNC<SCHAR>("SCHAR", {
				{ 0, 0 }
			,	{ 0x7f, 0 }
			,	{ 0x80, 0 }
			,	{ 0x81, 0 }
			,	{ 0xff, 0 }

			,	{ 0x0, 2 }
			,	{ 0x1, 2 }
			,	{ -0x1, 2 }
			});

			TEST_FUNC<UINT16>("UINT16", {
					{ 0, 0 }
				,	{ 0xff, 0 }
				,	{ 0x100, 0 }
				,	{ 0x101, 0 }
				,	{ 0x7fff, 0 }
				,	{ 0x8000, 0 }
				,	{ 0x8003, 0 }
				,	{ 0xf0f0, 0 }
				,	{ 0xffff, 0 }
				,	{ 0, 1 }
				,	{ 0, 3 }
				,	{ 1, 3 }
				,	{ 29, 7 }
			});

			TEST_FUNC<SINT16>("SINT16", {
					{ 0, 0 }
				,	{ 0xff, 0 }
				,	{ 0x100, 0 }
				,	{ 0x101, 0 }
				,	{ 0x7fff, 0 }
				,	{ 0x8000, 0 }
				,	{ 0x8003, 0 }
				,	{ 0xf0f0, 0 }
				,	{ 0xffff, 0 }

				,	{ 0, 1 }
				,	{ 1, 2 }
				,	{ -1, 2 }
				,	{ 0x3fff, 15 }
				,	{ -0x3fff, 15 }
			});

			TEST_FUNC<UINT32>("UINT32", {
					{ 0, 0 }
				,	{ 0xff, 0 }
				,	{ 0x100, 0 }
				,	{ 0x101, 0 }
				,	{ 0x7fff, 0 }
				,	{ 0x8000, 0 }
				,	{ 0x8003, 0 }
				,	{ 0xf0f0, 0 }
				,	{ 0xffff, 0 }
				,	{ 0x10000, 0 }
				,	{ 0x100000, 0 }
				,	{ 0x1000000, 0 }
				,	{ 0x10000000, 0 }
				,	{ 0x7fffffff, 0 }
				,	{ 0x80000000, 0 }
				,	{ 0x81234567, 0 }
				,	{ 0xf0f0f0f0, 0 }
				,	{ 0xffffffff, 0 }
				,	{ 0xdeadbeef, 0 }

				,	{ 0, 1 }
				,	{ 1, 1 }
				,	{ 2, 3 }
				,	{ 0x0345678, 25 }
			});

			TEST_FUNC<SINT32>("SINT32", {
					{ 0, 0 }
				,	{ 0xff, 0 }
				,	{ 0x100, 0 }
				,	{ 0x101, 0 }
				,	{ 0x7fff, 0 }
				,	{ 0x8000, 0 }
				,	{ 0x8003, 0 }
				,	{ 0xf0f0, 0 }
				,	{ 0xffff, 0 }
				,	{ 0x10000, 0 }
				,	{ 0x100000, 0 }
				,	{ 0x1000000, 0 }
				,	{ 0x10000000, 0 }
				,	{ 0x7fffffff, 0 }
				,	{ 0x80000000, 0 }
				,	{ 0x81234567, 0 }
				,	{ 0xf0f0f0f0, 0 }
				,	{ 0xffffffff, 0 }
				,	{ 0xdeadbeef, 0 }

				,	{ 0, 1 }
				,	{ 0, 2 }
				,	{ 1, 2 }
				,	{ -1, 2 }
				,	{ 0xffff, 17 }
			});

			TEST_FUNC<UINT64>("UINT64", {
					{ 0, 0 }
				,	{ 0xff, 0 }
				,	{ 0x100, 0 }
				,	{ 0x101, 0 }
				,	{ 0x7fff, 0 }
				,	{ 0x8000, 0 }
				,	{ 0x8003, 0 }
				,	{ 0xf0f0, 0 }
				,	{ 0xffff, 0 }
				,	{ 0x10000, 0 }
				,	{ 0x100000, 0 }
				,	{ 0x1000000, 0 }
				,	{ 0x10000000, 0 }
				,	{ 0x7fffffff, 0 }
				,	{ 0x80000000, 0 }
				,	{ 0x81234567, 0 }
				,	{ 0xf0f0f0f0, 0 }
				,	{ 0xffffffff, 0 }
				,	{ 0xdeadbeef, 0 }
				,	{ 0xf00dbeeffeedLL, 0 }
				,	{ 0xfeeddeadbeeff00dLL, 0 }

				,	{ 0x1, 1 }
				,	{ 0xfffffff, 28 }
			});

			TEST_FUNC<SINT64>("SINT64", {
					{ 0, 0 }
				,	{ 0xff, 0 }
				,	{ -0xff, 0 }
				,	{ 0x100, 0 }
				,	{ -0x100, 0 }
				,	{ 0x101, 0 }
				,	{ -0x101, 0 }
				,	{ 0x7fff, 0 }
				,	{ -0x7fff, 0 }
				,	{ 0x8000, 0 }
				,	{ -0x8000, 0 }
				,	{ 0x8003, 0 }
				,	{ -0x8003, 0 }
				,	{ 0xf0f0, 0 }
				,	{ -0xf0f0, 0 }
				,	{ 0xffff, 0 }
				,	{ -0xffff, 0 }
				,	{ 0x10000, 0 }
				,	{ -0x10000, 0 }
				,	{ 0x100000, 0 }
				,	{ -0x100000, 0 }
				,	{ 0x1000000, 0 }
				,	{ -0x1000000, 0 }
				,	{ 0x10000000, 0 }
				,	{ -0x10000000, 0 }
				,	{ 0x7fffffff, 0 }
				,	{ -0x7fffffff, 0 }
				,	{ 0x80000000, 0 }
				,	{ -0x80000000, 0 }
				,	{ 0x81234567, 0 }
				,	{ -0x81234567, 0 }
				,	{ 0xf0f0f0f0, 0 }
				,	{ -0xf0f0f0f0, 0 }
				,	{ 0xffffffff, 0 }
				,	{ -0xffffffff, 0 }
				,	{ 0xdeadbeef, 0 }
				,	{ -0xdeadbeef, 0 }
				,	{ 0xf00dbeeffeedLL, 0 }
				,	{ -0xf00dbeeffeedLL, 0 }
				,	{ 0xfeeddeadbeeff00dLL, 0 }
				,	{ -0xfeeddeadbeeff00dLL, 0 }

				,	{ 0, 1 }
				,	{ 0, 2 }
				,	{ 0, 31 }
				,	{ 0, 32 }
				,	{ 0, 33 }
				,	{ 0, 63 }

				,	{ 1, 2 }
				,	{ 1, 31 }
				,	{ 1, 32 }
				,	{ 1, 33 }
				,	{ 1, 63 }

				,	{ -1, 2 }
				,	{ -1, 31 }
				,	{ -1, 32 }
				,	{ -1, 33 }
				,	{ -1, 63 }
			});

			TEST_FUNC<FLOAT32>("FLOAT32", {
					{ 0, 0 }
				,	{ 1, 0 }
				,	{ -1, 0 }
				,	{ 0.0001, 0 }
				,	{ -0.0001, 0 }
				,	{ 1000, 0 }
				,	{ -1000, 0 }
				,	{ 1000.0001, 0 }
				,	{ -1000.0001, 0 }
				,	{ 65535.65535, 0 }
				,	{ -65535.65535, 0 }
				,	{ 0xdeadbeef, 0 }
			});

			TEST_FUNC<FLOAT64>("FLOAT64", {
					{ 0, 0 }
				,	{ 1, 0 }
				,	{ -1, 0 }
				,	{ 0.0001, 0 }
				,	{ -0.0001, 0 }
				,	{ 1000, 0 }
				,	{ -1000, 0 }
				,	{ 1000.0001, 0 }
				,	{ -1000.0001, 0 }
				,	{ 65535.65535, 0 }
				,	{ -65535.65535, 0 }
				,	{ 0xdeadbeef, 0 }
				,	{ (1LL << 36) - 1, 0 }
			});
#endif

		END_TEST_STAGE( "Teul Packers / Varying offsets" )

		START_TEST_STAGE( "Teul::FitBits Tests" )

			START_SUB_STAGE( "Unsigned" )
				for ( UINT32 i = 0; i < 64; ++i )
				{
					{
						bool fit = Teul::FitUnsignedBitValue((1ULL << i) - 1, 15);
						bool expectFit = i <= 15;
						CRITICAL( ExpectEqual( fit, expectFit ) );
					}
					{
						bool fit = Teul::FitUnsignedBitValue((1ULL << i), 15);
						bool expectFit = i < 15;
						CRITICAL( ExpectEqual( fit, expectFit ) );
					}
					{
						bool fit = Teul::FitUnsignedBitValue((1ULL << i) + 1, 15);
						bool expectFit = i < 15;
						CRITICAL( ExpectEqual( fit, expectFit ) );
					}
				}
			END_SUB_STAGE( "Unsigned" )

			START_SUB_STAGE( "Signed" )
				for ( UINT32 i = 1; i < 64; ++i )
				{
					{
						TestFocus("-(%u - 1) for %u", 1 << i, i);
						bool fit = Teul::FitSignedBitValue(-((1LL << i) - 1), 15);
						bool expectFit = i <= 14;
						CRITICAL( ExpectEqual( fit, expectFit ) );
					}
					{
						TestFocus("-(%u) for %u", 1 << i, i);
						bool fit = Teul::FitSignedBitValue(-((1LL << i)), 15);
						bool expectFit = i <= 14;
						CRITICAL( ExpectEqual( fit, expectFit ) );
					}
					{
						TestFocus("-(%u + 1) for %u", 1 << i, i);
						bool fit = Teul::FitSignedBitValue(-((1LL << i) + 1), 15);
						bool expectFit = i < 14;
						CRITICAL( ExpectEqual( fit, expectFit ) );
					}
				}
				{
					TestFocus("40 bits");
					bool fitPositive = Teul::FitSignedBitValue((1LL << 39) - 1LL, 40);
					CRITICAL( ExpectTrue( fitPositive ) );
					bool fitNegative = Teul::FitSignedBitValue(-(1LL << 39), 40);
					CRITICAL( ExpectTrue( fitNegative ) );
				}
			END_SUB_STAGE( "Signed" )
		END_TEST_STAGE( "Teul::FitBits" )

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
				out.localRawData = NULL;

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
				out.localRawData = NULL;

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

