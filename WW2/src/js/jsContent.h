#pragma once

#include <string>

#include "phui/Javascript.h"

typedef std::map<std::string, std::string> jsAttributeMap;

/************************************************************************************************
	jsContentData
*************************************************************************************************/

struct jsContentData : public jsAttributeMap
{
	typedef std::vector<std::pair<std::string, std::string> > LinkList;
	typedef std::vector<std::string> IncludeList;

	jsContentData() : type(0), body(""), links(NULL), includes(NULL), locMap(NULL)
	{
	}

	virtual ~jsContentData()
	{
		Reset();
	}

	void Reset();
	void AddLoc(const char* name, const wchar_t* value);
	void AddLink(const std::string& path, const std::string& title);
	void AddInclude(const char* path);
	void Merge(const jsContentData* from);
	const wchar_t* GetLoc(const char* name);

	int type;
	std::string body;
	LinkList* links;
	IncludeList* includes;

	// some content contains name/value pairs in the body
	// that are localization strings
	typedef std::map<std::string, std::wstring> LocMap;
	LocMap* locMap;
};

/************************************************************************************************
	jsContent
*************************************************************************************************/

class jsContent : public phui::js::Object
{
public:
	jsContent(JSContext* ctx, JSObject* parent, int type = -1);

	JS_CLASS_DATA;
	JS_FUNCTION(getloc);

	int Type(){ return _type; }

	void SetLoc(jsContentData* data);
	const wchar_t* GetLoc(const char* name);

	static std::string GetType(std::string &path);
	static jsContent* GetContent(JSContext*, JSObject* parent, const char* path);
	static bool GetData(const char* path,  jsContentData* data, bool match = true);
	static void GetIncludePath(std::string& path);
	static void ProcessBody(jsContent* content, jsContentData* data);

protected:
	~jsContent();

	int _type;
	jsContentData::LocMap* _locMap;
};

