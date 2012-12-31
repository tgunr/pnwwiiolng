#pragma once
// Date: Tue Sep  6 18:49:28 CDT 2005
//
// Defines the gr2Module class and types
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Troy Lowe

#include "gr2/gr2Data.h"

extern void gr2ModuleInit();

class gr2GameObject;

class gr2Module
{
public:

	static gr2GameObject* Create(gr2ModuleSpec* spec);

private: // can't instantiate

	gr2Module();
	virtual ~gr2Module();
};
