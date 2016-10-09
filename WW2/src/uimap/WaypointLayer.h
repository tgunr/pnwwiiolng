#pragma once

#include <map>
#include <string>

#include "chat/chatWaypoint.h"
#include "uimap/SuperCellMapView.h"
#include "wwii/wwiiFullObjID.h"

/************************************************************************************************
	WaypointLayer
*************************************************************************************************/
class WaypointLayer;

class WaypointMapObject : public SuperCellMapView::MapObject
{
public:
	WaypointMapObject(WaypointLayer*, Waypoint const&);

	virtual void DrawSelf(SuperCellMapView*);
	virtual void Select(SuperCellMapView*);
	virtual void Deselect(SuperCellMapView*);
	virtual bool IsHit(SuperCellMapView*, float x, float y) const;
	virtual void Over(SuperCellMapView*, int state);

	Waypoint _waypoint;
	bool _remove;
	WaypointLayer* _waypointlayer;
	bool _mouseover;
	bool _on;
	unsigned int _lasttime;
	std::wstring _name;
};

class WaypointLayer : public SuperCellMapView::Renderer
{
public:
	WaypointLayer();
	~WaypointLayer() {};

	virtual void RenderSuperCells(SuperCellMapView*, int scx1, int scy1, int scx2, int scy2);
	virtual SuperCell* NewSuperCell(Renderer* r, int scx, int scy) { return 0; }

	using WaypointMap = std::map<ChatHandle, phui::ref_ptr<WaypointMapObject>>;
	WaypointMap _waypoints;
	int _waypointtexid ,_contacttexid, _markertexid;

	int _aiCount;
	bool _aiInit;

	static FULL_OBJ_ID nearestCP;
};
