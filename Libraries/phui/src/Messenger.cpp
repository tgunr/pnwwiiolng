#include "phui-precompiled-header.h"

#include "phui/Messenger.h"
#include "phui/MessageLoop.h"
#include "phui/MessageHandler.h"

using namespace phui;

/************************************************************************************************
	Messenger
*************************************************************************************************/

Messenger::Messenger(MessageHandler* h)
{
	SetTarget(h);
}

Messenger::~Messenger()
{
}

/************************************************************************************************
	SetTarget
*************************************************************************************************/

int Messenger::SetTarget(MessageHandler* h)
{
	_handler = h;
	_loop = h->ParentLoop();
	
	return 0;
}


/************************************************************************************************
	SendMessage
*************************************************************************************************/

int Messenger::SendMessage(Message* msg)
{
	if(!MessageLoop::IsValidLoop(_loop)) return -1;

	return 0;
}

/************************************************************************************************
	==
*************************************************************************************************/

bool Messenger::operator ==(const Messenger& m)
{
	return (_loop == m._loop && _handler == m._handler);
}


