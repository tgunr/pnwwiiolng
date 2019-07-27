/*--------------------------------------------------------------------------------
load.cpp
---------------------------------------------------------------------------------*/
#include "syscfg.h" // Path is project specific.

#include <Windows.h>
#include "cfgData.h"

#include "load.h"
#include "file.h"
#include "language.h"
#include "registry.h"

#include "libxml/parser.h"
#include "libxml/tree.h"

#include <fcntl.h>
#include <io.h>
#include <sys/stat.h>
#include <map>

wwiiolpath_string GameSettingsFilename = L"wwiiol.xml";  //  -> wwiiol.xml

static bool _xmlfile = false;

static VALUE _message;

static int _xmlinputmatch(const char* name) {return 1;}
static int _bestLanguage();

static bool _loadXML(const wchar_t* file);
static bool _loadCFG(const char* file);
static bool _loadDefault();

static void _insertCFGType(WWIIOLXML* data, int type, const char* value);

#include <unordered_map>

typedef std::unordered_map<std::string, WWIIOLXML *> PrefsData;
static PrefsData _xmldata;

/*-------------------------------------------------------------
loadPreferences
--------------------------------------------------------------*/
bool loadPreferences()
{
    xmlRegisterInputCallbacks(_xmlinputmatch,
                             (xmlInputOpenCallback)fileOpen,
                             (xmlInputReadCallback)fileRead,
                             (xmlInputCloseCallback)fileClose);

	wwiiolpath_string homePath = WW2_userDocumentsPath + L"\\" + GameSettingsFilename;

	_loadDefault();

	bool xmlloaded = _loadXML(homePath.c_str());
	
	if(!xmlloaded)
	{
		wwiiolpath_string workingPath = WW2_userDocumentsPath + L"\\DATA\\" + GameSettingsFilename;

		xmlloaded = _loadXML(workingPath.c_str());
	}

    if(!xmlloaded || !getValue("config.authoritative"))
    {          
        if(!_loadCFG("wwiiol.cfg"))
        {
            // Do nothing
        }
    }
    else
    {
        _xmlfile = true;
    }

    xmlCleanupInputCallbacks();

    return true;
}

int region()
{
#if GREEN_INSTALL == 1
#if TAFAT
	return 0;
#else
	return 5;
#endif
#else
	std::wstring region = registryInstallValue(_T("Region"));
	
	if (region == L"china")
		return 5;
#endif // GREEN_INSTALL
	
	return 0;
}

/*-------------------------------------------------------------
getValue
--------------------------------------------------------------*/
VALUE *getValue(const char* key)
{
    PrefsData::iterator itr = _xmldata.find(key);

    if(itr != _xmldata.end())
    {
        return &itr->second->value;
    }

    return NULL;
}

/*-------------------------------------------------------------
loadKill
--------------------------------------------------------------*/
void loadKill()
{
    PrefsData::iterator itr;

    itr = _xmldata.begin();

    while(itr != _xmldata.end())
    {
        WWIIOLXML *data = itr->second;
        delete data;
        ++itr;
    }

    _xmldata.clear();
}

/*-------------------------------------------------------------
_parseXML
--------------------------------------------------------------*/
void _parseXML(xmlNode *root)
{
    xmlNode *node = root;

    for(; node != NULL; node = node->next)
    {
        if(node->type != XML_ELEMENT_NODE)
        {
            continue;
        }

        if(!xmlStrcmp((xmlChar*)"value", node->name) ||
           !xmlStrcmp((xmlChar*)"scalar", node->name))
        {
            xmlChar* name = xmlGetProp(node, (xmlChar*)"name");          
            xmlChar* type = xmlGetProp(node, (xmlChar*)"type");

            if(name && type)
            {
                PrefsData::iterator itr = _xmldata.find((char*)name);

                WWIIOLXML* data;

                // Loaded from default
                if(itr != _xmldata.end())
                {
                    data = itr->second;
                }
                else
                {
                    data = new WWIIOLXML;

                    data->name = (char*)name;
                    data->type = (char*)type;

                    _xmldata.insert(std::make_pair(data->name, data));
                }

                xmlChar* str = xmlGetProp(node, (xmlChar*)"value");

                if(!xmlStrcmp((xmlChar*)"bool", type))
                {
                    bool value = !xmlStrcmp((xmlChar*)"true", str);
                    data->value.set(value);
                    xmlFree(str);
                }
                else if(!xmlStrcmp((xmlChar*)"long", type))
                {
                    long value = atol((char*)str);
                    data->value.set(value);
                    xmlFree(str);
                }
                else if(!xmlStrcmp((xmlChar*)"string", type))
                {
                    data->value.set((char*)str);
                    xmlFree(str);
                }
                else if(!xmlStrcmp((xmlChar*)"float", type))
                {
                    float value = (float)atof((char*)str);
                    data->value.set(value);
                    xmlFree(str);
                }
                else if(!xmlStrcmp((xmlChar*)"double", type))
                {
                    double value = atof((char*)str);
                    data->value.set(value);
                    xmlFree(str);
                }
                else
                {
                    int stop = 0;
                }

                xmlFree(type);
                xmlFree(name);
            }
        }
    }
}

/*-------------------------------------------------------------
_loadDefault
--------------------------------------------------------------*/
bool _loadDefault()
{
	int language = 0;

    _message.set(0);

    WWIIOLXML *data = new WWIIOLXML;
    
    data->name = "config.authoritative";
    data->value.set(true);
    data->type = "bool";

    _xmldata.insert(std::make_pair(data->name , data));

    data = new WWIIOLXML;

    data->name = "app.video";
    data->value.set(getLoc(language, "settings.video.primary"));
    data->type = "string";

    _xmldata.insert(std::make_pair(data->name, data));

    data = new WWIIOLXML;

    data->name = "app.resolution";
    data->type = "string";

	// use our desktop resolution as the default
	DWORD desktopWidth	= GetSystemMetrics(SM_CXSCREEN);
	DWORD desktopHeight	= GetSystemMetrics(SM_CYSCREEN);
	
	wchar_t desktop[128];
	wsprintf(desktop, L"%dx%dx%d", (int)desktopWidth, (int)desktopHeight, 32);

    data->value.set(desktop);

    _xmldata.insert(std::make_pair(data->name, data));

    data = new WWIIOLXML;

    data->name = "app.language";
    data->type = "long";
    data->value.set(_bestLanguage());

    _xmldata.insert(std::make_pair(data->name, data));

    data = new WWIIOLXML;

    data->name = "app.usenetworkroute";
    data->value.set(0);
    data->type = "long";

    _xmldata.insert(std::make_pair(data->name, data));

    data = new WWIIOLXML;

    data->name = "app.sound";
    data->value.set(getLoc(language, "settings.sound.primary"));
    data->type = "string";

    _xmldata.insert(std::make_pair(data->name, data));

    data = new WWIIOLXML;

    data->name = "app.wantsse2";
    data->value.set(true);
    data->type = "bool";

    _xmldata.insert(std::make_pair(data->name, data));

    return true;
}

/*-------------------------------------------------------------
_loadXML
--------------------------------------------------------------*/
bool _loadXML(const wchar_t* filename)
{
	int file = _wopen(filename, O_RDONLY | O_BINARY);
	xmlDocPtr doc = xmlReadFd(file, NULL, NULL, XML_PARSE_RECOVER);

    if(!doc)
    {
        return false;
    }

    xmlNodePtr root = xmlDocGetRootElement(doc);

    if(!root)
    {
        return false;
    }

    if(!xmlStrcmp((xmlChar*)"message", root->name))
    {
        xmlChar* s = xmlGetProp(root, (xmlChar*)"what");
        
        if(s)
        {
            _message.set(atol((char*)s));
            xmlFree(s);
        }

        _parseXML(root->children);
		
		if(region() == 5)
        {
            VALUE* value = getValue("app.language");
            
            if(value)
            {
                // force chinese if region is china
			    value->set(5);
            }
        }
    }

    return true;
}

/*-------------------------------------------------------------
_insertCFGType:
--------------------------------------------------------------*/
void _insertCFGType(WWIIOLXML* data, int type, const char* value)
{
    switch(type)
    {
    case cfgTYPE_TRUE_FALSE:
        {
            bool b = !strncmp(value, "true", 4);
            data->value.set(b);

            _xmldata.insert(std::make_pair("bool", data));
            break;
        }
    case cfgTYPE_TEXTURE_SHADE_MODE:
    case cfgTYPE_STRING:
        {
            data->value.set(value);
            _xmldata.insert(std::make_pair("string", data));
            break;
        }
    case cfgTYPE_FLOAT:
        {
            float f = (float)atof(value);
            data->value.set(f);

            _xmldata.insert(std::make_pair("float", data));
            break;
        }
    case cfgTYPE_SHADOW_SIZE_SELECT:
    case cfgTYPE_INTEGER:
    case cfgTYPE_FSAA_SAMPLES:
        {
            long l = atol(value);
            data->value.set(l);

            _xmldata.insert(std::make_pair("long", data));
            break;
        }
    default:
        {
            break;
        }
    }
}

/*-------------------------------------------------------------
_parseCFG: All of this will be removed in the near future
--------------------------------------------------------------*/
void _parseCFG(FILE *fp)
{
    char line[80];
    char name[64] = {0};
    char value[64] = {0};
    
    int key = -1;

    char* p1 = NULL;

	int i = 0;

    // Insert our config authoritative, since we are going to now always save as XML
    WWIIOLXML* d = new WWIIOLXML;
    d->name = "config.authoritative";

    _insertCFGType(d, cfgTYPE_TRUE_FALSE, "true");

    _message.set(0);

    while(fgets(line, 79, fp) != NULL)
    {
        p1 = strtok(line, " ");
        strncpy(name, p1, 64);

        p1 = strtok(NULL, "\n");

        if(p1)
        {
            strncpy(value, p1, 64);
        }
        else
        {
            value[0] = 0;
        }

        key = -1;

        for(i = 0; i < cfgNUM_TOTAL_PARAMS; i++)
        {
            if(!strncmp(cfgParameters[i].Name, name, strlen(name)))
            {
                key = i;
                break;
            }
		}

        if(key == -1)
        {
            continue;
        }

        WWIIOLXML *data = new WWIIOLXML;

        data->name = "app.";
        data->name += name;

        _insertCFGType(data, cfgParameters[key].Type, value);
	}
}

/*-------------------------------------------------------------
_loadCFG
--------------------------------------------------------------*/
bool _loadCFG(const char* file)
{
    FILE *fp = fopen(file, "r");

    if(!fp)
    {
        return false;
    }

    _parseCFG(fp);
    
    fclose(fp);
    
    return true;
}

/*-------------------------------------------------------------
_saveAsXML
--------------------------------------------------------------*/
void _saveAsXML()
{
    xmlDocPtr doc = NULL;    
	xmlNodePtr node = NULL;

	doc = xmlNewDoc((xmlChar*)"1.0");
    node = xmlNewNode(NULL, (xmlChar*)"message");
    xmlDocSetRootElement(doc, node);
	
    PrefsData::iterator itr = _xmldata.begin();

    xmlNewProp(node, (xmlChar*)"what", (xmlChar*)_message.get());

    for(; itr != _xmldata.end(); ++itr)
    {
        WWIIOLXML *data = itr->second;

        xmlNodePtr child = xmlNewChild(node, NULL, (xmlChar*)"value", NULL);

        xmlNewProp(child, (xmlChar*)"name", (xmlChar*)data->name.c_str());
        xmlNewProp(child, (xmlChar*)"type", (xmlChar*)data->type.c_str());        
        xmlNewProp(child, (xmlChar*)"value", (xmlChar*)data->value.get());

        xmlAddChild(node, child);
    }

	xmlRegisterDefaultOutputCallbacks();

	xmlOutputBufferPtr buf = xmlAllocOutputBuffer(nullptr);
	if ( buf == nullptr )
		return; // ???

	wwiiolpath_string homepath = WW2_userDocumentsPath + L"\\" + GameSettingsFilename;
	
	buf->context = reinterpret_cast<void*>(fileOpenWrite(homepath.c_str()));
	buf->writecallback = (xmlOutputWriteCallback)fileWrite;
	buf->closecallback = (xmlOutputCloseCallback)fileClose;

	xmlSaveFormatFileTo(buf, doc, "UTF-8", 1);

	xmlFreeDoc(doc);
    xmlCleanupOutputCallbacks();
}

/*-------------------------------------------------------------
loadSave
--------------------------------------------------------------*/
void loadSave()
{
    _saveAsXML();
}

/*-------------------------------------------------------------
determines default language if one wasn't already set
--------------------------------------------------------------*/

int _bestLanguage()
{
	int language = 0;
	wchar_t name[LOCALE_NAME_MAX_LENGTH+1];

	if(GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SENGLANGUAGE, name, LOCALE_NAME_MAX_LENGTH) > 0)
	{
		if(_wcsicmp(name, L"Italian") == 0)
			language = 1;
		else if(_wcsicmp(name, L"French") == 0)
			language = 2;
		else if(_wcsicmp(name, L"German") == 0)
			language = 3;
		else if(_wcsicmp(name, L"Spanish") == 0)
			language = 4;
		else if(_wcsicmp(name, L"Chinese") == 0)
			language = 5;
		else if (_wcsicmp(name, L"Russian") == 0)
			language = 6;
	}

	if(region() == 5 && language != 5)
		language = 5;
	
	return language;
}
