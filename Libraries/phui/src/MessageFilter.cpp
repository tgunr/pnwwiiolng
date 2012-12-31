#include "phui-precompiled-header.h"

#include "phui/MessageFilter.h"

using namespace phui;

/************************************************************************************************
	MessageFilter
*************************************************************************************************/

MessageFilter::MessageFilter(int cmd, filter_hook hook, void* data) : Referenced(),  
				_hook(hook), _hookdata(data), _cmd(cmd)
{
}

MessageFilter::~MessageFilter()
{
}

/************************************************************************************************
	Filter
*************************************************************************************************/

bool MessageFilter::Filter(Message* msg, MessageHandler** target, MessageHandler** reply)
{
    if(_hook)
        return _hook(msg, target, reply, this, _hookdata);
	
	return false;
}

