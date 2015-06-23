#pragma once
// Copyright (C) 1999-2010 PlayNet Inc, All rights reserved.
// Not for redistribution except as part of a compiled library.
//
// @file	host/hostGridCell.h
// @author	Oliver 'kfs1' Smith <osmith@playnet.com>
// @brief	Declarations and prototypes for host/hostGridCell.cpp

class HostVehicle;

#include "wwii/wwiiHash.h"

namespace Grid
{
	struct	Key;			// Describes the Grid hash key.
	class	Cell;			// Describes a Grid Cell.

	//////////////////////////////////////////////////////////////////////
	// Constants.

	static const UINT64 MAX_VIS_RANGE_M = 6000;
	static const SINT64 MAX_RANGE_CM_SQ = (MAX_VIS_RANGE_M * 100) * (MAX_VIS_RANGE_M * 100);

	// Entities in a particular grid node.
	typedef std::unordered_set<HostVehicle*> VehicleSet;
	typedef std::unordered_set<Cell*> Cells;

	/// Patrol the grid for cells that have expired.
	bool PatrolExpirations();
}
