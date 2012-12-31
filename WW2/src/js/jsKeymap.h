#pragma once

#include "phui/Javascript.h"

/************************************************************************************************
	jsKeymap
*************************************************************************************************/

class jsKeymap : public phui::js::Object
{
public:
	jsKeymap(JSContext* ctx, JSObject* parent, int category, int vehidx, int i, int keyidx=-1);

	JS_CLASS_DATA;

	int category, vehicleIdx, functionidx;
	unsigned int functionCode;
	int isaxis, keyidx, ezlevel;

	virtual ~jsKeymap();
};

