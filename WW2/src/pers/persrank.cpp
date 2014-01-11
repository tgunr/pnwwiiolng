#include <syscfg.h> // Path is project specific.
#include <string.h>

#include "phui/Util.h"

#include "pers/persrank.h"
#include "wwii/wwiiTypes.h"
#include "ui/uiMain.h"
#include "wwii/wwiiBranchEnums.h"

/************************************************************************************************
	persrank
*************************************************************************************************/

static std::vector<persRank> _ranks;

static int 	xmlinputmatch(char const* filename){ return 1;}
static int 	_parseRankDoc(xmlDocPtr);
static void	_parseRankNode(xmlNodePtr);
static void	_parseCountryNode(xmlNodePtr, persRank&);
static void	_parseNameNode(xmlNodePtr, persRank&, int country);

/************************************************************************************************
	WideName
*************************************************************************************************/

const wchar_t* persRank::WideName(int country, int branch) const
{
	if(country > 0 && country <= (int)names.size())
	{
		if(branch > 0 && branch <= (int)names[country-1].size())
		{
			char key[15+1];
			snprintf(key, 15, "rank.%d.%d", level, branch);
			const std::wstring& localized = uiLocString(key);

			if(localized.at(0) != L'?')
				return localized.c_str();
			else
				return names[country-1][branch-1].c_str();
		}
	}

	return L"";
}

/************************************************************************************************
	Name
*************************************************************************************************/

const char* persRank::Name(int country, int branch) const
{
static char str[256];

	const wchar_t* wstr = WideName(country, branch);
	if(wstr)
	{
		char* s = str;
		for(int i = 0; i < 255 && *wstr; i++, wstr++, s++)
			*s = (char)*wstr; // convert to utf-8 instead maybe?
		*s = '\0';

		return str;
	}

	return nullptr;
}

/************************************************************************************************
	persrankLoad
*************************************************************************************************/

int persrankLoad(const char* name, xmlInputOpenCallback openfn, xmlInputReadCallback readfn, xmlInputCloseCallback closefn)
{
	int err = -1;

	xmlRegisterInputCallbacks(xmlinputmatch, openfn, readfn, closefn);
	xmlDocPtr doc = xmlReadFile(name, nullptr, XML_PARSE_RECOVER);
	if(doc)
	{
		err = _parseRankDoc(doc);
		xmlFreeDoc(doc);
	}

	xmlCleanupInputCallbacks();

	return err;
}

/************************************************************************************************
	persrank
*************************************************************************************************/

persRank const* persrankFind(int level)
{
	if(level < 1 || level > (int)_ranks.size()) return 0;

	return &_ranks[level-1];
}

int persrankMax()
{
	return (int)_ranks.size();
}

/************************************************************************************************
	_parseRankDoc
*************************************************************************************************/

static int _parseRankDoc(xmlDocPtr doc)
{
	xmlNodePtr rootnode = xmlDocGetRootElement(doc);
	if(!rootnode) return -1;

	if(xmlStrcmp((xmlChar*)"ranklist", rootnode->name) != 0) return -1;

	for(xmlNodePtr node = rootnode->children; node; node = node->next)
	{
		if(node->type != XML_ELEMENT_NODE) continue;

		if(xmlStrcmp((xmlChar*)"rank", node->name) == 0)
			_parseRankNode(node);
	}
	return 0;
}

/************************************************************************************************
	_parseRankNode
*************************************************************************************************/

static void _parseRankNode(xmlNodePtr ranknode)
{
	int level = -1, type = -1;
	xmlChar* xmlstr;

	xmlstr = xmlGetProp(ranknode, (xmlChar*)"level");

	if(xmlstr)
	{
		level = atoi((char*)xmlstr);
		xmlFree(xmlstr);
	}

	xmlstr = xmlGetProp(ranknode, (xmlChar*)"type");

	if(xmlstr)
	{
		if(xmlStrcmp((xmlChar*)"enlisted", xmlstr) == 0)
		{
			type = persRank::ENLISTED;
		}else if(xmlStrcmp((xmlChar*)"officer", xmlstr) == 0)
		{
			type = persRank::OFFICER;
		}else if(xmlStrcmp((xmlChar*)"command", xmlstr) == 0)
		{
			type = persRank::COMMAND;
		}else if(xmlStrcmp((xmlChar*)"staff", xmlstr) == 0)
		{
			type = persRank::STAFF;
		}
		xmlFree(xmlstr);
	}

	if(level > (int)_ranks.size()) _ranks.resize(level);

	persRank& rank = _ranks[level-1];
	rank.level = level;
	rank.type = type;

	for(xmlNodePtr node = ranknode->children; node; node = node->next)
	{
		if(node->type != XML_ELEMENT_NODE) continue;
		_parseCountryNode(node, rank);
	}
}

/************************************************************************************************
	_parseCountryNode
*************************************************************************************************/

static void _parseCountryNode(xmlNodePtr countrynode, persRank& rank)
{
	int country = 0;
	if(xmlStrcmp((xmlChar*)"british", countrynode->name) == 0)
		country = WWII_ENGLAND;
	else if(xmlStrcmp((xmlChar*)"american", countrynode->name) == 0)
		country = WWII_USA;
	else if(xmlStrcmp((xmlChar*)"french", countrynode->name) == 0)
		country = WWII_FRANCE;
	else if(xmlStrcmp((xmlChar*)"german", countrynode->name) == 0)
		country = WWII_GERMANY;

	if(country < 1) return;
	if(country > (int)rank.names.size()) rank.names.resize(country);

	for(xmlNodePtr node = countrynode->children; node; node = node->next)
	{
		if(node->type != XML_ELEMENT_NODE) continue;
		_parseNameNode(node, rank, country);
	}
}

/************************************************************************************************
	_parseNameNode
*************************************************************************************************/

static void	_parseNameNode(xmlNodePtr namenode, persRank& rank, int country)
{
	int branch = 0;

	if(xmlStrcmp((xmlChar*)"ground", namenode->name) == 0)
		branch = BRANCH_LAND;
	else if(xmlStrcmp((xmlChar*)"air", namenode->name) == 0)
		branch = BRANCH_AIR;
	else if(xmlStrcmp((xmlChar*)"navy", namenode->name) == 0)
		branch = BRANCH_SEA;

	if(branch < 1) return;

	if(branch > (int)rank.names[country-1].size())  rank.names[country-1].resize(branch);

	// this really should have been done as a attribute* oliver*
	if(namenode->children && namenode->children->content)
	{
		xmlChar* content = xmlNodeGetContent(namenode->children);
		rank.names[country-1][branch-1] = uiUTF8toUTF16(content);
		xmlFree(content);
	}
}
