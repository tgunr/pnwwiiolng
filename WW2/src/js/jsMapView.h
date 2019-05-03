#pragma once

#include "phui/DOM.h"
#include "uimap/SuperCellMapView.h"

class jsPosition;
class jsCPObject;
class jsStratObject;

/************************************************************************************************
	jsMapView
*************************************************************************************************/

class jsMapView : public phui::DOM::ElementDOM
{
public:
	jsMapView(SuperCellMapView*, phui::DOM::ElementDOM*);

	virtual JSBool GetProperty(JSContext* ctx, int propid, jsval* val);
	virtual JSBool SetProperty(JSContext* ctx, int propid, jsval* val);

	JS_FUNCTION(stratfilter);
	JS_FUNCTION(enablelayer);
	JS_FUNCTION(findcpobject);
	JS_FUNCTION(addlayer);
	JS_FUNCTION(addwaypoint);
	JS_FUNCTION(deletewaypoint);
	JS_FUNCTION(findmapobject);
	JS_FUNCTION(approvewaypoint);
	JS_FUNCTION(getnexthandle);
	JS_FUNCTION(setactivewaypoint);
	JS_FUNCTION(sethclinks);
	JS_FUNCTION(panto);
	JS_FUNCTION(findmapobjects);
	JS_FUNCTION(deletereports);
	JS_FUNCTION(approvereports);
	JS_FUNCTION(getposition);
	JS_FUNCTION(findstratob);

	JS_CLASS_DATA
	enum{ POSITION_PROP=100, SCALE_PROP, SELECTEDOBJECT_PROP, OBJECTCLICKED_PROP,
		MX_PROP, MY_PROP, MOUSEPOSITION_PROP, WAYPOINTS_PROP, ACTIVEWAYPOINT_PROP,
		REPORTS_PROP, OVEROBJECT_PROP, STABILIZED_PROP, MAP_SCALE_PROP };

	SuperCellMapView* _mapview;
	phui::js::root_ptr<jsPosition> _mappos;

protected:
	virtual ~jsMapView();

	phui::js::root_ptr<jsCPObject> _jsSelectedObject;
	phui::js::root_ptr<jsCPObject> _jsOverObject;
	phui::js::array_root_ptr _waypoints;
	phui::js::array_root_ptr _reports;
	phui::js::array_root_ptr _findlist;
};

class StratMapObject;

class jsCPObject : public phui::js::Object
{
public:
	jsCPObject(StratMapObject*);

	JS_CLASS_DATA;
	enum { STRATOBJ_PROP, OBJECTTYPE_PROP };

	virtual JSBool GetProperty(JSContext* ctx, int propid, jsval* val);

	phui::ref_ptr<StratMapObject> _mapobj;
	phui::js::root_ptr<jsStratObject> _jsstratobj;
};

class WaypointMapObject;

class jsWaypointObject : public phui::js::Object
{
public:
	jsWaypointObject(WaypointMapObject*);

	JS_CLASS_DATA;

	enum
	{
		OBJECTTYPE_PROP, LOCAL_PROP, SHARED_PROP, NAME_PROP, NOTES_PROP,
		POSITION_PROP, ACTIVE_PROP, TYPE_PROP, DISPLAYNAME_PROP, CREATOR_PROP
	};

	JS_FUNCTION(onrightclick);

	virtual JSBool GetProperty(JSContext* ctx, int propid, jsval* val);

	phui::ref_ptr<WaypointMapObject> _mapobj;
};

class VehicleMapObject;

class jsVehicleMapObject : public phui::js::Object
{
public:
	jsVehicleMapObject(VehicleMapObject*);

	JS_CLASS_DATA;
	enum { OBJECTTYPE_PROP, CALLSIGN_PROP };
	JS_FUNCTION(onrightclick);
	JS_FUNCTION(setchattoplayer);

	virtual JSBool GetProperty(JSContext* ctx, int propid, jsval* val);

	phui::ref_ptr<VehicleMapObject> _vehobj;
};

