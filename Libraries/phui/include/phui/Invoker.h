#ifndef PHUI_INVOKER_H
#define PHUI_INVOKER_H

namespace phui
{

class MessageLoop;
class MessageHandler;
class Message;

/************************************************************************************************
	Invoker
*************************************************************************************************/
/**
	An Invoker is a simple object that can be "invoked" to send a message to a target.
*/
class Invoker
{
public:
	Invoker(Message const*, MessageHandler*);
	Invoker();
	virtual ~Invoker(); 
	
	virtual int Invoke(Message* msg = 0);

	int SetTarget(MessageHandler*);
	int SetMessage(Message const*);

	Message* InvokeMessage() const { return _msg; }
	
private:
	Message* _msg;
	MessageLoop* _loop;
	MessageHandler* _handler;
};

}

#endif // PHUI_INVOKER_H
