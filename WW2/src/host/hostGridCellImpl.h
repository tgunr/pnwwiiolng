#pragma once
// Created: Tue Dec 14 13:41:04 CST 2010
//
// Copyright (C) PlayNet, Inc 1999-2010 - All rights reserved.
// Not for redistribution except as part of a compiled library.
//
// @file	host/hostGridCellImpl.h
// @author	Oliver 'kfs1' Smith <osmith@playnet.com>
// @brief 	Constants, classes and typedefs used by the Grid system.

#include "wwii/wwiiHash.h"				// For unordered_map
#include "host/hostGridCell.h"			// For MAX_VIS_RANGE_M etc
#include "teul/teulTime.h"
#include <map>

namespace Grid
{

//////////////////////////////////////////////////////////////////////
/// Identifies a grid node by its x/y position.

struct Key
{
	typedef SINT16 Coord;
	typedef SINT32 CoordSq;
	typedef UINT32 Value;

	public:	Key(Coord _x, Coord _y) : m_x(_x), m_y(_y) {}
	public:	Coord x() const { return m_x; }
	public:	Coord y() const { return m_y; }
	public: bool operator == (const Grid::Key& rhs) const { return rhs.m_value == m_value; }
	public: bool operator != (const Grid::Key& rhs) const { return rhs.m_value != m_value; }
	private:
		union
		{
			struct { Coord m_x, m_y; };
			Value m_value;
		};
};

//////////////////////////////////////////////////////////////////////
// Hash of Cells by location.
// A hash exists for each world instance.

typedef std::map<SINT16 /* x */, class Cell*> CellXHash;	// Hash of x -> cell*
typedef std::map<SINT16 /* y */, CellXHash> CellHash;		// Hash of y -> { x -> cell* }

//////////////////////////////////////////////////////////////////////
/// Describes a cell (or node) within the grid system.

class Cell
{
public:
	Cell(const Key& key, CellHash& inHash);
#ifndef ENABLE_TESTS
	~Cell();
#else
	virtual ~Cell();
#endif

public:
	/// Time unoccupied cells hang around after emptying.
	static const TeulTimeMS 	ExpiryTime_ms = 60 * 1000;

	/// Size of each Cell in meters. 1500m (1/4 of 6km vis range, just over 2x infantry range).
	static const SINT64 		Size_m = 1500;

	/// Size in cells from center to edge of a given neighborhood.
	static const SINT64 Radius_cells = ((Grid::MAX_VIS_RANGE_M + Size_m - 1) / Size_m) + 1;
	static const SINT64 Radius_cells2 = Radius_cells * Radius_cells; /// Square of Radius_cells

private:
	void _checkForExpiry();
	void _removeFromNeighborhood();

public:
	void activate();

	/// Add or transfer an entity.
	void addVehicle(HostVehicle*);
	/// Remove a vehicle without any fuss.
	void removeVehicle(HostVehicle* const vehicle) { m_vehicles.erase(vehicle); _checkForExpiry(); }

	/// Determines if this cell has expired.
	/// @return true when the cell can be deleted.
	bool expired() const { return (m_expiry > 0ULL && m_expiry < teulClock64 ); }

	/// Test if the cell is in-range of a given gridkey.
	bool inRange(const Key& rhs) const
	{
		Key::CoordSq xDelta = key().x() - rhs.x();
		Key::CoordSq yDelta = key().y() - rhs.y();
		return ((xDelta * xDelta) + (yDelta * yDelta)) <= Radius_cells2;
	}
	bool inRange(const Cell& rhs) const { return inRange(rhs.key()); }

	const VehicleSet& vehicles() const { return m_vehicles; }
	const Key& key() const { return m_key; }
	const Cells& neighbors() const { return m_neighbors; }
	const VehicleSet& vislist() const { return m_vislist; }

protected:
	Key				m_key;				///< X/Y of cell.
	CellHash&		m_hash;				///< Which instance hash we're in.
	Cells			m_neighbors;		///< Neighoring cells.
	VehicleSet		m_vehicles;			///< Vehicles in this cell.
	VehicleSet		m_vislist;			///< All active vehicles in neighborhood (self included).
	TeulTimeMS		m_expiry;
	bool			m_active;

	friend struct NeighborExtractionFunctor;
};

// Hash of CellHashes by instance ID.
typedef std::unordered_map<instanceid_t, CellHash> InstanceHash;

//////////////////////////////////////////////////////////////////////
// Functor for extracting a cell from it's neighbors.

struct NeighborExtractionFunctor : public std::unary_function<Grid::Cell*, void>
{
	NeighborExtractionFunctor(Grid::Cell* leaver) : m_leaver(leaver) {}
	Grid::Cell* m_leaver;
	void operator()(Grid::Cell* const neighbor) const { neighbor->m_neighbors.erase(m_leaver); }
};

}

#ifdef ENABLE_TESTS
extern Grid::InstanceHash instanceHash;
#endif

