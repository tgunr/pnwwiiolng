#ifndef PHUI_MESSAGELOOP_H
#define PHUI_MESSAGELOOP_H

#include "phui/MessageHandler.h"

namespace phui
{

class Message;
class MessageFilter;

/************************************************************************************************
	MessageLoop
*************************************************************************************************/
/**
	A loop for posting and dispatching messages
*/
class MessageLoop : public MessageHandler
{
public:
	MessageLoop();
	virtual ~MessageLoop();

	/// call this every frame to allow loop to dispatch all messages posted. Pass in absolute time in ms.
	virtual void Update(unsigned int ms);
	
	unsigned int FrameTime() const { return _time; } ///< current time in milliseconds
	
	bool AddHandler(MessageHandler*);
	bool RemoveHandler(MessageHandler*);
	int PostMessage(Message*, MessageHandler*, MessageHandler* replyhandler=0);
	
	MessageHandler* PreferredHandler() const { return _preferredhandler; }
	void SetPreferredHandler(MessageHandler*);
	
	int AddCommonFilter(MessageFilter*);
	int RemoveCommonFilter(MessageFilter*);
	
	Message* CurrentMessage() const { return _currentMessage; }
	bool ValidHandler(MessageHandler*);
	
static bool IsValidLoop(MessageLoop*); ///< can call to ensure loop is valid before posting.

protected:
	void DispatchMessage(Message*, MessageHandler*,MessageHandler*);

private:
class FilterList;
class HandlerList;
class MessageQueue;
	HandlerList* _handlers;
	MessageQueue* _messages;
	MessageHandler* _preferredhandler;
	Message* _currentMessage;
	FilterList* _commonfilters;
	int _id;
	int _time;
};

}
#endif // PHUI_MESSAGELOOP_H
