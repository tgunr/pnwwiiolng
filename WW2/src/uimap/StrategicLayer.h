#pragma once

#include <map>
#include "uimap/SuperCellMapView.h"
#include "wwii/wwiiHCUnits.h"

/************************************************************************************************
	StratLayer
*************************************************************************************************/

class StrategicLayer;
namespace StratDB{ class StratObject; }

class HCLinksMapObject : public SuperCellMapView::MapObject
{
public:
	HCLinksMapObject(StrategicLayer*);
	virtual void DrawSelf(SuperCellMapView*);
	virtual void Over(SuperCellMapView*,int state){}
	virtual bool IsHit(SuperCellMapView*, float x, float y) const { return false; }

	void SetDivision(HCUnitID id);
	void SetCP(StratDB::StratObject*);
	void SetUnits();
	void SetBounds(const HCUnit*);

	HCUnit* GetDivision(HCUnit*);

	HCUnit* _division;
	StratDB::StratObject* _cp;
	StrategicLayer* _strategiclayer;

	// bounding box coords bottom left to top right
	int _x1,_y1,_x2,_y2;

	using HCUnitMap = std::map<HCUnitID, HCUnit*>;
	HCUnitMap _units;
};

class StrategicLayer : public SuperCellMapView::Renderer
{
public:
    StrategicLayer() {};
	~StrategicLayer() {};

	virtual SuperCell* NewSuperCell(Renderer* r, int scx, int scy) { return 0; }

	HCLinksMapObject* AddHCLinksMapObject();

	void SetDivision(HCUnitID id);
	void SetCP(StratDB::StratObject* cp);

	enum
	{
		STRATEGIC_HCLINKS // draws lines between HC Units of the selected division
	};

	using StrategicMap = std::map<unsigned int, phui::ref_ptr<SuperCellMapView::MapObject> >;
	StrategicMap _strategic;
};
