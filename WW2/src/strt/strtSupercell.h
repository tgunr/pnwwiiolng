#pragma once
// Date: Wed Dec  7 21:27:34 CST 2005
//
// Defines the supercell class
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

class CPStruct;
class FacilityStruct;
class ObjectStruct;
class LinkStruct;
struct STRAT_FACILITY_TABLE;
struct STRAT_BUILDING_LIST;
class HostSupercellVislevelXfer;

//tolua_begin

class Supercell
{
public:
	Supercell()
		: index(nullptr)
		, indexSize(0)
		, cps(nullptr)
		, facilities(nullptr)
		, objects(nullptr)
		, links(nullptr)
		, facilityTable(nullptr)
		, buildingTable(nullptr)
		, m_xfer(nullptr)
		, m_modified(false)
		, x(0), y(0)
		, numCPs(0)
		, numFacilities(0)
		, numObjects(0)
		, numLinks(0)
		, numWaypoints(0)
		{
		}
	~Supercell();
	bool initialize(SINT16 scx, SINT16 scy, const char* filename);

	FULL_OBJ_ID* Ptr(UINT16 id) const
	{
		if (id < indexSize)
			return reinterpret_cast<FULL_OBJ_ID*>(index[id]);
		return nullptr;
	}

//tolua_end
	template<class ReturnType>
	ReturnType* Get(UINT16 id) const
	{
		FULL_OBJ_ID* fid = Ptr(id);
		if (PN_LIKELY(fid && fid->objClass == ReturnType::StratClass))
			return reinterpret_cast<ReturnType*>(fid);
		return nullptr;
	}
//tolua_begin

public:
	// All of the numXXX members of this structure count the actual number
	// of non-nullptr entries in the array, where maxXXX is the size
	// of the corresponding array.  For the normal, expected cases these
	// will be the same.  This is just CYA for the times when the terrain
	// editor leaves holes in the array of objects.
//tolua_end
	void**				index;
//tolua_begin

	UINT32				indexSize;

	CPStruct*			cps;			//dynamically alloced array for cps
	FacilityStruct*		facilities;		//dynamically alloced array for facilities
	ObjectStruct*		objects;		//dynamically alloced array for objects
	LinkStruct*			links;			//dynamically alloced array for links

	STRAT_FACILITY_TABLE*	facilityTable;
	STRAT_BUILDING_LIST*	buildingTable;

//tolua_end
	HostSupercellVislevelXfer*	m_xfer;	// Where to find any xfer data on this we have.
	bool				m_modified;
//tolua_begin

	SINT16				x;				//x of the supercell
	SINT16				y;				//y of the supercell

	UINT16  			numCPs;			//# of cps in the supercell
	UINT16  			numFacilities;	//# of facilities in the supercell
	UINT16  			numObjects;		//# of objects in the supercell
	UINT16  			numLinks;		//# of links in the supercell
	UINT16  			numWaypoints;
};

//tolua_end
