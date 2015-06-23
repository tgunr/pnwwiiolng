#ifndef PN_XML_XMLLOADER_H
#define PN_XML_XMLLOADER_H 1

#include <map>
#include <string>
#include "libxml/tree.h"
#include "libxml/parser.h"

/************************************************************************************************
	XMLLoader
*************************************************************************************************/

/*
	USAGE

	To use this api, simply declare an XMLLoader object. You can assign the path
	to the xml doc, the default handler and the xml flags through the constructor
	or via object methods.

	Once declared, simply add additional handlers via the AddHandler() method. The
	name of the handler is the same as the tagname encountered while processing.

	If a specific tag handler is found, it is used, otherwise the default tag handler
	is used if one was specified. If no tag handler is found, parsing simply continues.

	Handlers either return true or false. This returned value has different meaning
	depending on the XMLLoader object's HANDLER_RETURN_SCOPE setting. The HANDLER_RETURN_SCOPE
	setting can be either NODE or DOCUMENT. NODE is the default. Both the DOCUMENT
	and NODE scopes do the same thing if true is returned and that is to simply
	continue processing. If the scope is set to NODE and false is returned, then processing
	stops for the current node and its children. This allows a handler to let the XMLLoader
	object that it will handle its own children in its own way. This can be useful if you want
	to pass userData to handlers of tags that are children of the current tag. If the scope
	is set to DOCUMENT and false was returned, then processing of the document stops
	completely. This can be useful if you are looking for a specific tag and don't need to
	process once the tag is found. Simply handle the tag in your handler, then return false.

	If a tag's handler returns true, the handler is called once before its children are processed
	and once after. This gives you an opportunity to get ready to handle child data if need be. The
	children, if any, are processed by calling their handlers if they have one. Once
	the children are processed, the current node's handler is called once more so you
	can post process the node and its children if need be.

	While processing tags, you are also responsible for freeing all "new" resources
	like those returned from xmlGetProp() and xmlNodeGetContent(). The xmlDoc
	is freed for you in the destructor of the XMLLoader object or if you call
	Finish() or Reset().

	This class has a number of virtual methods. This allows subclassing the
	class for special xml handling if the need ever arises.

	For a real example, search for "XML DATA LOADING" in fpTPSdata.cpp and look over the
	functions below that line.

	A slim example:

	// function prototypes defined at the top of the file
	...

	... // called from some init function
	fpTPSLoadXMLData("path_to_my_xml_doc");
	..

	void fpTPSLoadXMLData(const char* path)
	{
		XMLLoader config(path, defaultXMLTag);

		config.AddHandler("weapons", weaponsXMLTag); // one of these tags in the xml doc
		config.AddHandler("mesh", meshXMLTag); // many of these tags within the weapons tag
		config.AddHandler("infantry", infantryXMLTag);
		config.AddHandler("weapons", infantryXMLTag);

		if(!config.Load())
		{
			// log the failure
		}

		// to re-use the object, do...

		config.Reset();
		config.SetPath("some other path");
		config.AddDefaultHandler(otherDefaultXMLHandler);
		config.AddHandler("tagname", tagHandler);
		config.AddHandler("tagname2", tagHandler2);

		if(!config.Load())
		{
			// log the failure
		}
	}

	bool defaultTagHandler(XMLLoader* loader, xmlNode* node, bool first, void* userData)
	{
		// each tag will get passed to this handler since its the default
		// you can selectivly handle tags by inspecting the node's name which
		// is the name of the tag being processed
		return true; // continue processing
	}

	bool infantryXMLTag(XMLLoader* loader, xmlNode* node, bool first, void* userData)
	{
		// parse the weapons node for content and properties
		return true;
	}

	bool weaponsXMLTag(XMLLoader* loader, xmlNode* node, bool first, void* userData)
	{
		int meshType = TPS_MESH_WEAPON;

		// loop over our children
		for(xmlNodePtr child = node->children; child; child = child->next)
		{
			// use the loader to continue the processing, but its now
			// under our excution path with our own userData
			if(!loader->Process(child, &meshType))
				break;
		}

		return false; // we handle our own children
	}

	bool meshXMLTag(XMLLoader* loader, xmlNode* node, bool first, void* userData)
	{
		// userData was set in the weaponsXMLTag handler because mesh tags
		// are children of weapons tags
		int meshType = *(int*)userData;

		switch(meshType)
		{
			// get properties and content associated with the mesh and store it
		}

		// no children expected, so go ahead and return false
		// if we returned true here, this function would get called
		// a 2nd time because the XMLLoader object would call it
		// once, then try its children, then call it again after
		// after the children
		return false;
	}
*/


/********* defines **********/
#define XML_GET_USERDATA(type,name) (type)(*(xmlDataMap*)userData)["name"]
#define XML_CMP_STRINGS(s1,s2) !stricmp((const char*)(s1), (const char*)(s2))
#define XML_IS_NODETYPE(type) !stricmp((const char*)node->name, "type")
#define XML_TAG_HANDLER(handler) bool handler(XMLLoader* loader, xmlNode* node, bool first, void* userData)
#define XML_PROCESS_CHILDREN(data) \
for(xmlNodePtr child = node->children; child; child = child->next) \
{ \
	if(!loader->Process(child, data)) \
		break; \
}

/********* types ************/

class XMLLoader;

/* tag processor prototype
 * takes an xmlNode* and a boolean
 * the handler is called before AND after
 * children are processed; the boolean will
 * be true when the handler is called before
 * the children are processed and false afterwards
 * this lets you "get ready" for child data
 */
typedef bool (*xmlTagHandler)(XMLLoader*, xmlNode*, bool, void*);
typedef std::map<std::string, xmlTagHandler> xmlTagHandlerMap;
typedef std::map<std::string, void*> xmlDataMap;

class XMLLoader
{
public:
	XMLLoader();
	XMLLoader(const char* path,
			  xmlTagHandler def = nullptr,
			  int flags = XML_PARSE_DTDLOAD | XML_PARSE_RECOVER);
	virtual ~XMLLoader();

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
	virtual void AddDefaultHandler(xmlTagHandler handler);
	virtual void AddHandler(const char* tagname, xmlTagHandler handler);

	// parses the xml doc and starts processing nodes; primary
	// entry into the load process
	virtual bool Load(void* userData = nullptr);

	// primary worker; recursively called on each node starting with
	// the root node from Load(); can be called externally
	virtual bool Process(xmlNode* node, void* userData = nullptr);

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

	// helper functions
	static const char* TrimString(const char*);

protected:

	int _flags;
	bool _complete;
	xmlDoc* _xmlDoc;
	std::string _path;
	xmlTagHandlerMap _handlers;
	HANDLER_RETURN_SCOPE _handlerReturnScope;
};

#endif // PN_XML_XMLLOADER_H
