#include "phui-precompiled-header.h"

#include <vector>
#include <string>
#include <algorithm>

#include "phui/MessageLoop.h"
#include "phui/MessageHandler.h"
#include "phui/MessageFilter.h"

using namespace phui;


class MessageHandler::FilterList : public std::vector<ref_ptr<MessageFilter> > {};
class MessageHandler::string : public std::string {};

/************************************************************************************************
	MessageHandler
*************************************************************************************************/

MessageHandler::~MessageHandler()
{    
	delete _filters;

 	if(_parentloop)
		_parentloop->RemoveHandler(this);
		
	delete _name;
}

/************************************************************************************************
	AddMessageFilter
*************************************************************************************************/

int MessageHandler::AddMessageFilter(MessageFilter* filter)
{
	if(!_filters) _filters = new FilterList;
	
	_filters->push_back(filter);
	filter->Acquire();
	
    return 0;
}

/************************************************************************************************
	RemoveMessageFilter
*************************************************************************************************/

int MessageHandler::RemoveMessageFilter(MessageFilter* filter)
{
	if(_filters)
	{
		FilterList::iterator it = find(_filters->begin(), _filters->end(), filter);
		if(it != _filters->end())
		{
			_filters->erase(it);
			
			return 0;
		}
    }
	return -1;
}

/************************************************************************************************
	Name
*************************************************************************************************/

const char* MessageHandler::Name() const
{
    if(_name)
		return _name->c_str();
	else
		return "";
}

/************************************************************************************************
	SetName
*************************************************************************************************/

void MessageHandler::SetName(const char* name)
{
    if(!_name) _name = new string;
	else _name->erase();
	
    (*_name) += name;
}

/************************************************************************************************
	FilterAt
*************************************************************************************************/

MessageFilter* MessageHandler::FilterAt(unsigned int i)
{
	if(_filters && i < _filters->size())
		return (*_filters)[i].ptr();

	return nullptr;
}

/************************************************************************************************
	SetNextHandler
*************************************************************************************************/

void MessageHandler::SetNextHandler(MessageHandler* handler)
{
    if(handler && handler->_parentloop != _parentloop)
        return;
    
    _nexthandler = handler;
}