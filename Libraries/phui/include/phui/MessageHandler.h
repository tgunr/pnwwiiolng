#ifndef PHUI_MESSAGEHANDLER_H
#define PHUI_MESSAGEHANDLER_H

namespace phui
{

class MessageFilter;
class MessageLoop;
class Message;

/************************************************************************************************
	MessageHandler
*************************************************************************************************/
/**
	Class for handling messages. 
*/
class MessageHandler
{
public:
	MessageHandler() : _parentloop(0ULL), _name(0ULL), _filters(0ULL), _nexthandler(0ULL) {}
	virtual ~MessageHandler();

	MessageLoop* ParentLoop() const { return _parentloop; }
	virtual void MessageReceived(Message* m, MessageHandler* reply) { if(_nexthandler) _nexthandler->MessageReceived(m, reply); }
    
    MessageHandler* NextHandler() const    { return _nexthandler; }
    void SetNextHandler(MessageHandler* h);
	
	/** Attached filters are automatically called in MessageReceived. Subclasses of EventHandler can
		also call the filters directly in response to specific subclass defined events, so 
		MessageFilters could act as "delegates" as well. */
    int AddMessageFilter(MessageFilter*);
    int RemoveMessageFilter(MessageFilter*);

	const char* Name() const;
	void SetName(const char*);

    MessageFilter* FilterAt(unsigned int);
		
protected:
friend class MessageLoop;
	MessageLoop* _parentloop;
 
private:
class FilterList;
class string;	

    string* _name;
	FilterList* _filters;
    MessageHandler* _nexthandler;
};

}
#endif // PHUI_MESSAGEHANDLER_H
