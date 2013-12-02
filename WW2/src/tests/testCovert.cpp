#include <syscfg.h> // Path is project specific.
// Unit tests to ensure Covert is working as expected.

#include "tests/testUnits.h"
#include "wwii/wwiiCovert.h"

class CovertTest : public TestUnit
{
public:	CovertTest() : TestUnit("CovertTest") {}

public:
	template<typename _Type>
	bool TypeTest(const char* description, _Type baseValue)
	{
		START_TEST_STAGE( description )

			_Type rawValue = baseValue;
			Covert<_Type> covert(baseValue);

			START_SUB_STAGE( "Initial equality" )
				CRITICAL( ExpectEqual( rawValue, covert.Get() ) && ExpectEqual( rawValue, covert ) );
			END_SUB_STAGE( "Initial equality" )

			START_SUB_STAGE( "Assignment" )
				rawValue = baseValue * 2;
				covert.Set(rawValue);
				CRITICAL( ExpectEqual( rawValue, covert.Get() ) && ExpectEqual( rawValue, covert ) );
				covert.Set((_Type)0);
				covert = rawValue;
				CRITICAL( ExpectEqual( rawValue, covert.Get() ) && ExpectEqual( rawValue, covert ) );
			END_SUB_STAGE( "Assignment" )

			START_SUB_STAGE( "operator++" )
				rawValue++;
				covert++;
				CRITICAL( ExpectEqual( rawValue, covert.Get() ) && ExpectEqual( rawValue, covert ) );
			END_SUB_STAGE( "operator++" )

			START_SUB_STAGE( "operator++ 100x" )
				for ( UINT32 i = 0; i < 100; ++i )
				{
					rawValue++;
					covert++;
				}
				CRITICAL( ExpectEqual( rawValue, covert.Get() ) && ExpectEqual( rawValue, covert ) );
			END_SUB_STAGE( "operator++ 100x" )

			START_SUB_STAGE( "operator--" )
				rawValue--;
				covert--;
				CRITICAL( ExpectEqual( rawValue, covert.Get() ) && ExpectEqual( rawValue, covert ) );
			END_SUB_STAGE( "operator--" )

			START_SUB_STAGE( "operator-- 100x" )
				for ( UINT32 i = 0; i < 100; ++i )
				{
					rawValue--;
					covert--;
				}
				CRITICAL( ExpectEqual( rawValue, covert.Get() ) && ExpectEqual( rawValue, covert ) );
			END_SUB_STAGE( "operator-- 100x" )

			START_SUB_STAGE( "operator+=" )
				rawValue += baseValue;
				covert += baseValue;
				CRITICAL( ExpectEqual( rawValue, covert.Get() ) && ExpectEqual( rawValue, covert ) );
			END_SUB_STAGE( "operator+=" )

			START_SUB_STAGE( "operator-=" )
				rawValue -= (_Type)5;
				covert -= (_Type)5;
				CRITICAL( ExpectEqual( rawValue, covert.Get() ) && ExpectEqual( rawValue, covert ) );
			END_SUB_STAGE( "operator-=" )

			START_SUB_STAGE( "operator*=" )
				rawValue *= baseValue;
				covert *= baseValue;
				CRITICAL( ExpectEqual( rawValue, covert.Get() ) && ExpectEqual( rawValue, covert ) );
			END_SUB_STAGE( "operator*=" )

			START_SUB_STAGE( "operator/=" )
				rawValue /= (_Type)2;
				covert /= (_Type)2;
				CRITICAL( ExpectEqual( rawValue, covert.Get() ) && ExpectEqual( rawValue, covert ) );
			END_SUB_STAGE( "operator*=" )

		END_TEST_STAGE( description )

		return true;
	}

public:
	virtual bool Run()
	{
		if (
			!TypeTest( "UCHAR", (UCHAR)4 ) ||
			!TypeTest( "SCHAR", (SCHAR)5 ) ||
			!TypeTest( "UINT16", (UINT16)6 ) ||
			!TypeTest( "SINT16", (SINT16)7 ) ||
			!TypeTest( "UINT32", (UINT32)8 ) ||
			!TypeTest( "SINT32", (SINT32)9 ) ||
			!TypeTest( "UINT64", (UINT64)10 ) ||
			!TypeTest( "SINT64", (SINT64)11 ) ||
			!TypeTest( "FLOAT32", (FLOAT32)20.0 ) ||
			!TypeTest( "FLOAT64", (FLOAT64)30.0 ) )
			return false;
		return true;
	}
};

static CovertTest covertTest;

