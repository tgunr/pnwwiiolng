#include <syscfg.h> // Path is project specific.
// Created: Fri Jun  5 17:48:27 CDT 2009
//
// Check that FullObjID conversion is working.
//
// Copyright (C) PlayNet, Inc 1999-2008 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "tests/testUnits.h"
#include "include/xyz_tuple.h"

class XYZTupleTest : public TestUnit
{
public:
	XYZTupleTest() : TestUnit("XYZTuple") {}

public: virtual bool Run()
	{
		START_TEST_STAGE( "Constructors" )
			START_SUB_STAGE( "Default" )
				XYZTuple<SINT32> zeros;
				CRITICAL( ExpectEqual( zeros.x(), 0 ) );
				CRITICAL( ExpectEqual( zeros.y(), 0 ) );
				CRITICAL( ExpectEqual( zeros.z(), 0 ) );
			END_SUB_STAGE( "Default" )

			START_SUB_STAGE( "Memberwise" )
				static const SINT32 x = 50, y = 42, z = -77;
				static const SINT32 offset = -22;	// So we don't just have values lying around.
				XYZTuple<SINT32> mwise(x + offset, y - offset, z * offset);
				CRITICAL( ExpectEqual( mwise.x(), x + offset) );
				CRITICAL( ExpectEqual( mwise.y(), y - offset) );
				CRITICAL( ExpectEqual( mwise.z(), z * offset ) );
			END_SUB_STAGE( "Memberwise" )

		END_TEST_STAGE( "Constructors" )

		START_TEST_STAGE( "Operators" )

			XYZTuple<SINT32> defaulted;
			XYZTuple<SINT32> zeros(0, 0, 0);
			XYZTuple<SINT32> ones(1, 1, 1);
			XYZTuple<SINT64> longOnes(1LL, 1LL, 1LL);
			XYZTuple<SINT32> twos(2, 2, 2);

			CRITICAL( ExpectTrue( defaulted.x() == 0 && defaulted.y() == 0 && defaulted.z() == 0 ) );
			CRITICAL( ExpectTrue( zeros.x() == 0 && zeros.y() == 0 && zeros.z() == 0 ) );
			CRITICAL( ExpectTrue( ones.x() == 1 && ones.y() == 1 && ones.z() == 1 ) );
			CRITICAL( ExpectTrue( longOnes.x() == 1LL && longOnes.y() == 1LL && longOnes.z() == 1LL ) );
			CRITICAL( ExpectTrue( twos.x() == 2 && twos.y() == 2 && twos.z() == 2 ) );

			START_SUB_STAGE( "Equality" )
				CRITICAL( ExpectTrue( defaulted == defaulted ) );
				CRITICAL( ExpectTrue( defaulted == zeros ) );
				CRITICAL( ExpectTrue( zeros == zeros ) );
				CRITICAL( ExpectTrue( zeros == defaulted ) );

				CRITICAL( ExpectTrue( ones == ones ) );
				CRITICAL( ExpectTrue( ones == longOnes ) );
				CRITICAL( ExpectTrue( longOnes == longOnes ) );
				CRITICAL( ExpectTrue( longOnes == ones ) );

				CRITICAL( ExpectFalse( defaulted == ones ) );
				CRITICAL( ExpectFalse( ones == defaulted ) );
				CRITICAL( ExpectFalse( longOnes == defaulted ) );
				CRITICAL( ExpectFalse( defaulted == longOnes ) );
			END_SUB_STAGE( "Equality" )

			START_SUB_STAGE( "Inequality" )
				CRITICAL( ExpectFalse( defaulted != zeros ) );
				CRITICAL( ExpectFalse( zeros != defaulted ) );
				CRITICAL( ExpectTrue( defaulted != ones ) );
				CRITICAL( ExpectTrue( ones != defaulted ) );
				CRITICAL( ExpectTrue( defaulted != longOnes ) );
				CRITICAL( ExpectTrue( longOnes != defaulted ) );
			END_SUB_STAGE( "Inequality" )

			START_SUB_STAGE( "+= and -=" )
				XYZTuple<SINT32> plusOne(zeros);

				CRITICAL( ExpectTrue( plusOne == zeros ) );
				CRITICAL( ExpectFalse( plusOne == ones ) );
				CRITICAL( ExpectFalse( plusOne == twos ) );

				plusOne += ones;

				CRITICAL( ExpectFalse( plusOne == zeros ) );
				CRITICAL( ExpectTrue( plusOne == ones ) );
				CRITICAL( ExpectFalse( plusOne == twos ) );

				plusOne += ones;

				CRITICAL( ExpectFalse( plusOne == zeros ) );
				CRITICAL( ExpectFalse( plusOne == ones ) );
				CRITICAL( ExpectTrue( plusOne == twos ) );

				plusOne += ones;

				CRITICAL( ExpectFalse( plusOne == zeros ) );
				CRITICAL( ExpectFalse( plusOne == ones ) );
				CRITICAL( ExpectFalse( plusOne == twos ) );

				plusOne -= ones;

				CRITICAL( ExpectFalse( plusOne == zeros ) );
				CRITICAL( ExpectFalse( plusOne == ones ) );
				CRITICAL( ExpectTrue( plusOne == twos ) );

				plusOne -= ones;

				CRITICAL( ExpectFalse( plusOne == zeros ) );
				CRITICAL( ExpectTrue( plusOne == ones ) );
				CRITICAL( ExpectFalse( plusOne == twos ) );
			END_SUB_STAGE( "+= and -=" )

			BEGIN_SUB_STAGE( "+" )
				{
					XYZTuple<SINT64> result(zeros + zeros);
					CRITICAL( ExpectTrue( result == zeros ) );
				}
				{
					XYZTuple<SINT64> result(zeros + ones);
					CRITICAL( ExpectTrue( result == ones ) );
				}
				{
					XYZTuple<SINT64> result(ones + zeros);
					CRITICAL( ExpectTrue( result == ones ) );
				}
				{
					XYZTuple<SINT64> result(ones + zeros + zeros);
					CRITICAL( ExpectTrue( result == ones ) );
				}
				{
					XYZTuple<SINT64> result(ones + zeros + ones);
					CRITICAL( ExpectTrue( result == twos ) );
				}
				{
					XYZTuple<SINT32> result(ones + ones);
					CRITICAL( ExpectTrue( result == twos ) );
				}
			END_SUB_STAGE( "+" )

			BEGIN_SUB_STAGE( "-" )
				{
					XYZTuple<SINT64> result(zeros - zeros);
					CRITICAL( ExpectTrue( result == zeros ) );
				}
				{
					XYZTuple<SINT64> result(ones - zeros - zeros);
					CRITICAL( ExpectTrue( result == ones ) );
				}
				{
					XYZTuple<SINT32> result(twos - zeros);
					CRITICAL( ExpectTrue( result == twos ) );
				}
				{
					XYZTuple<SINT32> result(twos - ones);
					CRITICAL( ExpectTrue( result == ones ) );
				}
				{
					XYZTuple<SINT32> result(ones - zeros - ones - zeros);
					CRITICAL( ExpectTrue( result == zeros ) );
				}
				{
					XYZTuple<SINT64> result(ones + ones + ones - twos);
					CRITICAL( ExpectTrue( result == ones ) );
				}
				{
					XYZTuple<SINT64> result(ones + ones + ones - twos - ones);
					CRITICAL( ExpectTrue( result == zeros ) );
				}
			END_SUB_STAGE( "-" )

		END_TEST_STAGE( "Operators" )

		return true;
	}
};

static XYZTupleTest test;
