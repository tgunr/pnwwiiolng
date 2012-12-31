#pragma once

#include "phui/View.h"

struct _xmlNode;

/************************************************************************************************
	uiKeymap
*************************************************************************************************/

phui::View* uiKeymapCreate(_xmlNode*, phui::View* parent);
phui::View* uiJoystickCalibrationCreate(_xmlNode*, phui::View* parent);

