#include "phui-precompiled-header.h"

#include "phui/View.h"
#include "phui/UIMessages.h"
#include "phui/MessageHandler.h"
#include "phui/Message.h"
#include "phui/DropDownTask.h"
#include "phui/Viewport.h"

using namespace phui;

/************************************************************************************************
	DropDownTask
*************************************************************************************************/

DropDownTask::DropDownTask(View* v, int k,int dir, int curdir) : MessageFilter(KEY_DOWN), Periodical(), _view(v), _keycode(k)
{
	_origdir = dir;
	_curdir = curdir ? curdir : -dir;
	_origY = 0;
}

DropDownTask::~DropDownTask()
{
}

/************************************************************************************************
	Execute
*************************************************************************************************/

void DropDownTask::Execute(unsigned int t)
{
	_view->MoveBy(0, 1200/1000.0f * (t - _lasttime) * _curdir);
	
	if(_origdir > 0)
	{
		if(_curdir > 0 && _view->Y() > _view->ParentViewport()->Height())
		{
			_view->MoveTo(_view->X(), _view->ParentViewport()->Height());
			_view->Hide();
			StopExecuting();		
		}else if (_curdir < 0 && _view->Y() < _origY)
		{
			_view->MoveTo(_view->X(), _origY);
			StopExecuting();
		}
	}else
	{
		if(_curdir < 0 && _view->Y() + _view->Height() < 0)
		{
			_view->MoveTo(_view->X(), -_view->Height());
			_view->Hide();
			StopExecuting();		
		}else if (_curdir > 0 && _view->Y() > _origY)
		{
			_view->MoveTo(_view->X(), _origY);
			StopExecuting();
		}	
	}
}

/************************************************************************************************
	Filter
*************************************************************************************************/

bool DropDownTask::Filter(Message* msg, MessageHandler** target, MessageHandler**)
{
	long keycode = 0;
	msg->FindLong(UI_KEYCODE, &keycode);
	
	if(keycode == _keycode)
	{			
		_curdir = -_curdir;

		if(!_executing && _curdir == _origdir)
			_origY = _view->ParentViewport()->Y();
		
		_view->MakeFocus(_curdir != _origdir);
		
		if(!_view->IsVisible())
			_view->Show();
		
		StartExecuting();
		return true;
	}
	
	return false;
}

