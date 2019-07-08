#pragma once

#include "phui/View.h"

struct _xmlNode;

/************************************************************************************************
	uiKeymap
*************************************************************************************************/

phui::View* uiKeymapCreate(_xmlNode*, phui::View* parent);
phui::View* uiJoystickCalibrationCreate(_xmlNode*, phui::View* parent);

// tolua_begin
UINT32 uiMapToggleIngameKeymapper(SINT32 keyState,  UINT32 functionNum, UINT32 data);
void uiMapHideIngameKeymapper(bool close=false);