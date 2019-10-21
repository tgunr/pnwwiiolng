#include "syscfg.h" // Path is project specific.
#include <iostream>
#include <sstream>

#include "weap/weapList.h"
#include "sdm/StratDB.h"
#include "wwii/wwiiHCUnits.h"
#include "xml/XMLLoader2.h"
#include "locl/loclLang.h"
#include "ui/uiMain.h"
#include "client/clientStrtQueue.h"
#include "map/mapSupercellVislevelXfer.h"
#include "trn/trnmain.h"
#include "client/clientStrtQueue.h"
#include "ui/uiChat.h"

using namespace StratDB;

/************************************************************************************************
	StratDB
*************************************************************************************************/

static bool operator < (const sdm_lookup_id& a, const sdm_lookup_id& b)
{
	if (a.ox != b.ox)
		return a.ox < b.ox;
	else if(a.oy != b.oy)
		return a.oy < b.oy;

	return a.id < b.id;
}

using supercell_id = std::pair<int,int>;
using StratObjectMap = std::map<sdm_lookup_id, StratObject*>;
using BuildingList = std::vector<StratObject*>;

struct supercell
{
	bool _loaded;
	int _sx, _sy;

public:
	supercell() : _loaded(0), _sx(0), _sy(0) {}
	~supercell()
	{
		StratObjectMap::iterator it = _stratobjs.begin();
		for(; it != _stratobjs.end(); ++it)
			delete it->second;
	}

	void load(int scx, int scy);
	void AddStratData(sdm_strat_data*);

	size_t numBuildings() const { return _buildings.size() ; }
	const StratObject* const building(const size_t i) const { return _buildings[i] ; }
	StratObject*& building(const size_t i) { return _buildings[i] ; }

	StratObjectMap _stratobjs;

protected:
	std::vector<StratObject*> _buildings;
};

using SuperCellMap = std::map<supercell_id, supercell>;
using ChildMap = std::map<sdm_lookup_id, StratObject::ChildList>;
using LinkMap = std::map<sdm_lookup_id, sdm_depot>;
using StratObjListMap = std::map<supercell_id, StratDB::StratObjList>;
using DisplayNameMap = std::map<std::string, std::wstring>;
using CPNameList = std::vector<CPListItem*>;

static int (*_get_strat_data)(int scx, int scy, sdm_strat_data*);

static SuperCellMap _scmap;
static ChildMap _childmap;
static LinkMap _linkmap;
static DisplayNameMap _nameMap;
static CPNameList _cpnameList; // this is loaded to enable cp name search in UI with/without low memory option

// strat localized name loader
class DisplayNameLoader : public XMLLoader2<DisplayNameLoader>
{
public:

	DisplayNameLoader(const char* path) : XMLLoader2<DisplayNameLoader>(path)
	{
		AddHandler("name", &DisplayNameLoader::DoName, this);
	}

	virtual ~DisplayNameLoader()
	{
	}

	// tag handlers
	XML_TAG_HANDLER(DoName)
	{
		if(first)
		{
			std::string key = xmlGetStringFromNode(node, "english");
			std::wstring value = uiUTF8toUTF16(xmlGetStringFromNode(node, "localized"));

			if(key.size() && value.size())
				_nameMap[key] = value;
		}

		return true;
	}
};

void StratDB::AllStratObjectsInWorld(StratObjList& objlist)
{
	objlist.clear();

	SuperCellMap::iterator it = _scmap.begin();
	for(; it != _scmap.end(); ++it)
	{
		StratObjectMap& somap = it->second._stratobjs;
		StratObjectMap::iterator stratit = somap.begin();
		for(; stratit != somap.end(); ++stratit)
			objlist.push_back(stratit->second);
	}
}

void StratDB::AllCPsInWorld(StratObjList& objlist)
{
	objlist.clear();

	SuperCellMap::iterator it = _scmap.begin();
	for(; it != _scmap.end(); ++it)
	{
		StratObjectMap& somap = it->second._stratobjs;
		StratObjectMap::iterator stratit = somap.begin();
		for(; stratit != somap.end(); ++stratit)
		{
			if(stratit->second->StratClass() == SDM_CLASS_CP)
				objlist.push_back(stratit->second);
		}
	}
}

void StratDB::CPsInSuperCell(StratObjList& objlist, int scx, int scy)
{
	objlist.clear();

	supercell& sc = _scmap[supercell_id(scx,scy)];
	if(!sc._loaded)
		sc.load(scx, scy);

	StratObjectMap::iterator stratit = sc._stratobjs.begin();
	for(; stratit != sc._stratobjs.end(); ++stratit)
	{
		StratObject* stratobj = stratit->second;
		if(stratobj->StratClass() == SDM_CLASS_CP)
		{
			unsigned int i = stratobj->GetIndex();
			if(i >= objlist.size())
				objlist.resize(i+1);

			objlist[i] = stratobj;
		}
	}
}

void StratDB::FacilitiesInSuperCell(StratObjList& objlist, int scx, int scy)
{
	objlist.clear();

	supercell& sc = _scmap[supercell_id(scx,scy)];
	if(!sc._loaded)
		sc.load(scx, scy);

	StratObjectMap::iterator stratit = sc._stratobjs.begin();
	for(; stratit != sc._stratobjs.end(); ++stratit)
	{
		StratObject* stratobj = stratit->second;
		if(stratobj->StratClass() == SDM_CLASS_FACILITY)
		{
			unsigned int i = stratobj->GetIndex();
			if(i >= objlist.size())
				objlist.resize(i+1);

			objlist[i] = stratobj;
		}
	}
}

static bool stratobj_index_cmp(const StratObject* const& a, const StratObject* const& b)
{
	return (a->GetIndex() < b->GetIndex());
}

StratObjList& StratDB::BuildingsInOctet(const int scx, const int scy, const int c, const int r)
{
    static int oldframetime = 0;
    static int oldsx = 0, oldsy = 0, oldr = 0, oldc = 0;

    static StratObjList list;

    // List already created for this frame
    if(frametime == oldframetime)
        return list;

    // No need to recreate list
    if(scx == oldsx && scy == oldsy && c == oldc && r == oldr)
        return list;

    list.clear();

	supercell& sc = _scmap[supercell_id(scx,scy)];
	if(!sc._loaded)
		sc.load(scx, scy);

	StratObjectMap::iterator stratit = sc._stratobjs.begin();
	for(; stratit != sc._stratobjs.end(); ++stratit)
	{
		StratObject* stratobj = stratit->second;

		if(stratobj->StratClass() == SDM_CLASS_BUILDING)
		{
            float x = stratobj->PosX() - scx * SDM_SUPERCELL_SIZE;
            float y = stratobj->PosY() - scy * SDM_SUPERCELL_SIZE;

            // Determine row/column in the supercell to octet level
            int col = (int)(x / SDM_OCTET_SIZE);
            int row = (int)(y / SDM_OCTET_SIZE);

			if(r != row || c != col) continue;

			list.push_back(stratobj);
		}
	}

	std::sort(list.begin(), list.end(), stratobj_index_cmp);

    oldframetime = frametime;
    oldsx = scx;
    oldsy = scy;
    oldr = r;
    oldc = c;

    return list;
}

void strtApplySuperCellVisLevels(const SupercellVislevelXfer& xfer)
{
	// Look up which supercell this xfer is for.
	const SINT16 scx = xfer.scx();
	const SINT16 scy = xfer.scy();

	supercell& sc = _scmap[supercell_id(scx, scy)];

	if(!sc._loaded) sc.load(scx, scy);

	// temp object to pass the data down
	Strat::EventData ed;

	const size_t numBuildings = sc.numBuildings();

	//uiChatPrintLine("Applying Builing Vis Levels for %d,%d (%d buildings)", (SINT32)scx, (SINT32)scy, (SINT32)numBuildings);

	for(size_t i = 0 ; i < numBuildings ; ++i)
	{
		StratObject* const stratobj = sc.building(i);
		const SupercellVislevelXfer::vis_t newLevel = xfer.getBuildingLevel(i);

		// update the strat object
		stratobj->SetVisLevel(newLevel);

		// if we found a fac, use its owner as our building's owner
		StratDB::StratObject* fac = stratobj->ParentStratObject();

		// set our strat data
		ed.fid		= *(stratobj->FullObjID());
		ed.level	= stratobj->VisLevel();
		ed.owner	= (fac) ? fac->Country() : stratobj->Country();

		// this will queue or apply the update depending on
		// whether the terrain detail is available or not
		clientApplyStratEventData(ed); // don't queue bulding events
	}
}

/************************************************************************************************
	Init
*************************************************************************************************/

int StratDB::Init(stratDatafn fn)
{
	_get_strat_data = fn;

	InitDisplayNames();

	return 0;
}

void StratDB::Kill()
{
	_scmap.clear();
	_childmap.clear();
	_linkmap.clear();

    for(size_t i = 0; i < _cpnameList.size(); i++)
        delete _cpnameList[i];

    _cpnameList.clear();
}

void StratDB::InitDisplayNames()
{
    static bool initialized = false;

    // Only initialize once
    if(initialized)
        return;

    initialized = true;

	_nameMap.clear();
    _cpnameList.clear();

	std::string path = std::string("languages/") + g_szLanguages[0] + "/strat.xml";
	DisplayNameLoader loader(path.c_str());
	loader.Load();

    char* text;
    unsigned int size = 0;
    fioLoad("ui/cplist.txt", text, &size ); // format: "name"|x|y|
    if ( size != 0 )
    {
        char s[256];
        std::stringstream ss( text, std::stringstream::in );
        while (ss.getline(s,255))
        {
			///FIXME: This is likely to crash in a bad way,
			///FIXME: replace with a sscanf or something.
            char* t1 = strtok(s,"|");
            int t2 = atoi(strtok(nullptr,"|"));
            int t3 = atoi(strtok(nullptr,"|"));

            DisplayNameMap::iterator it = _nameMap.find(t1);

            std::wstring dn = (it != _nameMap.end()) ? it->second : std::wstring(uiUTF8toUTF16(t1));
            CPListItem* cpl = new CPListItem(t1,dn,t2,t3);
            _cpnameList.push_back(cpl);
        }
    }

    if(text)
    {
        memFree(text);
        text = nullptr;
    }
}

/************************************************************************************************
	StratObject
*************************************************************************************************/

void StratObject::Init()
{
	memset(&_sdmdata, 0x0, sizeof(_sdmdata));
	_sx = _sy = 0;
	_id = _stratclass = _strattype = 0;
	_posx = _posy =
	_oside = _ocountry = 0;
	_subtype = 0;
	_side = _country = 0;
	_fullid.init();
	_parentid.init();
	_name = _filename = "";
	_parent = 0;
	_linkedDepot = 0;
	_stratflags = 0;
	_updatetime = 0;
	_loaded = false;
	_vislevel = 0;
	_index = 0;
}

StratObject::StratObject()
{
	Init();
}

StratObject::StratObject(sdm_cp const& cp, int sx, int sy)
{
	Init();
	_sx = sx;
	_sy = sy;
	Set(cp);
}

StratObject::StratObject(sdm_facility const& fac, int sx, int sy)
{
	Init();
	_sx = sx;
	_sy = sy;
	Set(fac);
}

StratObject::StratObject(sdm_building const& bld, int sx, int sy)
{
	Init();
	_sx = sx;
	_sy = sy;
	Set(bld);
}

/************************************************************************************************
	Set
*************************************************************************************************/

void StratObject::Set(sdm_cp const& cp)
{
	_sdmdata.cp = cp;

	_id = cp.oid.id;
	_stratclass = cp.oid.objClass;
	_strattype = cp.oid.objType;
	_posx = cp.posx;
	_posy = cp.posy;
	_side = _oside = cp.side;
	_country = _ocountry = cp.country;
	_subtype = cp.subType;
	_name = (const char*)cp.name;
	_filename = (const char*)cp.fileName;

	_fullid.cell.ox = static_cast<SINT16>(static_cast<UINT16>(_sx) << 4);
	_fullid.cell.oy = static_cast<SINT16>(static_cast<UINT16>(_sy) << 4);
	_fullid.id = cp.oid.id;
	_fullid.objClass = cp.oid.objClass;
	_fullid.objType = cp.oid.objType;

	_parentid.init();
	_parent = 0;

	_side = _oside;
	_country = _ocountry;
}

/************************************************************************************************
	Set
*************************************************************************************************/

void StratObject::Set(sdm_facility const& fac)
{
	_sdmdata.facility = fac;

	_id = fac.oid.id;
	_stratclass = fac.oid.objClass;
	_strattype = fac.oid.objType;
	_posx = fac.posx;
	_posy = fac.posy;
	_side = _oside = fac.originalSide;
	_country = _ocountry = fac.originalCountry;
	_subtype = fac.subType;
	_name = (const char*)fac.name;
	_filename = (const char*)fac.fileName;

	_fullid.cell.ox = static_cast<SINT16>(static_cast<UINT16>(_sx) << 4);
	_fullid.cell.oy = static_cast<SINT16>(static_cast<UINT16>(_sy) << 4);
	_fullid.id = fac.oid.id;
	_fullid.objClass = fac.oid.objClass;
	_fullid.objType = fac.oid.objType;

	_parentid = fac.cpid;
	_parentid.clearOctetBits();

	_parent = 0;

	if(_parentid.valid())
	{
		ChildList& childlist = _childmap[_parentid.lookup_id()];
		childlist.push_back(this);
	}
}

/************************************************************************************************
	Set
*************************************************************************************************/

void StratObject::Set(sdm_building const& bld)
{
	_sdmdata.building = bld;

	_id = bld.oid.id;
	_stratclass = bld.oid.objClass;
	_strattype = bld.oid.objType;
	_posx = bld.posx;
	_posy = bld.posy;
	_side = _oside = 0;
	_country = _ocountry = 0;
	_subtype = bld.subType;
	_name = (const char*)bld.name;
	_filename = (const char*)bld.fileName;

	_fullid.cell.ox = static_cast<SINT16>(static_cast<UINT16>(_sx) << 4);
	_fullid.cell.oy = static_cast<SINT16>(static_cast<UINT16>(_sy) << 4);
	_fullid.id = bld.oid.id;
	_fullid.objClass = bld.oid.objClass;
	_fullid.objType = bld.oid.objType;

	_parentid.cell.ox = _fullid.cell.ox;
	_parentid.cell.oy = _fullid.cell.oy;
	_parentid.id = bld.pid.id;
	_parentid.objClass = bld.pid.objClass;
	_parentid.objType = bld.pid.objType;

	_parent = 0;

	if(_parentid.valid())
	{
		ChildList& childlist = _childmap[_parentid.lookup_id()];
		childlist.push_back(this);
	}

}

/************************************************************************************************
	SetCountry
*************************************************************************************************/

void StratObject::SetCountry(int country)
{
	_country  = country;
	_side = (int)wwiiSide(_country);
}

void StratObject::SetName(const char* name)
{
	_name = name;
}

/************************************************************************************************
	ParentStratObject
*************************************************************************************************/

StratObject* StratObject::ParentStratObject() const
{
	if(_parent) return _parent;
	if(!_parentid.valid()) return 0;

	return CreateParentStratObject();
}

StratObject* StratObject::CreateParentStratObject() const
{
	int psx = _parentid.sx();
	int psy = _parentid.sy();

	supercell& sc = _scmap[supercell_id(psx,psy)];
	if(!sc._loaded)
		sc.load(psx, psy);

	StratObjectMap::iterator it = sc._stratobjs.find(_parentid.lookup_id());
	if(it != sc._stratobjs.end())
		_parent = it->second;
	else
		_parentid.objClass = -1; // sorry we can't find it

	return _parent;
}

/************************************************************************************************
	ChildStratObjects
*************************************************************************************************/

StratObject::ChildList StratObject::ChildStratObjects() const
{
	ChildMap::iterator it = _childmap.find(_fullid.lookup_id());
	if(it != _childmap.end())
		return it->second;

	return ChildList();
}

/************************************************************************************************
	FindBestUnit
*************************************************************************************************/

unsigned int StratObject::FindBestUnit(int s, int c, int b, bool ignoreGarrisons) const
{
	HCUnit* best = nullptr;
	HCUnit* unit = nullptr;

	for(int i = 0; i < (int)_units.size(); ++i)
	{
		unit = HCUnit::Lookup(_units[i]);

		if(unit && !unit->isTrainingUnit() && !(unit->isGarrison() && ignoreGarrisons))
		{
			if(!best)
				best = unit;
			else if(unit->side() == s && best->side() != s)
				best = unit;
			else if(unit->country() == c && best->country() != c)
				best = unit;
			else if(unit->country() == c && unit->branch() == b)
			{
				best = unit;
				break;
			}
		}
	}

	return (best) ? best->unitID(): 0;
}

// finds the best allied unit from an allied country
unsigned int StratObject::FindBestAlliedUnit(int s, int c) const
{
	unsigned int best = 0;
	HCUnit* unit = 0;

	for(int i = 0; i < (int)_units.size(); ++i)
	{
		unit = HCUnit::Lookup(_units[i]);

		if(unit && !unit->isTrainingUnit())
		{
			if(unit->side() == s && unit->country() != c)
			{
				best = _units[i];
				break;
			}
		}
	}

	return best;
}

static bool SortUnitsByDivision(const UINT32& lhs, const UINT32& rhs)
{
	const HCUnit* const lu = HCUnit::Lookup(lhs);
	const HCUnit* const ru = HCUnit::Lookup(rhs);

	if(lu->country() != ru->country())
		return lu->country() < ru->country();

	if(lu->branch() != ru->branch())
		return lu->branch() < ru->branch();

	if((!lu->isBrigade() && !lu->isDivision()) || (!ru->isBrigade() && !ru->isDivision()))
		return lu->unitID() < ru->unitID();

	if(lu == ru->parent())
		return true;

	if(!lu->parent() && ru->parent())
		return false;

	if(lu->parent() && !ru->parent())
		return true;

	if(lu->parent() == ru->parent())
		return (strcoll(lu->name(), ru->name()) < 0);

	if(lu->isDivision() && ru->isBrigade())
		return (strcoll(lu->name(), ru->parent()->name()) < 0);
	if(ru->isDivision() && lu->isBrigade())
		return strcoll(lu->parent()->name(), ru->name()) < 0;
	return strcoll(lu->name(), ru->name()) < 0;
}

void StratObject::AddUnit(unsigned int id)
{
	UnitList::iterator it = _units.begin();

	for(; it != _units.end(); ++it)
	{
		if(id == *it)
			return;
	}

	_units.push_back(id);

	std::sort(_units.begin(), _units.end(), SortUnitsByDivision);
}

void StratObject::RemoveUnit(unsigned int id)
{
	UnitList::iterator it = _units.begin();

	for(; it != _units.end(); ++it)
	{
		if(id == *it)
		{
			_units.erase(it);
			break;
		}
	}
}

bool StratObject::IsUnitHere(unsigned int id) const
{
	// only cp's have units
	if(StratClass() == SDM_CLASS_CP)
	{
		UnitList::const_iterator it = _units.begin();

		for(; it != _units.end(); ++it)
		{
			if(id == *it)
				return true;
		}
	}
	else
	{
		StratObject* parent = ParentStratObject();
		if(parent) return parent->IsUnitHere(id);
	}

	return false;
}

bool StratObject::IsSpawnable() const
{
	bool spawnable = false;

	if(StratClass() == SDM_CLASS_FACILITY)
	{
		switch(StratType())
		{
			case SDM_FACILITY_DEPOT:
			case SDM_FACILITY_FIREBASE:
			case SDM_FACILITY_AIRBASE:
			case SDM_FACILITY_ARMYBASE:
			case SDM_FACILITY_NAVALBASE:
				spawnable = true;
			break;
			default:
				// not spawnable
			break;
		}
	}

	return spawnable;
}

bool StratObject::IsDeepWaterPort() const
{
	// at some point this code should be replaced by testing cp state flags
	StratDB::StratObject* cp = (StratDB::StratObject*)this;

	// make sure we're operating on the CP
	while(cp != nullptr && cp->StratClass() != SDM_CLASS_CP)
		cp = cp->ParentStratObject();

	if(!cp)
		return false;

	StratObject::ChildList facilities = cp->ChildStratObjects();
	StratObject::ChildList::iterator fit = facilities.begin();

	for(; fit != facilities.end(); ++fit)
	{
		StratDB::StratObject* facility = *fit;

		if(facility->StratClass() == SDM_CLASS_FACILITY && facility->StratType() == SDM_FACILITY_NAVALBASE)
		{
			StratDB::StratObject::ChildList buildings = facility->ChildStratObjects();
			StratDB::StratObject::ChildList::iterator bit = buildings.begin();

			for(; bit != buildings.end(); ++bit)
			{
				if((*bit)->StratClass() == SDM_CLASS_BUILDING && (*bit)->StratType() == SDM_BUILDING_SPAWN)
				{
					StratDB::StratObject* building = *bit;

					// look for the ship spawner
					if(strcasecmp(building->Filename(), "svz34_spawn1c") == 0)
						return true;
				}
			}
		}
	}

	return false;
}

bool StratObject::IsRelated(const FULL_OBJ_ID id) const
{
	return IsRelated(StratDB::FindStratObject(id));
}

bool StratObject::IsRelated(const StratDB::StratObject* strat) const
{
	if(strat == nullptr)
		return false;

	StratDB::StratObject* myCP = (StratClass() == SDM_CLASS_CP) ?
									(StratDB::StratObject*)this:
									ParentStratObject();

	StratDB::StratObject* targetCP = (strat->StratClass() == SDM_CLASS_CP) ?
										(StratDB::StratObject*)strat:
										strat->ParentStratObject();

	while(myCP != nullptr && myCP->StratClass() != SDM_CLASS_CP)
		myCP = myCP->ParentStratObject();

	while(targetCP != nullptr && targetCP->StratClass() != SDM_CLASS_CP)
		targetCP = targetCP->ParentStratObject();

	return (myCP != nullptr && targetCP == myCP) ? true: false;
}


/************************************************************************************************
	GetLinkedDepot - given a depot facility, return the sister depot facility
*************************************************************************************************/

StratObject* StratObject::GetLinkedDepot() const
{
	// lookup and cache the linked depot facility if we're a depot and we haven't done this yet
	if(!_linkedDepot && _stratclass == SDM_CLASS_FACILITY && _strattype == SDM_FACILITY_DEPOT)
	{
		StratObject::ChildList children = ChildStratObjects();
		StratObject::ChildList::iterator it = children.begin();

		for(; it != children.end(); ++it)
		{
			LinkMap::const_iterator lit = _linkmap.find((*it)->FullId().lookup_id());
			if(lit != _linkmap.end())
			{
				StratObject* depotBuilding = FindStratObject(lit->second.endDepotId);

				if(depotBuilding)
					_linkedDepot = depotBuilding->ParentStratObject();

				break;
			}
		}
	}

	return _linkedDepot;
}

bool StratObject::GetAreaXY(int& ax, int& ay)
{
	const StratDB::StratObject* strat = this;

	while(strat)
	{
		if(strat->StratClass() == SDM_CLASS_CP)
			break;

		strat = strat->ParentStratObject();
	}

	if(!strat) return false;

	ax = strat->PosX();
	ay = strat->PosY();
	int count = 1;

	StratDB::StratObject::ChildList const& children = strat->ChildStratObjects();
	StratDB::StratObject::ChildList::const_iterator it;

	for(it = children.begin() ; it != children.end() ; ++it)
	{
		if((*it)->StratType() != SDM_FACILITY_FIREBASE)
		{
			ax += (*it)->PosX();
			ay += (*it)->PosY();
			++count;
		}
	}

	ax = int(ax/count);
	ay = int(ay/count);

	return true;
}

/************************************************************************************************
	DisplayName
*************************************************************************************************/

const wchar_t* StratObject::DisplayName() const
{
	if(_nameMap.size())
	{
		DisplayNameMap::iterator it = _nameMap.find(_name.c_str());

		if(it != _nameMap.end())
			return it->second.c_str();
	}

	return UTF16(_name.c_str());
}

/************************************************************************************************
	DisplayName
*************************************************************************************************/

unsigned int StratObject::StratFlags(bool filter) const
{
	return (filter) ? clientStrtFilterFlags((WWII_SIDE)Side(), _stratflags) : _stratflags;
}

/************************************************************************************************
	LoadSuperCell
*************************************************************************************************/

void StratDB::LoadSuperCell(int scx, int scy)
{
	supercell& sc = _scmap[supercell_id(scx,scy)];
	if(!sc._loaded)
		sc.load(scx, scy);
}

bool StratDB::SuperCellLoaded(int scx, int scy)
{
	return _scmap[supercell_id(scx,scy)]._loaded;
}

/************************************************************************************************
	FindStratObject
*************************************************************************************************/

StratObject* StratDB::FindStratObject(const sdm_full_obj_id& fullid)
{
	sdm_full_obj_id id = fullid;
	id.clearOctetBits();

	supercell& sc = _scmap[supercell_id(id.sx(),id.sy())];
	if(!sc._loaded)
		sc.load(id.sx(),id.sy());

	StratObjectMap::iterator it = sc._stratobjs.find(id.lookup_id());
	if (it == sc._stratobjs.end())
		return nullptr;
	return it->second;
}

StratObject* StratDB::FindStratObject(const FULL_OBJ_ID fullobjid)
{
	return FindStratObject(sdm_full_obj_id::from_foi(fullobjid));
}

StratObject* StratDB::FindStratObject(const char* cp, const char* fac)
{
	StratObject* found = nullptr;

	StratObjList list;
	AllCPsInWorld(list);

	StratObjList::iterator cit = list.begin();
	for(; cit != list.end(); cit++)
	{
		if(!strcasecmp((*cit)->Name(), cp))
		{
			if(fac && *fac)
			{
				StratObject::ChildList children = (*cit)->ChildStratObjects();
				StratObject::ChildList::iterator fit = children.begin();
				for(; fit != children.end(); fit++)
				{
					if(!strcasecmp((*fit)->Name(), fac))
					{
						found = *fit;
						break;
					}
				}
			}
			else
			{
				found = *cit;
			}

			break;
		}
	}

	return found;
}

bool StratDB::FindStratObjects(StratObjList& list, int klass, int type, int subtype, StratObject* strat)
{
	bool found = false;

	if(strat)
	{
		if((klass == -1 || strat->StratClass() == klass) &&
		   (type == -1 || type == strat->StratType()) &&
		   (subtype == -1 || subtype == strat->SubType()))
		{
			found = true;
			list.push_back(strat);
		}

		StratObject::ChildList children = strat->ChildStratObjects();
		StratObject::ChildList::iterator cit = children.begin();

		for(; cit != children.end(); ++cit)
		{
			if(FindStratObjects(list, klass, type, subtype, *cit))
				found = true;
		}
	}
	else
	{
		StratObjList search;
		AllCPsInWorld(search);

		StratObjList::iterator sit = search.begin();

		for(; sit != search.end(); sit++)
		{
			if(FindStratObjects(list, klass, type, subtype, *sit))
				found = true;
		}
	}

	return found;
}

/************************************************************************************************
	AddStratData
*************************************************************************************************/

void supercell::AddStratData(sdm_strat_data* strat)
{
	_loaded = true;

	sdm_lookup_id id = { static_cast<SINT16>(static_cast<UINT16>(_sx) << 4), static_cast<SINT16>(static_cast<UINT16>(_sy) << 4), 0 };
	StratObject* stratobj;

	for(int i = 0; i < strat->numcps; i++)
	{
		id.id = strat->cp[i].oid.id;
		_stratobjs[id] = stratobj = new StratObject(strat->cp[i], _sx, _sy);
		stratobj->SetIndex(i);
	}

	for(int i = 0; i < strat->numfacs; i++)
	{
		id.id = strat->facility[i].oid.id;
		_stratobjs[id] = stratobj = new StratObject(strat->facility[i], _sx, _sy);
		stratobj->SetIndex(i);
	}

	_buildings.resize(strat->numblds);
	for(int i = 0; i < strat->numblds; i++)
	{
		id.id = strat->building[i].oid.id;
		_stratobjs[id] = stratobj = new StratObject(strat->building[i], _sx, _sy);
		stratobj->SetIndex(i);
		_buildings[i] = stratobj;
	}

	for(int i = 0; i < strat->numdepots; i++)
	{
		sdm_depot const& depot = strat->depot[i];
		_linkmap[depot.startDepotId.lookup_id()] = depot;
	}
}

/************************************************************************************************
	loadsupercell
*************************************************************************************************/

void supercell::load(int scx, int scy)
{
	_sx = scx; _sy = scy;

    if ( scx == 0 && scy == 0 )
    {
        return;
    }

	static sdm_strat_data strat;
	if(_get_strat_data(scx, scy, &strat) == 0)
		AddStratData(&strat);
}
