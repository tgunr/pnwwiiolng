#include "phui-precompiled-header.h"

#include <list>
#include <vector>
#include <deque>
#include <algorithm>
#include <assert.h>

#include "phui/MessageFilter.h"
#include "phui/MessageLoop.h"
#include "phui/Message.h"
#include "phui/UIMessages.h"

using namespace phui;

static int _loopid = 1;

struct MessageQueueElem
{
	MessageQueueElem(const Message& m, MessageHandler* h, MessageHandler* reply) : _msg(m), _handler(h), _reply(reply) {}
	Message _msg;
	MessageHandler* _handler;
	MessageHandler* _reply;
};

class MessageLoop::FilterList 	: public std::vector<MessageFilter*> {};
class MessageLoop::HandlerList 	: public std::list<MessageHandler*> {};
class MessageLoop::MessageQueue : public std::deque<MessageQueueElem> {};

static std::list<MessageLoop*> _loops;

/************************************************************************************************
	MessageLoop
*************************************************************************************************/

MessageLoop::MessageLoop() : MessageHandler()
{
	_handlers = new HandlerList;
	_messages = new MessageQueue;
	_commonfilters = nullptr;
	_preferredhandler = nullptr;
	_currentMessage = 0;
	_parentloop = this;
	_id = _loopid++;
	
	_loops.push_back(this);
}

MessageLoop::~MessageLoop()
{
	_parentloop = nullptr;
	
	std::list<MessageLoop*>::iterator delit = find(_loops.begin(), _loops.end(), this);
	if(delit != _loops.end())
		_loops.erase(delit);
	
	// we don't own handlers just remove us as parent
	for(HandlerList::iterator it = _handlers->begin(); it != _handlers->end(); ++it)
	{
        (*it)->_parentloop = nullptr;
        if((*it)->NextHandler() == this)
            (*it)->SetNextHandler(nullptr);
	}
	
	if(_commonfilters)
	{
		for(FilterList::iterator it = _commonfilters->begin(); it != _commonfilters->end(); ++it)
			(*it)->Release();
		delete _commonfilters;
	}
	
	delete _handlers;
	delete _messages;
}

bool MessageLoop::ValidHandler(MessageHandler* handler)
{
	return (handler == this ||
			find(_handlers->begin(), _handlers->end(), handler) != _handlers->end());
}

/************************************************************************************************
	Update
*************************************************************************************************/

void MessageLoop::Update(unsigned int ms)
{
	_time = ms;
	while(_messages->size())
	{
		MessageQueueElem& elem = _messages->back();
		
		// make sure the handler still exists
		if(elem._handler == nullptr ||	ValidHandler(elem._handler))
		{ // send the message
			_currentMessage = &elem._msg; 
			DispatchMessage(&elem._msg, elem._handler, elem._reply);
		}
		
		_messages->pop_back();
	}
	_currentMessage = 0;
}
	
/************************************************************************************************
	AddHandler
*************************************************************************************************/

bool MessageLoop::AddHandler(MessageHandler* handler)
{
	if(handler->ParentLoop() == nullptr)
    {
        handler->SetNextHandler(this);
        handler->_parentloop = this;
		_handlers->push_back(handler);
        
        return true;
    }
    
    return false;
}

/************************************************************************************************
	RemoveHandler
*************************************************************************************************/

bool MessageLoop::RemoveHandler(MessageHandler* handler)
{
	if(handler->ParentLoop() != this) return false;
	
	HandlerList::iterator it = find(_handlers->begin(), _handlers->end(), handler);
	
	if(it != _handlers->end())
	{
        handler->SetNextHandler(nullptr);
        handler->_parentloop = nullptr;
		if(handler == PreferredHandler())
			SetPreferredHandler(nullptr);

		_handlers->erase(it);
        
        return true;
	}
    
    return false;
}

/************************************************************************************************
	PostMessage
*************************************************************************************************/

int MessageLoop::PostMessage(Message* msg, MessageHandler* handler, MessageHandler* replyhandler)
{
	if(handler == nullptr || handler == this || find(_handlers->begin(), _handlers->end(), handler) != _handlers->end() )
	{
		_messages->push_front(MessageQueueElem(*msg, handler,replyhandler));
		return 0;
	}
	
	return -1;
}

/************************************************************************************************
	DispatchMessage
*************************************************************************************************/

void MessageLoop::DispatchMessage(Message* msg, MessageHandler* handler,MessageHandler* replyhandler)
{
	if(handler == nullptr) // use preferred handler
		handler = PreferredHandler();

    if(handler == nullptr)
        handler = this;
    
	MessageFilter* filter;
	
	if(_commonfilters)
	{
		for(FilterList::iterator it = _commonfilters->begin(); it != _commonfilters->end(); ++it)
		{
			if(!ValidHandler(handler)) return;

			filter = *it;
			if((filter->Command() == phui::ANY_EVENT || filter->Command() == msg->what) &&
				filter->Filter(msg, &handler, &replyhandler))
			{
				return;
			}
		}
	}
	
	for(int i = 0; ValidHandler(handler) && (filter = handler->FilterAt(i)) != nullptr; i++)
	{
		if((filter->Command() == phui::ANY_EVENT || filter->Command() == msg->what) &&
			filter->Filter(msg, &handler, &replyhandler))
		{
			return;
		}
	}
	
	// need to handle case of filter redirecting target to a handler in another looper
	if(handler && ValidHandler(handler))
		handler->MessageReceived(msg, replyhandler);
}

/************************************************************************************************
	SetPreferredHandler
*************************************************************************************************/

void MessageLoop::SetPreferredHandler(MessageHandler* handler)
{
	if(handler == nullptr)
	{
		_preferredhandler = this;
	} else if(handler->_parentloop == this)
	{
		_preferredhandler = handler;
	}
}


/************************************************************************************************
	AddCommonFilter
*************************************************************************************************/

int MessageLoop::AddCommonFilter(MessageFilter* filter)
{
	assert( filter != nullptr );

    if(!_commonfilters) _commonfilters = new FilterList;
	_commonfilters->push_back(filter);
	filter->Acquire();
    return 0;
}

/************************************************************************************************
	RemoveCommonFilter
*************************************************************************************************/

int MessageLoop::RemoveCommonFilter(MessageFilter* filter)
{
	if(_commonfilters)
	{
		FilterList::iterator it = find(_commonfilters->begin(), _commonfilters->end(), filter);
		if(it != _commonfilters->end())
		{
			(*it)->Release();
			_commonfilters->erase(it);
			return 0;
		}
	}   
	return -1;
}

/************************************************************************************************
	IsValidLoop
*************************************************************************************************/

bool MessageLoop::IsValidLoop(MessageLoop* loop)
{
	std::list<MessageLoop*>::iterator it = find(_loops.begin(), _loops.end(), loop);
	return it != _loops.end();
}

