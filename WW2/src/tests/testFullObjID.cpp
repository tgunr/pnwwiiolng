#include <syscfg.h> // Path is project specific.
//
// Created: Fri Jun  5 17:48:27 CDT 2009
//
// Check that FullObjID conversion is working.
//
// Copyright (C) PlayNet, Inc 1999-2008 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith
//

#include "tests/testSuite.h"
#include "tests/testUnits.h"
#include "wwii/wwiiFullObjID.h"

class FullObjIDTest : public TestUnit
{
public:
	FullObjIDTest() : TestUnit("FullObjID") {}

public:
	bool TestID(SINT16 ox, SINT16 oy, UINT16 id, bool isNullObjID)
	{
		char stageName[128];
		snprintf(stageName, 127, "FullObjID[x=%d,y=%d,id=%d] %s"
									, ox, oy, id
									, (isNullObjID ? "nullObjID" : "normal id")
									);

		START_TEST_STAGE( stageName )
			FULL_OBJ_ID testId = { { ox, oy }, id, 0, 0 };
			CRITICAL( ExpectTrue(testId.sameCell(ox, oy) ) );
			CRITICAL( ExpectTrue(testId.sameCell(testId.cell) ) );
			CRITICAL( ExpectTrue(testId.sameCell(testId) ) );
			CRITICAL( ExpectTrue(testId == testId) );
			ExpectEqual( testId, testId );

			if ( isNullObjID )
			{
				CRITICAL( ExpectTrue(testId == nullObjID) );
			}
			else
			{
				CRITICAL( ExpectFalse(testId == nullObjID) );
			}
			CRITICAL( ExpectFalse(testId != testId) );
			if ( isNullObjID )
			{
				CRITICAL( ExpectFalse(testId != nullObjID) );
			}
			else
			{
				CRITICAL( ExpectTrue(testId != nullObjID) );
			}

			UINT64 testKey1 = testId.Key();
			UINT64 testKey2 = testId.Key();

			CRITICAL( ExpectEqual(testKey1, testKey2) );

			FULL_OBJ_ID reconstructedId = nullObjID;
			reconstructedId.FromKey(testKey1);
			CRITICAL( ExpectTrue( testId == reconstructedId ) );
			CRITICAL( ExpectEqual( testId, reconstructedId ) );

			reconstructedId.FromKey(testKey2);
			CRITICAL( ExpectTrue( testId == reconstructedId ) );
			CRITICAL( ExpectEqual( testId, reconstructedId ) );

			CRITICAL( ExpectEqual(reconstructedId.cell.ox & 0xFFF0, ox & 0xFFF0 ) );
			CRITICAL( ExpectEqual(reconstructedId.cell.oy & 0xFFF0, oy & 0xFFF0) );
			CRITICAL( ExpectEqual(reconstructedId.id, id) );

		END_TEST_STAGE( stageName )

		return true;
	}

public: virtual bool Run()
	{
		TestID(0, 0, 0, true);
		for ( SINT32 x = -1; x < 8; ++x )
		{
			SINT32 n = 1 + (x * 17);
			UINT32 id = 1 + (x + 1) * (rand() % 32);
			TestID(n, n, id++, false);
			TestID(-n, n, id++, false);
			TestID(n, -n, id++, false);
			TestID(-n, -n, id++, false);
		}
		return true;
	}
};

static FullObjIDTest test;
