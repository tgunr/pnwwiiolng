#include <syscfg.h> // Path is project specific.
//////////////////////////////////////////////////////////////////////
// Module: loclLang.cpp
// Description: TODO: ***REDO with STL***
//////////////////////////////////////////////////////////////////////

#include "locl/loclLang.h"
#include "mem/mem.h"

#if platBuild == BUILD_CLIENT
#include "fio/fiod.h"
#endif

#define LANG_ERROR "########"

#include <string>

static LANGTABLE* loclLangReadFile(SCHAR* pszPath);
static const SCHAR* _GetString(LANGTABLE* langTable, UINT32 uID);

static ENUM_LANGUAGES g_language;

static LANGTABLE g_englishTable[5];

///////////////////////////////////////////////////
// Function: loclLangInit
// Description: Top level init for local languages.
///////////////////////////////////////////////////
LOGICAL loclLangInit(ENUM_LANGUAGES enumLang)
{
    g_language = enumLang;

	/* no longer used
	SINT32 i;

	loclLangKill();

    memset(g_LangTable, 0, sizeof(LANGTABLE) * NUMBER_OF_CATEGORIES);

    ///////////////////////////////
    // Create table of categories.
    ///////////////////////////////
    for(i = 0; i < NUMBER_OF_CATEGORIES; i++)
    {
		g_LangTable[i].next = loclLangTableLoad(enumLang, i);

        // Load up english table if langauge is not english. Use to find strings
        // in english if the foreign langauge string has yet to be translated.
        if(enumLang != IDE_LANGUAGE_ENGLISH)
        {
            g_englishTable[i].next = loclLangTableLoad(IDE_LANGUAGE_ENGLISH, i);
        }

		if(g_LangTable[i].next)
        {
			g_LangTable[i].next->prev = &g_LangTable[i];
        }
        else
        {
// Need front end defined, since this module is used for settings.exe
#if platBuild == BUILD_CLIENT
            Write_Debug("[loclLangInit] **Loading language table FAILED**");
#endif
            return FALSE;
        }
    }
	*/

    return TRUE;
}

///////////////////////////////////////////////////
// Function: loclLangKill
// Description: Kill routine for loclLang.
///////////////////////////////////////////////////
LOGICAL loclLangKill()
{
	/* no longer used
    SINT32 i;

    for(i = 0; i < NUMBER_OF_CATEGORIES; i++)
    {
        loclLangTableFree(g_LangTable[i].next);
        loclLangTableFree(g_englishTable[i].next);

        g_LangTable[i].next = nullptr;
        g_englishTable[i].next = nullptr;
    }
	*/

    return TRUE;
}

/////////////////////////////////////////////////////
// Function: loclLangReadFile
// Description: Read localization file into a buffer.
/////////////////////////////////////////////////////
LANGTABLE* loclLangReadFile(SCHAR* pszPath)
{
	LANGTABLE* langTable = nullptr;

/*#if platBuild == BUILD_CLIENT
    LOGICAL langDirectory = FALSE;

#ifdef BETA_RELEASE
    if(fiocfgAccess("languages"))
    {
        langDirectory = TRUE; // variable not being used?
    }
#endif
#endif*/

    SINT32 h = fioOpen(pszPath);

    if(h == -1)
    {
        return FALSE;
    }

    SCHAR szBuff[256];

    SINT32 pos;

    memset(szBuff, 0, sizeof(SCHAR) * 256);

    while(fioGetString(h, szBuff, 255) != 0)
    {
        SCHAR* c = strchr(szBuff, 13);
        if(c) *c = 0;
        c = strchr(szBuff, 10);
        if(c) *c = 0;

        std::string str = szBuff;

        if(str.length() == 0)
            continue;

        pos = str.find(',');

        if(pos == -1) continue;

        std::string strId = str.substr(0, pos);

        LANGTABLE* lpNode = (LANGTABLE*)memAlloc(sizeof(LANGTABLE));

        if(!lpNode) return nullptr;

        lpNode->uID = strtoul(strId.c_str(), nullptr, 0);

        pos++;

        std::string strLine = str.substr(pos, str.length());

        strcpy((SCHAR*)lpNode->szStr, strLine.c_str());

        if(langTable) langTable->prev = lpNode;

		lpNode->next = langTable;
		langTable = lpNode;
    }

    fioClose(h);

    return langTable;
}

/////////////////////////////////////////////////////
// Function: loclLangGetString
// Description: Search list for specific string.
/////////////////////////////////////////////////////
UCHAR* loclLangGetString(ENUM_CATEGORIES enumCategory, UINT32 uID)
{
	static char buffer[128+1];
	snprintf(buffer, 128, "loc.%s.%d", loclGetStringCategory(enumCategory), uID);
	return (UCHAR*)buffer;

	/*
    SINT32 category = (SINT32)enumCategory;

    if(category < 0 || category >= NUMBER_OF_CATEGORIES)
    {
        return (UCHAR*)LANG_ERROR;
    }

    const SCHAR* p = _GetString(g_LangTable[category].next, uID);

    // Could not find the string. If english language, return LANG_ERROR to indicate
    // string could not be found. If a foreign language, check if the string is in
    // the english file.
    if(!p)
    {
        if(g_language == IDE_LANGUAGE_ENGLISH)
        {
            p = LANG_ERROR;
        }
        else
        {
            p = _GetString(g_englishTable[category].next, uID);

            if(!p) p = LANG_ERROR;
        }
    }

    return (UCHAR*)p;
	*/
}

////////////////////////////////////////////////////
// Function: loclGetStringCategory
// Description: returns the category string name
/////////////////////////////////////////////////////
UCHAR* loclGetStringCategory(ENUM_CATEGORIES e)
{
    if(e < 0 || e >= NUMBER_OF_CATEGORIES)
        return nullptr;

    return (UCHAR*)g_szFiles[e];
}

////////////////////////////////////////////////////
// Function: loclLangLoadTable
// Description: Load language table for specified language and category
/////////////////////////////////////////////////////
LANGTABLE* loclLangTableLoad(ENUM_LANGUAGES enumLang, SINT32 category)
{
    SCHAR pszPath[128];

	if(enumLang < 0	|| enumLang >= NUMBER_OF_LANGUAGES)
        return nullptr;

    if(category < 0 || category >= NUMBER_OF_CATEGORIES)
        return nullptr;

    snprintf(pszPath, 128, "%s%s.loc", g_szPaths[enumLang], g_szFiles[category]);

    return loclLangReadFile(pszPath);
}

//////////////////////////////////////////////////////////
// Function: loclGetLangTableString
// Description: For LANGTABLEs created outside the module.
//////////////////////////////////////////////////////////
const SCHAR* loclGetLangTableString(LANGTABLE* lt, UINT32 ID)
{
    return _GetString(lt, ID);
}

////////////////////////////////////////////////////
// Function: loclGetString
// Description:
/////////////////////////////////////////////////////
LOGICAL loclGetString(ENUM_CATEGORIES e, UINT32 uID, const SCHAR** returnStr)
{
    const SCHAR* p = _GetString(g_LangTable[e].next, uID);

    // Could not find the string. If english language, return LANG_ERROR to indicate
    // string could not be found. If a foreign language, check if the string is in
    // the english file.
    if(!p)
    {
        if(g_language == IDE_LANGUAGE_ENGLISH)
        {
            return FALSE;
        }
        else
        {
            p = _GetString(g_englishTable[e].next, uID);

            if(!p)
            {
                return FALSE;
            }
        }
    }

    *returnStr = p;

    return TRUE;
}

////////////////////////////////////////////////////
// Function: _GetString
// Description: Search list for specific string.
/////////////////////////////////////////////////////
const SCHAR* _GetString(LANGTABLE* langTable, UINT32 uID)
{
	LANGTABLE* lpHead = nullptr;
	const SCHAR* p = nullptr;

    if(langTable == nullptr)
        return LANG_ERROR;

    lpHead = langTable;

    for(; lpHead != nullptr; lpHead = lpHead->next)
    {
        if(lpHead->uID == uID)
        {
            p = (SCHAR*)lpHead->szStr;
            break;
        }
    }

    return p;
}

////////////////////////////////////////////////////
// Function: loclLangTableFree
// Description: Free memory allocated by table
/////////////////////////////////////////////////////
void loclLangTableFree(LANGTABLE* langTable)
{
    if(langTable == nullptr)
        return;

   	LANGTABLE* lpHead = nullptr;
    LANGTABLE* lpFree = nullptr;

	lpHead = langTable;

    while(lpHead != nullptr)
    {
        lpFree = lpHead;
        lpHead = lpHead->next;
        memFree(lpFree);
    }
}
