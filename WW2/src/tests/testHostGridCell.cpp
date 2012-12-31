#include <syscfg.h> // Path is project specific.
// Created: Tue Dec 14 13:18:20 CST 2010
//
// Tests for Chat::MissionStatus
//
// Copyright (C) PlayNet, Inc 1999-2010 - All rights reserved.
// Not for redistribution.
//
/// @file	tests/testHostGridCell.cpp
/// @author	Oliver 'kfs1' Smith <osmith@playnet.com>
///	@brief	Unit tests for hostGridCell.
//

#include "tests/testUnits.h"
#include "host/hostGridCell.h"
#include "host/hostGridCellImpl.h"
#include "host/hostGridTypes.h"
#include <set>

// Test the functionality of GridKey.
class HostGridKeyTest : public TestUnit
{
public:	HostGridKeyTest() : TestUnit("Host::Grid::Key") {}
public: virtual ~HostGridKeyTest() {}

public:	bool TestXYConstructor(Grid::Key::Coord x, Grid::Key::Coord y)
	{

		START_SUB_STAGE( "TestXYConstructor" )
			char xy[32] ;
			sprintf(xy, "%d,%d", x, y) ;

			TestFocus(xy) ;

			Grid::Key key(x, y) ;
			if ( !ExpectEqual( key.x(), x ) ) return false ;
			if ( !ExpectEqual( key.y(), y ) ) return false ;

		END_SUB_STAGE( "TestXYConstructor" )

		return true ;
	}

public: virtual bool Run()
	{
		START_TEST_STAGE( "Constructor/destructor" )
			CRITICAL(
				TestXYConstructor(0, 0)
				&& TestXYConstructor(-1, 0)
				&& TestXYConstructor(0, -1)
				&& TestXYConstructor(-1, -1)
				&& TestXYConstructor(-5125, 2327)
			) ;

		END_TEST_STAGE( "Constructor/destructor" )

		START_TEST_STAGE( "Operators" )

			// Series of keys with random values,
			// in ascending order.
			Grid::Key key1(100, -2345) ;
			Grid::Key key2(0, 0) ;
			Grid::Key key3(0, 1) ;
			Grid::Key key4(-32343, 777) ;
			Grid::Key key5(32343, 777) ;

			CRITICAL( ExpectTrue( key1 == key1 ) ) ;
			CRITICAL( ExpectTrue( key2 == key2 ) ) ;
			CRITICAL( ExpectTrue( key3 == key3 ) ) ;
			CRITICAL( ExpectTrue( key4 == key4 ) ) ;
			CRITICAL( ExpectTrue( key5 == key5 ) ) ;

			CRITICAL( ExpectFalse( key1 == key2 ) ) ;
			CRITICAL( ExpectFalse( key2 == key3 ) ) ;
			CRITICAL( ExpectFalse( key3 == key4 ) ) ;
			CRITICAL( ExpectFalse( key4 == key5 ) ) ;

			CRITICAL( ExpectFalse( key2 == key1 ) ) ;
			CRITICAL( ExpectTrue( key1 != key2 ) ) ;
			CRITICAL( ExpectFalse( key1 != key1 ) ) ;

		END_TEST_STAGE( "Operators" )

		return true ;
	}
} keyTest ;

class HostGridCellTest ;

std::set<void*> allocations ;

//////////////////////////////////////////////////////////////////////
// Test-class derived from Grid::Cell.

class TestCell : public Grid::Cell
{
public:
	TestCell(const Grid::Key& _key, Grid::CellHash& _hash)
		: Grid::Cell(_key, _hash)
		{
			allocations.insert(this) ;
		}
	virtual ~TestCell()
	{
		allocations.erase(this) ;
	}

	const Grid::CellHash& hash() const { return m_hash ; }
	TeulTimeMS expiry() const { return m_expiry ; }
	bool active() const { return m_active ; }
	TeulTimeMS expiryTimeMS() const { return ExpiryTime_ms ; }
} ;

struct SampleGrids
{
	bool		inRangeOfCenter ;
	Grid::Key	key ;
} ;

// Test unit class.
class HostGridCellTest : public TestUnit
{
public: HostGridCellTest() : TestUnit("Host::Grid::Cell") {}
public: virtual ~HostGridCellTest() {}

	// For a given pair of offsets, establish a neighbor hood of
	// test cells at and around the center point and verify that
	// the neighborhood populates correctly and that the correct
	// cells can see-/not see- each other.
	bool _testNeighborhoodWithOffset(Grid::Key::Coord xOffset, Grid::Key::Coord yOffset)
	{
		Grid::CellHash& hash = instanceHash[(size_t)(xOffset * xOffset) + (yOffset * yOffset)] ;
		CRITICAL( ExpectTrue( hash.empty() ) ) ;

		///TODO: Replace with a more exhaustive test.
		// Create an assortment of grid cells at locations centered
		// on offset. This should create a busy neighborhood.
		// We want to verify that the neighborhood is properly
		// populated and that cells within NEIGHBORHOOD_RADIUS_CELLS can see
		// each other. Grids in the corner of this square should
		// not be within visibility range of the center node.

		const SampleGrids keys[] =
		{
			{	true,	Grid::Key(xOffset, yOffset) }
		,	{	true,	Grid::Key(xOffset + 1, yOffset) }
		,	{	true,	Grid::Key(xOffset - 1, yOffset) }
		,	{	true,	Grid::Key(xOffset, yOffset + 1) }
		,	{	true, 	Grid::Key(xOffset, yOffset - 1) }
		,	{	true,	Grid::Key(xOffset + 1, yOffset + 1) }
		,	{	true,	Grid::Key(xOffset + 1, yOffset - 1) }
		,	{	true,	Grid::Key(xOffset - 1, yOffset + 1) }
		,	{	true,	Grid::Key(xOffset - 1, yOffset - 1) }

		,	{	false,	Grid::Key(xOffset + (Grid::Cell::Radius_cells + 1), yOffset) }
		,	{	false,	Grid::Key(xOffset - (Grid::Cell::Radius_cells + 1), yOffset) }
		,	{	false,	Grid::Key(xOffset, yOffset + (Grid::Cell::Radius_cells + 1)) }
		,	{	false,	Grid::Key(xOffset, yOffset - (Grid::Cell::Radius_cells + 1)) }
		,	{	false,	Grid::Key(xOffset - (Grid::Cell::Radius_cells + 1), yOffset + (Grid::Cell::Radius_cells + 1)) }
		,	{	false,	Grid::Key(xOffset + (Grid::Cell::Radius_cells + 1), yOffset - (Grid::Cell::Radius_cells + 1)) }

		,	{	true,	Grid::Key(xOffset + (Grid::Cell::Radius_cells - 1), yOffset) }
		,	{	true,	Grid::Key(xOffset - (Grid::Cell::Radius_cells - 1), yOffset) }
		,	{	true,	Grid::Key(xOffset, yOffset + (Grid::Cell::Radius_cells - 1)) }
		,	{	true,	Grid::Key(xOffset, yOffset - (Grid::Cell::Radius_cells - 1)) }

		,	{	true,	Grid::Key(xOffset + Grid::Cell::Radius_cells, yOffset) }
		,	{	true,	Grid::Key(xOffset - Grid::Cell::Radius_cells, yOffset) }
		,	{	true,	Grid::Key(xOffset, yOffset + Grid::Cell::Radius_cells) }
		,	{	true,	Grid::Key(xOffset, yOffset - Grid::Cell::Radius_cells) }

		,	{	false,	Grid::Key(xOffset - (Grid::Cell::Radius_cells - 1), yOffset + (Grid::Cell::Radius_cells - 1)) }
		,	{	false,	Grid::Key(xOffset + (Grid::Cell::Radius_cells - 1), yOffset - (Grid::Cell::Radius_cells - 1)) }
		} ;
		static const size_t numKeys = sizeof(keys) / sizeof(decltype(keys[0]));

		///SANITY: Vector is in the right order and top node is the base one.
		assert( keys[0].key.x() == xOffset && keys[0].key.y() == yOffset ) ;

		std::vector<TestCell*> cells ;

		START_SUB_STAGE( "Hash Population" )
			for ( size_t i = 0 ; i < numKeys ; ++i )
			{
				const Grid::Key::Coord yValue = keys[i].key.y() ;
				const Grid::Key::Coord xValue = keys[i].key.x() ;
				///SANITY: No duplicate keys.
				ExpectTrue( hash[yValue].find(xValue) == hash[yValue].end() ) ;
				cells.push_back(new TestCell(keys[i].key, hash)) ;
				ExpectTrue( hash[yValue].find(xValue) != hash[yValue].end() ) ;
			}
			///SANITY: TestCells should have stored all the cell pointers in 'allocations'.
			assert( allocations.empty() == false && allocations.size() == cells.size() ) ;
		END_SUB_STAGE( "Hash Population" )

		for ( size_t i = 0 ; i < numKeys ; ++i )
		{
			char comparison[32] ;
			sprintf(comparison, "cell[%d]", i) ;
			START_SUB_STAGE( comparison )
				// Test if inRange returns the expected (true/false) result.
				ExpectTrue( cells[0]->inRange(cells[i]->key()) == keys[i].inRangeOfCenter ) ;
			END_SUB_STAGE( comparison )
		}

		return true ;
	}

	bool TestNeighborhoodFunction(Grid::Key::Coord xOffset, Grid::Key::Coord yOffset)
	{
		bool success = false ;
		char center[64] ;
		sprintf(center, "Center[%d,%d]", xOffset, yOffset) ;
		START_SUB_STAGE( center )
			try
			{
				success = _testNeighborhoodWithOffset(xOffset, yOffset) ;
			}
			catch ( ... )
			{
				success = false ;
			}

			START_SUB_STAGE( "Neighborhood expiration" )
				AdvanceClock_ms(Grid::Cell::ExpiryTime_ms + 1000) ;
				Grid::PatrolExpirations() ;
				CRITICAL( ExpectTrue( instanceHash.empty() ) ) ;
				CRITICAL( ExpectEqual( allocations.size(), 0 ) ) ;
				CRITICAL( ExpectTrue( allocations.empty() ) ) ;
			END_SUB_STAGE( "Neighborhood expiration" )
		END_SUB_STAGE( center )

		return success ;
	}

	bool TestNeighborhoodRangeFunction(Grid::Key::Coord offset)
	{
		START_SUB_STAGE( "Range" )
			CRITICAL( TestNeighborhoodFunction(offset, offset) ) ;
			CRITICAL( TestNeighborhoodFunction(-offset, offset) ) ;
			CRITICAL( TestNeighborhoodFunction(offset, -offset) ) ;
			CRITICAL( TestNeighborhoodFunction(-offset, -offset) ) ;

			CRITICAL( TestNeighborhoodFunction(offset + 1, -offset) ) ;
			CRITICAL( TestNeighborhoodFunction(-offset, offset - 1) ) ;
		END_SUB_STAGE( "Range" )

		return true ;
	}

	//////////////////////////////////////////////////////////////////////
	// Main body of the test.
public: virtual bool Run()
	{
		static const instanceid_t inst = 1234 ;

		START_TEST_STAGE( "Environment" )

			// Instance hash should be empty on startup.
			CRITICAL( ExpectTrue( instanceHash.empty() ) ) ;
			// Accessing a non-extant element should construct an empty hash.
			CRITICAL( ExpectTrue( instanceHash[inst].empty() ) ) ;
			// And the instance hash should no-longer be empty.
			CRITICAL( ExpectFalse( instanceHash.empty() ) ) ;

			// Calling Patrol should result in a purge.
			START_SUB_STAGE( "Expirations" )

				Grid::PatrolExpirations() ;
				CRITICAL( ExpectTrue( instanceHash.empty() ) ) ;

			END_SUB_STAGE( "Expirations" )

			CRITICAL( ExpectTrue( allocations.empty() ) ) ;

		END_TEST_STAGE( "Environment" )

		START_TEST_STAGE( "Constructor/destructor" )

			// Create a hash.
			instanceHash[inst].clear() ;
			Grid::CellHash& hash = instanceHash[inst] ;

			static const Grid::Key::Coord testX = 1333, testY = 2333 ;
			Grid::Key key(testX, testY) ;

			TestCell* cell = new TestCell(key, hash) ;

			CRITICAL( ExpectFalse( allocations.empty() ) ) ;
			CRITICAL( ExpectEqual( allocations.size(), 1 ) ) ;
			CRITICAL( ExpectEqual( *allocations.begin(), (void*)cell ) ) ;

			START_SUB_STAGE( "Activation" )
				// Construction finishes with activation.
				CRITICAL( ExpectTrue( cell->active() ) ) ;
				// It should have added us to the hash.
				CRITICAL( ExpectFalse( hash.empty() ) ) ;
				CRITICAL( ExpectEqual( hash.size(), 1 ) ) ;

				// Check that it registered to the right place.
				const bool yValueHashEntryCreated = hash.find(testY) != hash.end() ;
				CRITICAL( ExpectTrue( yValueHashEntryCreated ) ) ;

				const bool xValueHashEntryCreated = hash[testY].find(testX) != hash[testY].end() ;
				CRITICAL( ExpectTrue( xValueHashEntryCreated ) ) ;

				const bool correctCellPointerInHash = (hash[testY][testX] == cell) ;
				CRITICAL( ExpectTrue( correctCellPointerInHash ) ) ;
			END_SUB_STAGE( "Activation" )

			START_SUB_STAGE( "Initialization" )
				ExpectTrue( cell->key() == key ) ;
				ExpectTrue( &cell->hash() == &hash ) ;
				ExpectFalse( cell->neighbors().empty() ) ;	// Expect self in neighborhood.
				ExpectTrue( cell->vehicles().empty() ) ;
				ExpectEqual( cell->expiry(), BaseClock() + cell->expiryTimeMS() ) ;

				ExpectFalse( cell->expired() ) ;
				ExpectTrue( cell->inRange(key)) ;

				ExpectEqual( allocations.size(), 1 ) ;
				CRITICAL( ExpectEqual( *allocations.begin(), (void*)cell ) ) ;
			END_SUB_STAGE( "Initialization" )

			START_SUB_STAGE( "Initial Neighborhood" )
				// Check the neighborhood: it should contain me, and just me.
				ExpectTrue( cell->neighbors().size() == 1 ) ;
				ExpectTrue( cell->neighbors().find(cell) != cell->neighbors().end() ) ;
			END_SUB_STAGE( "Initial Neighborhood" )

			// Being fresh and new, the cell shouldn't get expired right now.
			START_SUB_STAGE( "Non-expiration" )
				Grid::PatrolExpirations() ;
				CRITICAL( ExpectFalse( instanceHash.empty() ) ) ;
				CRITICAL( ExpectFalse( hash.empty() ) ) ;
				ExpectEqual( allocations.size(), 1 ) ;
				CRITICAL( ExpectEqual( *allocations.begin(), (void*)cell ) ) ;
			END_SUB_STAGE(" Non-expiration" )

			// Advance the clock far enough and it should go away.

			AdvanceClock_ms(cell->expiryTimeMS() + 1000) ;

			START_SUB_STAGE( "Expiration" )
				ExpectFalse( instanceHash.empty() ) ;
				ExpectFalse( allocations.empty() ) ;
				CRITICAL( ExpectTrue( cell->expired() ) ) ;
				Grid::PatrolExpirations() ;
				ExpectTrue( instanceHash.empty() ) ;
				ExpectTrue( allocations.empty() ) ;
			END_SUB_STAGE( "Expiration" )

		END_TEST_STAGE( "Constructor/destructor" )

		START_TEST_STAGE( "Neighborhood tests" )

			TestNeighborhoodFunction(0, 0) ;
			for ( Grid::Key::Coord i = 1 ; i <= Grid::Cell::Radius_cells + 1 ; ++i )
			{
				TestNeighborhoodRangeFunction(i) ;
			}

		END_TEST_STAGE( "Neighborhood tests" )

/*
		START_TEST_STAGE( "Presence tests" )

			CRITICAL( ExpectTrue( instanceHash.empty() ) ) ;

			TestPresence veh1(1000, 0, 0) ;
			TestPresence veh2(1000, 0, 0) ;

			START_SUB_STAGE( "Presence construction" )
				CRITICAL( ExpectTrue( veh1.instID() == 1000 ) ) ;
				CRITICAL( ExpectTrue( veh1.x_cm() == 0 ) ) ;
				CRITICAL( ExpectTrue( veh1.y_cm() == 0 ) ) ;
				CRITICAL( ExpectTrue( veh1.cell() == NULL ) ) ;

				CRITICAL( ExpectTrue( veh2.instID() == 1000 ) ) ;
				CRITICAL( ExpectTrue( veh2.x_cm() == 0 ) ) ;
				CRITICAL( ExpectTrue( veh2.y_cm() == 0 ) ) ;
				CRITICAL( ExpectTrue( veh2.cell() == NULL ) ) ;
			END_SUB_STAGE( "Presence construction" )

			START_SUB_STAGE( "Veh1 deployment" )
				veh1.updateGrid() ;

				CRITICAL( ExpectFalse( instanceHash.empty() ) ) ;
				CRITICAL( ExpectFalse( veh1.cell() == NULL ) ) ;

				CRITICAL( ExpectTrue( veh1.cell()->key().x() == 0 ) ) ;
				CRITICAL( ExpectTrue( veh1.cell()->key().y() == 0 ) ) ;

				bool veh1InCorrectHash = *(instanceHash[veh1.instID()][0][0]->presences().begin()) == &veh1 ;
				CRITICAL( ExpectTrue( veh1InCorrectHash ) ) ;
			END_SUB_STAGE( "Veh1 deployment" )

			START_SUB_STAGE( "Veh2 deployment" )
				veh2.updateGrid() ;
				CRITICAL( ExpectTrue( veh1.cell() == veh2.cell() ) ) ;

				bool vehiclesInSameHash = instanceHash[veh1.instID()][0][0]->presences().size() == 2 ;
				CRITICAL( ExpectTrue( vehiclesInSameHash ) ) ;
			END_SUB_STAGE( "Veh2 deployment" )

			START_SUB_STAGE( "Presence::Update veh2" )
				veh2.move(veh2.x_cm() + Grid::Cell::Size_m * 100, veh2.y_cm()) ;
				veh2.updateGrid() ;
				bool hash1Contains1Vehicle = instanceHash[veh1.instID()][0][0]->presences().size() == 1 ;
				CRITICAL( ExpectTrue( hash1Contains1Vehicle ) ) ;
				bool hash2Contains1Vehicle = instanceHash[veh1.instID()][0][1]->presences().size() == 1 ;
				CRITICAL( ExpectTrue( hash2Contains1Vehicle ) ) ;
				bool veh2InCorrectHash = *(instanceHash[veh2.instID()][0][1]->presences().begin()) == &veh2 ;
				CRITICAL( ExpectTrue( veh2InCorrectHash ) ) ;
			END_SUB_STAGE( "Presence::Update veh2" )

			START_SUB_STAGE( "Presence::Update veh1" )
				veh1.move(veh2.x_cm(), veh1.y_cm() + 150) ;
				veh1.updateGrid() ;

				bool hash1IsEmpty = instanceHash[veh1.instID()][0][0]->presences().empty() ;
				CRITICAL( ExpectTrue( hash1IsEmpty ) ) ;
				const Grid::Cell::PresenceSet& presences = instanceHash[veh1.instID()][0][1]->presences() ;
				bool veh1InCorrectHash = presences.find(&veh1) != presences.end() ;
				CRITICAL( ExpectTrue( presences.size() == 2 && veh1InCorrectHash ) ) ;
			END_SUB_STAGE( "Presence::Update veh1" )

			START_SUB_STAGE( "Instance separation (veh3)" )
				const Grid::Cell::PresenceSet& presences = instanceHash[veh1.instID()][0][1]->presences() ;
				for ( size_t i = 1 ; i < 33 ; ++i )
				{
					TestPresence veh3(veh1.instID() + i, veh1.x_cm(), veh1.y_cm()) ;
					veh3.updateGrid() ;
					CRITICAL( ExpectTrue( instanceHash.size() == i + 1 ) ) ;
					CRITICAL( ExpectTrue( presences.find(&veh3) == presences.end() ) ) ;
				}
			END_SUB_STAGE( "Instance separation (veh3)" )

		END_TEST_STAGE( "Presence tests" )

		START_TEST_STAGE( "Mass expiry" )
			AdvanceClock_ms(Grid::Cell::ExpiryTime_ms + 1000) ;
			Grid::PatrolExpirations() ;
			CRITICAL( ExpectTrue( instanceHash.empty() ) ) ;
		END_TEST_STAGE( "Mass expiry" )
*/

#ifdef I_WANT_TO_KNOW_MORE
		fprintf(stderr, "CellRad: %lld, CellRad2: %lld\n", Grid::Cell::Radius_cells, Grid::Cell::Radius_cells2) ;

		UINT32 width = World::Meters::Width / Grid::Cell::Size_m ;
		UINT32 height = World::Meters::Height / Grid::Cell::Size_m ;

		fprintf(stderr, "World: %u,%u. Cells: %u,%u => %u => %uKb\n"
						, World::Meters::Width, World::Meters::Height
						, width, height
						, width * height
						, (width * height * 4) / 1024
						) ;

		UINT32 metaW = width / Grid::Cell::Radius_cells ;
		UINT32 metaH = height / Grid::Cell::Radius_cells ;

		fprintf(stderr, "World: %u,%u. Meta: %u,%u => %u => %uKb\n"
						, World::Meters::Width, World::Meters::Height
						, metaW, metaH
						, metaW * metaH
						, (metaW * metaH * 4) / 1024
						) ;

		Grid::Key base(0, 0) ;
		Grid::Cell* cell = new Grid::Cell(base, instanceHash[0]) ;
		size_t n = 0 ;			// number of in-ranges
		for ( Grid::Key::Coord y = Grid::Cell::Radius_cells + 1 ; y >= -(Grid::Cell::Radius_cells + 1) ; --y )
		{
			for ( Grid::Key::Coord x = -(Grid::Cell::Radius_cells + 1) ; x <= Grid::Cell::Radius_cells + 1 ; ++x )
			{
				Grid::Key subject(x, y) ;
				if ( cell->inRange(subject) ) n += 1 ;
				fprintf(stderr, "%c ", (x == y && x == 0) ? '*' : (cell->inRange(subject) ? 'X' : '-')) ;
			}
			fprintf(stderr, "\n") ;
		}
		fprintf(stderr, "in-range: %u\n", n) ;
		delete cell ;
#endif

		return true ;
	}
} cellTest ;

