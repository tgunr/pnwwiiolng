#include <syscfg.h> // Path is project specific.
#include <string.h>

#include "phui/Util.h"

#include "js/jsMapView.h"
#include "js/jsPosition.h"
#include "js/jsStratDB.h"
#include "uimap/CPRenderer.h"
#include "uimap/uiMap.h"
#include "uimap/WaypointLayer.h"
#include "client/GameClient.h"
#include "include/vars.h"
#include "veh/vehmain.h"
#include "pers/persmain.h"
#include "uimap/PlayerLayer.h"
#include "uimap/StrategicLayer.h"
#include "ui/uiChat.h"
#include "ui/uiUTF.h"
#include "chat/chatHandle.h"


#ifdef BETA_RELEASE
#include "ui/uiDotCommands.h"
#endif

/************************************************************************************************
	jsMapView
*************************************************************************************************/

JSClass jsCPObject::jsclass = JS_CLASS_MACRO("cpmapobject");
JSFunctionSpec jsCPObject::jsfunctions[] =
{
	{0}
};
JSPropertySpec jsCPObject::jsproperties[] =
{
	{"stratobject",		jsCPObject::STRATOBJ_PROP,		JSPROP_PERMANENT | JSPROP_READONLY},
	{"objecttype",		jsCPObject::OBJECTTYPE_PROP,	JSPROP_PERMANENT | JSPROP_READONLY},
	{0}
};

JSClass jsWaypointObject::jsclass = JS_CLASS_MACRO("waypointmapobject");
JSFunctionSpec jsWaypointObject::jsfunctions[] =
{
	JS_FUNCTION_SPEC(jsWaypointObject, onrightclick, 0),
	{0}
};
JSPropertySpec jsWaypointObject::jsproperties[] =
{
	{"objecttype",		jsWaypointObject::OBJECTTYPE_PROP,	JSPROP_PERMANENT | JSPROP_READONLY},
	{"local",				jsWaypointObject::LOCAL_PROP,					JSPROP_PERMANENT | JSPROP_READONLY},
	{"shared",			jsWaypointObject::SHARED_PROP,			JSPROP_PERMANENT | JSPROP_READONLY},
	{"name",				jsWaypointObject::NAME_PROP,					JSPROP_PERMANENT | JSPROP_READONLY},
	{"notes",			jsWaypointObject::NOTES_PROP,			JSPROP_PERMANENT | JSPROP_READONLY},
	{"position",			jsWaypointObject::POSITION_PROP,			JSPROP_PERMANENT | JSPROP_READONLY},
	{"active",			jsWaypointObject::ACTIVE_PROP,			JSPROP_PERMANENT | JSPROP_READONLY},
	{"type",				jsWaypointObject::TYPE_PROP,					JSPROP_PERMANENT | JSPROP_READONLY},
	{"displayname",	jsWaypointObject::DISPLAYNAME_PROP,		JSPROP_PERMANENT | JSPROP_READONLY},
	{0}
};

JSClass jsVehicleMapObject::jsclass = JS_CLASS_MACRO("vehiclemapobject");
JSFunctionSpec jsVehicleMapObject::jsfunctions[] =
{
	JS_FUNCTION_SPEC(jsVehicleMapObject, onrightclick, 0),
	JS_FUNCTION_SPEC(jsVehicleMapObject, setchattoplayer, 0),
	{0}
};
JSPropertySpec jsVehicleMapObject::jsproperties[] =
{
	{"objecttype",	jsVehicleMapObject::OBJECTTYPE_PROP,	JSPROP_PERMANENT | JSPROP_READONLY},
	{"callsign",	jsVehicleMapObject::CALLSIGN_PROP,		JSPROP_PERMANENT | JSPROP_READONLY},
	{0}
};

JSClass jsMapView::jsclass = JS_CLASS_MACRO("mapview");
JSFunctionSpec jsMapView::jsfunctions[] =
{
	JS_FUNCTION_SPEC(jsMapView, stratfilter, 2),
	JS_FUNCTION_SPEC(jsMapView, enablelayer, 2),
	JS_FUNCTION_SPEC(jsMapView, findcpobject,1),
	JS_FUNCTION_SPEC(jsMapView, addlayer,1),
	JS_FUNCTION_SPEC(jsMapView, addwaypoint,3),
	JS_FUNCTION_SPEC(jsMapView, deletewaypoint,1),
	JS_FUNCTION_SPEC(jsMapView, approvewaypoint,1),
	JS_FUNCTION_SPEC(jsMapView, getnexthandle,1),
	JS_FUNCTION_SPEC(jsMapView, setactivewaypoint,1),
	JS_FUNCTION_SPEC(jsMapView, sethclinks,1),
	JS_FUNCTION_SPEC(jsMapView, panto,3),
	JS_FUNCTION_SPEC(jsMapView, findmapobjects,2),
	JS_FUNCTION_SPEC(jsMapView, approvereports,0),
	JS_FUNCTION_SPEC(jsMapView, deletereports,1),
	JS_FUNCTION_SPEC(jsMapView, getposition,2),
	JS_FUNCTION_SPEC(jsMapView, findstratob,2),
	{0}
};
JSPropertySpec jsMapView::jsproperties[] =
{
	{"position",		jsMapView::POSITION_PROP,   	JSPROP_PERMANENT},
	{"scale",			jsMapView::SCALE_PROP,   		JSPROP_PERMANENT},
	{"selectedobject",	jsMapView::SELECTEDOBJECT_PROP, JSPROP_PERMANENT},
	{"objectclicked",	jsMapView::OBJECTCLICKED_PROP,	JSPROP_PERMANENT|JSPROP_READONLY},
	{"mx",				jsMapView::MX_PROP,				JSPROP_PERMANENT|JSPROP_READONLY},
	{"my",				jsMapView::MY_PROP,				JSPROP_PERMANENT|JSPROP_READONLY},
	{"mouseposition",	jsMapView::MOUSEPOSITION_PROP,	JSPROP_PERMANENT|JSPROP_READONLY},
	{"waypoints",		jsMapView::WAYPOINTS_PROP,		JSPROP_PERMANENT|JSPROP_READONLY},
	{"activewaypoint",	jsMapView::ACTIVEWAYPOINT_PROP,	JSPROP_PERMANENT|JSPROP_READONLY},
	{"reports",			jsMapView::REPORTS_PROP,		JSPROP_PERMANENT|JSPROP_READONLY},
	{"overobject",		jsMapView::OVEROBJECT_PROP,		JSPROP_PERMANENT|JSPROP_READONLY},
	{"stabilized",		jsMapView::STABILIZED_PROP,		JSPROP_PERMANENT|JSPROP_READONLY},
	{"mapscale",		jsMapView::MAP_SCALE_PROP,		JSPROP_PERMANENT|JSPROP_READONLY},
	{0}
};

/************************************************************************************************
	jsMapView
*************************************************************************************************/

jsMapView::jsMapView(SuperCellMapView* v, phui::DOM::ElementDOM* w) : ElementDOM(v,w, &jsMapView::jsclass)
{
	JS_CONSTRUCTOR_LOCK;
	DefineFunctions(jsMapView::jsfunctions);
	DefineProperties(jsMapView::jsproperties);

	_mapview = v;
	_mappos = new jsPosition(0, 0, _mapview);
	_waypoints = new phui::js::Array;
	_reports = new phui::js::Array;
	_findlist = new phui::js::Array;
}

jsMapView::~jsMapView()
{
}

JSBool jsMapView::GetProperty(JSContext* ctx, int propid, jsval* val)
{
	switch(propid)
	{
		case POSITION_PROP:
		{
			*val = OBJECT_TO_JSVAL(_mappos->jsObject());
		}
		break;
		case SCALE_PROP:
		{
			JS_NewDoubleValue(_jsctx, _mapview->Scale(), val);
		}
		break;
		case SELECTEDOBJECT_PROP:
		{
			StratMapObject* cp = dynamic_cast<StratMapObject*>(_mapview->SelectedMapObject());
			if(cp)
			{
				if(!_jsSelectedObject.valid() || _jsSelectedObject->_mapobj.ptr() != cp)
                {
#ifdef BETA_RELEASE
					OverrideSpawner = NO_OVERRIDE;
#endif
					_jsSelectedObject = new jsCPObject(cp);
                }

				*val = _jsSelectedObject->jsVal();
			}
			else
			{
				*val = JSVAL_NULL;
			}
		}
		break;
		case OVEROBJECT_PROP:
		{
			StratMapObject* cp = dynamic_cast<StratMapObject*>(_mapview->OverMapObject());
			if(cp)
			{
				if(!_jsOverObject.valid() || _jsOverObject->_mapobj.ptr() != cp)
					_jsOverObject = new jsCPObject(cp);

				*val = _jsOverObject->jsVal();
			}
			else
			{
				*val = JSVAL_NULL;
			}
		}
		break;
		case OBJECTCLICKED_PROP:
			*val = BOOLEAN_TO_JSVAL(_mapview->ObjectClicked());
		break;
		case MX_PROP:
		{
			float x, y;
			_mapview->MousePosition(&x, &y);
			JS_NewDoubleValue(_jsctx, x, val);
		}
		break;
		case MY_PROP:
		{
			float x, y;
			_mapview->MousePosition(&x, &y);
			JS_NewDoubleValue(_jsctx, y, val);
		}
		break;
		case MOUSEPOSITION_PROP:
		{
			float x, y;

			_mapview->MousePosition(&x, &y);
			_mapview->LocalToWorldCoordinates(&x, &y);

			*val = (new jsPosition(x, y))->jsVal();
		}
		break;
		case REPORTS_PROP:
		{
			_reports->clear();

			SuperCellMapView::RendererList rlist = _mapview->RenderList();
			SuperCellMapView::RendererList::iterator it = rlist.begin();

			for(; it != rlist.end(); ++it)
			{
				WaypointLayer* wl = dynamic_cast<WaypointLayer*>(it->ptr());

				if(wl)
				{
					WaypointLayer::WaypointMap::const_iterator it = wl->_waypoints.begin();
					int i = 0;

					for(; it != wl->_waypoints.end(); ++it)
					{
						if(it->second.ptr()->_waypoint.type() == WPT_ENEMY)
						{
							jsWaypointObject* wp = new jsWaypointObject(it->second.ptr());
							_reports->SetElement(wp, i++);
						}
					}

					break;
				}
			}

			*val = _reports->jsVal();
		}
		break;
		case WAYPOINTS_PROP:
		{
			_waypoints->clear();

			SuperCellMapView::RendererList rlist = _mapview->RenderList();
			SuperCellMapView::RendererList::iterator it = rlist.begin();

			for(; it != rlist.end(); ++it)
			{
				WaypointLayer* wl = dynamic_cast<WaypointLayer*>(it->ptr());

				if(wl)
				{
					WaypointLayer::WaypointMap::const_iterator it = wl->_waypoints.begin();
					int i = 0;

					for(; it != wl->_waypoints.end(); ++it)
					{
						if(it->second.ptr()->_waypoint.type() != WPT_ENEMY)
						{
							jsWaypointObject* wp = new jsWaypointObject(it->second.ptr());
							_waypoints->SetElement(wp, i++);
						}
					}

					break;
				}
			}

			*val = _waypoints->jsVal();
		}
		break;
		case ACTIVEWAYPOINT_PROP:
		{
			jsWaypointObject* wp = NULL;
			ChatHandle activehandle = PlayerClient()->ActiveWaypointHandle();

			if(activehandle)
			{
				SuperCellMapView::RendererList rlist = _mapview->RenderList();
				SuperCellMapView::RendererList::iterator it = rlist.begin();

				for(; it != rlist.end(); ++it)
				{
					WaypointLayer* wl = dynamic_cast<WaypointLayer*>(it->ptr());

					if(wl)
					{
						WaypointLayer::WaypointMap::const_iterator it = wl->_waypoints.begin();

						for(; it != wl->_waypoints.end(); ++it)
						{
							if(it->second.ptr()->_waypoint.handle() == activehandle)
							{
								wp = new jsWaypointObject(it->second.ptr());
								break;
							}
						}

						break;
					}
				}
			}

			*val = (wp) ? wp->jsVal(): JSVAL_NULL;
		}
		break;
		case STABILIZED_PROP:
		{
			*val = BOOLEAN_TO_JSVAL(_mapview->Stabilized());
		}
		break;
		case MAP_SCALE_PROP:
		{
			char scalestr[32];

			/*
			float navscale	= _mapview->NavScale();

			if(navscale)
			{
				float width = _mapview->Width();
				float height = _mapview->Height();

				float x = (width/2.0f) + (width * navscale)/2.0f;
				float y = (height/2.0f) + (height * navscale)/2.0f;

				// convert our view coordinates to world coordinates
				_mapview->LocalToWorldCoordinates(&x, &y);

				vehVehicle* v = playerGetVehicle();

				if(v)
				{
					double distance = v->DistanceTo(x, y, (v->ModelData) ? v->ModelData->VehiclePos.z: v->PosData.Pos.z);

					if(distance >= 1000.0f)
						snprintf(scalestr, 31, "%.1f km", distance/1000.0f);
					else
						snprintf(scalestr, 31, "%.0f m", distance);
				}
				else
				{
					scalestr[0] = 0;
				}
			}
			else
			{
			*/
				float mapscale = _mapview->MapScale();

				if(mapscale <= 1)
					snprintf(scalestr, 31, "%.0f km", 1/mapscale);
				else
					snprintf(scalestr, 31, "%.0f m", 1000.0/mapscale);
			//}

			*val = phui::js::NewString(scalestr);
		}
		break;
		default:
			return ElementDOM::GetProperty(ctx, propid, val);
		break;
	}

	return JS_TRUE;
}

JSBool jsMapView::SetProperty(JSContext* ctx, int propid, jsval* val)
{
	jsdouble dval;

	switch(propid)
	{
		case POSITION_PROP:
		{
			jsPosition* jspos = phui::js::Cast<jsPosition*>(_jsctx,*val);
			if(jspos)
			{
				_mapview->SetPositionXY(jspos->X(), jspos->Y());
			}
		}
		break;
		case SCALE_PROP:
		{
			JS_ValueToNumber(_jsctx, *val, &dval);
			_mapview->SetScale((float)dval);
		}
		break;
		case SELECTEDOBJECT_PROP:
		{
			jsCPObject* cpobj = phui::js::Cast<jsCPObject*>(_jsctx,*val);
			if(cpobj == NULL)
			{
				_jsSelectedObject = NULL;
				_mapview->SelectMapObject(NULL);
			}else
			{
				_jsSelectedObject = cpobj;
				_mapview->SelectMapObject(cpobj->_mapobj.ptr());
			}
		}
		break;
		default:
			return ElementDOM::SetProperty(ctx, propid, val);
		break;
	}

	return JS_TRUE;
}

/************************************************************************************************
	enablelayer
*************************************************************************************************/

JS_FUNCTION_DEF(jsMapView, enablelayer)
{
	jsMapView* dom = phui::js::Cast<jsMapView*>(ctx,jsobj);
	if(!dom) return JS_FALSE;

	const char* name = NULL;
	long val = 0;
	if(argc > 0)
		name = JS_GetStringBytes(JS_ValueToString(ctx, argv[0]));
	if(argc > 1)
		JS_ValueToInt32(ctx, argv[1], &val);

	if(name)
	{
		SuperCellMapView* mv = dom->_mapview;

		SuperCellMapView::RendererList rlist = mv->RenderList();
		SuperCellMapView::RendererList::iterator it = rlist.begin();

		for(; it != rlist.end(); ++it)
		{
			if(it->ptr()->name == name)
			{
				it->ptr()->enabled = val;
				break;
			}
		}
	}

	return JS_TRUE;
}

/************************************************************************************************
	findmapobject
*************************************************************************************************/

JS_FUNCTION_DEF(jsMapView, findmapobjects)
{
	*rval = JSVAL_NULL;

	if(argc < 2) return JS_FALSE;
	jsMapView* dom = phui::js::Cast<jsMapView*>(ctx,jsobj);
	if(!dom) return JS_FALSE;

	int32 x, y;
	JS_ValueToInt32(ctx, argv[0], &x);
	JS_ValueToInt32(ctx, argv[1], &y);

	SuperCellMapView::MapObjectList list;
	dom->_mapview->FindObjectsAt((float)x,(float)y,list);

	dom->_findlist->clear();

	if(list.size())
	{
		int i = 0;
		SuperCellMapView::MapObjectList::const_iterator it = list.begin();

		for(; it != list.end(); ++it)
		{
			SuperCellMapView::MapObject* mapobj = it->ptr();

			StratMapObject* cpobj = dynamic_cast<StratMapObject*>(mapobj);
			if(cpobj)
			{
				dom->_findlist->SetElement((new jsCPObject(cpobj))->jsVal(), i++);
				continue;
			}

			VehicleMapObject* vehobj = dynamic_cast<VehicleMapObject*>(mapobj);
			if(vehobj)
			{
				dom->_findlist->SetElement((new jsVehicleMapObject(vehobj))->jsVal(), i++);
				continue;
			}

			WaypointMapObject* wpobj = dynamic_cast<WaypointMapObject*>(mapobj);
			if(wpobj)
			{
				dom->_findlist->SetElement((new jsWaypointObject(wpobj))->jsVal(), i++);
				continue;
			}
		}
	}

	*rval = dom->_findlist->jsVal();

	return JS_TRUE;
}

/************************************************************************************************
	stratfilter
*************************************************************************************************/

JS_FUNCTION_DEF(jsMapView, stratfilter)
{
	if(argc < 2) return JS_FALSE;

	jsMapView* dom = phui::js::Cast<jsMapView*>(ctx,jsobj);
	if(!dom) return JS_FALSE;

	const char* name = NULL;
	const char* value = NULL;

	name	= JS_GetStringBytes(JS_ValueToString(ctx, argv[0]));
	value	= JS_GetStringBytes(JS_ValueToString(ctx, argv[1]));

	if(name && value)
	{
		SuperCellMapView* mv = dom->_mapview;

		SuperCellMapView::RendererList rlist = mv->RenderList();
		SuperCellMapView::RendererList::iterator it = rlist.begin();

		for(; it != rlist.end(); ++it)
		{
			CPRenderer* cr = dynamic_cast<CPRenderer*>(it->ptr());

			if(cr)
				cr->FilterStrat(name, (unsigned char*)value);
		}
	}

	return JS_TRUE;
}

/************************************************************************************************
	findcpobject
*************************************************************************************************/

JS_FUNCTION_DEF(jsMapView, findcpobject)
{
	*rval = JSVAL_NULL;

	jsMapView* dom = phui::js::Cast<jsMapView*>(ctx,jsobj);
	if(!dom) return JS_FALSE;

	jsStratObject* jstratobj = phui::js::Cast<jsStratObject*>(ctx, argv[0]);
	if(!jstratobj) return JS_FALSE;

	if(!jstratobj->stratobj) return JS_TRUE;

	SuperCellMapView* mv = dom->_mapview;
	SuperCellMapView::RendererList renderers = mv->RenderList();
	SuperCellMapView::RendererList::iterator it = renderers.begin();

	for(; it != renderers.end(); ++it)
	{
		SuperCellMapView::Renderer* r = it->ptr();
		CPRenderer* cpr = dynamic_cast<CPRenderer*>(r);
		if(!cpr) continue;

		StratMapObject* mapobj = cpr->FindObject(jstratobj->stratobj->FullId());
		if(!mapobj) break;

		*rval = (new jsCPObject(mapobj))->jsVal();

		break;
	}

	return JS_TRUE;
}

JS_FUNCTION_DEF(jsMapView, findstratob)
{
	*rval = JSVAL_NULL;

	jsMapView* dom = phui::js::Cast<jsMapView*>(ctx,jsobj);
	if(!dom) return JS_FALSE;

	const char* cp	= NULL;
	const char* fac = NULL;

	if(argc > 0) cp		= JS_GetStringBytes(JS_ValueToString(ctx, argv[0]));
	if(argc > 1) fac	= JS_GetStringBytes(JS_ValueToString(ctx, argv[1]));

	if(cp && *cp)
	{
		StratDB::StratObject* strat = StratDB::FindStratObject(cp, fac);
		if(strat) *rval = (new jsStratObject(strat->FullObjID()))->jsVal();
	}

	return JS_TRUE;
}

/************************************************************************************************
	deletewaypoint
*************************************************************************************************/

JS_FUNCTION_DEF(jsMapView, deletewaypoint)
{
	if(argc < 1) return JS_FALSE;

	jsWaypointObject* wayobj = phui::js::Cast<jsWaypointObject*>(ctx, argv[0]);
	if(wayobj)
	{
		PlayerClient()->RemoveWaypoint(wayobj->_mapobj->_waypoint.handle());
	}

	return JS_TRUE;
}

/************************************************************************************************
	deletereports
*************************************************************************************************/

JS_FUNCTION_DEF(jsMapView, deletereports)
{
	jsMapView* dom = phui::js::Cast<jsMapView*>(ctx,jsobj);
	if(!dom) return JS_FALSE;

	const char* type = NULL;
	if(argc > 0) type = JS_GetStringBytes(JS_ValueToString(ctx, argv[0]));

	const std::vector<Waypoint>& waypoints = PlayerClient()->Waypoints();

	for(int i = 0; i < (int)waypoints.size(); i++)
	{
		const Waypoint& wp = waypoints.at(i);

		if(type && !strcasecmp(type, "allwaypoints"))
		{
			switch(wp.type())
			{
				case WPT_MARKER:
				case WPT_RALLY:
				case WPT_ATTACK:
				case WPT_DEFEND:
				case WPT_WAYPOINT:
				case WPT_PATROL:
				case WPT_MORTAR:
					PlayerClient()->RemoveWaypoint(wp.handle());
				break;
			}
		}
		else
		{
			if(wp.type() == WPT_ENEMY)
				PlayerClient()->RemoveWaypoint(wp.handle());
		}
	}

	return JS_TRUE;
}

/************************************************************************************************
	deletewaypoint
*************************************************************************************************/

JS_FUNCTION_DEF(jsMapView, setactivewaypoint)
{
	if(argc < 1) return JS_FALSE;

	jsWaypointObject* wayobj = phui::js::Cast<jsWaypointObject*>(ctx, argv[0]);
	if(wayobj)
		PlayerClient()->SetActiveWaypoint(wayobj->_mapobj->_waypoint.handle());

	return JS_TRUE;
}

/************************************************************************************************
	sethclinks
*************************************************************************************************/

JS_FUNCTION_DEF(jsMapView, sethclinks)
{
	*rval = JSVAL_NULL;

	jsMapView* dom = phui::js::Cast<jsMapView*>(ctx,jsobj);
	if(!dom) return JS_FALSE;

	SuperCellMapView::RendererList rlist = dom->_mapview->RenderList();
	SuperCellMapView::RendererList::iterator it = rlist.begin();

	for(; it != rlist.end(); ++it)
	{
		StrategicLayer* sl = dynamic_cast<StrategicLayer*>(it->ptr());

		if(sl)
		{
			jsBrigade* unit = phui::js::Cast<jsBrigade*>(ctx, argv[0]);

			if(unit)
			{
				sl->SetDivision(unit->Detail().unitID());
			}
			else
			{
				jsStratObject* cp = phui::js::Cast<jsStratObject*>(ctx, argv[0]);

				// have to look it up again because it was complaining about stratobj being const
				if(cp) sl->SetCP(StratDB::FindStratObject(*(cp->stratobj->FullObjID())));
			}

			break;
		}
	}

	return JS_TRUE;
}

/************************************************************************************************
	enablelayer
*************************************************************************************************/

JS_FUNCTION_DEF(jsMapView, panto)
{
	jsMapView* dom = phui::js::Cast<jsMapView*>(ctx,jsobj);
	if(!dom) return JS_FALSE;

	if(argc < 2) return JS_FALSE;

	jsdouble x, y, scale;
	x = y = scale = 0;

	JS_ValueToNumber(ctx, argv[0], &x);
	JS_ValueToNumber(ctx, argv[1], &y);

	if(argc > 2)
		JS_ValueToNumber(ctx, argv[2], &scale);

	if(x || y)
	{
		SuperCellMapView* mv = dom->_mapview;
		mv->SetPanPositionXY((float)x, (float)y, (float)scale);
	}

	return JS_TRUE;
}

/************************************************************************************************
	addlayer
*************************************************************************************************/

static int xmlinputmatch(char const* filename){ return 1;}

JS_FUNCTION_DEF(jsMapView, addlayer)
{
	jsMapView* dom = phui::js::Cast<jsMapView*>(ctx,jsobj);
	if(!dom) return JS_FALSE;

	if(argc < 1) return JS_FALSE;

	const char* xml = JS_GetStringBytes(JS_ValueToString(ctx, argv[0]));
	if(!xml) return JS_FALSE;

	phui::memIOAddBuffer("mapview.addlayer", (void*)xml, strlen(xml));

	xmlRegisterInputCallbacks(xmlinputmatch,
								(xmlInputOpenCallback)phui::memIOCallbacks.openread,
								(xmlInputReadCallback)phui::memIOCallbacks.read,
								(xmlInputCloseCallback)phui::memIOCallbacks.close);

	xmlDocPtr doc = xmlReadFile("mapview.addlayer", NULL, XML_PARSE_DTDLOAD | XML_PARSE_RECOVER);
	if(doc)
	{
		xmlNodePtr node = xmlDocGetRootElement(doc);

		if(node)
			uiMapAddLayer(dom->_mapview, node);

		xmlFreeDoc(doc);
	}
	xmlCleanupInputCallbacks();

	phui::memIORemoveBuffer("mapview.addlayer");

	return JS_TRUE;
}

/************************************************************************************************
	addwaypoint
*************************************************************************************************/

static ChatHandle GetUniqueHandle(char prefix)
{
	int idx = 1;
	ChatHandle handle = 0;

	for(;;)
	{
		bool done = true;

		std::vector<Waypoint>::const_iterator it = PlayerClient()->Waypoints().begin();
		for(; it != PlayerClient()->Waypoints().end(); ++it)
		{
			char buf[32];
			snprintf(buf, 31, "%c%d", prefix, idx);
			handle = chatGetHandle(buf);

			if(it->handle() == handle)
			{
				idx++;
				done = false;
				break;
			}
		}

		if(done)
			break;
	}

	if(!handle)
	{
		char buf[32];
		snprintf(buf, 31, "%c1", prefix);
		handle = chatGetHandle(buf);
	}

	return handle;
}

JS_FUNCTION_DEF(jsMapView, addwaypoint)
{
	*rval = BOOLEAN_TO_JSVAL(JS_FALSE);

	if(!Online) return JS_FALSE;

	if(argc < 3) return JS_FALSE;

	jsMapView* dom = phui::js::Cast<jsMapView*>(ctx,jsobj);
	if(!dom) return JS_FALSE;

	jsdouble jx, jy;
	const char* type = JS_GetStringBytes(JS_ValueToString(ctx, argv[0]));
	JS_ValueToNumber(ctx, argv[1], &jx);
	JS_ValueToNumber(ctx, argv[2], &jy);

	const wchar_t* wname		= L"";
	const wchar_t* wnotes		= L"";
	const char* name			= "";
	const char* notes			= "";

	if(argc >= 4)
	{
		wname = (const wchar_t*)JS_GetStringChars(JS_ValueToString(ctx, argv[3]));
		name = UTF8(wname);
	}

	if(argc >= 5)
	{
		wnotes = (const wchar_t*)JS_GetStringChars(JS_ValueToString(ctx, argv[4]));
		notes = UTF8(wnotes);
	}

	char prefix = 'R';
	ChatHandle handle=0;
	WP_TYPE wptype = WPT_INVALID;
	unsigned int subtype=0;

	vehVehicle* Vehicle = persGetPersonaVehiclePtr(); // change this to mission check so we can do in ui later

	if(!strcmp(type, "air"))
	{
		wptype = WPT_ENEMY;
		subtype = WTP_ENEMY_AIR;
	}
	else if(!strcmp(type, "land"))
	{
		wptype = WPT_ENEMY;
		subtype = WTP_ENEMY_LAND;
	}
	else if(!strcmp(type, "sea"))
	{
		wptype = WPT_ENEMY;
		subtype = WTP_ENEMY_SEA;
	}
	else if(!strcmp(type, "troop"))
	{
		wptype = WPT_ENEMY;
		subtype = WTP_ENEMY_TROOP;
	}
	else if(!strcmp(type, "gun"))
	{
		wptype = WPT_ENEMY;
		subtype = WTP_ENEMY_GUN;
	}
	else if(!strcmp(type, "truck"))
	{
		wptype = WPT_ENEMY;
		subtype = WTP_ENEMY_TRUCK;
	}
	else if(!strcmp(type, "danger"))
	{
		wptype = WPT_ENEMY;
		subtype = WTP_ENEMY_DANGER;
	}
	else if(!strcmp(type, "unknown"))
	{
		wptype = WPT_ENEMY;
		subtype = WTP_ENEMY_UNKNOWN;
	}
	else if(!strcmp(type, "rally"))
	{
		wptype = WPT_RALLY;
		prefix = 'R';
	}
	else if(!strcmp(type, "attack"))
	{
		wptype = WPT_ATTACK;
		prefix = 'A';
	}
	else if(!strcmp(type, "mortar"))
	{
		wptype = WPT_ENEMY;
		subtype = WTP_ENEMY_MORTAR;
		prefix = 'M';
	}

	if(wptype == WPT_ENEMY && Vehicle && Vehicle->IsMissionLeader)
		prefix = 'C';

	float x = (float)jx;
	float y = (float)jy;
	dom->_mapview->LocalToWorldCoordinates(&x,&y);

	ChatHandle playerHandle = (Vehicle) ? Vehicle->Chat_Handle : 0;

	if(name && *name)
	{
		handle = chatGetHandle(wname);
	}
	else if(prefix)
	{
		handle = GetUniqueHandle(prefix);
	}

	Waypoint waypoint(true, handle, playerHandle, wptype, subtype, (int)x, (int)y, 0, notes);

	PlayerClient()->SetWaypoint(waypoint);

	*rval = BOOLEAN_TO_JSVAL(JS_TRUE);

	return JS_TRUE;
}

/************************************************************************************************
	getnexthandle
*************************************************************************************************/

JS_FUNCTION_DEF(jsMapView, getnexthandle)
{
	if(argc < 1) return JS_FALSE;
	jsMapView* dom = phui::js::Cast<jsMapView*>(ctx,jsobj);
	if(!dom) return JS_FALSE;

	const char* type = JS_GetStringBytes(JS_ValueToString(ctx, argv[0]));

	ChatHandle id = 0;
	char callsign[MAX_HANDLE_SIZE];

	if(!strcmp(type, "rally"))
		id = GetUniqueHandle('R');
	else if(!strcmp(type, "attack"))
		id = GetUniqueHandle('A');
	else if(!strcmp(type, "mortar"))
		id = GetUniqueHandle('M');

	chatGetCallsign(callsign, id);

	*rval = phui::js::NewString(callsign);

	return JS_TRUE;
}

/************************************************************************************************
	approvewaypoint
*************************************************************************************************/

JS_FUNCTION_DEF(jsMapView, approvewaypoint)
{
	if(argc < 1) return JS_FALSE;
	jsMapView* dom = phui::js::Cast<jsMapView*>(ctx,jsobj);
	if(!dom) return JS_FALSE;

	jsWaypointObject* wayobj = phui::js::Cast<jsWaypointObject*>(ctx, argv[0]);
	if(wayobj)
	{
		Waypoint& orig = wayobj->_mapobj->_waypoint;
		if(orig.local() && orig.shared())
		{
			PlayerClient()->RemoveWaypoint(orig.handle());
			Waypoint waypoint(true,GetUniqueHandle('C'),orig.playerHandle(),orig.type(),orig.subType(),orig.x(),orig.y(),orig.posted(),orig.notes());
			PlayerClient()->SetWaypoint(waypoint);
		}
	}

	return JS_TRUE;
}

/************************************************************************************************
	approvereports
*************************************************************************************************/

JS_FUNCTION_DEF(jsMapView, approvereports)
{
	jsMapView* dom = phui::js::Cast<jsMapView*>(ctx,jsobj);
	if(!dom) return JS_FALSE;

	int i = 0;
	unsigned int id = 1;
	char reportCallsign[MAX_HANDLE_SIZE]; // SIZE includes null byte
	const std::vector<Waypoint>& waypoints = PlayerClient()->Waypoints();

	std::vector<ChatHandle> removals;
	std::vector<Waypoint> approvals;

	// get the next unique handle, then pull the id from it
	sscanf(chatGetCallsign(GetUniqueHandle('C')), "C%u", &id);

	for(i = 0; i < (int)waypoints.size(); i++)
	{
		const Waypoint& wp = waypoints.at(i);

		if(wp.type() == WPT_ENEMY && wp.local() && wp.shared())
		{
			// create the current report callsign
			snprintf(reportCallsign, MAX_HANDLE_LEN, "C%u", id);

			// create a new one with the right handle
			Waypoint waypoint(true,chatGetHandle(reportCallsign),wp.playerHandle(),wp.type(),wp.subType(),wp.x(),wp.y(),wp.posted(),wp.notes());

			removals.push_back(wp.handle());
			approvals.push_back(waypoint);

			// increment the id for the next report's handle
			++id;
		}
	}

	for(i = 0; i < (int)removals.size(); i++)
		PlayerClient()->RemoveWaypoint(removals.at(i));

	for(i = 0; i < (int)approvals.size(); i++)
		PlayerClient()->SetWaypoint(approvals.at(i));

	return JS_TRUE;
}

JS_FUNCTION_DEF(jsMapView, getposition)
{
	jsMapView* dom = phui::js::Cast<jsMapView*>(ctx,jsobj);
	if(!dom) return JS_FALSE;

	jsdouble jx, jy;
	jx = jy = 0;

	if(argc >= 2)
	{
		JS_ValueToNumber(ctx, argv[0], &jx);
		JS_ValueToNumber(ctx, argv[1], &jy);
	}

	*rval = (new jsPosition((float)jx, (float)jy))->jsVal();

	return JS_TRUE;
}

/************************************************************************************************
	jsCPObject
*************************************************************************************************/

jsCPObject::jsCPObject(StratMapObject* mapobj)
		: phui::js::Object(&jsCPObject::jsclass, jsfunctions,jsproperties)
		, _mapobj(mapobj)
{
	JS_CONSTRUCTOR_LOCK;
	if(_mapobj.valid())
		_jsstratobj =  new jsStratObject((&_mapobj->_cp)->FullObjID());
}

JSBool jsCPObject::GetProperty(JSContext* ctx, int propid, jsval* val)
{
	switch(propid)
	{
		case STRATOBJ_PROP:
		{
			if(_jsstratobj.valid())
				*val = _jsstratobj->jsVal();
			else
				*val = JSVAL_NULL;
			break;
		}
		case OBJECTTYPE_PROP:
		{
			*val = phui::js::NewString("strat");
			break;
		}
	}

	return JS_TRUE;
}

/************************************************************************************************
	jsWaypointObject
*************************************************************************************************/

jsWaypointObject::jsWaypointObject(WaypointMapObject* mapobj)
		: phui::js::Object(&jsCPObject::jsclass, jsfunctions,jsproperties)
		, _mapobj(mapobj)
{
	JS_CONSTRUCTOR_LOCK;
}

JS_FUNCTION_DEF(jsWaypointObject, onrightclick)
{
	jsWaypointObject* dom = phui::js::Cast<jsWaypointObject*>(ctx,jsobj);
	if(dom)
	{
		if(dom->_mapobj->_waypointlayer->_mv)
		{
			phui::View* menu = dom->_mapobj->_waypointlayer->_mv->FindView("waypointmenu");
			if(menu)
				menu->Show();
		}
	}
	return JS_TRUE;
}

JSBool jsWaypointObject::GetProperty(JSContext* ctx, int propid, jsval* val)
{
	switch(propid)
	{
		case OBJECTTYPE_PROP:
			*val = phui::js::NewString("waypoint");
		break;
		case LOCAL_PROP:
			*val = BOOLEAN_TO_JSVAL(_mapobj->_waypoint.local());
		break;
		case SHARED_PROP:
			*val = BOOLEAN_TO_JSVAL(_mapobj->_waypoint.shared());
		break;
		case NAME_PROP:
			*val = phui::js::NewString(chatGetWideCallsign(_mapobj->_waypoint.handle()));
		break;
		case NOTES_PROP:
			*val = phui::js::NewString(UTF16(_mapobj->_waypoint.notes()));
		break;
		case POSITION_PROP:
			*val = (new jsPosition((float)_mapobj->_waypoint.x(), (float)_mapobj->_waypoint.y()))->jsVal();
		break;
		case ACTIVE_PROP:
			*val = BOOLEAN_TO_JSVAL(_mapobj->_waypoint.handle() == PlayerClient()->ActiveWaypointHandle());
		break;
		case TYPE_PROP:
			*val = INT_TO_JSVAL(_mapobj->_waypoint.type());
		break;
		case DISPLAYNAME_PROP:
			*val = phui::js::NewString(_mapobj->_name.c_str());
		break;
	}

	return JS_TRUE;
}

/************************************************************************************************
	jsVehicleMapObject
*************************************************************************************************/

jsVehicleMapObject::jsVehicleMapObject(VehicleMapObject* mapobj)
		: phui::js::Object(&jsCPObject::jsclass,jsfunctions,jsproperties)
		, _vehobj(mapobj)
{
	JS_CONSTRUCTOR_LOCK;
}

JS_FUNCTION_DEF(jsVehicleMapObject, onrightclick)
{
	jsVehicleMapObject* dom = phui::js::Cast<jsVehicleMapObject*>(ctx,jsobj);
	if(dom)
	{
		if(dom->_vehobj->_players->_mv)
		{
			phui::View* menu = dom->_vehobj->_players->_mv->FindView("contactmenu");
			if(menu)
				menu->Show();
		}
	}
	return JS_TRUE;
}

JS_FUNCTION_DEF(jsVehicleMapObject, setchattoplayer)
{
	jsVehicleMapObject* dom = phui::js::Cast<jsVehicleMapObject*>(ctx,jsobj);
	if(dom)
		uiChatMessagePlayer(dom->_vehobj->_callsign);

	return JS_TRUE;
}

JSBool jsVehicleMapObject::GetProperty(JSContext* ctx, int propid, jsval* val)
{
	switch(propid)
	{
		case OBJECTTYPE_PROP:
		{
			*val = phui::js::NewString("vehicle");
			break;
		}
		case CALLSIGN_PROP:
		{
			*val = phui::js::NewString(_vehobj->_callsign);
			break;
		}
	}

	return JS_TRUE;
}
