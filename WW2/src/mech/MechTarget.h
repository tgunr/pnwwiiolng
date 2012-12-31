#pragma once
// Date: Tue Sep  6 18:49:28 CDT 2005
//
// Defines the MechTarget class
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Troy Lowe

#include "mech/MechVehicle.h"
#include "gr2/gr2Main.h"
#include "phui/View.h"

// pre declares

struct _xmlNode;

phui::View* uiMechTargetViewCreate(_xmlNode* node, phui::View* parent);

class MechTargetView : public phui::View
{
public:

	MechTargetView() : phui::View(), lineLength(.1f)
	{
	}

	virtual void DrawSelf();
	void SetLineLength(float ll){ lineLength = ll; }

	float lineLength;
};

