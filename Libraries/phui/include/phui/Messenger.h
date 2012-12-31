#ifndef PHUI_MESSEGENGER_H
#define PHUI_MESSEGENGER_H

namespace phui
{

class Message;
class MessageHandler;
class MessageLoop;

/************************************************************************************************
	Messenger
*************************************************************************************************/
/**
	Class for posting Messages to a Handler. This is the "safe" way to hold on to a reference to
	a handler as it will save off the Handler's associated MessageLoop and check the validity of the
	loop before trying to post the message. 
*/
class Messenger
{
public:
	Messenger(MessageHandler*);
	~Messenger();
	
	int SetTarget(MessageHandler*);
	int SendMessage(Message*);
	
	bool operator ==(const Messenger&);
	
private:
	MessageLoop* _loop;
	MessageHandler* _handler;
};

}
#endif // PHUI_MESSEGENGER_H
