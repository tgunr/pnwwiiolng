#pragma once

#include "phui/Javascript.h"

#include "veh/vehtype.h"

class jsCountry;
class jsWeaponCounter;
class WeaponCounter;

/************************************************************************************************
	jsVehicle
*************************************************************************************************/

class jsVehtype : public phui::js::Object
{
public:
	jsVehtype(vehType const*);
	jsVehtype(vehType const*, WeaponCounter const&);

	JS_CLASS_DATA;
	enum {CATEGORY_PROP, CLASS_PROP, DESCRIPTION_PROP};

	virtual JSBool GetProperty(JSContext* ctx, int propid, jsval* val);

	vehType const* vehtype;

protected:
	void Init();
	~jsVehtype();
};

