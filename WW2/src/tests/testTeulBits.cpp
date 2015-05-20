#include <syscfg.h> // Path is project specific.
#include "tests/testUnits.h"
// Test module for Teul::Bits

#include "teul/teulBits.h"

class TeulBitsTest : public TestUnit
{
public:
	TeulBitsTest() : TestUnit("TeulBits") {}

public: virtual bool Run() OVERRIDE_FN
	{
		START_TEST_STAGE( "Bits::Size" );

			CRITICAL( ExpectEqual(Teul::Bits::Size<UCHAR>(), 8) );
			CRITICAL( ExpectEqual(Teul::Bits::Size<SCHAR>(), 8) );
			CRITICAL( ExpectEqual(Teul::Bits::Size<UINT16>(), 16) );
			CRITICAL( ExpectEqual(Teul::Bits::Size<SINT16>(), 16) );
			CRITICAL( ExpectEqual(Teul::Bits::Size<UINT32>(), 32) );
			CRITICAL( ExpectEqual(Teul::Bits::Size<SINT32>(), 32) );
			CRITICAL( ExpectEqual(Teul::Bits::Size<UINT64>(), 64) );
			CRITICAL( ExpectEqual(Teul::Bits::Size<SINT64>(), 64) );

		END_TEST_STAGE( "Bits::Size" );

		TestBitsCount();
		TestBitsFit();

		return true;
	}

public: bool TestBitsCount()
	{
#define EXPECT_FIT(val_type, val, bit_count) \
	CRITICAL( ExpectTrue( Teul::Bits::Fit<val_type>(val_type(val), bit_count) ) ) 

#define EXPECT_NOT(val_type, val, bit_count) \
	CRITICAL( ExpectFalse( Teul::Bits::Fit<val_type>(val_type(val), bit_count) ) ) 

		START_TEST_STAGE( "TestBitsCount" )

			EXPECT_NOT(uint32_t, 0, 0);
			EXPECT_NOT(uint32_t, 1, 0);
			EXPECT_NOT(uint32_t, 2, 0);
			EXPECT_NOT(uint32_t, 3, 0);
			EXPECT_NOT(uint32_t, 2, 1);
			EXPECT_NOT(uint32_t, 3, 1);
			EXPECT_NOT(uint32_t, 4, 1);
			EXPECT_NOT(uint32_t, 5, 1);
			EXPECT_NOT(uint32_t, 6, 1);
			EXPECT_NOT(uint32_t, 7, 1);
			EXPECT_NOT(uint32_t, 8, 1);
			EXPECT_NOT(uint32_t, 4, 2);
			EXPECT_NOT(uint32_t, 5, 2);
			EXPECT_NOT(uint32_t, 6, 2);
			EXPECT_NOT(uint32_t, 7, 2);
			EXPECT_NOT(uint32_t, 8, 2);
			EXPECT_NOT(uint32_t, 8, 3);
			EXPECT_NOT(uint32_t, 255, 3);
			EXPECT_NOT(uint32_t, 256, 3);
			EXPECT_NOT(uint32_t, 256, 8);
			EXPECT_NOT(uint32_t, ~0, 1);
			EXPECT_NOT(uint32_t, ~0, 2);
			EXPECT_NOT(uint32_t, ~0, 3);
			EXPECT_NOT(uint32_t, ~0, 4);

			EXPECT_FIT(uint32_t, 0, 1);
			EXPECT_FIT(uint32_t, 1, 1);
			EXPECT_FIT(uint32_t, 1, 2);
			EXPECT_FIT(uint32_t, 2, 2);
			EXPECT_FIT(uint32_t, 3, 2);
			EXPECT_FIT(uint32_t, 1, 3);
			EXPECT_FIT(uint32_t, 2, 3);
			EXPECT_FIT(uint32_t, 3, 3);
			EXPECT_FIT(uint32_t, 4, 3);
			EXPECT_FIT(uint32_t, 5, 3);
			EXPECT_FIT(uint32_t, 6, 3);
			EXPECT_FIT(uint32_t, 7, 3);
			EXPECT_FIT(uint32_t, 255, 8);
			EXPECT_FIT(uint32_t, 0xffffffffU, 32);

			EXPECT_NOT(int32_t, 0, 0);
			EXPECT_NOT(int32_t, 1, 0);
			EXPECT_NOT(int32_t, 2, 0);
			EXPECT_NOT(int32_t, 2, 1);
			EXPECT_NOT(int32_t, 3, 1);
			EXPECT_NOT(int32_t, 4, 1);
			EXPECT_NOT(int32_t, 5, 1);
			EXPECT_NOT(int32_t, 6, 1);
			EXPECT_NOT(int32_t, 7, 1);
			EXPECT_NOT(int32_t, 8, 1);
			EXPECT_NOT(int32_t, 4, 2);
			EXPECT_NOT(int32_t, 5, 2);
			EXPECT_NOT(int32_t, 6, 2);
			EXPECT_NOT(int32_t, 7, 2);
			EXPECT_NOT(int32_t, 8, 2);
			EXPECT_NOT(int32_t, 8, 3);
			EXPECT_NOT(int32_t, 255, 3);
			EXPECT_NOT(int32_t, 256, 3);
			EXPECT_NOT(int32_t, 256, 8);

			EXPECT_NOT(int32_t, 0, 0);
			EXPECT_NOT(int32_t, 1, 0);
			EXPECT_NOT(int32_t, 2, 0);
			EXPECT_NOT(int32_t, 3, 0);
			EXPECT_NOT(int32_t, 2, 1);
			EXPECT_NOT(int32_t, 3, 1);
			EXPECT_NOT(int32_t, 4, 1);
			EXPECT_NOT(int32_t, 5, 1);
			EXPECT_NOT(int32_t, 6, 1);
			EXPECT_NOT(int32_t, 7, 1);
			EXPECT_NOT(int32_t, 8, 1);
			EXPECT_NOT(int32_t, 4, 2);
			EXPECT_NOT(int32_t, 5, 2);
			EXPECT_NOT(int32_t, 6, 2);
			EXPECT_NOT(int32_t, 7, 2);
			EXPECT_NOT(int32_t, 8, 2);
			EXPECT_NOT(int32_t, 8, 3);
			EXPECT_NOT(int32_t, 255, 3);
			EXPECT_NOT(int32_t, 256, 3);
			EXPECT_NOT(int32_t, 256, 8);

			// sign is required
			EXPECT_NOT(int32_t, 0, 1);
			EXPECT_NOT(int32_t, 1, 1);
			EXPECT_NOT(int32_t, 2, 2);
			EXPECT_NOT(int32_t, 3, 2);
			EXPECT_NOT(int32_t, 4, 3);
			EXPECT_NOT(int32_t, 5, 3);
			EXPECT_NOT(int32_t, 6, 3);
			EXPECT_NOT(int32_t, 7, 3);
			EXPECT_NOT(int32_t, 255, 8);

			EXPECT_NOT(int32_t, -1, 1);
			EXPECT_NOT(int32_t, -2, 1);
			EXPECT_NOT(int32_t, -3, 1);
			EXPECT_NOT(int32_t, -3, 2);

			EXPECT_FIT(int32_t, 0, 2);
			EXPECT_FIT(int32_t, 1, 2);
			EXPECT_FIT(int32_t, 1, 3);
			EXPECT_FIT(int32_t, 2, 3);
			EXPECT_FIT(int32_t, 3, 3);
			EXPECT_FIT(int32_t, 1, 4);
			EXPECT_FIT(int32_t, 2, 4);
			EXPECT_FIT(int32_t, 3, 4);
			EXPECT_FIT(int32_t, 4, 4);
			EXPECT_FIT(int32_t, 5, 4);
			EXPECT_FIT(int32_t, 6, 4);
			EXPECT_FIT(int32_t, 7, 4);
			EXPECT_FIT(int32_t, 255, 9);
			EXPECT_FIT(int32_t, ~0, 32);
			EXPECT_FIT(int32_t, -1, 2);
			EXPECT_FIT(int32_t, -2, 2);
			EXPECT_FIT(int32_t, -1, 3);
			EXPECT_FIT(int32_t, -2, 3);
			EXPECT_FIT(int32_t, -3, 3);
			EXPECT_FIT(int32_t, -4, 3);
			EXPECT_FIT(int32_t, -1, 4);
			EXPECT_FIT(int32_t, -2, 4);
			EXPECT_FIT(int32_t, -3, 4);
			EXPECT_FIT(int32_t, -4, 4);
			EXPECT_FIT(int32_t, -5, 4);
			EXPECT_FIT(int32_t, -6, 4);
			EXPECT_FIT(int32_t, -7, 4);
			EXPECT_FIT(int32_t, -8, 4);
			EXPECT_FIT(int32_t, std::numeric_limits<int32_t>::min(), 32);

		END_TEST_STAGE( "TestBitsCount" )

#undef EXPECT_FIT
#undef EXPECT_NOT

		return true;
	}

public: bool TestBitsFit()
	{
		START_TEST_STAGE( "Teul::FitBits Tests" )

			START_SUB_STAGE( "Unsigned" )
				for ( UINT32 i = 0; i < 64; ++i )
				{
					{
						bool fit = Teul::Bits::Fit((1ULL << i) - 1, 15);
						bool expectFit = i <= 15;
						CRITICAL( ExpectEqual( fit, expectFit ) );
					}
					{
						bool fit = Teul::Bits::Fit((1ULL << i), 15);
						bool expectFit = i < 15;
						CRITICAL( ExpectEqual( fit, expectFit ) );
					}
					{
						bool fit = Teul::Bits::Fit((1ULL << i) + 1, 15);
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
						bool fit = Teul::Bits::Fit(-((1LL << i) - 1), 15);
						bool expectFit = i <= 14;
						CRITICAL( ExpectEqual( fit, expectFit ) );
					}
					{
						TestFocus("-(%u) for %u", 1 << i, i);
						bool fit = Teul::Bits::Fit(-((1LL << i)), 15);
						bool expectFit = i <= 14;
						CRITICAL( ExpectEqual( fit, expectFit ) );
					}
					{
						TestFocus("-(%u + 1) for %u", 1 << i, i);
						bool fit = Teul::Bits::Fit(-((1LL << i) + 1), 15);
						bool expectFit = i < 14;
						CRITICAL( ExpectEqual( fit, expectFit ) );
					}
				}
				{
					TestFocus("40 bits");
					bool fitPositive = Teul::Bits::Fit((1LL << 39) - 1LL, 40);
					CRITICAL( ExpectTrue( fitPositive ) );
					bool fitNegative = Teul::Bits::Fit(-(1LL << 39), 40);
					CRITICAL( ExpectTrue( fitNegative ) );
				}
			END_SUB_STAGE( "Signed" )
		END_TEST_STAGE( "Teul::FitBits" )

		return true;
	}
};

static TeulBitsTest test;
