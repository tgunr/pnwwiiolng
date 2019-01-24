#ifndef PN_XML_XMLLOADER2_H
#define PN_XML_XMLLOADER2_H 1

#include <string>
#include <map>
#include "libxml/tree.h"
#include "libxml/parser.h"
#include "fio/fio.h"
#include "str/stringLib.h"

void fioXmlRegisterInputCallbacks();

/************************************************************************************************
	XMLLoader2
*************************************************************************************************/

/********* how to use **********/

// describe how to use this class
/*

1. Declare a YourSubClassedXMLLoader and inherit from or wrap XMLLoader2<YourSubClassedXMLLoader>

2. Make sure your destructor is virtual if you subclass

3. Declare some handler member functions

	XML_TAG_HANDLER(HandleSomeTag);

4. In YourSubClassedXMLLoader' constructor, initialize the tag handler map

	AddHandler("tagname", &YourSubClassedXMLLoader::HandleSomeTag, this);
	... // multiple calls; one for each tag

5. Define the HandleSomeTag member function of YourSubClassedXMLLoader

	// simple tag handler example
	bool YourSubClassedXMLLoader::HandleSomeTag(xmlNode* node, bool first, void* userData)
	{
		// handler is called once before children are processed and once after
		// first will be true when its the call before children are processed
		// we don't need to prcess the tag a second time because the 1st
		// call got us the data we wanted
		if(first)
		{
			// xmlGetStringFromNode and xmlGetNodeValue are helper
			// functions that get data from the node content or
			// a node attribute

			// modelFile is a global std::string
			modelFile = xmlGetStringFromNode(node);
		}

		// returning true causes this tag's children to be
		// processed normally
		return true;
	}

	// more complex example that fills out instance member data
	bool YourSubClassedXMLLoader::HandleSomeTag(xmlNode* node, bool first, void* userData)
	{
		// "spec" is an instance member variable used to collect data

		// first time through, lets clear out "spec" so our children
		// will have a clean slate to work with
		if(first)
		{
			spec.Reset(); // clears out data; clean slate
		}
		else
		{
			// at this point our children have been processed and "spec"
			// should have good data in it

			if(!spec.Valid())
				return false;	// our "spec" wasn't filled out by our children
								// possibly we have a poorly formatted xml doc

			// _specMap is a global map of specs
			// we can copy the newly collected spec to a global for use later
			// or just keep it in this object if the object will be kept around
			(*_specMap)[spec.name] = spec;
		}

		// since we want our children to be processed normally, just
		// return true here
		return true;
	}

	// complex example with custom userData
	bool YourSubClassedXMLLoader::HandleSomeTag(xmlNode* node, bool first, void* userData)
	{
		// handler is called once before children are processed and once after
		// first will be true when its the call before children are processed
		if(first)
		{
			// this doesn't have to be a local variable, it could be a
			// instance member variable of the class or it could be global
			// data thats populated by this handler
			DataCollector spec;

			// xmlGetStringFromNode and xmlGetNodeValue are helper
			// functions that get data from the node content or
			// a node attribute
			spec.name = xmlGetStringFromNode(node, "name");
			spec.baseDir = xmlGetStringFromNode(node, "base");

			// here I pass in userData associated with this tag
			// its children will expect a DataCollector object
			// and will populated it;
			if(ProcessChildren(node, &spec))
			{
				// no children returned false, so good data was collected
				// _specMap is a global map of specs
				(*_specMap)[spec.name] = spec;
			}
		}

		// I return false here because I've elected to handle child processing
		// myself; this is so I can pass in userData to my tag's children; if I
		// returned true here, the children would be processed normally, but with
		// the userData this tag handler got from its parent (which is possibly null)
		return false;
	}

5. From an subclassed instance, call

	YourSubClassedXMLLoader myInstance("pathtoxml");

	if(!myInstance.Load())
		return; // failure

	// from here your accessing your own class' interface
	// to get at collected data; you may not even need to access
	// any data if YourSubClassedXMLLoader simply populated
	// global data or something
	myInstance.accessCollectedData(); <- not defined by XMLLoader2

6. If you wrapped the XMLLoader2 instead of subclassing it, simply call xmlLoaderInstance.Load()
from within one of YourSubClassedXMLLoader's functions

*/

/********* defines **********/

#define XML_DEFAULT_FLAGS XML_PARSE_DTDLOAD | XML_PARSE_RECOVER
#define XML_IS_NODETYPE(n, t) (!strcasecmp((const char*)(n)->name, t))
#define XML_DEFAULT_HANDLER "_DEFAULT_"
#define MAX_XML_TRIM_SIZE 1024
#define XML_TAG_HANDLER(handler) bool handler(xmlNode* node, bool first = true, void* userData = nullptr)

/********* types ************/

enum XML_DATA_TYPE
{
	XML_DATA_BOOL,
	XML_DATA_SINT,
	XML_DATA_UINT,
	XML_DATA_LONG,
	XML_DATA_FLOAT,
	XML_DATA_DOUBLE,
	XML_DATA_STRING
};

/********* declares ************/

const char* xmlTrimString(const char* value);
bool xmlGetNodeValue(xmlNode* node, XML_DATA_TYPE type, void* value, const char* attribute = nullptr);
const char* xmlGetStringFromNode(xmlNode* node, const char* attribute = nullptr);
void xmlRegisterUIFIO();

/********* classes ************/

template<class T>
class XMLLoader2
{
	typedef bool (T::*FN)(xmlNode*, bool, void*);

private:
	template<class U>
	struct TAG_HANDLER
	{
		TAG_HANDLER(U* o = nullptr, FN f = nullptr) : object(o), func(f){}
		bool IsValid(){ return (object && func); }

		U* object;
		FN func;
	};

public:

	using xmlTagHandlerMap = std::map<std::string, TAG_HANDLER<T>>;

	XMLLoader2();
	XMLLoader2(const char* path, int flags = XML_DEFAULT_FLAGS, bool uiFio = false);
	virtual ~XMLLoader2();

    XMLLoader2<T>(const XMLLoader2<T> &obj) {
        _flags = obj._flags;
        _complete = obj._complete;
        _uiFio = obj._uiFio;
        _xmlDoc = obj._xmlDoc;
        _path = obj._path;
        _handlers = obj._handlers;
        _handlerReturnScope = obj._handlerReturnScope;
    }

public:

	enum HANDLER_RETURN_SCOPE
	{
		DOCUMENT,
		NODE
	};

	void SetPath(const char* path);
	void SetFlags(int flags);
	void SetHandlerReturnScope(HANDLER_RETURN_SCOPE scope);

	// handler management
	virtual void AddDefaultHandler(FN handler, T* object = nullptr);
	virtual void AddHandler(const char* tagname, FN handler, T* object = nullptr);

	// parses the xml doc and starts processing nodes; primary
	// entry into the load process
	virtual bool Load(void* userData = nullptr);

	// primary worker; recursively called on each node starting with
	// the root node from Load(); can be called externally
	virtual bool Process(xmlNode* node, void* userData = nullptr);

	// processes children; can be called by Process or by a user defined
	// handler if the handler wants to take over processing in order to
	// pass in its own userData
	virtual bool ProcessChildren(xmlNode* node, void* userData = nullptr);

	// accessors; returns nullptr if Load() hasn't been called yet
	// or if there was a problem parsing the xml doc
	virtual const xmlDoc* GetDoc() const;
	virtual const xmlNode* GetRootNode() const;

	// allows you to cleanup the xmlcallbacks and
	// doc before the destructor is called; optional behavior
	virtual void Finish();

	// allows you to reset the object for multiple uses
	// calls Finish for you
	virtual void Reset();

	// lets you set whether we're using normal or ui fio callbacks
	virtual void SetUIFIO(bool uiFio)
	{
		_uiFio = uiFio;
	}

protected:

	int _flags;
	bool _complete, _uiFio;
	xmlDoc* _xmlDoc;
	std::string _path;
	xmlTagHandlerMap _handlers;
	HANDLER_RETURN_SCOPE _handlerReturnScope;
};


/********* definitions ************/

template<class T>
XMLLoader2<T>::XMLLoader2() :
	_flags(XML_DEFAULT_FLAGS),
    _complete(false),
    _uiFio(false),
    _xmlDoc(nullptr),
    _path(""),
	_handlerReturnScope(NODE)
{
	fioXmlRegisterInputCallbacks();
}

template<class T>
XMLLoader2<T>::XMLLoader2(const char* path, int flags, bool uiFio) :
	_flags(flags),
	_complete(false),
    _uiFio(uiFio),
	_xmlDoc(nullptr),
    _path(path),
    _handlerReturnScope(NODE)
{
#if platBuild != BUILD_SERVER
	if(_uiFio)
		xmlRegisterUIFIO();
	else
#endif
		fioXmlRegisterInputCallbacks();
}

template<class T>
XMLLoader2<T>::~XMLLoader2()
{
	Finish();
}

template<class T>
void XMLLoader2<T>::SetPath(const char* path)
{
	_path = path;
}

template<class T>
void XMLLoader2<T>::SetFlags(int flags)
{
	_flags = flags;
}

template<class T>
void XMLLoader2<T>::SetHandlerReturnScope(HANDLER_RETURN_SCOPE scope)
{
	_handlerReturnScope = scope;
}

template<class T>
void XMLLoader2<T>::AddDefaultHandler(FN handler, T* object)
{
	if(handler)
		AddHandler(XML_DEFAULT_HANDLER, handler, object);
}

template<class T>
void XMLLoader2<T>::AddHandler(const char* tagname, FN handler, T* object)
{
	if(!object)
		object = (T*)this;

	if(handler)
		_handlers[tagname] = TAG_HANDLER<T>(object, handler);
}

template<class T>
bool XMLLoader2<T>::Load(void* userData)
{
	bool loaded = false;

	if(_path.size())
	{
		_xmlDoc = xmlReadFile(_path.c_str(), nullptr, _flags);

		if(_xmlDoc)
		{
			xmlNodePtr root = xmlDocGetRootElement(_xmlDoc);

			if(root)
			{
				Process(root, userData);
				loaded = true;
			}
		}
	}

    return loaded;
}

template<class T>
const xmlDoc* XMLLoader2<T>::GetDoc() const
{
    return _xmlDoc;
}

template<class T>
const xmlNode* XMLLoader2<T>::GetRootNode() const
{
	if(_xmlDoc)
		return xmlDocGetRootElement(_xmlDoc);

    return nullptr;
}

template<class T>
bool XMLLoader2<T>::Process(xmlNode* node, void* userData)
{
	if(!node || node->type != XML_ELEMENT_NODE)
		return true; // don't process this node, but continue the processing

	// handler for this tag
	TAG_HANDLER<T> handler;

	// what to return on fail
	bool returnOnFalse = (_handlerReturnScope == NODE);

	if(_handlers.size())
	{
		if(_handlers.find((char*)node->name) != _handlers.end())
			handler = _handlers[(char*)node->name];
		else if(_handlers.find(XML_DEFAULT_HANDLER) != _handlers.end())
			handler = _handlers[XML_DEFAULT_HANDLER];
	}

	// if we found a handler, run it passing true
	// since this is the pre-child processing stage
	if(handler.IsValid() && !(handler.object->*handler.func)(node, true, userData))
		return returnOnFalse;

	// process this node's children
	if(!ProcessChildren(node, userData))
		return returnOnFalse;

	// run the post child handler again after processing
	// children, passing false this time
	if(handler.IsValid() && !(handler.object->*handler.func)(node, false, userData))
		return returnOnFalse;

	// nothing returned false, so keep going
	return true;
}

template<class T>
bool XMLLoader2<T>::ProcessChildren(xmlNode* node, void* userData)
{
	// process this node's children
	for(xmlNodePtr child = node->children; child; child = child->next)
	{
		if(!Process(child, userData))
			return false;
	}

	// nothing returned false, so keep going
	return true;
}

template<class T>
void XMLLoader2<T>::Finish()
{
	if(!_complete)
	{
		if(_xmlDoc)
			xmlFreeDoc(_xmlDoc);

		xmlCleanupInputCallbacks();

		_complete = true;
	}
}

template<class T>
void XMLLoader2<T>::Reset()
{
	// make sure resources are released
	Finish();

	// re-init the io callbacks
#if platBuild != BUILD_SERVER
	if(_uiFio)
		xmlRegisterUIFIO();
	else
#endif
		fioXmlRegisterInputCallbacks();

	// reset the private data
	_path = "";
	_flags = XML_DEFAULT_FLAGS;
	_xmlDoc = nullptr;
	_complete = false;
	_handlers.clear();
}

#endif // PN_XML_XMLLOADER2_H
