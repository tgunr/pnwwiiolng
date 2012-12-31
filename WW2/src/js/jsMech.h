#pragma once

#include "js/jsGR2Object.h"

/************************************************************************************************
	jsMech
*************************************************************************************************/

class gr2Vehicle;
class MechVehicle;
class jsMech;
class jsDamageModel;
class jsVehtype;

class jsMech : public jsGR2Object
{
public:
	jsMech(MechVehicle* m);

	enum
	{
		TARGET_PROP,
		DAMAGEMODEL_PROP,
		VEHICLE_PROP,
		CALLSIGN_PROP,
		TYPE_PROP,
		DESCRIPTION_PROP,
		TOTAL_MECH_PROPS
	};

	virtual void InitJSProps(int index = 0);
	virtual JSBool GetProperty(JSContext* ctx, int propid, jsval* val);

	MechVehicle* mech;
	const gr2Vehicle* currentTarget;

	static JSClass jsmechclass;

protected:

	~jsMech();

	phui::js::root_ptr<jsMech> target;
	phui::js::root_ptr<jsVehtype> vehicle;
	phui::js::root_ptr<jsDamageModel> damageModel;
};

