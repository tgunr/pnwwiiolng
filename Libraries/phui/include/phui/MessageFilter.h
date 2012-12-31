#ifndef PHUI_MESSAGEFILTER_H
#define PHUI_MESSAGEFILTER_H

#include "phui/Referenced.h"

namespace phui
{

class Message;
class MessageHandler;

/************************************************************************************************
	MessageFilter
*************************************************************************************************/
/**
	Attached to event handlers to intercept messages. Subclasses of EventHandlers can also
	call their message filters to inform them certain events have happened.
	Filters are ref counted so will be deleted when the last reference to them is gone.
*/
class MessageFilter : public virtual Referenced
{
public:
typedef bool (*filter_hook)(Message*, MessageHandler**, MessageHandler**, MessageFilter*,void* data);
	MessageFilter(int command, filter_hook hook = 0, void* data = 0);
    
	int Command() const { return _cmd; }
    virtual bool Filter(Message* msg, MessageHandler** target, MessageHandler** reply);///< return true to prevent handler from getting message, can also change target of message as well
		
protected:
	virtual ~MessageFilter();
	
    filter_hook _hook;
	void* _hookdata;
	int _cmd;
};

}
#endif // PHUI_MESSAGEFILTER_H
