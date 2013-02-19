#pragma once
// Date: Wed Dec  7 21:27:34 CST 2005
//
// Defines the supercell class
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

class CPStruct ;
class FacilityStruct ;
class ObjectStruct ;
class LinkStruct ;
struct STRAT_FACILITY_TABLE ;
struct STRAT_BUILDING_LIST ;

//tolua_begin

class Supercell
{
public:
	Supercell()
		: x(0), y(0)
		, index(nullptr)
		, numCPs(0)
		, numFacilities(0)
		, numObjects(0)
		, numLinks(0)
		, numWaypoints(0)
		, indexSize(0)
		, cps(nullptr)
		, facilities(nullptr)
		, objects(nullptr)
		, links(nullptr)
		, facilityTable(nullptr)
		, buildingTable(nullptr)
		, m_xfer(nullptr)
		, m_modified(false)
		{
		}
	~Supercell() ;
	bool initialize(SINT16 scx, SINT16 scy, const char* filename) ;

public:
	SINT16			x;		//x of the supercell
	SINT16			y;		//y of the supercell

	// All of the numXXX members of this structure count the actual number
	// of non-nullptr entries in the array, where maxXXX is the size
	// of the corresponding array.  For the normal, expected cases these
	// will be the same.  This is just CYA for the times when the terrain
	// editor leaves holes in the array of ojects.
//tolua_end
	void**            index;
//tolua_begin

	UINT16  		numCPs;			//# of cps in the supercell
	UINT16  		numFacilities;	//# of facilities in the supercell
	UINT16  		numObjects;		//# of objects in the supercell
	UINT16  		numLinks;		//# of links in the supercell
	UINT16  		numWaypoints;
	UINT32  		indexSize;

	CPStruct*		cps;			//dynamically alloced array for cps
	FacilityStruct*	facilities;	//dynamically alloced array for facilities
	ObjectStruct*	objects;		//dynamically alloced array for objects
	LinkStruct*		links;			//dynamically alloced array for links

	STRAT_FACILITY_TABLE*	facilityTable;
	STRAT_BUILDING_LIST*	buildingTable;

	class HostSupercellVislevelXfer*	m_xfer ;	// Where to find any xfer data on this we have.
	bool			m_modified ;
} ;

//tolua_end
