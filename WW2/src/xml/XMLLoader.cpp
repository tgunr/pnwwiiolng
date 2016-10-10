#include <syscfg.h> // Path is project specific.
#include "xml/XMLLoader.h"

#include <string>
#include "fio/fio.h"
#include "str/stringLib.h"

// defines
#define XML_DEFAULT_HANDLER "_DEFAULT_"
#define MAX_XML_TRIM_SIZE 1024

// XMLLoader definitions
XMLLoader::XMLLoader() :
	_flags(XML_PARSE_DTDLOAD | XML_PARSE_RECOVER),
	_complete(false),
    _xmlDoc(nullptr),
	_path(""),
    _handlerReturnScope(NODE)
{
	fioXmlRegisterInputCallbacks();
}

XMLLoader::XMLLoader(const char* path, xmlTagHandler def, int flags) :
	_flags(flags),
	_complete(false),
    _xmlDoc(nullptr),
	_path(path),
    _handlerReturnScope(NODE)
{
	if(def)
		AddDefaultHandler(def);

	fioXmlRegisterInputCallbacks();
}

XMLLoader::~XMLLoader()
{
	Finish();
}

void XMLLoader::SetPath(const char* path)
{
	_path = path;
}

void XMLLoader::SetFlags(int flags)
{
	_flags = flags;
}

void XMLLoader::SetHandlerReturnScope(HANDLER_RETURN_SCOPE scope)
{
	_handlerReturnScope = scope;
}

void XMLLoader::AddDefaultHandler(xmlTagHandler handler)
{
	if(handler)
		AddHandler(XML_DEFAULT_HANDLER, handler);
}

void XMLLoader::AddHandler(const char* tagname, xmlTagHandler handler)
{
	if(handler)
		_handlers[tagname] = handler;
}

bool XMLLoader::Load(void* userData)
{
	bool loaded = false;

	if(_path.size())
	{
		_xmlDoc = xmlReadFile(_path.c_str(), nullptr, _flags);

		if(_xmlDoc)
		{
			xmlNodePtr root = xmlDocGetRootElement(_xmlDoc);

			if(_xmlDoc)
			{
				Process(root, userData);
				loaded = true;
			}
		}
	}

    return loaded;
}

const xmlDoc* XMLLoader::GetDoc() const
{
    return _xmlDoc;
}

const xmlNode* XMLLoader::GetRootNode() const
{
	if(_xmlDoc)
		return xmlDocGetRootElement(_xmlDoc);

    return nullptr;
}

bool XMLLoader::Process(xmlNode* node, void* userData)
{
	if(!node || node->type != XML_ELEMENT_NODE)
		return true; // don't process this node, but continue the processing

	// handler for this tag
	xmlTagHandler handler = nullptr;

	// what to return on fail
	bool returnOnFalse = (_handlerReturnScope == NODE);

	if(_handlers.size())
	{
		handler = _handlers[(char*)node->name];

		if(!handler)
			handler = _handlers[XML_DEFAULT_HANDLER];
	}

	// if we found a handler, run it passing true
	// since this is the pre-child processing stage
	if(handler && !handler(this, node, true, userData))
		return returnOnFalse;

	// process this node's children
	for(xmlNodePtr child = node->children; child; child = child->next)
	{
		if(!Process(child, userData))
			return returnOnFalse;
	}

	// run the post child handler again after processing
	// children, passing false this time
	if(handler && !handler(this, node, false, userData))
		return returnOnFalse;

	// nothing returned false, so keep going
	return true;
}

void XMLLoader::Finish()
{
	if(!_complete)
	{
		if(_xmlDoc)
			xmlFreeDoc(_xmlDoc);

		xmlCleanupInputCallbacks();

		_complete = true;
	}
}

void XMLLoader::Reset()
{
	// make sure resources are released
	Finish();

	// re-init the io callbacks
	fioXmlRegisterInputCallbacks();

	// reset the private data
	_path = "";
	_flags = XML_PARSE_DTDLOAD | XML_PARSE_RECOVER;
	_xmlDoc = nullptr;
	_complete = false;
	_handlers.clear();
}

const char* XMLLoader::TrimString(const char* value)
{
	static char buffer[MAX_XML_TRIM_SIZE + 1]{};

	if (value && value[0]) {
		strsafecpy(buffer, value);
		return rmlead(rmtrail(buffer));
	}
	return "";
}

