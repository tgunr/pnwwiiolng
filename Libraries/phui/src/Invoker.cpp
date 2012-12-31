#include "phui-precompiled-header.h"

#include "phui/Invoker.h"
#include "phui/Message.h"
#include "phui/MessageLoop.h"

using namespace phui;

/************************************************************************************************
	Invoker
*************************************************************************************************/

Invoker::Invoker(Message const* m, MessageHandler* h) : _handler(h)
{	
	_msg = new Message(*m);
	if(h)
		_loop = h->ParentLoop();
}

Invoker::Invoker() : _msg(0)
{
}

Invoker::~Invoker()
{
	if(_msg) delete _msg;
}

/************************************************************************************************
	Invoke
*************************************************************************************************/

int Invoker::Invoke(Message* m)
{
	Message* msg = m ? m : _msg;
	
	if(msg && MessageLoop::IsValidLoop(_loop))
		return _loop->PostMessage(msg, _handler);
	
	return -1;
}

/************************************************************************************************
	SetTarget
*************************************************************************************************/

int Invoker::SetTarget(MessageHandler* h)
{
	_handler = h;
	_loop = h->ParentLoop();
	return 0;
}

/************************************************************************************************
	SetMessage
*************************************************************************************************/

int Invoker::SetMessage(Message const* msg)
{
	if(_msg == 0) 
		_msg = new Message(*msg);
	else
		*_msg = *msg;

	return 0;
}


