/*--------------------------------------------------------------------------------
language.cpp
---------------------------------------------------------------------------------*/
#include "language.h"
#include "file.h"
#include "libxml/parser.h"
#include "libxml/tree.h"

#include "stdafx.h"
#include "settings.h"

#include "ConvertUTF.h"

#include <string>
#include <map>

typedef std::map<std::string, std::wstring> Language;
static Language _lang[MAX_LANGUAGES];

static int _loadingLanguage = 0;

static int _xmlinputmatch(const char *name) {return 1;}

const char* langNames[MAX_LANGUAGES] = {
	"english",
	"italian",
	"french",
	"german",
	"spanish",
	"chinese"
};

/*------------------------------------------------------------------------
_process
-------------------------------------------------------------------------*/
void _process(xmlEntityPtr entity, void *notused)
{
    const wchar_t *s = utf8_to_utf16((const unsigned char *)entity->content);
    _lang[_loadingLanguage][(const char *)entity->name] = s;
}

/*------------------------------------------------------------------------
langInit
-------------------------------------------------------------------------*/
bool langInit(int language, const char *path)
{
    _loadingLanguage = language;

	xmlRegisterInputCallbacks(_xmlinputmatch,
		                     (xmlInputOpenCallback)fileOpen,
		                     (xmlInputReadCallback)fileRead,
		                     (xmlInputCloseCallback)fileClose);
    
    xmlDtdPtr dtd = xmlParseDTD(NULL, (xmlChar *)path);

    if(!dtd)
    {
        return false;
    }

    if(!dtd->children)
    {
        return false;
    }

    if(dtd)
    {
        xmlHashScan((xmlHashTablePtr)dtd->entities, (xmlHashScanner)_process, NULL);
        xmlFreeDtd(dtd);
    }
    else
    {
        return false;
    }

    return true;
}

/*------------------------------------------------------------------------
getLoc
-------------------------------------------------------------------------*/
const wchar_t *getLoc(int language, const char *key)
{
    Language::iterator find = _lang[language].find(key);

    if(find == _lang[language].end())
    {
        return utf8_to_utf16((unsigned char *)"#######");
    }

    return find->second.c_str();
}

/*------------------------------------------------------------------------
str_to_wstr
-------------------------------------------------------------------------*/
wchar_t *str_to_wstr(wchar_t *wstr, const char *str, int maxlen)
{
	wchar_t *w = wstr;

	for(int i = 0; i < maxlen && *str ; i++, w++, str++)
    {
        *w = (unsigned char)*str;
    }
    
    *w = 0;

	return wstr;
}

/*------------------------------------------------------------------------
wstr_to_str
-------------------------------------------------------------------------*/
char *wstr_to_str(char *str, const wchar_t *wstr, int maxlen)
{   
	char *s = str;

	for(int i = 0; i < maxlen && *wstr; i++, wstr++, s++)
    {
        *s = (char)*wstr;
    }

	*s = '\0';

	return str;
}

/*------------------------------------------------------------------------
utf8_to_utf16
-------------------------------------------------------------------------*/
const wchar_t *utf8_to_utf16(const char *val)
{
	return utf8_to_utf16((const unsigned char*)val);
}

const wchar_t *utf8_to_utf16(const unsigned char *val)
{
	const UTF8* valptr = (const UTF8 *)val;

	static wchar_t wval[8192];
	wchar_t *wvalptr = wval;

    ConversionResult result = ConvertUTF8toUTF16(&valptr, 
                                                (UTF8 *)val+strlen((char*)val), 
                                                (UTF16**)&wvalptr, 
                                                (UTF16*)wval+8191, 
                                                lenientConversion);
    *wvalptr = 0;
	
	if(result != conversionOK)
    {
		str_to_wstr(wval, (char*)val, 8191);
    }

	return wval;
}

/*------------------------------------------------------------------------
utf16_to_utf8
-------------------------------------------------------------------------*/
const unsigned char *utf16_to_utf8(const wchar_t *wval)
{
	const UTF16* wvalptr = (UTF16*)wval;

	static unsigned char val[8192];
	unsigned char *valptr = val;
	
	ConvertUTF16toUTF8(&wvalptr, 
                       (UTF16*)wval+wcslen(wval), 
                       &valptr, 
                       val+8191, 
                       lenientConversion);
	*valptr = 0;

	return val;
}
