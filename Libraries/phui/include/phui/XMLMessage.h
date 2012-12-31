#ifndef PHUI_XMLMESSAGE_H
#define PHUI_XMLMESSAGE_H

namespace phui
{
class Message;
struct IOCallbacks;

/************************************************************************************************
	XMLMessage
*************************************************************************************************/
/** Class for archiving Message as XML text. */
class XMLMessage
{
public:
///
	XMLMessage(IOCallbacks const*);
///
	~XMLMessage();
///
	void Archive(const Message*, const char* name);
///
	void Restore(Message*, const char* name);

private:
	IOCallbacks const* _io;
};
}

#endif // PHUI_XMLMESSAGE_H
