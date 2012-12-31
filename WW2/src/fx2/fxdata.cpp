#include <syscfg.h> // Path is project specific.
//////////////////////////////////////////////////////////////////////
// Module: fxdata.cpp
// Description:
//////////////////////////////////////////////////////////////////////
#include "pi/pi.h"
#include "include/vars.h"
#include "libxml/parser.h"
#include "libxml/tree.h"
#include "fio/fio.h"
#include "ld/ldpix.h"
#include "ob/obmain.h"
#include "ob/obcd.h"
#include "ob/obfx.h"

#include <map>
#include <string>

#include "fx2/fxdata.h"

FxData _fxdata;

typedef std::map<std::string, EFFECT *> effectMap;
static effectMap _effect;

static SINT32 xmlinputmatch(const SCHAR* name) {return 1;}

//////////////////////////////////////////////////////////////////////
// Function: FxData
// Description:
//////////////////////////////////////////////////////////////////////
FxData::FxData()
{
}

//////////////////////////////////////////////////////////////////////
// Function: ~FxData
// Description:
//////////////////////////////////////////////////////////////////////
FxData::~FxData()
{
// destructor gets called after game exits main because of static
// allocation of _fxdata which causes a crash on shutdown
 //   kill();
}

//////////////////////////////////////////////////////////////////////
// Function: killeffects
// Description:
//////////////////////////////////////////////////////////////////////
void FxData::killeffects()
{
    if(_effect.empty())
    {
        return;
    }

    effectMap::iterator itr = _effect.begin();

    for(; itr != _effect.end(); ++itr)
    {
        itr->second->fx.clear();
    }

    _effect.clear();
    /*
    effectMap::iterator free;

    while(!_effect.empty())
    {
        free = _effect.begin();

        for(UINT32 i = 0; i < free->second->fx.size(); i++)
        {
            FX* fx = free->second->fx[i];
            delete fx;
        }

        free->second->fx.clear();
        delete free->second;

        _effect.erase(free);
    }
    */
}

//////////////////////////////////////////////////////////////////////
// Function: kill
// Description:
//////////////////////////////////////////////////////////////////////
void FxData::kill()
{
    if(piIsTexture(_particletex, 0))
    {
        piFreeMipTextures(_particletex);
    }

    if(piIsTexture(_bullettex, 0))
    {
        piFreeMipTextures(_bullettex);
    }

    killeffects();

    Write_Log(LOG_OLD, LOG_INFO, "[FxData::kill]");
}

//////////////////////////////////////////////////////////////////////
// Function: getuv
// Description:
//////////////////////////////////////////////////////////////////////
void FxData::getuv(FLOAT32& u1, FLOAT32& v1, FLOAT32& u2, FLOAT32& v2, SINT32 i)
{
    u1 = _uvcoords[i].x;
    v1 = _uvcoords[i].y;
    u2 = _uvcoords[i].z;
    v2 = _uvcoords[i].w;
}

//////////////////////////////////////////////////////////////////////
// Function: _uvelements
// Description:
//////////////////////////////////////////////////////////////////////
void _uvelements(xmlNodePtr root)
{
    xmlNodePtr node = root;

    SINT32 id = 0;
    piQUAT uv;

	for(; node != NULL; node = node->next)
	{
        if(node->type != XML_ELEMENT_NODE)
        {
            continue;
        }

        if(!xmlStrcmp((xmlChar*)"id", node->name))
        {
            xmlChar* s = xmlNodeGetContent(node);

            if(s)
            {
                id = atol((char*)s);
                xmlFree(s);
            }

            continue;
        }

        if(!xmlStrcmp((xmlChar*)"u1", node->name))
        {
            xmlChar* s = xmlNodeGetContent(node);

            if(s)
            {
                uv.x = (float)atof((char*)s);
                xmlFree(s);
            }

            continue;
        }

        if(!xmlStrcmp((xmlChar*)"v1", node->name))
        {
            xmlChar* s = xmlNodeGetContent(node);

            if(s)
            {
                uv.y = 1.0f - (float)atof((char*)s);
                xmlFree(s);
            }

            continue;
        }

        if(!xmlStrcmp((xmlChar*)"u2", node->name))
        {
            xmlChar* s = xmlNodeGetContent(node);

            if(s)
            {
                uv.z = (float)atof((char*)s);
                xmlFree(s);
            }

            continue;
        }

        if(!xmlStrcmp((xmlChar*)"v2", node->name))
        {
            xmlChar* s = xmlNodeGetContent(node);

            if(s)
            {
                uv.w = 1.0f - (float)atof((char*)s);
                xmlFree(s);
            }

            continue;
        }
    }

    _fxdata._uvcoords.push_back(uv);

    // IDs must be unique
    if(id != (SINT32)_fxdata._uvcoords.size() - 1)
    {
        Write_Log(LOG_OLD, LOG_INFO, "FX2: id is wrong!");

        volatile int x = 1;
        volatile int y = 0;

        x = x/y;
    }
}

//////////////////////////////////////////////////////////////////////
// Function: _uvtree
// Description:
//////////////////////////////////////////////////////////////////////
void _uvtree(xmlNodePtr root)
{
    xmlNodePtr node = root;

	for(; node != NULL; node = node->next)
	{
        if(node->type != XML_ELEMENT_NODE)
        {
            continue;
        }

        if(!xmlStrcmp((xmlChar*)"particle", node->name))
        {
            _uvelements(node->children);
        }
	}
}

//////////////////////////////////////////////////////////////////////
// Function: loaduvxml
// Description:
//////////////////////////////////////////////////////////////////////
void loaduvxml()
{
    xmlRegisterInputCallbacks(xmlinputmatch,
                             (xmlInputOpenCallback)fioOpen,
                             (xmlInputReadCallback)fioRead,
                             (xmlInputCloseCallback)fioClose);

    SCHAR* name = "fx/xml/fx_uv.xml";

    xmlDocPtr doc = xmlReadFile(name, NULL, XML_PARSE_DTDLOAD | XML_PARSE_RECOVER);

    if(doc)
    {
	    xmlNodePtr root = xmlDocGetRootElement(doc);

        if(!root)
        {
            return;
        }

        _fxdata._uvcoords.clear();

        if(!xmlStrcmp((xmlChar*)"FXUV", root->name))
        {
            _uvtree(root->children);
        }

	    xmlFreeDoc(doc);
    }

    xmlCleanupInputCallbacks();
}

//////////////////////////////////////////////////////////////////////
// Function: _effectelements2
// Description:
//////////////////////////////////////////////////////////////////////
void _effectelements2(xmlNodePtr root, FX* fx)
{
    xmlNodePtr node = root;

    for(; node != NULL; node = node->next)
	{
        if(node->type != XML_ELEMENT_NODE)
        {
            continue;
        }

        if(!xmlStrcmp((xmlChar*)"number", node->name))
        {
            xmlChar* s = xmlNodeGetContent(node);

            if(s)
            {
                SINT32 i = atol((char*)s);
                fx->number.push_back(i);
                xmlFree(s);
            }

            continue;
        }

        if(!xmlStrcmp((xmlChar*)"texture", node->name))
        {
            xmlChar* s = xmlNodeGetContent(node);

            if(s)
            {
                SINT32 i = atol((char*)s);
                fx->texture.push_back(i);
                xmlFree(s);
            }

            continue;
        }

        if(!xmlStrcmp((xmlChar*)"time", node->name))
        {
            xmlChar* s = xmlNodeGetContent(node);

            fx->particleflags |= part_has_time_stages;

            if(s)
            {
                SINT32 i = atol((char*)s);
                fx->time.push_back(i);
                xmlFree(s);
            }

            continue;
        }

        if(!xmlStrcmp((xmlChar*)"delaystart", node->name))
        {
            xmlChar* s = xmlNodeGetContent(node);

            if(s)
            {
                fx->delaystart = atol((char*)s);
                xmlFree(s);
            }

            continue;
        }

        if(!xmlStrcmp((xmlChar*)"delayeachparticle", node->name))
        {
            xmlChar* s = xmlNodeGetContent(node);

            if(s)
            {
                fx->delayeachparticle = atol((char*)s);
                xmlFree(s);
            }

            continue;
        }

        if(!xmlStrcmp((xmlChar*)"lifetime", node->name))
        {
            xmlChar* s = xmlNodeGetContent(node);

            if(s)
            {
                fx->lifetime = atol((char*)s);
                xmlFree(s);
            }

            continue;
        }

        if(!xmlStrcmp((xmlChar*)"numpersec", node->name))
        {
            xmlChar* s = xmlNodeGetContent(node);

            if(s)
            {
                fx->numpersec = atol((char*)s);
                xmlFree(s);
            }

            continue;
        }

        if(!xmlStrcmp((xmlChar*)"alpha", node->name))
        {
            xmlChar* s = xmlNodeGetContent(node);

            fx->particleflags |= part_has_alpha_stages;

            if(s)
            {
                FLOAT32 i = (FLOAT32)atof((char*)s);
                fx->alpha.push_back(i);
                xmlFree(s);
            }

            continue;
        }

        if(!xmlStrcmp((xmlChar*)"use", node->name))
        {
            xmlChar* s = xmlNodeGetContent(node);

            if(s)
            {
                std::string str = (SCHAR*)s;
                fx->use.push_back(str);
            }
        }

        if(!xmlStrcmp((xmlChar*)"attach", node->name))
        {
            xmlChar* s = xmlNodeGetContent(node);

            if(s)
            {
                std::string str = (SCHAR*)s;
                fx->attach.push_back(str);
            }
        }

        if(!xmlStrcmp((xmlChar*)"size", node->name))
        {
            xmlChar* s = xmlNodeGetContent(node);

            fx->particleflags |= part_has_size_stages;

            if(s)
            {
                FLOAT32 i = (FLOAT32)atof((char*)s);
                fx->size.push_back(i);
                xmlFree(s);
            }

            continue;
        }

        if(!xmlStrcmp((xmlChar*)"posvariance", node->name))
        {
            xmlChar* s = xmlNodeGetContent(node);

            fx->fxflags |= fx_has_posvariance;

            if(s)
            {
                sscanf((char*)s,
                       "%f %f %f %f %f %f",
                       &fx->posvariance[0],
                       &fx->posvariance[1],
                       &fx->posvariance[2],
                       &fx->posvariance[3],
                       &fx->posvariance[4],
                       &fx->posvariance[5]);

                xmlFree(s);
            }

            continue;
        }

        if(!xmlStrcmp((xmlChar*)"drag", node->name))
        {
            if(fx->isemitter)
            {
                fx->emitterflags |= emitter_has_drag;
            }
            else
            {
                fx->particleflags |= part_has_drag;
            }

            xmlChar* s = xmlNodeGetContent(node);

            if(s)
            {
                fx->dragcoef = (FLOAT32)atof((char*)s);
                xmlFree(s);
            }

            continue;
        }

        if(!xmlStrcmp((xmlChar*)"rotate", node->name))
        {
            fx->particleflags |= part_has_rotate;

            xmlChar* s = xmlNodeGetContent(node);

            if(s)
            {
                fx->rotate = (FLOAT32)atof((char*)s);
                xmlFree(s);
            }

            continue;
        }

        if(!xmlStrcmp((xmlChar*)"stretch", node->name))
        {
            fx->particleflags |= part_has_stretch;
            fx->stretchscale = 0.1f;

            xmlChar* s = xmlNodeGetContent(node);

            if(s)
            {
                if(s[0])
                {
                    fx->stretchscale = (FLOAT32)atof((char*)s);
                }

                xmlFree(s);
            }

            continue;
        }

        if(!xmlStrcmp((xmlChar*)"fixstretch", node->name))
        {
            fx->particleflags |= part_has_fixstretch;
            fx->stretchscale = 8.0f;

            xmlChar* s = xmlNodeGetContent(node);

            if(s)
            {
                fx->stretchscale = (FLOAT32)atof((char*)s);
                xmlFree(s);
            }
        }

        if(!xmlStrcmp((xmlChar*)"fountain", node->name))
        {
            fx->emitterflags |= emitter_is_fountain;

            xmlChar* s = xmlNodeGetContent(node);

            if(s)
            {
                fx->numpersec = atoi((char*)s);
                xmlFree(s);
            }
        }

        if(!xmlStrcmp((xmlChar*)"separation", node->name))
        {
            xmlChar* s = xmlNodeGetContent(node);

            if(s)
            {
                fx->separation = (FLOAT32)atof((char*)s);
                xmlFree(s);
            }
        }

        if(!xmlStrcmp((xmlChar*)"glow", node->name))
        {
            fx->particleflags |= part_has_glow;

            xmlChar* s = xmlNodeGetContent(node);

            if(s)
            {
                if(s[0])
                {
                    fx->glowtime = atol((char*)s);
                }

                xmlFree(s);
            }

            continue;
        }

        if(!xmlStrcmp((xmlChar*)"outdofdirection", node->name))
        {
            fx->fxflags |= fx_out_dofdirection;
            continue;
        }

        if(!xmlStrcmp((xmlChar*)"overridedirection", node->name))
        {
            xmlChar* s = xmlNodeGetContent(node);

            if(s)
            {
                fx->fxflags |= fx_overridedirection;

                sscanf((char*)s,
                       "%f %f %f",
                       &fx->overridedir.x,
                       &fx->overridedir.y,
                       &fx->overridedir.z);
            }

            continue;
        }

        if(!xmlStrcmp((xmlChar*)"stretchalways", node->name))
        {
            // No turning off stretch even if directly facing particle.
            // Good for water hits and such. Bad for other things.
            fx->particleflags |= part_always_stretches;
            continue;
        }

        if(!xmlStrcmp((xmlChar*)"randomdirection", node->name))
        {
            fx->fxflags |= fx_random_direction;

            xmlChar* s = xmlNodeGetContent(node);

            if(s)
            {
                if(s[0])
                {
                    fx->randomdirection = atol((char*)s);
                }

                xmlFree(s);
            }

            continue;
        }

        if(!xmlStrcmp((xmlChar*)"stopstretch", node->name))
        {
            xmlChar* s = xmlNodeGetContent(node);

            if(s)
            {
                if(s[0])
                {
                    fx->stopstretch = atol((char*)s);
                }

                xmlFree(s);
            }
        }

        if(!xmlStrcmp((xmlChar*)"alphadegradation", node->name))
        {
            fx->emitterflags |= emitter_has_alphadegradation;
            continue;
        }

        if(!xmlStrcmp((xmlChar*)"noflip", node->name))
        {
            fx->particleflags |= part_no_flip;
            continue;
        }

        if(!xmlStrcmp((xmlChar*)"gravity", node->name))
        {
            if(fx->isemitter)
            {
                fx->emitterflags |= emitter_has_gravity;
            }
            else
            {
                fx->particleflags |= part_has_gravity;
            }

            xmlChar* s = xmlNodeGetContent(node);

            if(s)
            {
                fx->gravitycoef = (FLOAT32)atof((char*)s);
                xmlFree(s);
            }

            continue;
        }

        if(!xmlStrcmp((xmlChar*)"speed", node->name))
        {
            xmlChar* s = xmlNodeGetContent(node);

            if(s)
            {
                fx->speed = (FLOAT32)atof((char*)s);
                xmlFree(s);
            }

            continue;
        }

        if(!xmlStrcmp((xmlChar*)"velvariance", node->name))
        {
            xmlChar* s = xmlNodeGetContent(node);

            fx->fxflags |= fx_has_velvariance;
            fx->particleflags |= part_has_velvariance;

            if(s)
            {
                sscanf((char*)s,
                       "%f %f %f %f %f %f",
                       &fx->velvariance[0],  //xmin
                       &fx->velvariance[1],  //ymin
                       &fx->velvariance[2],  //zmin
                       &fx->velvariance[3],  //xmax
                       &fx->velvariance[4],  //ymax
                       &fx->velvariance[5]); //zmax

                xmlFree(s);
            }

            continue;
        }

        if(!xmlStrcmp((xmlChar*)"velreduction", node->name))
        {
            xmlChar* s = xmlNodeGetContent(node);

            fx->fxflags |= fx_has_velreduction;

            if(s)
            {
                fx->velreduction = (FLOAT32)atof((char*)s);
                xmlFree(s);
            }

            continue;
        }

        if(!xmlStrcmp((xmlChar*)"alphastart", node->name))
        {
            xmlChar* s = xmlNodeGetContent(node);

            if(s)
            {
                fx->alpha.push_back((FLOAT32)atof((char*)s));
                xmlFree(s);
            }

            continue;
        }

        if(!xmlStrcmp((xmlChar*)"alphaend", node->name))
        {
            xmlChar* s = xmlNodeGetContent(node);

            if(s)
            {
                fx->alpha.push_back((FLOAT32)atof((char*)s));
                xmlFree(s);
            }

            continue;
        }

        if(!xmlStrcmp((xmlChar*)"sizestart", node->name))
        {
            xmlChar* s = xmlNodeGetContent(node);

            if(s)
            {
                fx->size.push_back((FLOAT32)atof((char*)s));
                xmlFree(s);
            }

            continue;
        }

        if(!xmlStrcmp((xmlChar*)"sizeend", node->name))
        {
            xmlChar* s = xmlNodeGetContent(node);

            if(s)
            {
                fx->size.push_back((FLOAT32)atof((char*)s));
                xmlFree(s);
            }

            continue;
        }

        if(!xmlStrcmp((xmlChar*)"material", node->name))
        {
            xmlChar* s = xmlNodeGetContent(node);

            if(s)
            {
                fx->matid = piFindMaterialName((char*)s);
                xmlFree(s);
            }

            continue;
        }
    }
}

//////////////////////////////////////////////////////////////////////
// Function: _effectelements
// Description:
//////////////////////////////////////////////////////////////////////
void _effectelements(xmlNodePtr root, EFFECT* effect)
{
    xmlNodePtr node = root;

//	SINT32 id = 0;
//	SINT32 effectcount = 1;

//	FX* fx = NULL;

	for(; node != NULL; node = node->next)
	{
        if(node->type != XML_ELEMENT_NODE)
        {
            continue;
        }

        if(!xmlStrcmp((xmlChar*)"fxparticle", node->name))
        {
            FX* fx = new FX;

            effect->fx.push_back(fx);

            _effectelements2(node->children, fx);

            continue;
        }

        if(!xmlStrcmp((xmlChar*)"fxemitter", node->name))
        {
            FX* fx = new FX;
            fx->isemitter = true;

            effect->fx.push_back(fx);

            _effectelements2(node->children, fx);

            continue;
        }
    }
}

//////////////////////////////////////////////////////////////////////
// Function: _effectstree
// Description:
//////////////////////////////////////////////////////////////////////
void _effectstree(xmlNodePtr root)
{
    xmlNodePtr node = root;

	for(; node != NULL; node = node->next)
	{
        if(node->type != XML_ELEMENT_NODE)
        {
            continue;
        }

        if(!xmlStrcmp((xmlChar*)"effect", node->name))
        {
            xmlChar* s = xmlGetProp(node, (xmlChar*)"name");

            if(s)
            {
                EFFECT* effect = new EFFECT;

                effect->name = (SCHAR*)s;

                _effect[effect->name] = effect;

                _effectelements(node->children, effect);

                xmlFree(s);
            }
        }
	}
}

//////////////////////////////////////////////////////////////////////
// Function: loadeffectsxml
// Description:
//////////////////////////////////////////////////////////////////////
void loadeffectsxml()
{
    xmlRegisterInputCallbacks(xmlinputmatch,
                             (xmlInputOpenCallback)fioOpen,
                             (xmlInputReadCallback)fioRead,
                             (xmlInputCloseCallback)fioClose);

    SCHAR* name = "fx/xml/effects.xml";

    xmlDocPtr doc = xmlReadFile(name, NULL, XML_PARSE_DTDLOAD | XML_PARSE_RECOVER);

    if(doc)
    {
	    xmlNodePtr root = xmlDocGetRootElement(doc);

        if(!root)
        {
            return;
        }

        if(!xmlStrcmp((xmlChar*)"FXEFFECTS", root->name))
        {
            _effectstree(root->children);
        }

	    xmlFreeDoc(doc);
    }

    xmlCleanupInputCallbacks();
}

//////////////////////////////////////////////////////////////////////
// Function: init
// Description:
//////////////////////////////////////////////////////////////////////
void FxData::init()
{
    // UV coords to fx2.dds
    loaduvxml();
    // Effects xml
    loadeffectsxml();

    LPMATERIAL m;
    piGetMaterials(&m);

    _smokematerial = piFindMaterialName("smoke");

    if(!piIsMaterial(_smokematerial))
    {
        _smokematerial = piFindMaterialName("forest1");
    }

    _firematerial = piFindMaterialName("fire");

    if(!piIsMaterial(_firematerial))
    {
        _firematerial = piFindMaterialName("forest1");
    }

    _watermaterial = piFindMaterialName("dkwater1");

    if(!piIsMaterial(_watermaterial))
    {
        _watermaterial = piFindMaterialName("forest1");
    }

    _particletex = ldpixLoadImageFileToSurface("fx\\fx2.dds", 0, LDPIX_NO_FLIP);
    //_bullettex = ldpixLoadImageFileToSurface("fx\\bullet.dds", 0, LDPIX_NO_FLIP);

    _wind.x = -0.1f;
    _wind.y = -0.1f;
    _wind.z = 0.02f;
}

//////////////////////////////////////////////////////////////////////
// Function: update
// Description:
//////////////////////////////////////////////////////////////////////
void FxData::update()
{
#ifdef BETA_RELEASE
    // For debugging only...reload file if it has changed.
    static SINT32 m1 = 0;
    static SINT32 m2 = 0;

    static bool first = true;

    if(first)
    {
        fioModified(&m1, "fx/xml/fx_uv.xml");
        first = FALSE;
    }

    fioModified(&m2, "fx/xml/fx_uv.xml");

    if(m1 < m2)
    {
        m1 = m2;
        loaduvxml();
    }

    static SINT32 m3 = 0;
    static SINT32 m4 = 0;

    static bool first2 = true;

    if(first2)
    {
        fioModified(&m3, "fx/xml/effects.xml");
        first2 = FALSE;
    }

    fioModified(&m4, "fx/xml/effects.xml");

    if(m3 < m4)
    {
        m3 = m4;
        killeffects();
        loadeffectsxml();
    }
#endif // BETA_RELEASE
}

//////////////////////////////////////////////////////////////////////
// Function: geteffect
// Description:
//////////////////////////////////////////////////////////////////////
EFFECT* FxData::geteffect(const SCHAR* name)
{
    effectMap::iterator find;

    find = _effect.find(name);

    if(find != _effect.end())
    {
        return find->second;
    }

    return NULL;
}