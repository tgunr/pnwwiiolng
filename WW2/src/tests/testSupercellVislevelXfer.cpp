#include <syscfg.h> // Path is project specific.
// Unit tests to ensure Map::SupercellVislevelXfer works.

#include "tests/testSuite.h"
#include "tests/testUnits.h"
#include "strt/strtObjectStruct.h"
#include "map/mapHostSupercellVislevelXfer.h"
#include "strt/strtCPStruct.h"
#include "strt/strtFacilityStruct.h"

FacilityStruct::~FacilityStruct()
{
}

typedef HostSupercellVislevelXfer testtype_t;

//////////////////////////////////////////////////////////////////////
// Provides consistent baseline test conditions for each test.

class Fixture
{
public:
	std::map<UINT32, Supercell> cells;
	UINT32 numBuildings;
	UINT32 maxBuildings;
	void* m_allocation;
	size_t m_size;

	Fixture()
		: cells()
		, numBuildings(~0)
		, maxBuildings(~0)
		, m_allocation(NULL)
		, m_size(0)
		{}

	const size_t& size() const { return m_size; }
	const void* allocation() const { return m_allocation; }

	testtype_t* xfer(size_t byteOffset=0) { return (testtype_t*)((char*)m_allocation + byteOffset); }

	// Attempt to allocate a blob for this data.
	void allocate()
	{
		std::pair<void*, size_t> result = testtype_t::allocateFor(cells.begin(), cells.end(), numBuildings, maxBuildings);
		m_allocation = result.first;
		m_size = result.second;
	}

	~Fixture()
	{
		if ( m_allocation != NULL )
		{
			memFree(m_allocation);
			m_allocation = NULL;
		}
	}

	// Add a building to a particular supercell.
	void addBuilding(size_t scNumber, UCHAR visLevel, UCHAR smokeLevel)
	{
		Supercell& sc = cells[scNumber];
		if ( sc.numObjects == 0 )
		{
			sc.objects = (ObjectStruct*)memCalloc(1, sizeof(ObjectStruct));
		}
		else
		{
			sc.objects = (ObjectStruct*)memRealloc(sc.objects, (sc.numObjects + 1) * sizeof(ObjectStruct));
			memset(&sc.objects[sc.numObjects], 0, sizeof(ObjectStruct));
		}
		sc.objects[sc.numObjects].m_bid = sc.numObjects;
		sc.objects[sc.numObjects].m_visLevel = visLevel;
		sc.objects[sc.numObjects].m_smokeLevel = smokeLevel;
		++sc.numObjects;
	}
};

//////////////////////////////////////////////////////////////////////
// The actual unit test.

class SupercellVislevelXferTest : public TestUnit
{
public:	SupercellVislevelXferTest() : TestUnit("SupercellVislevelXferTest") {}

private:
	bool _testBuildingLevels(testtype_t* const xfer, const UINT32 bid)
	{
		for ( UINT32 i = 0; i < 5; ++i )
		{
			TestFocus("vislevel = %u", i);
			xfer->setBuildingLevel(bid, i);
			CRITICAL( ExpectEqual( xfer->getBuildingLevel(bid), i ) );
		}
		TestFocus("vislevel = 255");
		xfer->setBuildingLevel(bid, 255);
		CRITICAL( ExpectEqual( xfer->getBuildingLevel(bid), 255 ) );

		return true;
	}

public:
	virtual bool Run()
	{
		CRITICAL( ExpectEqual( sizeof(testtype_t), (testtype_t::HeaderBits + testtype_t::FlagBits) / 8 ) );

		START_SUB_STAGE( "No supercells" )
			Fixture fixture;
			fixture.allocate();

			CRITICAL( ExpectEqual( fixture.allocation(), (void*)NULL ) );
			CRITICAL( ExpectEqual( fixture.size(), (size_t)0 ) );
		END_SUB_STAGE( "No supercells" )

		START_SUB_STAGE( "One supercell, one building" )
			Fixture fixture;
			Supercell& sc = fixture.cells[0];
			sc.x = 1;
			sc.y = 2;

			// We haven't given the supercell any buildings.
			fixture.allocate();
			CRITICAL( ExpectEqual( fixture.allocation(), (void*)NULL ) );
			CRITICAL( ExpectEqual( fixture.size(), (size_t)0 ) );

			CRITICAL( ExpectTrue( sc.m_xfer == NULL ) );

			fixture.addBuilding(0, 0, 0);
			CRITICAL( ExpectEqual( sc.numObjects, 1 ) );
			CRITICAL( ExpectTrue( sc.objects != NULL ) );

			const size_t expecting_bits = testtype_t::HeaderBits + testtype_t::FlagBits;
			const size_t expecting_bytes = (expecting_bits + 7) / 8;
			CRITICAL( ExpectEqual( testtype_t::calculateBytesNeededFor(sc), expecting_bytes ) );

			fixture.allocate();

			CRITICAL( ExpectFalse( fixture.allocation() == NULL ) );
			CRITICAL( ExpectFalse( fixture.xfer() == NULL ) );
			CRITICAL( ExpectFalse( fixture.size() == 0 ) );

			CRITICAL( ExpectFalse( sc.m_xfer == NULL ) );

			CRITICAL( ExpectTrue( fixture.xfer()->scx() ==  1 ) );
			CRITICAL( ExpectTrue( fixture.xfer()->scy() ==  2 ) );

			CRITICAL( ExpectEqual( fixture.xfer()->size_bytes(), expecting_bytes ) );

			ObjectStruct& obj = sc.objects[0];
			CRITICAL( ExpectEqual( obj.visLevel(), 0 ) );
			CRITICAL( ExpectEqual( obj.smokeLevel(), 0 ) );
			CRITICAL( ExpectEqual( obj.effectiveVisLevel(), 0 ) );

			CRITICAL( ExpectEqual( fixture.xfer(), sc.m_xfer ) );
			CRITICAL( ExpectEqual( sc.m_xfer->getBuildingBaseOffset(0), 0 ) );
			CRITICAL( ExpectEqual( sc.m_xfer->getBuildingLevel(0), 0 ) );

			// Now check all the vislevel combos.
			_testBuildingLevels(sc.m_xfer, 0);

		END_SUB_STAGE( "One supercell, one building" )

		START_SUB_STAGE( "One supercell, three buildings" )
			Fixture fixture;
			Supercell& sc = fixture.cells[0];
			sc.x = -20;
			sc.y = 59;

			fixture.addBuilding(0, 1, 0);
			CRITICAL( ExpectEqual( sc.numObjects, 1 ) );
			CRITICAL( ExpectTrue( sc.objects != NULL ) );
			CRITICAL( ExpectEqual( sc.objects[0].visLevel(), 1 ) && ExpectEqual( sc.objects[0].smokeLevel(), 0 ) && ExpectEqual( sc.objects[0].effectiveVisLevel(), 1 ) );
			fixture.addBuilding(0, 1, 1);	// effective Vislevel should be 2.
			CRITICAL( ExpectEqual( sc.numObjects, 2 ) );
			CRITICAL( ExpectTrue( sc.objects != NULL ) );
			CRITICAL( ExpectEqual( sc.objects[0].visLevel(), 1 ) && ExpectEqual( sc.objects[0].smokeLevel(), 0 ) && ExpectEqual( sc.objects[0].effectiveVisLevel(), 1 ) );
			CRITICAL( ExpectEqual( sc.objects[1].visLevel(), 1 ) && ExpectEqual( sc.objects[1].smokeLevel(), 1 ) && ExpectEqual( sc.objects[1].effectiveVisLevel(), 2 ) );
			fixture.addBuilding(0, 255, 2); // Vislevel should be 255.
			CRITICAL( ExpectEqual( sc.numObjects, 3 ) );
			CRITICAL( ExpectTrue( sc.objects != NULL ) );
			CRITICAL( ExpectEqual( sc.objects[0].visLevel(), 1 ) && ExpectEqual( sc.objects[0].smokeLevel(), 0 ) && ExpectEqual( sc.objects[0].effectiveVisLevel(), 1 ) );
			CRITICAL( ExpectEqual( sc.objects[1].visLevel(), 1 ) && ExpectEqual( sc.objects[1].smokeLevel(), 1 ) && ExpectEqual( sc.objects[1].effectiveVisLevel(), 2 ) );
			CRITICAL( ExpectEqual( sc.objects[2].visLevel(), 255 ) && ExpectEqual( sc.objects[2].smokeLevel(), 2 ) && ExpectEqual( sc.objects[2].effectiveVisLevel(), 255 ) );

			fixture.allocate();

			CRITICAL( ExpectFalse( fixture.allocation() == NULL ) );
			CRITICAL( ExpectFalse( fixture.xfer() == NULL ) );
			CRITICAL( ExpectFalse( fixture.size() == 0 ) );

			CRITICAL( ExpectFalse( sc.m_xfer == NULL ) );

			CRITICAL( ExpectTrue( fixture.xfer()->scx() ==  -20 ) );
			CRITICAL( ExpectTrue( fixture.xfer()->scy() ==  59 ) );

			const size_t expecting_bits = (testtype_t::HeaderBits) + (testtype_t::FlagBits * 2);
			const size_t expecting_bytes = (expecting_bits + 7) / 8;
			const size_t actualBytes = fixture.xfer()->size_bytes();
			CRITICAL( ExpectEqual( actualBytes, expecting_bytes ) );

			CRITICAL( ExpectEqual( sc.objects[0].visLevel(), 1 ) );
			CRITICAL( ExpectEqual( sc.objects[0].smokeLevel(), 0 ) );
			CRITICAL( ExpectEqual( sc.objects[0].effectiveVisLevel(), 1 ) );

			CRITICAL( ExpectEqual( sc.objects[1].visLevel(), 1 ) );
			CRITICAL( ExpectEqual( sc.objects[1].smokeLevel(), 1 ) );
			CRITICAL( ExpectEqual( sc.objects[1].effectiveVisLevel(), 2 ) );

			CRITICAL( ExpectEqual( sc.objects[2].visLevel(), 255 ) );
			CRITICAL( ExpectEqual( sc.objects[2].smokeLevel(), 2 ) );
			CRITICAL( ExpectEqual( sc.objects[2].effectiveVisLevel(), 255 ) );

			CRITICAL( ExpectEqual( fixture.xfer()->getBuildingBaseOffset(0), 0 ) );
			CRITICAL( ExpectEqual( fixture.xfer()->getBuildingBaseOffset(1), 0 ) );
			CRITICAL( ExpectEqual( fixture.xfer()->getBuildingBaseOffset(2), 1 ) );

			CRITICAL( ExpectEqual( fixture.xfer()->getBuildingLevel(0), 1 ) );
			CRITICAL( ExpectEqual( fixture.xfer()->getBuildingLevel(1), 2 ) );
			CRITICAL( ExpectEqual( fixture.xfer()->getBuildingLevel(2), 255 ) );

			_testBuildingLevels(fixture.xfer(), 0);
			_testBuildingLevels(fixture.xfer(), 1);
			_testBuildingLevels(fixture.xfer(), 2);

		END_SUB_STAGE( "One supercell, three buildings" )

		START_SUB_STAGE( "Multiple cells, multiple buildings" )
			Fixture fixture;

			// Default properties for each cell and it's buildings.
			struct Properties { int scx, scy; int levels[20]; };
			static const size_t numTestCells = 11;
			Properties properties[numTestCells] =
			{
				{ 0, 0, { 2, 0, 3, 255, 0, 2, 3, 1, -1 } }
			,	{ -1, 1, { 1, 1, 1, 2, 2, 2, 3, 3, 3, 255, 0, 255, 0, -1 }  }
			,	{ -2, 2, { 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, -1 } }
			,	{ -5, 1, { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, -1 } }
			,	{ 9, 9, { -1 } }	// No buildings.
			,	{ -200, 280, { 0, 0, 0, 0, 0, 0, 0, -1 } }
			,	{ 280, -200, { 255, 255, 255, 255, 255, 255, 255, 255, 255, -1 } }
			,	{ 200, -280, { 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, -1 } }
			,	{ -280, 200, { 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, -1 } }
			,	{ -280, -200, { 255, 0, 1, 0, 255, 0, 1, 255, 0, 1, 255, 0, 1, 0, -1 } }
			,	{ 280, 200, { 255, 0, 1, 2, 3, 4, 5, 4, 3, 2, 1, 0, 255, -1 } }
			};

			size_t expectingSize_bits = 0;

			START_SUB_STAGE( "Populating fixture" )

				for ( size_t scNo = 0; scNo < numTestCells; ++scNo )
				{
					TestFocus("sc#%u", scNo);
					Supercell& sc = fixture.cells[scNo];
					const Properties& props = properties[scNo];

					sc.x = props.scx;
					sc.y = props.scy;

					size_t bldgNo;
					size_t scSize_bits = testtype_t::HeaderBits;
					for ( bldgNo = 0; props.levels[bldgNo] != -1; ++bldgNo )
					{
						TestFocus("sc#%u, bldg#%u", scNo, bldgNo);
						testtype_t::vis_t vis = props.levels[bldgNo];
						testtype_t::vis_t smoke = 0;
						if ( vis > 1 && vis < 255 )
						{
							smoke = vis - 1;
							vis = 1;
						}
						fixture.addBuilding(scNo, vis, smoke);
						scSize_bits += testtype_t::BitsPerObject;
						const size_t expectedScSize_bytes = (scSize_bits + 7) / 8;
						CRITICAL( ExpectEqual( expectedScSize_bytes, fixture.xfer()->calculateBytesNeededFor(sc) ) );
					}

					if ( bldgNo > 0 )
					{
						expectingSize_bits += (scSize_bits + 7) & ~7;
					}
				}
			END_SUB_STAGE( "Populating fixture" )

			CRITICAL( ExpectTrue( fixture.allocation() == NULL ) );
			CRITICAL( ExpectTrue( fixture.xfer() == NULL ) );
			CRITICAL( ExpectTrue( fixture.size() == 0 ) );

			// Create the allocation.
			fixture.allocate();

			CRITICAL( ExpectFalse( fixture.allocation() == NULL ) );
			CRITICAL( ExpectFalse( fixture.xfer() == NULL ) );

			CRITICAL( ExpectEqual( fixture.size(), expectingSize_bits / 8 ) );

			// Make sure the first entry of the allocation is the first supercell.
			CRITICAL( ExpectEqual( fixture.xfer(), fixture.cells[0].m_xfer ) );

			BEGIN_SUB_STAGE( "Verifying data" )
				for ( size_t scNo = 0; scNo < numTestCells; ++scNo )
				{
					TestFocus("sc#%u", scNo);

					const Supercell& sc = fixture.cells[scNo];
					const Properties& props = properties[scNo];

					// Redundant? No, checking for memory stomps.
					CRITICAL( ExpectEqual( sc.x, props.scx ) );
					CRITICAL( ExpectEqual( sc.y, props.scy ) );

					if ( props.levels[0] == -1 )
					{
						ExpectEqual( sc.numObjects, 0 );
						continue;
					}

					CRITICAL( ExpectFalse( sc.m_xfer == NULL ) );

					CRITICAL( ExpectEqual( sc.m_xfer->scx(), sc.x ) );
					CRITICAL( ExpectEqual( sc.m_xfer->scy(), sc.y ) );

					for ( size_t bldgNo = 0; bldgNo < sc.numObjects; ++bldgNo )
					{
						TestFocus("sc#%u, bldg#%u", scNo, bldgNo);
						const testtype_t::vis_t gotLevel = sc.m_xfer->getBuildingLevel(bldgNo);
						ExpectEqual( props.levels[bldgNo], gotLevel );
					}
				}
			END_SUB_STAGE( "Verifying data" )

		END_SUB_STAGE( "Multiple cells, multiple buildings" )
		return true;
	}
};

static SupercellVislevelXferTest supercellVislevelXferTest;

