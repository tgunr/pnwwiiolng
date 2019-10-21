#include "syscfg.h" // Path is project specific.
/*-----------------------------------------------------------------------
Module: osgfxdata.cpp
Description:
-----------------------------------------------------------------------*/
#include "libxml/parser.h"
#include "libxml/tree.h"
#include "fio/fio.h"
#include "log/log.h"

#include <map>
#include <string>
#include <queue>

#include "fx/DataFx.h"
#include "ld/ldtexture.h"
#include "ld/ldpix.h"

#define max_particle_effects 512
#define max_emitter_effects 512
#define max_transform_effects 512

static DataFx* _datafx;

static std::map<std::string, TransformFx*> _transformeffects;
static std::map<std::string, ParticleEffect*> _particleeffects;
static std::map<std::string, int> _materials;

static ParticleEffect* ParticleEffectChunk = nullptr;
static Emitterfx* EmitterFxChunk = nullptr;
static TransformFx* TransformFxChunk = nullptr;

static std::deque<ParticleEffect*> _freeparticleeffects;
static std::deque<Emitterfx*> _freeemittereffects;
static std::deque<TransformFx*> _freetransformeffects;

static int xmlinputmatch(const char*name) {return 1;}
static std::map<std::string, int> uvStringToId;
static int texid = 0;

/*-----------------------------------------------------------------------
Function: FxGet
Description:
-----------------------------------------------------------------------*/
DataFx& FxGet()
{
	return *_datafx;
}

/*-----------------------------------------------------------------------
Function: DataFxInit
Description:
-----------------------------------------------------------------------*/
void DataFxInit()
{
    ParticleEffectChunk = new ParticleEffect[max_particle_effects];
    EmitterFxChunk = new Emitterfx[max_emitter_effects];
    TransformFxChunk = new TransformFx[max_transform_effects];

    for(int i = 0; i < max_particle_effects; i++)
    {
        _freeparticleeffects.push_back(&ParticleEffectChunk[i]);
    }

    for(int i = 0; i < max_emitter_effects; i++)
    {
        _freeemittereffects.push_back(&EmitterFxChunk[i]);
    }

    for(int i = 0; i < max_transform_effects; i++)
    {
        _freetransformeffects.push_back(&TransformFxChunk[i]);
    }

	_datafx = new DataFx;
    _datafx->init();
}

/*-----------------------------------------------------------------------
Function: DataFxKill
Description:
-----------------------------------------------------------------------*/
void DataFxKill()
{
    _datafx->killeffects();

    delete [] ParticleEffectChunk;
    delete [] EmitterFxChunk;
    delete [] TransformFxChunk;

    delete _datafx;
}

/*-----------------------------------------------------------------------
Function: DataFx
Description:
-----------------------------------------------------------------------*/
DataFx::DataFx()
{
}

/*-----------------------------------------------------------------------
Function: ~DataFx
Description:
-----------------------------------------------------------------------*/
DataFx::~DataFx()
{
}

/*-----------------------------------------------------------------------
Function: killeffects
Description:
-----------------------------------------------------------------------*/
void DataFx::killeffects()
{
    std::map<std::string, TransformFx*>::iterator itr = _transformeffects.begin();

    for(; itr != _transformeffects.end(); ++itr)
    {
        for(size_t i = 0; i < itr->second->_emitters.size(); i++)
        {
            releaseemittereffect(itr->second->_emitters[i]);
        }

        releasetransformeffect(itr->second);
    }

    _transformeffects.clear();

    std::map<std::string, ParticleEffect*>::iterator itr2 = _particleeffects.begin();

    for(; itr2 != _particleeffects.end(); ++itr2)
    {
        releaseparticleeffect(itr2->second);
    }

    _particleeffects.clear();
}

/*-----------------------------------------------------------------------
Function: getuv
Description:
-----------------------------------------------------------------------*/
void DataFx::getuv(Vec4f& uv, int i)
{
    uv = _uvcoords[i];
}

/*-----------------------------------------------------------------------
Function: getTextureId
Description:
-----------------------------------------------------------------------*/
int DataFx::getTextureId(const char* name)
{
    std::map<std::string, int>::iterator itr = uvStringToId.find(name);

    if(itr != uvStringToId.end())
    {
        return itr->second;
    }

    return -1;
}

/*-----------------------------------------------------------------------
Function: getTextureId
Description:
-----------------------------------------------------------------------*/
int DataFx::getMaterialId(const char* name)
{
    if(_materials.find(name) == _materials.end())
    {
        return oglFindMaterialName(name);
    }

    return _materials.find(name)->second;
}

/*-----------------------------------------------------------------------
Function: getuv
Description:
-----------------------------------------------------------------------*/
bool DataFx::getuv(Vec4f& uv, const char* str)
{
    std::map<std::string, int>::iterator itr = uvStringToId.find(str);

    if(itr == uvStringToId.end())
    {
        return false;
    }

    uv = _uvcoords[itr->second];

    return true;
}

/*-----------------------------------------------------------------------
Function: _parseuvelements
Description:
-----------------------------------------------------------------------*/
void _parseuvelements(xmlNodePtr root, const char* name)
{
    xmlNodePtr node = root;

    Vec4d uv;

    uvStringToId[name] = texid++;

	for(; node != nullptr; node = node->next)
	{
        if(node->type != XML_ELEMENT_NODE)
        {
            continue;
        }

        if(!xmlStrcmp((xmlChar*)"u1", node->name))
        {
            xmlChar* s = xmlNodeGetContent(node);

            if(s)
            {
                uv.x() = (float)atof((char*)s);
                xmlFree(s);
            }

            continue;
        }

        if(!xmlStrcmp((xmlChar*)"v1", node->name))
        {
            xmlChar* s = xmlNodeGetContent(node);

            if(s)
            {
                uv.y() = 1.0f - (float)atof((char*)s);
                xmlFree(s);
            }

            continue;
        }

        if(!xmlStrcmp((xmlChar*)"u2", node->name))
        {
            xmlChar* s = xmlNodeGetContent(node);

            if(s)
            {
                uv.z() = (float)atof((char*)s);
                xmlFree(s);
            }

            continue;
        }

        if(!xmlStrcmp((xmlChar*)"v2", node->name))
        {
            xmlChar* s = xmlNodeGetContent(node);

            if(s)
            {
                uv.w() = 1.0f - (float)atof((char*)s);
                xmlFree(s);
            }

            continue;
        }
    }

    // texid for string name (uvStringToId) matches this size
    _datafx->_uvcoords.push_back(uv);
}

/*-----------------------------------------------------------------------
Function: _parseuvtree
Description:
-----------------------------------------------------------------------*/
void _parseuvtree(xmlNodePtr root)
{
    xmlNodePtr node = root;

	for(; node != nullptr; node = node->next)
	{
        if(node->type != XML_ELEMENT_NODE)
        {
            continue;
        }

        if(!xmlStrcmp((xmlChar*)"particle", node->name))
        {
            xmlChar* s = xmlGetProp(node, (xmlChar*)"name");

            if(s)
            {
                _parseuvelements(node->children, (char*)s);
                xmlFree(s);
            }
        }
	}
}

/*-----------------------------------------------------------------------
Function: loadTextureUVs
Description:
-----------------------------------------------------------------------*/
void loadTextureUVs()
{
    xmlRegisterInputCallbacks(xmlinputmatch,
                             (xmlInputOpenCallback)fioOpen,
                             (xmlInputReadCallback)fioRead,
                             (xmlInputCloseCallback)fioClose);

    const char* name = "fx/xml/fx_uv2.xml";

    uvStringToId.clear();
    texid = 0;

    xmlDocPtr doc = xmlReadFile(name, nullptr, XML_PARSE_DTDLOAD | XML_PARSE_RECOVER);

    if(doc)
    {
	    xmlNodePtr root = xmlDocGetRootElement(doc);

        if(!root)
        {
            return;
        }

        _datafx->_uvcoords.clear();

        if(!xmlStrcmp((xmlChar*)"FXUV", root->name))
        {
            _parseuvtree(root->children);
        }

	    xmlFreeDoc(doc);
    }

    xmlCleanupInputCallbacks();
}

/*-----------------------------------------------------------------------
Function: ParseParticleElements
Description:
-----------------------------------------------------------------------*/
void ParseParticleElements(xmlNodePtr root, ParticleEffect* fx)
{
    xmlNodePtr node = root;

    for(; node != nullptr; node = node->next)
	{
        if(node->type != XML_ELEMENT_NODE)
        {
            continue;
        }

        // Some particles emit particles too
        if(!xmlStrcmp((xmlChar*)"transformeffect", node->name))
        {
            // Attach a transform effect to this particle
            xmlChar* s = xmlNodeGetContent(node);

            if(s)
            {
                fx->attachedtransforms.push_back((char*)s);
                xmlFree(s);
            }
        }

        if(!xmlStrcmp((xmlChar*)"number", node->name))
        {
            xmlChar* s = xmlNodeGetContent(node);

            if(s)
            {
                int i = atol((char*)s);
                fx->number.push_back(i);
                xmlFree(s);
            }

            continue;
        }

        if(!xmlStrcmp((xmlChar*)"animate", node->name))
        {
            xmlChar* s = xmlNodeGetContent(node);

            if(s)
            {
                // Start and end id in fx_uv.xml
                int start = 0, end = 0;
                sscanf((char*)s, "%d %d", &start, &end);

                fx->texture.push_back(start);
                fx->texture.push_back(end);

                xmlFree(s);
            }

            continue;
        }

        if(!xmlStrcmp((xmlChar*)"texture", node->name))
        {
            xmlChar* s = xmlNodeGetContent(node);

            if(s)
            {
                std::map<std::string, int>::iterator itr = uvStringToId.find((char*)s);

                if(itr == uvStringToId.end())
                {
                    piExitProgram(_FL, "Bad Texture Name");
                }

                fx->texture.push_back(itr->second);
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
                int i = atol((char*)s);
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

        if(!xmlStrcmp((xmlChar*)"alpha", node->name))
        {
            xmlChar* s = xmlNodeGetContent(node);

            fx->particleflags |= part_has_alpha_stages;

            if(s)
            {
                float i = (float)atof((char*)s);
                fx->alpha.push_back(i);
                xmlFree(s);
            }

            continue;
        }

        if(!xmlStrcmp((xmlChar*)"color", node->name))
        {
            xmlChar* s = xmlNodeGetContent(node);

            fx->particleflags |= part_has_color_stages;

            if(s)
            {
                float r, g, b;
                sscanf((char*)s, "%f %f %f", &r, &g, &b);
                fx->color.push_back(Vec3d(r, g, b));
                xmlFree(s);
            }

            continue;
        }

        if(!xmlStrcmp((xmlChar*)"colorstart", node->name))
        {
            xmlChar* s = xmlNodeGetContent(node);

            if(s)
            {
                float r, g, b;
                sscanf((char*)s, "%f %f %f", &r, &g, &b);
                fx->color.push_back(Vec3d(r, g, b));
                xmlFree(s);
            }

            continue;
        }

        if(!xmlStrcmp((xmlChar*)"colorend", node->name))
        {
            xmlChar* s = xmlNodeGetContent(node);

            if(s)
            {
                float r, g, b;
                sscanf((char*)s, "%f %f %f", &r, &g, &b);
                fx->color.push_back(Vec3d(r, g, b));
                xmlFree(s);
            }

            continue;
        }

        if(!xmlStrcmp((xmlChar*)"size", node->name))
        {
            xmlChar* s = xmlNodeGetContent(node);

            fx->particleflags |= part_has_size_stages;

            if(s)
            {
                float i = (float)atof((char*)s);
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
                       &fx->posvariance[0], //x min
                       &fx->posvariance[1], //y min
                       &fx->posvariance[2], //z min
                       &fx->posvariance[3], //x max
                       &fx->posvariance[4], //y max
                       &fx->posvariance[5]);//z max

                // Back-ass-wards coordinate system
                fx->posvariance[2] = -fx->posvariance[2];
                fx->posvariance[5] = -fx->posvariance[5];

                xmlFree(s);
            }

            continue;
        }

        if(!xmlStrcmp((xmlChar*)"drag", node->name))
        {
            fx->particleflags |= part_has_drag;
            xmlChar* s = xmlNodeGetContent(node);

            if(s)
            {
                fx->dragcoef = (float)atof((char*)s);
                xmlFree(s);
            }

            continue;
        }

        if(!xmlStrcmp((xmlChar*)"randomsize", node->name))
        {
            xmlChar* s = xmlNodeGetContent(node);

            if(s)
            {
                fx->particleflags |= part_has_random_size;

                float mins, maxs;
                sscanf((char*)s, "%f %f", &mins, &maxs);

                fx->size.push_back(mins);
                fx->size.push_back(maxs);

                xmlFree(s);
            }

            continue;
        }

        if(!xmlStrcmp((xmlChar*)"rotate", node->name))
        {
            xmlChar* s = xmlNodeGetContent(node);

            if(s)
            {
                fx->rotate = (float)atof((char*)s);
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
                    fx->stretchscale = (float)atof((char*)s);
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
                fx->stretchscale = (float)atof((char*)s);
                xmlFree(s);
            }
        }

        if(!xmlStrcmp((xmlChar*)"legacystretch", node->name))
        {
            fx->particleflags |= part_has_legacy_stretch;

            fx->stretchscale = 1.0f;

            xmlChar* s = xmlNodeGetContent(node);

            if(s)
            {
                fx->stretchscale = (float)atof((char*)s);
                xmlFree(s);
            }
        }

        if(!xmlStrcmp((xmlChar*)"separation", node->name))
        {
            xmlChar* s = xmlNodeGetContent(node);

            if(s)
            {
                fx->separation = (float)atof((char*)s);
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

        if(!xmlStrcmp((xmlChar*)"randomglow", node->name))
        {
            fx->fxflags |= fx_has_randomglow;
            continue;
        }

        if(!xmlStrcmp((xmlChar*)"outdofdirection", node->name))
        {
            fx->fxflags |= fx_out_dofdirection;
            continue;
        }

        if(!xmlStrcmp((xmlChar*)"rotate_to_dir", node->name))
        {
            fx->fxflags |= fx_rotate_particle_to_dir;
            continue;
        }

        if(!xmlStrcmp((xmlChar*)"dontupdateposition", node->name))
        {
            fx->particleflags |= part_dont_update_position;
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

        if(!xmlStrcmp((xmlChar*)"randomxydirection", node->name))
        {
            fx->fxflags |= fx_random_direction;

            xmlChar* s = xmlNodeGetContent(node);

            if(s)
            {
                if(s[0])
                {
                    fx->randomxydirection = atol((char*)s);
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

        if(!xmlStrcmp((xmlChar*)"noflip", node->name))
        {
            fx->particleflags |= part_no_flip;
            continue;
        }

        if(!xmlStrcmp((xmlChar*)"gravity", node->name))
        {
            fx->particleflags |= part_has_gravity;

            xmlChar* s = xmlNodeGetContent(node);

            if(s)
            {
                fx->gravitycoef = (float)atof((char*)s);
                xmlFree(s);
            }

            continue;
        }

        if(!xmlStrcmp((xmlChar*)"speed", node->name))
        {
            xmlChar* s = xmlNodeGetContent(node);

            if(s)
            {
                fx->speed = (float)atof((char*)s);
                xmlFree(s);
            }

            continue;
        }

        if(!xmlStrcmp((xmlChar*)"clampemitter", node->name))
        {
            xmlChar* s = xmlNodeGetContent(node);

            if(s)
            {
                fx->fxflags |= fx_clamp_emitter;
                fx->clampemitter = atol((char*)s);
                xmlFree(s);
            }

            continue;
        }

        if(!xmlStrcmp((xmlChar*)"clampbeam", node->name))
        {
            xmlChar* s = xmlNodeGetContent(node);

            if(s)
            {
                fx->fxflags |= fx_clamp_beam;
                fx->clampbeam = (float)atof((char*)s);
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

                //Back-ass-wards coordinate system
                fx->velvariance[2] = -fx->velvariance[2];
                fx->velvariance[5] = -fx->velvariance[5];

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
                fx->velreduction = (float)atof((char*)s);
                xmlFree(s);
            }

            continue;
        }

        if(!xmlStrcmp((xmlChar*)"alphastart", node->name))
        {
            xmlChar* s = xmlNodeGetContent(node);

            if(s)
            {
                fx->alpha.push_back((float)atof((char*)s));
                xmlFree(s);
            }

            continue;
        }

        if(!xmlStrcmp((xmlChar*)"alphaend", node->name))
        {
            xmlChar* s = xmlNodeGetContent(node);

            if(s)
            {
                fx->alpha.push_back((float)atof((char*)s));
                xmlFree(s);
            }

            continue;
        }

        if(!xmlStrcmp((xmlChar*)"sizestart", node->name))
        {
            xmlChar* s = xmlNodeGetContent(node);

            if(s)
            {
                fx->size.push_back((float)atof((char*)s));
                xmlFree(s);
            }

            continue;
        }

        if(!xmlStrcmp((xmlChar*)"sizeend", node->name))
        {
            xmlChar* s = xmlNodeGetContent(node);

            if(s)
            {
                fx->size.push_back((float)atof((char*)s));
                xmlFree(s);
            }

            continue;
        }

        if(!xmlStrcmp((xmlChar*)"material", node->name))
        {
            xmlChar* s = xmlNodeGetContent(node);

            if(s)
            {
                std::map<std::string, int>::iterator itr = _materials.find((char*)s);

                if(itr == _materials.end())
                {
                    // Default
                    fx->matid = _materials.find("smoke")->second;
                }
                else
                {
                    fx->matid = itr->second;
                }

                xmlFree(s);
            }

            continue;
        }
    }
}

/*-----------------------------------------------------------------------
Function: ParseEmitterElements
Description:
-----------------------------------------------------------------------*/
static void ParseEmitterElements(xmlNodePtr root, Emitterfx* e)
{
    xmlNodePtr node = root;

	for(; node != nullptr; node = node->next)
	{
        if(node->type != XML_ELEMENT_NODE)
        {
            continue;
        }

        if(!xmlStrcmp((xmlChar*)"lifetime", node->name))
        {
            xmlChar* s = xmlNodeGetContent(node);

            if(s)
            {
                e->lifetime = atol((char*)s);
                xmlFree(s);
            }

           continue;
        }

        if(!xmlStrcmp((xmlChar*)"alphadegradation", node->name))
        {
            e->emitterflags |= emitter_has_alphadegradation;
            continue;
        }

        if(!xmlStrcmp((xmlChar*)"particleeffect", node->name))
        {
            xmlChar* s = xmlNodeGetContent(node);

            if(s)
            {
                e->_particleeffectnames.push_back((char*)s);
                xmlFree(s);
            }

           continue;
        }

        if(!xmlStrcmp((xmlChar*)"fountain", node->name))
        {
            xmlChar* s = xmlNodeGetContent(node);

            e->emitterflags |= emitter_is_fountain;

            if(s)
            {
                e->numpersecond = (float)atof((char*)s);
                xmlFree(s);
            }

           continue;
        }

        if(!xmlStrcmp((xmlChar*)"flags", node->name))
        {
            xmlChar* s = xmlNodeGetContent(node);

            if(s)
            {
                if(!xmlStrcmp((xmlChar*)"particleeffectonly", s))
                {
                    e->emitterflags |= emitter_particle_effect_only;
                }
                else if(!xmlStrcmp((xmlChar*)"particlebeam", s))
                {
                    e->emitterflags |= emitter_particle_beam;
                }
                else if(!xmlStrcmp((xmlChar*)"animation", s))
                {
                    e->emitterflags |= emitter_is_animation;
                }
                else if(!xmlStrcmp((xmlChar*)"watersurface", s))
                {
                    e->emitterflags |= emitter_is_watersurfaceeffect;
                }

                xmlFree(s);
            }

           continue;
        }
    }
}

/*-----------------------------------------------------------------------
Function: ParseTransformEffects
Description:
-----------------------------------------------------------------------*/
static void ParseTransformEffects(xmlNodePtr root, TransformFx* te)
{
    xmlNodePtr node = root;

	for(; node != nullptr; node = node->next)
	{
        if(node->type != XML_ELEMENT_NODE)
        {
            continue;
        }

        if(!xmlStrcmp((xmlChar*)"flags", node->name))
        {
            xmlChar* s = xmlNodeGetContent(node);

            if(s)
            {
                if(!xmlStrcmp((xmlChar*)"standalone", s))
                {
                    te->flags |= transform_is_standalone;
                }

                xmlFree(s);
            }

            continue;
        }

        if(!xmlStrcmp((xmlChar*)"speed", node->name))
        {
            xmlChar* s = xmlNodeGetContent(node);

            if(s)
            {
                te->speed = (float)atof((char*)s);
                xmlFree(s);
            }

            continue;
        }

        if(!xmlStrcmp((xmlChar*)"drag", node->name))
        {
            xmlChar* s = xmlNodeGetContent(node);

            if(s)
            {
                te->drag = (float)atof((char*)s);
                xmlFree(s);
            }

            continue;
        }

        if(!xmlStrcmp((xmlChar*)"gravity", node->name))
        {
            xmlChar* s = xmlNodeGetContent(node);

            if(s)
            {
                te->gravity = (float)atof((char*)s);
                xmlFree(s);
            }

            continue;
        }

        // Only applies to standalone transforms
        if(!xmlStrcmp((xmlChar*)"randomdirection", node->name))
        {
            xmlChar* s = xmlNodeGetContent(node);
            te->flags |= transform_random_direction;

            if(s)
            {
                if(s[0])
                {
                    te->randomdirection = atol((char*)s);
                }

                xmlFree(s);
            }

            continue;
        }

        if(!xmlStrcmp((xmlChar*)"emitter", node->name))
        {
            Emitterfx* e = FxGet().getfreeemittereffect();
            ParseEmitterElements(node->children, e);
            te->_emitters.push_back(e);

            continue;
        }
    }
}

/*-----------------------------------------------------------------------
Function: _parseeffectstree
Description:
-----------------------------------------------------------------------*/
void _parseeffectstree(xmlNodePtr root)
{
    xmlNodePtr node = root;

	for(; node != nullptr; node = node->next)
	{
        if(node->type != XML_ELEMENT_NODE)
        {
            continue;
        }

        if(!xmlStrcmp((xmlChar*)"transformeffect", node->name))
        {
            xmlChar* s = xmlGetProp(node, (xmlChar*)"name");

            if(s)
            {
                TransformFx* te = FxGet().getfreetransformeffect();

                te->name = (char*)s;

                _transformeffects[te->name] = te;

                ParseTransformEffects(node->children, te);

                xmlFree(s);
            }

            continue;
        }

        if(!xmlStrcmp((xmlChar*)"particleeffect", node->name))
        {
            xmlChar* s = xmlGetProp(node, (xmlChar*)"name");

            if(s)
            {
                ParticleEffect* pe = FxGet().getfreeparticleeffect();

                pe->name = (char*)s;

                _particleeffects[pe->name] = pe;

                ParseParticleElements(node->children, pe);

                xmlFree(s);
            }

            continue;
        }
	}
}

/*-----------------------------------------------------------------------
Function: loadeffects
Description:
-----------------------------------------------------------------------*/
void loadeffects()
{
    xmlRegisterInputCallbacks(xmlinputmatch,
                             (xmlInputOpenCallback)fioOpen,
                             (xmlInputReadCallback)fioRead,
                             (xmlInputCloseCallback)fioClose);

    const char* name = "fx/xml/effects2.xml";

    xmlDocPtr doc = xmlReadFile(name, nullptr, XML_PARSE_DTDLOAD | XML_PARSE_RECOVER);

    if(doc)
    {
	    xmlNodePtr root = xmlDocGetRootElement(doc);

        if(!root)
        {
            return;
        }

        if(!xmlStrcmp((xmlChar*)"FXEFFECTS", root->name))
        {
            _parseeffectstree(root->children);
        }

	    xmlFreeDoc(doc);
    }

    xmlCleanupInputCallbacks();
}

/*----------------------------------------------------------------------
Function: init
Description:
-----------------------------------------------------------------------*/
void DataFx::init()
{
    piMATERIAL mat;
    memset(&mat, 0, sizeof(piMATERIAL));

    mat.diffuse.set(1.0f, 1.0f, 1.0f, 1.0f);
    mat.ambient.set(0.525490f, 0.525490f, 0.525490f, 1.0f);
    mat.emissive.set(1.0f, 1.0f, 1.0f, 1.0f);

    int id = oglCreateMaterial(&mat);

    _materials["fire"] = id;
    oglSetMaterialName(id, "particlefire");

    mat.diffuse.set(0.5f, 0.5f, 0.5f, 1.0f);
    mat.ambient.set(0.525490f, 0.525490f, 0.525490f, 1.0f);
    mat.emissive.set(0, 0, 0, 0);

    id = oglCreateMaterial(&mat);

    _materials["smoke"] = id;
    oglSetMaterialName(id, "particlesmoke");

    mat.diffuse.set(1.0f, 1.0f, 1.0f, 1.0f);
    mat.ambient.set(0.05f, 0.05f, 0.05f, 1.0f);
    mat.emissive.set(0, 0, 0, 0);

    id = oglCreateMaterial(&mat);

    _materials["water"] = id;
    oglSetMaterialName(id, "particlewater");

    mat.diffuse.set(1.0f, 1.0f, 1.0f, 1.0f);
    mat.ambient.set(0.05f, 0.05f, 0.05f, 1.0f);
    mat.emissive.set(1.0f, 1.0f, 1.0f, 1.0f);

    id = oglCreateMaterial(&mat);

    _materials["waterwake"] = id;
    oglSetMaterialName(id, "particlewaterwake");

    mat.diffuse.set(1.0f, 1.0f, 1.0f, 1.0f);
    mat.ambient.set(0.525490f, 0.525490f, 0.525490f, 1.0f);
    mat.emissive.set(0, 0, 0, 0);

    id = oglCreateMaterial(&mat);

    _materials["stone"] = id;
    oglSetMaterialName(id, "particlestone");

    mat.diffuse.set(0.909804f, 0.909804f, 0.909804f, 1.0f);
    mat.ambient.set(0.525490f, 0.525490f, 0.525490f, 1.0f);
    mat.emissive.set(0, 0, 0, 0);

    id = oglCreateMaterial(&mat);

    _materials["wood"] = id;
    oglSetMaterialName(id, "particlewood");

    _fxtexture = ldTextureLoad("fx/fx2.dds", LDPIX_CLAMP_TO_BORDER);

    // UV coords to fx2.dds
    loadTextureUVs();

    // Effects xml
    loadeffects();

    _wind.x() = -0.1f;
    _wind.y() = -0.1f;
    _wind.z() = 0.02f;
}

/*----------------------------------------------------------------------
Function: update
Description:
-----------------------------------------------------------------------*/
void DataFx::update()
{
#ifdef BETA_RELEASE
    // For debugging only...reload file if it has changed.
    static int m1 = 0;
    static int m2 = 0;

    static bool first = true;

    if(first)
    {
        fioModified(&m1, "fx/xml/fx_uv2.xml");
        first = FALSE;
    }

    fioModified(&m2, "fx/xml/fx_uv2.xml");

    if(m1 < m2)
    {
        m1 = m2;
        loadTextureUVs();
        killeffects();
        loadeffects();
    }

    static int m3 = 0;
    static int m4 = 0;

    static bool first2 = true;

    if(first2)
    {
        fioModified(&m3, "fx/xml/effects2.xml");
        first2 = FALSE;
    }

    fioModified(&m4, "fx/xml/effects2.xml");

    if(m3 < m4)
    {
        m3 = m4;

        killeffects();
        loadeffects();
    }
#endif // BETA_RELEASE
}

/*----------------------------------------------------------------------
Function: gettransformeffect
Description:
-----------------------------------------------------------------------*/
TransformFx* DataFx::gettransformeffect(const char* name)
{
    std::map<std::string, TransformFx*>::iterator find;

    find = _transformeffects.find(name);

    if(find != _transformeffects.end())
    {
        return find->second;
    }

    return nullptr;
}

/*----------------------------------------------------------------------
Function: releaseparticleeffect
Description: When creating dynamic particle effects
-----------------------------------------------------------------------*/
void DataFx::releaseparticleeffect(ParticleEffect* pe)
{
    _freeparticleeffects.push_back(pe);
}

/*----------------------------------------------------------------------
Function: releaseemittereffect
Description:
-----------------------------------------------------------------------*/
void DataFx::releaseemittereffect(Emitterfx* e)
{
    _freeemittereffects.push_back(e);
}

/*----------------------------------------------------------------------
Function: releasetransformeffect
Description:
-----------------------------------------------------------------------*/
void DataFx::releasetransformeffect(TransformFx* e)
{
    _freetransformeffects.push_back(e);
}

/*----------------------------------------------------------------------
Function: clone
Description:
-----------------------------------------------------------------------*/
ParticleEffect* DataFx::cloneparticleeffect(const char* effect)
{
    ParticleEffect* pe = getparticleeffect(effect);

    if(pe)
    {
        ParticleEffect* p = getfreeparticleeffect();

        p->isclone = true;

        p->stretchscale = pe->stretchscale;
        p->speed = pe->speed;
        p->dragcoef = pe->dragcoef;
        p->gravitycoef = pe->gravitycoef;
        p->rotate = pe->rotate;
        p->velreduction = pe->velreduction;
        p->fxflags = pe->fxflags;
        p->particleflags = pe->particleflags;
        p->lifetime = pe->lifetime;
        p->randomdirection = pe->randomdirection;
        p->delaystart = pe->delaystart;
        p->delayeachparticle = pe->delayeachparticle;
        p->separation = pe->separation;
        p->clampemitter = pe->clampemitter;
        p->clampbeam = pe->clampbeam;
        p->glowtime = pe->glowtime;
        p->stopstretch = pe->stopstretch;
        p->matid = pe->matid;

        memcpy(p->posvariance, pe->posvariance, sizeof(float) * 6);
        memcpy(p->velvariance, pe->velvariance, sizeof(float) * 6);

        p->name = pe->name;
        p->texture = pe->texture;
        p->number = pe->number;
        p->time = pe->time;
        p->alpha = pe->alpha;
        p->color = pe->color;
        p->size = pe->size;

        return p;
    }

    return nullptr;
}

/*----------------------------------------------------------------------
Function: getfreeparticleeffect
Description: When creating dynamic particle effects
-----------------------------------------------------------------------*/
ParticleEffect* DataFx::getfreeparticleeffect()
{
    ParticleEffect* pe = nullptr;

    if(_freeparticleeffects.empty())
    {
        pe =  new ParticleEffect;
    }
    else
    {
        pe = _freeparticleeffects.front();
        _freeparticleeffects.pop_front();
    }

    pe->reset();

    return pe;
}

/*----------------------------------------------------------------------
Function: getfreeemittereffect
Description:
-----------------------------------------------------------------------*/
Emitterfx* DataFx::getfreeemittereffect()
{
    Emitterfx* e = nullptr;

    if(_freeemittereffects.empty())
    {
        e =  new Emitterfx;
    }
    else
    {
        e = _freeemittereffects.front();
        _freeemittereffects.pop_front();
    }

    e->reset();

    return e;
}

/*----------------------------------------------------------------------
Function: getfreetransformeffect
Description:
-----------------------------------------------------------------------*/
TransformFx* DataFx::getfreetransformeffect()
{
    TransformFx* t = nullptr;

    if(_freeemittereffects.empty())
    {
        t =  new TransformFx;
    }
    else
    {
        t = _freetransformeffects.front();
        _freetransformeffects.pop_front();
    }

    t->reset();

    return t;
}

/*----------------------------------------------------------------------
Function: getparticleeffect
Description:
-----------------------------------------------------------------------*/
ParticleEffect* DataFx::getparticleeffect(const char* name)
{
    std::map<std::string, ParticleEffect*>::iterator find;

    find = _particleeffects.find(name);

    if(find != _particleeffects.end())
    {
        return find->second;
    }

    return nullptr;
}
