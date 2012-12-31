#pragma once
// Date: Tue Sep  6 18:49:28 CDT 2005
//
// Defines the gr2JSObject class
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Troy Lowe

#include "phui/Javascript.h"

class jsGR2Object;

class gr2JSObject
{
public:

	gr2JSObject(){}
	virtual ~gr2JSObject(){}

	virtual void InitJSProps(jsGR2Object* ui, int index){}
	virtual JSBool GetJSProperty(JSContext* ctx, int propid, jsval* val){ return JS_FALSE; }
	virtual JSBool SetJSProperty(JSContext* ctx, int propid, jsval* val){ return JS_FALSE; }
};

