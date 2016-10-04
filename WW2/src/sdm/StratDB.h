#pragma once

#include <vector>
#include <string>

#include "sdm/sdm.h"

struct FULL_OBJ_ID;

namespace StratDB
{

/************************************************************************************************
	StratDB
*************************************************************************************************/
// don't use StratDB from thread unless sdm's are fully preloaded

class StratObject;

typedef std::vector<StratObject*> StratObjList;
typedef int (*stratDatafn)(int scx, int scy, sdm_strat_data*);

int Init(stratDatafn);
void Kill();

StratObject* FindStratObject(const sdm_full_obj_id&);
StratObject* FindStratObject(const FULL_OBJ_ID);
StratObject* FindStratObject(const char* cp, const char* fac = nullptr);

// returns a list of objects with klass and type starting from start
// if start is null, loops through all objects (probably expensive)
// returns true if it found anything
bool FindStratObjects(StratObjList& list, int klass = -1, int type = -1, int subtype = -1, StratObject* strat = nullptr);

void AllStratObjectsInWorld(StratObjList&);
void AllCPsInWorld(StratObjList&);

void CPsInSuperCell(StratObjList&, int scx, int scy);
void FacilitiesInSuperCell(StratObjList&, int scx, int scy);

// layout of building list mimics old sdmHash crap for compat
// with STRAT_CELL_PACKET update. don't use for new code.
StratObjList& BuildingsInOctet(int scx, int scy, int c, int r);

void LoadSuperCell(int scx, int scy);
bool SuperCellLoaded(int scx, int scy);

// load up strat.xml for the current language if its not english
// these are translated strat names; for china
void InitDisplayNames();

/************************************************************************************************
	StratObject
*************************************************************************************************/

class StratObject
{
public:
	sdm_full_obj_id FullId() const	{ return _fullid; }
	FULL_OBJ_ID const* FullObjID() const { return (FULL_OBJ_ID*)&_fullid; }

	StratObject* ParentStratObject() const;
	int SupercellX() const  { return _sx; }
	int SupercellY() const  { return _sy; }

	int Id() const			{ return _id; }
	int StratClass() const  { return _stratclass; }
	int StratType() const   { return _strattype; }

	int PosX() const		{ return _posx; }
	int PosY() const		{ return _posy; }
	int OriginalSide() const	{ return _oside; }
	int OriginalCountry() const { return _ocountry; }

	int SubType() const			{ return _subtype; }
	const char* Name() const	{ return _name.c_str(); }
	const char* Filename() const{ return _filename.c_str(); }
	const wchar_t* DisplayName() const;

	sdm_full_obj_id const& ParentId()  { return _parentid;}

	unsigned int StratFlags(bool filter = false) const;
	unsigned int UpdateTime() const { return _updatetime; }
	int VisLevel() const { return _vislevel;}

// updated from strat server
	int Side() const		{ return (Online) ? _side : _oside; }
	int Country() const  { return (Online) ? _country : _ocountry; }

	StratObject();
	StratObject(sdm_cp const&, int sx, int sy);
	StratObject(sdm_facility const&, int sx, int sy);
	StratObject(sdm_building const&, int sx, int sy);

	// Comparison operator == just compare addresses (so that nullptr ptr compares work too)
	bool operator==(const StratObject& rhs) const { return this == &rhs; }
	bool operator==(const StratObject* rhs) const { return this == rhs; }
	bool operator!=(const StratObject& rhs) const { return this != &rhs; }
	bool operator!=(const StratObject* rhs) const { return this != rhs; }

	void SetName(const char* name);
	void SetCountry(int country);
	void SetUpdateTime(unsigned int t) { _updatetime = t; }
	void SetStratFlags(unsigned int flags)  { _stratflags = flags; }
	void SetVisLevel(int vislevel) { _vislevel = vislevel; }
	void AddStratFlags(unsigned int flags)  { _stratflags |= flags; }
	void ClearStratFlags(unsigned int flags)  { _stratflags &= ~flags; }

	// ordering info in SDM used for strat update messages
	void SetIndex(unsigned int i) { _index = i; }
	unsigned int GetIndex() const { return _index; }

typedef std::vector<StratObject*> ChildList;
typedef std::vector<UINT32> UnitList;

	ChildList ChildStratObjects() const;

	void AddUnit(unsigned int id);
	void RemoveUnit(unsigned int id);
	bool IsUnitHere(unsigned int id) const;
	const UnitList& GetUnits() const { return _units; }
	unsigned int GetUnitCount() const { return (unsigned int)_units.size(); }
	unsigned int FindBestUnit(int s, int c, int b) const;
	unsigned int FindBestAlliedUnit(int c, int b) const;

	bool IsSpawnable() const;
	bool IsDeepWaterPort() const;
	bool IsRelated(const FULL_OBJ_ID id) const;
	bool IsRelated(const StratDB::StratObject* strat) const;

	StratDB::StratObject* GetLinkedDepot() const;
	bool GetAreaXY(int& ax, int& ay); // gets a coordinate that is the average center for the cp and its facilities

protected:
	void Init();

	StratObject* CreateParentStratObject() const;

	void Set(sdm_cp const&);
	void Set(sdm_facility const&);
	void Set(sdm_building const&);

union StratTypeUnion
{
	sdm_cp			cp;
	sdm_facility	facility;
	sdm_building	building;
	sdm_depot		depot;
	sdm_waypoint	waypoint;
};
	StratTypeUnion _sdmdata;

	int _sx, _sy;
	int _id, _stratclass, _strattype;
	int _posx, _posy;
	int _oside, _ocountry;
	int _subtype;

	int _side, _country;
	unsigned int _stratflags;
	unsigned int _updatetime;
	int _vislevel;

	std::string _name;
	std::string _filename;

	sdm_full_obj_id _fullid;
	mutable sdm_full_obj_id _parentid;
	mutable StratObject* _parent;
	mutable StratObject* _linkedDepot; // only useful with depot strat objects

	UnitList _units; // hc units deployed at this strat object

	unsigned int _index;

	StratObject(const StratObject&) = delete;	// Prevent copy ctor
};

    class CPListItem
    {
    public:
        std::string name;
        std::wstring displayName;
        int positionX, positionY;

        CPListItem(std::string nam, std::wstring display, int x, int y )
        {
            name = nam;
            displayName = display;
            positionX = x;
            positionY = y;
        }
    };

}
