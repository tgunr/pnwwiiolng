#include <syscfg.h> // Path is project specific.
//////////////////////////////////////////////////////////////////////
// Module: trndata.cpp
// Description:
//////////////////////////////////////////////////////////////////////
#include "libxml/parser.h"
#include "libxml/tree.h"
#include "fio/fio.h"
#include "pi/pi.h"
#include "trn/trncloud.h"

#ifdef BETA_RELEASE
#include "rn/rnmain.h"
#endif

#include <vector>
#include <string>

#include "trn/trndata.h"

static std::vector<TERRAINSKY *> _lightSettings;
static std::vector<TERRAINFOG *> _fogSettings;

static SINT32 xmlinputmatch(const SCHAR* name) {return 1;}


//////////////////////////////////////////////////////////////////////
// Function: _getcolor
// Description:
//////////////////////////////////////////////////////////////////////
static void _getcolor(xmlNodePtr root, piCOLORVALUE* c)
{
    xmlNodePtr node = root;

	for(; node != NULL; node = node->next)
	{
        if(node->type != XML_ELEMENT_NODE)
        {
            continue;
        }

        if(!xmlStrcmp((xmlChar*)"red", node->name))
        {
            xmlChar* s = xmlNodeGetContent(node);

            if(s)
            {
                c->r = (FLOAT32)atof((char*)s);
                xmlFree(s);
            }

            continue;
        }

        if(!xmlStrcmp((xmlChar*)"green", node->name))
        {
            xmlChar* s = xmlNodeGetContent(node);

            if(s)
            {
                c->g = (FLOAT32)atof((char*)s);
                xmlFree(s);
            }

            continue;
        }

        if(!xmlStrcmp((xmlChar*)"blue", node->name))
        {
            xmlChar* s = xmlNodeGetContent(node);

            if(s)
            {
                c->b = (FLOAT32)atof((char*)s);
                xmlFree(s);
            }

            continue;
        }
    }
}

//////////////////////////////////////////////////////////////////////
// Function: _terrainsky
// Description:
//////////////////////////////////////////////////////////////////////
static void _terrainsky(xmlNodePtr root, TERRAINSKY* s)
{
    xmlNodePtr node = root;

	for(; node != NULL; node = node->next)
	{
        if(node->type != XML_ELEMENT_NODE)
        {
            continue;
        }

        if(!xmlStrcmp((xmlChar*)"scale", node->name))
        {
            xmlChar* str = xmlNodeGetContent(node);

            if(str)
            {
                s->scale = (FLOAT32)atof((char*)str);
                xmlFree(str);
            }

            continue;
        }

        if(!xmlStrcmp((xmlChar*)"sky_color", node->name))
        {
            _getcolor(node->children, &s->_skycolor);
            continue;
        }

        if(!xmlStrcmp((xmlChar*)"sky_color_night", node->name))
        {
            _getcolor(node->children, &s->_skycolornight);
            continue;
        }

        if(!xmlStrcmp((xmlChar*)"sky_clear_ambient", node->name))
        {
            _getcolor(node->children, &s->_skyclearambient);
            continue;
        }

        if(!xmlStrcmp((xmlChar*)"sky_night_ambient", node->name))
        {
            _getcolor(node->children, &s->_skynightambient);
            continue;
        }

        if(!xmlStrcmp((xmlChar*)"sun_noon", node->name))
        {
            _getcolor(node->children, &s->_sunnoon);
            continue;
        }

        if(!xmlStrcmp((xmlChar*)"sun_itself_noon", node->name))
        {
            _getcolor(node->children, &s->_sunitselfnoon);
            continue;
        }

        if(!xmlStrcmp((xmlChar*)"sun_night", node->name))
        {
            _getcolor(node->children, &s->_sunnight);
            continue;
        }

        if(!xmlStrcmp((xmlChar*)"sunset", node->name))
        {
            _getcolor(node->children, &s->_sunset);
            continue;
        }

        if(!xmlStrcmp((xmlChar*)"horizon", node->name))
        {
            _getcolor(node->children, &s->_horizon);
            continue;
        }

        if(!xmlStrcmp((xmlChar*)"moon", node->name))
        {
            _getcolor(node->children, &s->_moon);
            continue;
        }

        if(!xmlStrcmp((xmlChar*)"moon_day", node->name))
        {
            _getcolor(node->children, &s->_moonday);
            continue;
        }
    }
}

//////////////////////////////////////////////////////////////////////
// Function: _getfogoptions
// Description:
//////////////////////////////////////////////////////////////////////
static void _getfogoptions(xmlNodePtr root, FOGOPTION* option)
{
    xmlNodePtr node = root;

	for(; node != NULL; node = node->next)
	{
        if(node->type != XML_ELEMENT_NODE)
        {
            continue;
        }

        if(!xmlStrcmp((xmlChar*)"fog", node->name))
        {
            xmlChar* s = xmlNodeGetContent(node);

            if(s)
            {
                FLOAT32 start, end;
                SINT32 time;

                sscanf((char*)s, "%d %f %f", &time, &start, &end);
                xmlFree(s);

                SINT32 hour = time / 100;
                SINT32 minute = time % 100;

                time = hour * 3600 + minute * 60;

                option->start.push_back(start);
                option->end.push_back(end);
                option->time.push_back(time);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////
// Function: _terrainfog
// Description:
//////////////////////////////////////////////////////////////////////
static void _terrainfog(xmlNodePtr root, TERRAINFOG* f)
{
    xmlNodePtr node = root;

	for(; node != NULL; node = node->next)
	{
        if(node->type != XML_ELEMENT_NODE)
        {
            continue;
        }

        if(!xmlStrcmp((xmlChar*)"Option", node->name))
        {
            FOGOPTION* option = new FOGOPTION;

            _getfogoptions(node->children, option);

            f->options.push_back(option);
            option->fogoption = (SINT32)f->options.size();
        }
    }
}

//////////////////////////////////////////////////////////////////////
// Function: ScaleColor
// Description:
//////////////////////////////////////////////////////////////////////
static void ScaleColor(piCOLORVALUE& c, float scale)
{
    c.r *= scale;
    c.g *= scale;
    c.b *= scale;
}

//////////////////////////////////////////////////////////////////////
// Function: _lightproperties
// Description:
//////////////////////////////////////////////////////////////////////
static void _lightproperties(xmlNodePtr root)
{
    xmlNodePtr node = root;

	for(; node != NULL; node = node->next)
	{
        if(node->type != XML_ELEMENT_NODE)
        {
            continue;
        }

        if(!xmlStrcmp((xmlChar*)"TerrainSky", node->name))
        {
            TERRAINSKY* s = new TERRAINSKY;

            _terrainsky(node->children, s);

            _lightSettings.push_back(s);
            s->setting = (SINT32)_lightSettings.size();

            if(s->scale != 1.0f)
            {
                ScaleColor(s->_horizon, s->scale);
                ScaleColor(s->_moon, s->scale);
                ScaleColor(s->_moonday, s->scale);
                ScaleColor(s->_skyclearambient, s->scale);
                ScaleColor(s->_skycolor, s->scale);
                ScaleColor(s->_skycolornight, s->scale);
                ScaleColor(s->_skynightambient, s->scale);
                ScaleColor(s->_sunitselfnoon, s->scale);
                ScaleColor(s->_sunnight, s->scale);
                ScaleColor(s->_sunnoon, s->scale);
                ScaleColor(s->_sunset, s->scale);
            }
        }
    }
}


//////////////////////////////////////////////////////////////////////
// Function: _fogproperties
// Description:
//////////////////////////////////////////////////////////////////////
static void _fogproperties(xmlNodePtr root)
{
    xmlNodePtr node = root;

	for(; node != NULL; node = node->next)
	{
        if(node->type != XML_ELEMENT_NODE)
        {
            continue;
        }

        if(!xmlStrcmp((xmlChar*)"TerrainFog", node->name))
        {
            TERRAINFOG* c = new TERRAINFOG;

            _terrainfog(node->children, c);

            _fogSettings.push_back(c);
            c->setting = (SINT32)_fogSettings.size();
        }
    }
}

//////////////////////////////////////////////////////////////////////
// Function: _loadLightData
// Description:
//////////////////////////////////////////////////////////////////////
static void _loadLightData()
{
    xmlRegisterInputCallbacks(xmlinputmatch,
                             (xmlInputOpenCallback)fioOpen,
                             (xmlInputReadCallback)fioRead,
                             (xmlInputCloseCallback)fioClose);

    SCHAR* name = "terrain/xml/light.xml";

    xmlDocPtr doc = xmlReadFile(name, NULL, XML_PARSE_DTDLOAD | XML_PARSE_RECOVER);

    if(doc)
    {
	    xmlNodePtr root = xmlDocGetRootElement(doc);

        if(!root)
        {
            Write_Log(LOG_OLD, LOG_INFO, "*** %s FAILED ***", name);
            return;
        }

        if(!xmlStrcmp((xmlChar*)"lightdata", root->name))
        {
            _lightproperties(root->children);
        }

	    xmlFreeDoc(doc);
    }

    xmlCleanupInputCallbacks();
}


//////////////////////////////////////////////////////////////////////
// Function: _loadFogData
// Description:
//////////////////////////////////////////////////////////////////////
static void _loadFogData()
{
    xmlRegisterInputCallbacks(xmlinputmatch,
                             (xmlInputOpenCallback)fioOpen,
                             (xmlInputReadCallback)fioRead,
                             (xmlInputCloseCallback)fioClose);

    SCHAR* name = "terrain/xml/fog.xml";

    xmlDocPtr doc = xmlReadFile(name, NULL, XML_PARSE_DTDLOAD | XML_PARSE_RECOVER);

    if(doc)
    {
	    xmlNodePtr root = xmlDocGetRootElement(doc);

        if(!root)
        {
            Write_Log(LOG_OLD, LOG_INFO, "*** %s FAILED ***", name);
            return;
        }

        if(!xmlStrcmp((xmlChar*)"fogdata", root->name))
        {
            _fogproperties(root->children);
        }

	    xmlFreeDoc(doc);
    }

    xmlCleanupInputCallbacks();
}

//////////////////////////////////////////////////////////////////////
// Function: trndataInit
// Description:
//////////////////////////////////////////////////////////////////////
void trndataInit()
{
    _loadLightData();
    _loadFogData();
}

//////////////////////////////////////////////////////////////////////
// Function: _killFog
// Description:
//////////////////////////////////////////////////////////////////////
void _killFog()
{
    for(UINT32 i = 0; i < _fogSettings.size(); i++)
    {
        TERRAINFOG* f = _fogSettings[i];

        for(UINT32 j = 0; j < f->options.size(); j++)
        {
            FOGOPTION* option = f->options[j];

            option->end.clear();
            option->start.clear();
            option->time.clear();

            delete option;
        }

        f->options.clear();

        delete f;
    }

    _fogSettings.clear();
}

//////////////////////////////////////////////////////////////////////
// Function: _killLight
// Description:
//////////////////////////////////////////////////////////////////////
void _killLight()
{
    for(UINT32 i = 0; i < _lightSettings.size(); i++)
    {
        TERRAINSKY* s = _lightSettings[i];
        delete s;
    }

    _lightSettings.clear();
}


//////////////////////////////////////////////////////////////////////
// Function: trndataKill
// Description:
//////////////////////////////////////////////////////////////////////
void trndataKill()
{
    _killLight();
    _killFog();

    Write_Log(LOG_OLD, LOG_INFO, "[trndataKill]");
}

//////////////////////////////////////////////////////////////////////
// Function: trndataGetSkyData
// Description:
//////////////////////////////////////////////////////////////////////
TERRAINSKY* trndataGetSkyData(SINT32 i)
{
    if(_lightSettings.empty())
    {
        return NULL;
    }

    if(i < 0 || i >= (SINT32)_lightSettings.size())
    {
        // return something
        return _lightSettings[0];
    }

    return _lightSettings[i];
}


//////////////////////////////////////////////////////////////////////
// Function: trndataGetFogData
// Description:
//////////////////////////////////////////////////////////////////////
FOGOPTION* trndataGetFogData(SINT32 i, SINT32 option)
{
    if(_fogSettings.empty())
    {
        return NULL;
    }

    if(i < 0 || i >= (SINT32)_fogSettings.size())
    {
        // return something
        return _fogSettings[0]->options[0];
    }

    if(option < 0 || option >= (SINT32)_fogSettings[i]->options.size())
    {
        // return something
        return _fogSettings[i]->options[0];
    }

    return _fogSettings[i]->options[option];
}

//////////////////////////////////////////////////////////////////////
// Function: trndataPrint
// Description:
//////////////////////////////////////////////////////////////////////
void trndataPrint()
{
#ifdef BETA_RELEASE
    rnmainListText("weather <1 - %d>", _lightSettings.size());

    for(UINT32 i = 0; i < _fogSettings.size(); i++)
    {
        rnmainListText("For setting %d: fog <1 - %d>", i + 1, _fogSettings[i]->options.size());
    }
#endif
}

//////////////////////////////////////////////////////////////////////
// Function: trndataUpdate
// Description:
//////////////////////////////////////////////////////////////////////
void trndataUpdate()
{
#ifdef BETA_RELEASE
    // This is for debugging only...to reload the file when testing
    // different fog values during beta release.
    static SINT32 fog1 = 0;
    static SINT32 fog2 = 0;

    // Initialize first run for fog1 time value
    if(!fog1)
    {
        fioModified(&fog1, "terrain/xml/fog.xml");
    }

    fioModified(&fog2, "terrain/xml/fog.xml");

    // Reload if file time has changed.
    if(fog1 < fog2)
    {
        fog1 = fog2;
        _killFog();
        _loadFogData();
    }

    static SINT32 light1 = 0;
    static SINT32 light2 = 0;

    // Initialize first run for fog1 time value
    if(!light1)
    {
        fioModified(&light1, "terrain/xml/light.xml");
    }

    fioModified(&light2, "terrain/xml/light.xml");

    // Reload if file time has changed.
    if(light1 < light2)
    {
        light1 = light2;
        _killLight();
        _loadLightData();
    }
#endif
}
