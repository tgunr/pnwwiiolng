#pragma once

#include "phui/Javascript.h"

namespace StratDB { class StratObject; }

/************************************************************************************************
	jsPosition
*************************************************************************************************/

class SuperCellMapView;

class jsPosition : public phui::js::Object
{
public:
	jsPosition(float x=0, float y=0, SuperCellMapView* mv = nullptr);

	JS_CLASS_DATA;
	enum {LON_PROP, LAT_PROP, SX_PROP, SY_PROP, X_PROP, Y_PROP};

	virtual JSBool GetProperty(JSContext* ctx, int propid, jsval* val);
	virtual JSBool SetProperty(JSContext* ctx, int propid, jsval* val);

	void SetX(float);
	void SetY(float);

	void SetLon(float);
	void SetLat(float);

	void SetSuperCellX(float);
	void SetSuperCellY(float);

	float Lon();
	float Lat();

	float X();
	float Y();

	float SuperCellX();
	float SuperCellY();

protected:
	void readsync();
	void writesync();

	float _x, _y;
	SuperCellMapView* _mv;

protected:
	virtual ~jsPosition();

};

/************************************************************************************************
	jsBoundingBox
*************************************************************************************************/

class jsBoundingBox : public phui::js::Object
{
public:
	jsBoundingBox(StratDB::StratObject const*);
	jsBoundingBox(jsBoundingBox *, jsBoundingBox*);

	JS_FUNCTION(contains);

	JS_CLASS_DATA;

protected:
	jsPosition* topleft, *bottomright;

	virtual ~jsBoundingBox() {}
};

