#include "syscfg.h" // Path is project specific.
#include "pi/pi.h"
#include "trn/trntypes.h"
#include "sdm/sdm.h"

#include "js/jsPosition.h"
#include "js/jsMapView.h"
#include "js/jsStratDB.h"

/************************************************************************************************
	jsPosition
*************************************************************************************************/

JSClass jsPosition::jsclass = JS_CLASS_MACRO("position");
JSFunctionSpec jsPosition::jsfunctions[] =
{
	{0}
};

JSPropertySpec jsPosition::jsproperties[] =
{
	{"lon",	jsPosition::LON_PROP,  	JSPROP_PERMANENT},
	{"lat",	jsPosition::LAT_PROP,  	JSPROP_PERMANENT},
	{"sx",	jsPosition::SX_PROP,   	JSPROP_PERMANENT},
	{"sy",	jsPosition::SY_PROP,   	JSPROP_PERMANENT},
	{"x",	jsPosition::X_PROP,   	JSPROP_PERMANENT},
	{"y",	jsPosition::Y_PROP,   	JSPROP_PERMANENT},

	{0}
};

jsPosition::jsPosition(float x, float y, SuperCellMapView* mv) :
		 phui::js::Object(&jsclass,jsfunctions ,jsproperties),
		 _x(x),
		 _y(y),
		 _mv(mv)
{
	JS_CONSTRUCTOR_LOCK;
}

jsPosition::~jsPosition()
{
}

JSBool jsPosition::GetProperty(JSContext* ctx, int propid, jsval* val)
{
	switch(propid)
	{
		case LON_PROP:
			JS_NewDoubleValue(_jsctx, Lon(), val);
		break;
		case LAT_PROP:
			JS_NewDoubleValue(_jsctx, Lat(), val);
		break;
		case SX_PROP:
			JS_NewDoubleValue(_jsctx, SuperCellX(), val);
		break;
		case SY_PROP:
			JS_NewDoubleValue(_jsctx, SuperCellY(), val);
		break;
		case X_PROP:
			JS_NewDoubleValue(_jsctx, X(), val);
		break;
		case Y_PROP:
			JS_NewDoubleValue(_jsctx, Y(), val);
		break;
		default:
		break;
	}
	return JS_TRUE;
}

JSBool jsPosition::SetProperty(JSContext* ctx, int propid, jsval* val)
{
	jsdouble dval;

	switch(propid)
	{
		case LON_PROP:
			JS_ValueToNumber(_jsctx, *val, &dval);
			SetLon((float)dval);
		break;
		case LAT_PROP:
			JS_ValueToNumber(_jsctx, *val, &dval);
			SetLat((float)dval);
		break;
		case SX_PROP:
			JS_ValueToNumber(_jsctx, *val, &dval);
			SetSuperCellX((float)dval);
		break;
		case SY_PROP:
			JS_ValueToNumber(_jsctx, *val, &dval);
			SetSuperCellY((float)dval);
		break;
		case X_PROP:
			JS_ValueToNumber(_jsctx, *val, &dval);
			SetX((float)dval);
		break;
		case Y_PROP:
			JS_ValueToNumber(_jsctx, *val, &dval);
			SetY((float)dval);
		break;
		default:
		break;
	}
	return JS_TRUE;
}

void jsPosition::SetX(float x)
{
	_x = x;
	writesync();
}

void jsPosition::SetY(float y)
{
	_y = y;
	writesync();
}

void jsPosition::SetLon(float lon)
{
	_x = (float)(lon * WORLD_WIDTH / WORLD_ARC_WIDTH);
	writesync();
}

void jsPosition::SetLat(float lat)
{
	_y = (float)(lat * WORLD_HEIGHT / WORLD_ARC_HEIGHT);
	writesync();
}

void jsPosition::SetSuperCellX(float sx)
{
	_x = sx * SDM_SUPERCELL_SIZE;
	writesync();
}

void jsPosition::SetSuperCellY(float sy)
{
	_y = sy * SDM_SUPERCELL_SIZE;
	writesync();
}

float jsPosition::Lon()
{
	readsync();
	return  (float)(_x / WORLD_WIDTH * WORLD_ARC_WIDTH);
}

float jsPosition::Lat()
{
	readsync();
	return (float)(_y / WORLD_HEIGHT * WORLD_ARC_HEIGHT);
}

float jsPosition::X()
{
	readsync();
	return  _x;
}

float jsPosition::Y()
{
	readsync();
	return _y;
}

float jsPosition::SuperCellX()
{
	readsync();
	return  _x/SDM_SUPERCELL_SIZE;
}

float jsPosition::SuperCellY()
{
	readsync();
	return _y/SDM_SUPERCELL_SIZE;
}

void jsPosition::readsync()
{
	if(_mv)
		_mv->PositionXY(&_x, &_y);
}

void jsPosition::writesync()
{
	if(_mv)
		_mv->SetPositionXY(_x, _y);
}

/************************************************************************************************
	jsBoundingBox
*************************************************************************************************/

JSClass jsBoundingBox::jsclass = JS_CLASS_MACRO("boundingbox");
JSFunctionSpec jsBoundingBox::jsfunctions[] =
{
	JS_FUNCTION_SPEC(jsBoundingBox, contains, 2),
	{0}
};

JSPropertySpec jsBoundingBox::jsproperties[] =
{
	{0}
};

/************************************************************************************************
	jsBoundingBox
*************************************************************************************************/

jsBoundingBox::jsBoundingBox(StratDB::StratObject const* strat)
			: phui::js::Object(&jsStratObject::jsclass, jsfunctions,jsproperties)
{
	JS_CONSTRUCTOR_LOCK;
	if(!strat)
	{
		topleft = new jsPosition;
		bottomright = new jsPosition;
	}else
	{
		// The object might not have any children, so bound to itself
		// as the default and initial default values
		int leftx, rightx, topy, bottomy;
		leftx = rightx = strat->PosX();
		topy = bottomy = strat->PosY();

		StratDB::StratObject::ChildList const& children = strat->ChildStratObjects();
		StratDB::StratObject::ChildList::const_iterator it;
		for(it = children.begin(); it != children.end(); ++it)
		{
			if((*it)->StratType() != SDM_FACILITY_FIREBASE)
			{
				leftx = std::min(leftx, (*it)->PosX());
				rightx = std::max(rightx, (*it)->PosX());
				topy = std::max(topy, (*it)->PosY());
				bottomy = std::min(bottomy, (*it)->PosY());
			}
		}

		topleft = new jsPosition((float)leftx, (float)topy);
		bottomright = new jsPosition((float)rightx, (float)bottomy);
	}

	DefineProperty("topleft", topleft);
	DefineProperty("bottomright", bottomright);
}

jsBoundingBox::jsBoundingBox(jsBoundingBox* lhs, jsBoundingBox* rhs)
			: phui::js::Object(&jsStratObject::jsclass, jsfunctions,jsproperties)
{
	JS_CONSTRUCTOR_LOCK;
	if(lhs == nullptr && rhs == nullptr)
	{
		topleft = new jsPosition;
		bottomright = new jsPosition;
	}else
	{
		float leftx = 0, topy = 0, rightx = 0, bottomy = 0;

		// If either is null, just use the same pointer twice. I want sleep -- OGS
		if(lhs == nullptr) lhs = rhs;
		else if(rhs == nullptr) rhs = lhs;

		leftx = std::min(lhs->topleft->X(), rhs->topleft->X());
		topy = std::max(lhs->topleft->Y(), rhs->topleft->Y());
		rightx = std::max(lhs->bottomright->X(), rhs->bottomright->X());
		bottomy = std::max(lhs->bottomright->Y(), rhs->bottomright->Y());

		topleft = new jsPosition(leftx, topy);
		bottomright = new jsPosition(rightx, bottomy);
	}

	DefineProperty("topleft", topleft);
	DefineProperty("bottomright", bottomright);
}

/************************************************************************************************
	contains
*************************************************************************************************/

JS_FUNCTION_DEF(jsBoundingBox, contains)
{
	*rval = BOOLEAN_TO_JSVAL(JS_FALSE);

	if(argc < 1)
		return JS_FALSE;

	int32 buffer = 0;
	jsBoundingBox* box = phui::js::Cast<jsBoundingBox*>(ctx,jsobj);
	jsPosition* pos = phui::js::Cast<jsPosition*>(ctx, argv[0]);

	if(!box || !pos)
		return JS_FALSE;

	if(argc == 2)
		JS_ValueToInt32(ctx, argv[1], &buffer);

	if(pos->X() >= (box->topleft->X() - buffer) &&
	   pos->X() <= (box->bottomright->X() + buffer) &&
	   pos->Y() <= (box->topleft->Y() + buffer) &&
	   pos->Y() >= (box->bottomright->Y() - buffer))
	{
		*rval = BOOLEAN_TO_JSVAL(JS_TRUE);
	}

	return JS_TRUE;
}
