#include <syscfg.h> // Path is project specific.
#include <stdio.h>	// var args
#include <stdarg.h>	// var args

#include "scen/ScenarioManager.h"
#include "scen/ScenarioLoader.h"
#include "jsContent.h"

// this is defined in jsContent.cpp but no where else

#define CONTENT_LOAD_FLAGS XML_PARSE_NOENT|XML_PARSE_RECOVER|XML_PARSE_DTDLOAD

// defines

// private variables

// static functions


/************************************************************************************************
	ScenarioLoader
*************************************************************************************************/

ScenarioLoader::ScenarioLoader(const char* path) :
	XMLLoader2<ScenarioLoader>(path, CONTENT_LOAD_FLAGS, true)
{
	AddHandler("content", &ScenarioLoader::DoContent, this);
	AddHandler("link", &ScenarioLoader::DoLink, this);
}

bool ScenarioLoader::DoContent(xmlNode* node, bool first, void* userData)
{
	if(first && userData)
	{
		jsContentData* data = (jsContentData*)userData;
		xmlGetNodeValue(node, XML_DATA_SINT, &data->type, "type");
	}

	return true;
}

bool ScenarioLoader::DoNode(xmlNode* node, bool first, void* userData)
{
	if(first && userData)
	{
		jsContentData* data = (jsContentData*)userData;

		const char* name = (const char*)node->name;
		xmlChar* body = xmlNodeGetContent(node);

		if(body)
		{
			if(*body)
			{
				if(!strcasecmp(name, "body"))
					data->body = (const char*)body;
				else if(!strcasecmp(name, "include"))
					data->AddInclude((const char*)body);
				else
					(*data)[name] = (const char*)body;
			}

			xmlFree(body);
		}
	}

	return true;
}

bool ScenarioLoader::DoLink(xmlNode* node, bool first, void* userData)
{
	if(first && userData)
	{
		jsContentData* data = (jsContentData*)userData;

		std::string path = xmlGetStringFromNode(node, "path");
		std::string title = xmlGetStringFromNode(node, "title");

		if(path.length())
			data->AddLink(path, title);
	}

	return true;
}