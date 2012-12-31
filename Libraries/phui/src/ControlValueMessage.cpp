#include "phui-precompiled-header.h"

#include <stdio.h>

#include "phui/View.h"
#include "phui/Control.h"
#include "phui/Message.h"
#include "phui/ControlValueMessage.h"

using namespace phui;

static void	archive_control_value(View const* v, Message* msg);
static void	restore_control_value(View* v, Message const* msg);

/************************************************************************************************
	RestoreControlValues
*************************************************************************************************/

void phui::RestoreControlValues(phui::View* v, phui::Message const* msg)
{
	restore_control_value(v, msg);
	
	unsigned int count = v->ChildCount();
	for(unsigned int i = 0; i < count; i++)
	{
		RestoreControlValues(v->ChildAt(i), msg);
	}
}

/************************************************************************************************
	ArchiveControlValues
*************************************************************************************************/

void phui::ArchiveControlValues(phui::View const* v, phui::Message* msg)
{
	archive_control_value(v, msg);
	
	unsigned int count = v->ChildCount();
	for(unsigned int i = 0; i < count; i++)
	{
		ArchiveControlValues(v->ChildAt(i), msg);
	}
}

/************************************************************************************************
	archive_control_value
*************************************************************************************************/

static void	archive_control_value( View const* v, Message* msg)
{
	if(v->Name()[0] && dynamic_cast<Control*>((View*)v) && v->GetValue().Type() != Scalar::UndefT)
	{
		Scalar val = v->GetValue();
		msg->ReplaceOrAddScalar(v->Name(), val);
	}
}

/************************************************************************************************
	restore_control_value
*************************************************************************************************/

static void	restore_control_value(View* v, Message const* msg)
{
	Scalar val;
	if(msg->FindScalar(v->Name(), &val) == Message::OK)
	{
		v->SetValue(val);
	}
}

