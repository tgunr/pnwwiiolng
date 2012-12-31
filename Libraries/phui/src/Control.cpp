#include "phui-precompiled-header.h"

#include <string.h>

#include "phui/Viewport.h"
#include "phui/Control.h"
#include "phui/Message.h"
#include "phui/MessageLoop.h"
#include "phui/UIMessages.h"

using namespace phui;

/************************************************************************************************
	Control
*************************************************************************************************/

/************************************************************************************************
	Invoke
*************************************************************************************************/

int Control::Invoke(Message* m)
{
	if(m == 0 && InvokeMessage() == 0) return -1;
	if(IsClosing()) return 0;
	
	Message msg(m ? *m : *InvokeMessage());
	
	msg.AddPointer(UI_CONTROL_SOURCE, this);
	msg.AddLong(UI_WHEN, ParentLoop()->FrameTime());
	msg.AddScalar(UI_CONTROL_VALUE, GetValue());

	return Invoker::Invoke(&msg);
}