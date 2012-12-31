#ifndef PHUI_DROPDOWNTASK_H
#define PHUI_DROPDOWNTASK_H

#include "phui/MessageFilter.h"
#include "phui/Periodical.h"

namespace phui
{

class View;
class Message;
class MessageHandler;

/************************************************************************************************
	DropDownTask
*************************************************************************************************/
/**
	A little utility class that moves a view out of the window with the specificed keystroke
*/
class DropDownTask: public MessageFilter, public Periodical
{
public:
	DropDownTask(View* v, int keycode, int dir, int curdir=0);	
	virtual ~DropDownTask();
	
	virtual void Execute(unsigned int t);	
	virtual bool Filter(Message* msg, MessageHandler** target,  MessageHandler** reply);
	
private:
	View* _view;
	int _keycode;
	int _curdir;
	int _origdir;
	float _origY;
};
}

#endif // PHUI_DROPDOWNTASK_H
