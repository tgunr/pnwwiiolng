#include "syscfg.h" // Path is project specific.
#include <string>

#include "veh/vehxml.h"

/************************************************************************************************
	vehxml
*************************************************************************************************/

static int xmlinputmatch(char const* filename){ return 1;}
static void vehxmlExpandVehicleNodes(xmlDocPtr);
static void vehxmlAddVehicleFile(xmlDocPtr, xmlNodePtr, const char* filename);
static void vehxmlExpandIncludeNode(xmlNodePtr parent, xmlNodePtr includenode, const char* dir);
static void vehxmlCopyVariantNode(xmlNodePtr parent, xmlNodePtr variantnode, const xmlChar*, const char* dir);
static void vehxmlCopyNode(xmlNodePtr parentnode, xmlNodePtr vehnode, const char* dir);
static void vehxmlCopyChildNodes(xmlNodePtr parentnode, xmlNodePtr vehnode, const char* dir);
static void vehxmlCopyVehicleAttributes(xmlNodePtr from, xmlNodePtr to);

static std::string getdir(const char* filename);
static char* nobackslash(char*);

/************************************************************************************************
	vehxmlLoad
*************************************************************************************************/

xmlDocPtr vehxmlLoad(const char* name, xmlInputOpenCallback openfn, xmlInputReadCallback readfn, xmlInputCloseCallback closefn)
{
	xmlRegisterInputCallbacks(xmlinputmatch, openfn, readfn, closefn);

	xmlDocPtr doc = xmlReadFile(name, nullptr, XML_PARSE_RECOVER);
	if(doc)
		vehxmlExpandVehicleNodes(doc);

	xmlCleanupInputCallbacks();

//	xmlDocFormatDump(stdout, doc, 1);

	return doc;
}

/************************************************************************************************
	vehxmlExpandVehicleNodes
*************************************************************************************************/

static void vehxmlExpandVehicleNodes(xmlDocPtr doc)
{
	xmlNodePtr rootnode = xmlDocGetRootElement(doc);
	if(!rootnode) return;

	if(xmlStrcmp((xmlChar*)"vehicles", rootnode->name) != 0) return;

	for(xmlNodePtr node = rootnode->children; node; node = node->next)
	{
		if(node->type != XML_ELEMENT_NODE) continue;

		if(xmlStrcmp((xmlChar*)"vehicle", node->name) == 0)
		{
			xmlChar* filename = xmlGetProp(node, (xmlChar*)"file");

			if(filename)
			{
				nobackslash((char*)filename);
				vehxmlAddVehicleFile(doc, node, (char*)filename);
				xmlFree(filename);
			}
		}
	}
}

/************************************************************************************************
	vehxmlAddVehicleFile
*************************************************************************************************/

static void vehxmlAddVehicleFile(xmlDocPtr doc, xmlNodePtr parentnode, const char* filename)
{
	xmlDocPtr vehdoc = xmlReadFile(filename, nullptr, XML_PARSE_RECOVER);
	if(!vehdoc) return;

	xmlChar* vehid = xmlGetProp(parentnode, (xmlChar*)"id");
	if(!vehid) return;

	xmlNodePtr vehnode = xmlDocGetRootElement(vehdoc);
	if(vehnode && xmlStrcmp((xmlChar*)"vehicle", vehnode->name) == 0)
	{
		vehxmlCopyVehicleAttributes(vehnode, parentnode);

	    std::string dir = getdir(filename);

		// expand include nodes
	   	for(xmlNodePtr node = vehnode->children; node; node = node->next)
		{
			if(node->type != XML_ELEMENT_NODE) continue;

			if(xmlStrcmp((xmlChar*)"include", node->name) == 0)
				vehxmlExpandIncludeNode(parentnode, node, dir.c_str());
		}

		// parse variant last so it overrides any previous entries
	   	for(xmlNodePtr node = vehnode->children; node; node = node->next)
		{
			if(node->type != XML_ELEMENT_NODE) continue;

			if(xmlStrcmp((xmlChar*)"variant", node->name) == 0)
				vehxmlCopyVariantNode(parentnode, node, vehid, dir.c_str());
		}
	}

	xmlFree(vehid);
	xmlFreeDoc(vehdoc);
}

/************************************************************************************************
	vehxmlExpandIncludeNode
*************************************************************************************************/

static void vehxmlExpandIncludeNode(xmlNodePtr parentnode, xmlNodePtr node, const char* dir)
{
	xmlChar* xfile = xmlGetProp(node, (xmlChar*)"file");
	xmlDocPtr includedoc = nullptr;
	xmlNodePtr includenode = nullptr;

	std::string filename = dir;
	if(xfile)
	{
		nobackslash((char*)xfile);

		if(xfile[0] == '/')
		{
			filename = (char*)&xfile[1];
		}else
		{
			filename += "/";
			filename += (char*)xfile;
		}

// so we can xmlify stuff for OSG without worrying about old code
		if(filename.find(".mass.xml") != std::string::npos)
		includedoc = xmlReadFile(filename.c_str(), nullptr, XML_PARSE_RECOVER);
		xmlFree(xfile);
	}

	if(includedoc)
		includenode = xmlDocGetRootElement(includedoc);

	if(includenode)
	{
		std::string newdir = getdir(filename.c_str());
		vehxmlCopyNode(parentnode, includenode, newdir.c_str());
	}else
	{
		vehxmlCopyNode(parentnode, node, dir);
	}

	if(includedoc)
		xmlFreeDoc(includedoc);
}

/************************************************************************************************
	vehxmlCopyVariantNode
*************************************************************************************************/

static void vehxmlCopyVariantNode(xmlNodePtr vehnode, xmlNodePtr variantnode, const xmlChar* vehid, const char* dir)
{
	xmlChar* variantid = xmlGetProp(variantnode, (xmlChar*)"id");

	if(variantid)
	{
		if(xmlStrcmp(vehid, variantid) == 0)
		{
			vehxmlCopyVehicleAttributes(variantnode, vehnode);
			vehxmlCopyChildNodes(vehnode, variantnode, dir);
		}
		xmlFree(variantid);
	}
}

/************************************************************************************************
	vehxmlCopyNode
*************************************************************************************************/

static void vehxmlCopyNode(xmlNodePtr parentnode, xmlNodePtr addnode, const char* dir)
{
	if(xmlStrcmp((xmlChar*)"include", addnode->name) == 0)
	{
		xmlChar* xfile = xmlGetProp(addnode, (xmlChar*)"file");
		if(xfile)
		{
			xmlFree(xfile);
		}else
		{
			vehxmlCopyChildNodes(parentnode, addnode, dir);
			return;
		}
	}

	xmlNodePtr copynode = xmlCopyNode(addnode, 1);
	xmlAddChild(parentnode, copynode);
}

/************************************************************************************************
	vehxmlCopyChildNodes
*************************************************************************************************/

static void vehxmlCopyChildNodes(xmlNodePtr parentnode, xmlNodePtr addnode, const char* dir)
{
	for(xmlNodePtr node = addnode->children; node; node = node->next)
	{
		if(node->type != XML_ELEMENT_NODE) continue;

		if(xmlStrcmp((xmlChar*)"include", node->name) == 0)
		{
			vehxmlExpandIncludeNode(parentnode, node, dir);
		}else
		{
			vehxmlCopyNode(parentnode, node, dir);
		}
	}
}

/************************************************************************************************
	vehxmlCopyVehicleAttributes
*************************************************************************************************/

static void vehxmlCopyVehicleAttributes(xmlNodePtr srcnode, xmlNodePtr dstnode)
{
	// add vehicle attributes to existing vehicle node
   	for(xmlAttrPtr attr = srcnode->properties; attr; attr = attr->next)
    {
    	if(xmlStrcmp((xmlChar*)"id", attr->name) != 0)
    	{
			if(attr->children && attr->children->content)
			{
	    		if(xmlHasProp(dstnode, attr->name))
	    		{
	    			xmlSetProp(dstnode, attr->name, attr->children->content);
	    		}else
	    		{
	    			xmlNewProp(dstnode, attr->name, attr->children->content);
	    		}
	    	}
    	}
    }
}

/************************************************************************************************
	getdir
*************************************************************************************************/

static std::string getdir(const char* filename)
{
    std::string dir = filename;
    std::string::size_type pos = dir.rfind('/');
    if(pos != std::string::npos)
    	dir.erase(pos);
	else
		dir = "";

	return dir;
}

/************************************************************************************************
	nobackslash
*************************************************************************************************/

static char* nobackslash(char* filename)
{
	for(char* s = (char*)filename; *s; s++) { if(*s == '\\') *s = '/'; }
	return filename;
}
