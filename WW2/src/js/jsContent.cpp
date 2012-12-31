#include <syscfg.h> // Path is project specific.
#include <string>

#include <libxml/parser.h>
#include <libxml/tree.h>
#if platOS == OS_WIN32
#include <regex>
#define RENS	std
#else
#include <boost/regex.hpp>
#define RENS	boost
#endif
#include "map/mapPersonaSummary.h"

#include "js/jsContent.h"
#include "ui/uiMain.h"
#include "xml/XMLLoader2.h"
#include "wwii/wwiiUTF.h"
#include "client/GameClient.h"

#define MAX_CONTENT_NAME 128
#define MAX_CONTENT_VALUE (8192 * 2)
#define CONTENT_LOAD_FLAGS XML_PARSE_NOENT|XML_PARSE_RECOVER|XML_PARSE_DTDLOAD

void jsContentData::Reset()
{
	type = 0;
	body = "";

	if(links)
	{
		links->clear();
		delete links;
		links = NULL;
	}

	if(includes)
	{
		includes->clear();
		delete includes;
		includes = NULL;
	}

	if(locMap)
	{
		delete locMap;
		locMap = NULL;
	}

	clear();
}

void jsContentData::AddLink(const std::string& path, const std::string& title)
{
	if(!links) links = new LinkList;
	if(links)  links->push_back(std::make_pair(path, title));
}

void jsContentData::AddInclude(const char* path)
{
	if(!includes) includes = new IncludeList;
	if(includes)  includes->push_back(path);
}

void jsContentData::Merge(const jsContentData* from)
{
	type = from->type;

	if(from->body.size())
		body += from->body;

	if(from->links && from->links->size())
	{
		for(UINT32 l = 0; l < from->links->size(); ++l)
			AddLink(from->links->at(l).first, from->links->at(l).second);
	}

	// merge our attributes
	jsContentData::const_iterator it = from->begin();

	for(; it != from->end(); ++it)
		(*this)[it->first] = it->second;
}


void jsContentData::AddLoc(const char* name, const wchar_t* value)
{
	if(!locMap)
		locMap = new LocMap;

	(*locMap)[name] = value;
	//uiConsoleprintf("ADDING BODY LOC: %s -> %s", name, UTF8(value));
}

const wchar_t* jsContentData::GetLoc(const char* name)
{
	if(locMap && locMap->find(name) != locMap->end())
	{
		//uiConsoleprintf("GETTING BODY LOC: %s -> %s", name, UTF8(_locMap[name].c_str()));
		return (*locMap)[name].c_str();
	}

	return L"";
}

class jsContentLoader : public XMLLoader2<jsContentLoader>
{
public:

	jsContentLoader(const char* path);
	virtual ~jsContentLoader(){}

	// tag handlers
	XML_TAG_HANDLER(DoContent);
	XML_TAG_HANDLER(DoLink);
	XML_TAG_HANDLER(DoNode);
};

jsContentLoader::jsContentLoader(const char* path) :
	XMLLoader2<jsContentLoader>(path, CONTENT_LOAD_FLAGS, true)
{
	AddHandler("content", &jsContentLoader::DoContent, this);
	AddHandler("link", &jsContentLoader::DoLink, this);
	AddDefaultHandler(&jsContentLoader::DoNode, this);
}

bool jsContentLoader::DoContent(xmlNode* node, bool first, void* userData)
{
	if(first && userData)
	{
		jsContentData* data = (jsContentData*)userData;
		xmlGetNodeValue(node, XML_DATA_SINT, &data->type, "type");
	}

	return true;
}

bool jsContentLoader::DoNode(xmlNode* node, bool first, void* userData)
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
				if(!stricmp(name, "body"))
					data->body = (const char*)body;
				else if(!stricmp(name, "include"))
					data->AddInclude((const char*)body);
				else
					(*data)[name] = (const char*)body;
			}

			xmlFree(body);
		}
	}

	return true;
}

bool jsContentLoader::DoLink(xmlNode* node, bool first, void* userData)
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

/************************************************************************************************
	jsContent
*************************************************************************************************/

JSClass jsContent::jsclass = JS_CLASS_MACRO("content");

JSFunctionSpec jsContent::jsfunctions[] =
{
	JS_FUNCTION_SPEC(jsContent, getloc, 1),
	{0}
};

JSPropertySpec jsContent::jsproperties[] =
{
	{0}
};

/************************************************************************************************
	getloc -- lookup a content specific localized string
*************************************************************************************************/

JS_FUNCTION_DEF(jsContent, getloc)
{
	*rval = JSVAL_NULL;

	if(argc >= 1)
	{
		jsContent* jscontent = phui::js::Cast<jsContent*>(ctx,jsobj);

		if(jscontent)
		{
			const char* name = JS_GetStringBytes(JS_ValueToString(ctx, argv[0]));

			if(name && *name)
			{
				const wchar_t* value = jscontent->GetLoc(name);

				if(value && *value)
					*rval = phui::js::NewString(value);
			}
		}
	}

	return JS_TRUE;
}

#if(0)
static int xmlinputmatch(char const* filename){ return 1; }
#endif

/************************************************************************************************
	GetContent
*************************************************************************************************/

jsContent* jsContent::GetContent(JSContext* ctx, JSObject* parent, const char* target)
{
	jsContent* content = 0;

	std::string contentPath = target;
	std::string contentType = jsContent::GetType(contentPath);
	std::string fullPath	= "loc://" + contentPath;

	jsContentData data;

	if(GetData(fullPath.c_str(), &data, false))
	{
		//uiConsoleprintf("PULLED CONTENT: %s", fullPath.c_str());

		content = new jsContent(ctx, parent, data.type);

		content->DefineProperty("type", phui::js::NewString(contentType.c_str()));
		content->DefineProperty("path", phui::js::NewString(contentPath.c_str()));

		ProcessBody(content, &data);

		for(jsContentData::iterator it = data.begin(); it != data.end(); ++it)
		{
			const std::string& name = it->first;
			const std::string& value = it->second;

			content->DefineProperty(name.c_str(), phui::js::NewString(uiUTF8toUTF16(value.c_str())));
		}

		if(data.links && data.links->size())
		{
			phui::js::Array* links = new phui::js::Array;
			content->DefineProperty("links", links);

			for(UINT32 l = 0; l < data.links->size(); ++l)
			{
				std::pair<std::string, std::string>& link = data.links->at(l);

				jsContent* contentLink = new jsContent(ctx, content->jsObject());

				std::string contentType = jsContent::GetType(link.first);

				if(link.second.size())
					contentLink->DefineProperty("title", phui::js::NewString(uiUTF8toUTF16(link.second.c_str())));

				contentLink->DefineProperty("path", phui::js::NewString(link.first.c_str()));
				contentLink->DefineProperty("type", phui::js::NewString(contentType.c_str()));

				links->SetElement(contentLink, l);
			}
		}
	}
	else
	{
		//uiConsoleprintf("FAILED PULLING CONTENT: %s", fullPath.c_str());
	}

	return content;
}

bool jsContent::GetData(const char* path, jsContentData* data, bool match)
{
	// do caching here

	jsContentLoader loader(path);

	if(loader.Load(data))
	{
		loader.Finish();

		if(data->includes && data->includes->size())
		{
			jsContentData include;
			std::string incPath = "";

			for(UINT32 i = 0; i < data->includes->size(); ++i)
			{
				include.Reset();

				incPath = "loc://";
				incPath += data->includes->at(i);

				// includes can be persona specific, so lets check and update the path
				GetIncludePath(incPath);

				// don't include ourself as that would lead to a recursive loop
				if(!stricmp(incPath.c_str(), path))
					continue;

				if(GetData(incPath.c_str(), &include))
					data->Merge(&include);
			}
		}

		if(data->type == 5 || data->find("bodyloc") != data->end())
		{
			// go ahead and add something here for legacy sake
			std::wstring wbody = L"";
			char buf[MAX_CONTENT_VALUE+1];
			wchar_t wbuf[MAX_CONTENT_VALUE+1];

			strncpy(buf, data->body.c_str(), MAX_CONTENT_VALUE);

			if(!WWII::ConvertChars(buf, wbuf, sizeof(wbuf)))
				return false;

			// store it in a std::wstring so wregex can operate on it
			wbody = wbuf;

			// our regular expressions
			RENS::wregex split(L"\\n");
			RENS::wregex kvre(L"^\\s*([\\w\\.]+)\\s*=\\s*(.+)\\s*$");
			RENS::wregex nlre(L"\\[NL\\]");

			// our tokenizing iterators
			RENS::wsregex_token_iterator p(wbody.begin(), wbody.end(), split, -1);
			RENS::wsregex_token_iterator end;

			// key/value pair matches
			RENS::wcmatch matches;

			// iterate over all lines
			for (;p != end; ++p)
			{
				std::wstring kv(p->first, p->second);

				// change [NL] into newlines
				static const std::wstring newline(L"\\n");
				kv =  RENS::regex_replace(kv, nlre, newline);

				try
				{
					if(RENS::regex_search(kv.c_str(), matches, kvre) && matches.size() > 0)
					{
						data->AddLoc(UTF8(matches.str(1).c_str()), matches.str(2).c_str());
					}
				}
				catch(RENS::regex& /* e */)
				{
					// skip this line
				}
			}
		}

		return true;
	}

	return false;
}

void jsContent::GetIncludePath(std::string& path)
{
	// try different variants of the file based on country and branch of current persona
	size_t period = path.rfind('.');

	if(period != std::string::npos)
	{
		char checkPath[255+1];
		std::string basePath = path.substr(0, period);
		const PersonaSelection& persona = PlayerClient()->PersonaSelected();

		// set the
		for(int i = 0; i < 3; i++)
		{
			switch(i)
			{
				case 0:
					snprintf(checkPath, 255, "%s.%d.%d.xml", basePath.c_str(), persona.m_countryID, persona.m_branchID);
				break;
				case 1:
					snprintf(checkPath, 255, "%s.%d.0.xml", basePath.c_str(), persona.m_countryID);
				break;
				case 2:
					snprintf(checkPath, 255, "%s.0.%d.xml", basePath.c_str(), persona.m_branchID);
				break;
			}

			if(uiHasLocFile(checkPath))
			{
				// we found one, use it
				path = checkPath;
				break;
			}
		}
	}
}

void jsContent::ProcessBody(jsContent* content, jsContentData* data)
{
	if(!data || !content)
		return;

	if(data->locMap)
	{
		// empty string
		content->DefineProperty("body", phui::js::NewString(L""));

		// let our content control the loc map
		content->SetLoc(data);
	}
	else if(data->body.length())
	{
		// we didn't have a body loc map but we did have a body defined
		// add it to our content
		content->DefineProperty("body", phui::js::NewString(UTF16(data->body.c_str())));
	}

}

/************************************************************************************************
	jsContent
*************************************************************************************************/

jsContent::jsContent(JSContext* ctx, JSObject* parent, int type) :
	phui::js::Object(&jsclass, ctx, parent, jsfunctions, jsproperties),
	_type(type),
	_locMap(NULL)
{
	JS_CONSTRUCTOR_LOCK;
};

jsContent::~jsContent()
{
	if(_locMap)
	{
		delete _locMap;
		_locMap = NULL;
	}
}

/************************************************************************************************
	jsContent::GetType - returns the type based on the path, adjust path
*************************************************************************************************/

std::string jsContent::GetType(std::string &path)
{
	std::string type = "file";

	// is this a directory or file?
	if(path.rfind("index.xml", path.length() - 1) != std::string::npos)
	{
		type = "directory";
	}
	else if(path.rfind(".", path.length() - 1) == std::string::npos)
	{
		if(path[path.length() - 1] != '/')
			path += "/";

		path += "index.xml";
		type = "directory";
	}

	return type;
}

const wchar_t* jsContent::GetLoc(const char* name)
{
	if(_locMap && _locMap->find(name) != _locMap->end())
	{
		//uiConsoleprintf("GETTING BODY LOC: %s -> %s", name, UTF8(_locMap[name].c_str()));
		return (*_locMap)[name].c_str();
	}

	return L"";
}

void jsContent::SetLoc(jsContentData* data)
{
	if(!data)
		return;

	// take control
	_locMap = data->locMap;

	// the data no longer need its
	data->locMap = NULL;
}
