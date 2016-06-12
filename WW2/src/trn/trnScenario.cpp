#include <syscfg.h> // Path is project specific.
/*--------------------------------------------------------------------
Module: trnScenario.cpp
Description: Module used for testing terrain stuff. Should not be used
for runtime 'anything'.
---------------------------------------------------------------------*/
#include "trn/trnScenario.h"
#include "spt/sptmain.h"
#include "ui/uiDotCommands.h"
#include "ogl/oglMaterial.h"
#include "ld/ldtexture.h"
#include "ogl/oglMesh.h"
#include "trn/speedgrass.h"
#include "xml/XMLLoader2.h"
#include "osgImport/ShadowMap.h"
#include "ob/obdraw.h"
#include "ob/obMgr.h"
#include "trn/trnmain.h"
#include "trn/trncd.h"
#include "math/mathnum.h"
#include "scen/ScenarioManager.h"
#include "trn/trnvars.h"
#include "ld/ldpix.h"
#include "physics/colliderMain.h"
#include "ogl/States.h"
#include "sdm/sdmDefines.h"

static ref_ptr<TerrainScenario> terrainscenario;

#define BAD_FLOAT   -9999999.0f
#define BAD_INT     -9999999

#define MAX_MASKS   8

static ref_ptr<oglMaterial> shadowmaterial;

namespace ScenarioColors
{
    enum
    {
        RED,
        GREEN,
        BLUE,
        NUM_COLORS
    };
};

namespace ScenarioParams
{
    enum
    {
        ROTATION,
        ZOFFSET
    };
};

struct BlendMask
{
    std::string blendmask;
    std::vector<std::string> layers;
    std::vector<float> multitexturescale;
    std::vector<std::string> materials;
};

struct TerrainTile
{
    std::string name;
    std::vector<struct BlendMask*> blends;
    int layercount;
    std::string shader;
    std::vector<std::string> cluttermask;
    std::string treemask;
    std::string grassmask;
    std::string splatmask;
};

struct GrassObjects
{
    int grassweights[ScenarioColors::NUM_COLORS];
    int grassclamps[ScenarioColors::NUM_COLORS];
    float grassscales[ScenarioColors::NUM_COLORS];
    std::vector<int> types[ScenarioColors::NUM_COLORS];

    int GetGrassWeight(int color)
    {
        return grassweights[color];
    }

    int GetGrassClamp(int color)
    {
        return grassclamps[color];
    }

    float GetGrassScale(int color)
    {
        return grassscales[color];
    }

    int GetGrassType(int color, unsigned int seed)
    {
        int idx = seed % types[color].size();
        return types[color][idx];
    }
};

struct TreeObjects
{
    int treeweights[ScenarioColors::NUM_COLORS];
    int treeclamps[ScenarioColors::NUM_COLORS];
    std::vector<int> types[ScenarioColors::NUM_COLORS];

    int GetTreeWeight(int color)
    {
        return treeweights[color];
    }

    int GetTreeClamp(int color)
    {
        return treeclamps[color];
    }

    int GetTreeType(int color, unsigned int seed)
    {
        int idx = seed % types[color].size();
        return types[color][idx];
    }
};

struct ClutterObjects
{
    int objclutterclamps[ScenarioColors::NUM_COLORS];
    int objclutterweights[ScenarioColors::NUM_COLORS];
    std::vector<float> clutterobjectparams[ScenarioColors::NUM_COLORS];
    std::vector<std::string> clutterobjects[ScenarioColors::NUM_COLORS];

    const char* GetClutterObject(int color, unsigned int seed)
    {
        if(clutterobjects[color].empty())
            return nullptr;

        int idx = seed % clutterobjects[color].size();
        return clutterobjects[color][idx].c_str();
    }

    int GetClutterWeight(int color)
    {
        return objclutterweights[color];
    }

    int GetClutterClamp(int color)
    {
        return objclutterclamps[color];
    }

    float GetClutterRotation(int color)
    {
        if(clutterobjectparams[color].size() == ScenarioParams::ROTATION + 1)
            return clutterobjectparams[color][ScenarioParams::ROTATION];

        return 0.0f;
    }

    float GetClutterZOffset(int color)
    {
        if(clutterobjectparams[color].size() == ScenarioParams::ZOFFSET + 1)
            return clutterobjectparams[color][ScenarioParams::ZOFFSET];

        return 0.0f;
    }
};

typedef std::map<std::string, TerrainTile*> TileMap;
static TileMap tilemap;

typedef std::map<std::string, GrassObjects*> GrassObjectMap;
static GrassObjectMap grassmap;

typedef std::map<std::string, TreeObjects*> TreeObjectMap;
static TreeObjectMap treemap;

typedef std::map<std::string, ClutterObjects*> ClutterObjectMap;
static ClutterObjectMap cluttermap;

typedef std::map<std::string, std::string> LoadedTerrainMap;
static LoadedTerrainMap loadedmap;

class ExposedLoader : public XMLLoader2<ExposedLoader>
{
public:

    enum
    {
        LOADING_TREES = 0x1,
        LOADING_GRASS = 0x2,
        LOADING_OBJECT_CLUTTER = 0x8,
        LOADING_FOG_PARAMS = 0x10,
        LOADING_TERRAIN_TILE = 0x20
    };

    enum
    {
        LAYER0,
        LAYER1,
        LAYER2,
        LAYER3,
        LAYER4,
        NUM_LAYERS
    };

    ExposedLoader() : XMLLoader2<ExposedLoader>()
    {
        loaded = false;

        fogstart = BAD_FLOAT;
        fogend = BAD_FLOAT;
        fogcolor = BAD_INT;

        whichmask = 0;

        AddHandler("trees", &ExposedLoader::ReadTrees, this);
        AddHandler("grass", &ExposedLoader::ReadGrass, this);
        AddHandler("redtypeint", &ExposedLoader::ReadRedInt, this);
        AddHandler("greentypeint", &ExposedLoader::ReadGreenInt, this);
        AddHandler("bluetypeint", &ExposedLoader::ReadBlueInt, this);
        AddHandler("redweight", &ExposedLoader::ReadRedWeight, this);
        AddHandler("greenweight", &ExposedLoader::ReadGreenWeight, this);
        AddHandler("blueweight", &ExposedLoader::ReadBlueWeight, this);
        AddHandler("redclamp", &ExposedLoader::ReadRedClamp, this);
        AddHandler("grnclamp", &ExposedLoader::ReadGreenClamp, this);
        AddHandler("bluclamp", &ExposedLoader::ReadBlueClamp, this);
        AddHandler("redscale", &ExposedLoader::ReadRedScale, this);
        AddHandler("grnscale", &ExposedLoader::ReadGreenScale, this);
        AddHandler("bluscale", &ExposedLoader::ReadBlueScale, this);
        AddHandler("layer0", &ExposedLoader::ReadLayer, this);
        AddHandler("layer1", &ExposedLoader::ReadLayer, this);
        AddHandler("layer2", &ExposedLoader::ReadLayer, this);
        AddHandler("layer3", &ExposedLoader::ReadLayer, this);
        AddHandler("layer4", &ExposedLoader::ReadLayer, this);
        AddHandler("objectclutter", &ExposedLoader::ReadObjectClutter, this);
        AddHandler("redtypestring", &ExposedLoader::ReadRedString, this);
        AddHandler("greentypestring", &ExposedLoader::ReadGreenString, this);
        AddHandler("bluetypestring", &ExposedLoader::ReadBlueString, this);
        AddHandler("redparams", &ExposedLoader::ReadRedParams, this);
        AddHandler("greenparams", &ExposedLoader::ReadGreenParams, this);
        AddHandler("blueparams", &ExposedLoader::ReadBlueParams, this);
        AddHandler("fogparams", &ExposedLoader::ReadFogParams, this);
        AddHandler("start", &ExposedLoader::ReadStart, this);
        AddHandler("end", &ExposedLoader::ReadEnd, this);
        AddHandler("fogcolor", &ExposedLoader::ReadColor, this);
        AddHandler("terraintile", &ExposedLoader::ReadTerrainTile, this);
        AddHandler("shader", &ExposedLoader::ReadShader, this);
        AddHandler("material", &ExposedLoader::ReadMaterial, this);
        AddHandler("multitexture", &ExposedLoader::ReadMultiTexture, this);
        AddHandler("blendmask", &ExposedLoader::ReadBlendMask, this);
        AddHandler("treemask", &ExposedLoader::ReadTreeMask, this);
        AddHandler("cluttermask", &ExposedLoader::ReadClutterMask, this);
        AddHandler("grassmask", &ExposedLoader::ReadGrassMask, this);
        AddHandler("splatmask", &ExposedLoader::ReadSplatMask, this);
    }

    virtual ~ExposedLoader()
    {
    }

    XML_TAG_HANDLER(ReadSplatMask)
    {
        if(first)
        {
            if(loadflag & LOADING_TERRAIN_TILE)
            {
                TerrainTile* tt = (TerrainTile*)userData;
                tt->splatmask = xmlGetStringFromNode(node);
            }
        }

        return false;
    }

    XML_TAG_HANDLER(ReadGrassMask)
    {
        if(first)
        {
            if(loadflag & LOADING_TERRAIN_TILE)
            {
                TerrainTile* tt = (TerrainTile*)userData;
                tt->grassmask = xmlGetStringFromNode(node);
            }
        }

        return false;
    }

    XML_TAG_HANDLER(ReadTreeMask)
    {
        if(first)
        {
            if(loadflag & LOADING_TERRAIN_TILE)
            {
                TerrainTile* tt = (TerrainTile*)userData;
                tt->treemask = xmlGetStringFromNode(node);
            }
        }

        return false;
    }

    XML_TAG_HANDLER(ReadClutterMask)
    {
        if(first)
        {
            if(loadflag & LOADING_TERRAIN_TILE)
            {
                TerrainTile* tt = (TerrainTile*)userData;
                tt->cluttermask.push_back(xmlGetStringFromNode(node));
            }
        }

        return false;
    }

    XML_TAG_HANDLER(ReadBlendMask)
    {
        if(first)
        {
            if(loadflag & LOADING_TERRAIN_TILE)
            {
                TerrainTile* tt = (TerrainTile*)userData;
                struct BlendMask* mask = new BlendMask;

                tt->blends.push_back(mask);

                mask->blendmask = xmlGetStringFromNode(node, "name");

                mask->materials.resize(NUM_LAYERS);
                mask->layers.resize(NUM_LAYERS);
                mask->multitexturescale.resize(NUM_LAYERS);

                mask->materials[LAYER0] = "default";
                mask->materials[LAYER1] = "default";
                mask->materials[LAYER2] = "default";
                mask->materials[LAYER3] = "default";
                mask->materials[LAYER4] = "default";

                mask->multitexturescale[LAYER0] = 128.0f;
                mask->multitexturescale[LAYER1] = 128.0f;
                mask->multitexturescale[LAYER2] = 512.0f;
                mask->multitexturescale[LAYER3] = 512.0f;
                mask->multitexturescale[LAYER4] = 512.0f;

                if(ProcessChildren(node, tt))
                    loaded = true;
            }
        }

        return false;
    }

    XML_TAG_HANDLER(ReadShader)
    {
        if(first)
        {
            if(loadflag & LOADING_TERRAIN_TILE)
            {
                TerrainTile* tt = (TerrainTile*)userData;
                tt->shader = xmlGetStringFromNode(node);
            }
        }

        return false;
    }

    XML_TAG_HANDLER(ReadMaterial)
    {
        if(first)
        {
            if(loadflag & LOADING_TERRAIN_TILE)
            {
                TerrainTile* tt = (TerrainTile*)userData;
                BlendMask& mask = *tt->blends.back();
                mask.materials[layer] = xmlGetStringFromNode(node);
            }
        }

        return false;
    }

    XML_TAG_HANDLER(ReadTerrainTile)
    {
        if(first)
        {
            loadflag = LOADING_TERRAIN_TILE;

            const char* s = xmlGetStringFromNode(node, "name");

            TerrainTile* tt = new TerrainTile;
            tt->layercount = 0;

            tilemap[s] = tt;

            tt->name = s;

            if(ProcessChildren(node, tt))
                loaded = true;
        }

        return false;
    }

    XML_TAG_HANDLER(ReadFogParams)
    {
        if(first)
        {
            loadflag = LOADING_FOG_PARAMS;
        }

        return true;
    }

    XML_TAG_HANDLER(ReadMultiTexture)
    {
        if(first)
        {
            if(loadflag == LOADING_TERRAIN_TILE)
            {
                TerrainTile* tt = (TerrainTile*)userData;
                const char* s = xmlGetStringFromNode(node);

                BlendMask& mask = *tt->blends.back();
                mask.multitexturescale[layer] = (float)atof(s);
            }
        }

        return false;
    }

    XML_TAG_HANDLER(ReadStart)
    {
        if(first)
        {
            const char* s = xmlGetStringFromNode(node);

            if(*s && (loadflag & LOADING_FOG_PARAMS))
				fogstart = (float)atof(s);
        }

        return false;
    }

    XML_TAG_HANDLER(ReadEnd)
    {
        if(first)
        {
            const char* s = xmlGetStringFromNode(node);

            if(*s && (loadflag & LOADING_FOG_PARAMS))
				fogend = (float)atof(s);
        }

        return false;
    }

    XML_TAG_HANDLER(ReadColor)
    {
        if(first)
        {
            const char* s = xmlGetStringFromNode(node);

            if(*s && (loadflag & LOADING_FOG_PARAMS))
            {
                char* p = strtok((char*)s, " ");

                std::vector<int> color;

                int count = 0;

                while(p)
                {
                    int i = atol(p);
                    color.push_back(i);

                    p = strtok(nullptr, " ");
                    count++;
                }

                fogcolor = piRGBA_MAKE(color[0], color[1], color[2], color[3]);
            }
        }

        return false;
    }

    XML_TAG_HANDLER(ReadRedParams)
    {
        if(first)
        {
             const char* s = xmlGetStringFromNode(node);
             float deg, zoffset;

             if(*s)
             {
                 int fields = sscanf(s, "%f %f", &deg, &zoffset);

                 if(fields > 0)
                 {
                     if(loadflag & LOADING_OBJECT_CLUTTER)
                     {
                         ClutterObjects* co = (ClutterObjects*)userData;
                         co->clutterobjectparams[ScenarioColors::RED].push_back(deg);

                         if(fields > 1)
                             co->clutterobjectparams[ScenarioColors::RED].push_back(zoffset);
                     }
                 }
             }
        }

        return false;
    }

    XML_TAG_HANDLER(ReadGreenParams)
    {
        if(first)
        {
             const char* s = xmlGetStringFromNode(node);
             float deg, zoffset;

             if(*s)
             {
                 int fields = sscanf(s, "%f %f", &deg, &zoffset);

                 if(fields > 0)
                 {
                     if(loadflag & LOADING_OBJECT_CLUTTER)
                     {
                         ClutterObjects* co = (ClutterObjects*)userData;
                         co->clutterobjectparams[ScenarioColors::GREEN].push_back(deg);

                         if(fields > 1)
                             co->clutterobjectparams[ScenarioColors::GREEN].push_back(zoffset);
                     }
                 }
             }
        }

        return false;
    }

    XML_TAG_HANDLER(ReadBlueParams)
    {
        if(first)
        {
             const char* s = xmlGetStringFromNode(node);
             float deg, zoffset;

            if(*s)
            {
                int fields = sscanf(s, "%f %f", &deg, &zoffset);

                if(loadflag & LOADING_OBJECT_CLUTTER)
                {
                    ClutterObjects* co = (ClutterObjects*)userData;
                    co->clutterobjectparams[ScenarioColors::BLUE].push_back(deg);

                    if(fields > 1)
                        co->clutterobjectparams[ScenarioColors::BLUE].push_back(zoffset);
                }
            }
        }

        return false;
    }

    XML_TAG_HANDLER(ReadObjectClutter)
    {
        if(first)
        {
            loadflag = LOADING_OBJECT_CLUTTER;

            const char* s = xmlGetStringFromNode(node, "name");

            ClutterObjects* co = new ClutterObjects;

            if(!(*s))
                s = "cluttermask.dds";

            cluttermap[s] = co;

            for(int i = 0; i < ScenarioColors::NUM_COLORS; i++)
            {
                co->objclutterclamps[i] = 255;
                co->objclutterweights[i] = 255;
            }

            if(ProcessChildren(node, co))
                loaded = true;
        }

        whichmask++;

        return false;
    }

    XML_TAG_HANDLER(ReadLayer)
    {
        if(first)
        {
            if(loadflag & LOADING_TERRAIN_TILE)
            {
                const char* ss = xmlGetStringFromNode(node, "name");

                TerrainTile* tt = (TerrainTile*)userData;

                if(!xmlStrcmp(node->name, (xmlChar*)"layer0"))
                {
                    layer = 0;
                    BlendMask& mask = *tt->blends.back();
                    mask.layers[0] = ss;
                    tt->layercount++;
                }
                else if(!xmlStrcmp(node->name, (xmlChar*)"layer1"))
                {
                    layer = 1;
                    BlendMask& mask = *tt->blends.back();
                    mask.layers[1] = ss;
                    tt->layercount++;
                }
                else if(!xmlStrcmp(node->name, (xmlChar*)"layer2"))
                {
                    layer = 2;
                    BlendMask& mask = *tt->blends.back();
                    mask.layers[2] = ss;
                    tt->layercount++;
                }
                else if(!xmlStrcmp(node->name, (xmlChar*)"layer3"))
                {
                    layer = 3;
                    BlendMask& mask = *tt->blends.back();
                    mask.layers[3] = ss;
                    tt->layercount++;
                }
                else if(!xmlStrcmp(node->name, (xmlChar*)"layer4"))
                {
                    layer = 4;
                    BlendMask& mask = *tt->blends.back();
                    mask.layers[4] = ss;
                    tt->layercount++;
                }
            }
        }

        return true;
    }

    XML_TAG_HANDLER(ReadTrees)
    {
        if(first)
        {
            loadflag = LOADING_TREES;

            const char* s = xmlGetStringFromNode(node, "name");

            if(!(*s))
                s = "treemask.dds";

            TreeObjects* to = new TreeObjects;
            treemap[s] = to;

            for(int i = 0; i < ScenarioColors::NUM_COLORS; i++)
            {
                to->treeweights[i] = 0;
                to->treeclamps[i] = 0;
            }

            if(ProcessChildren(node, to))
                loaded = true;
        }

        return false;
    }

    XML_TAG_HANDLER(ReadGrass)
    {
        if(first)
        {
            loadflag = LOADING_GRASS;

            const char* s = xmlGetStringFromNode(node, "name");

            if(!(*s))
                s = "grassmask.dds";

            GrassObjects* grass = new GrassObjects;
            grassmap[s] = grass;

            for(int i = 0; i < ScenarioColors::NUM_COLORS; i++)
            {
                grass->grassweights[i] = 0;
                grass->grassclamps[i] = 0;
                grass->grassscales[i] = 1.0f;
            }

            if(ProcessChildren(node, grass))
                loaded = true;
        }

        return false;
    }

    XML_TAG_HANDLER(ReadRedString)
    {
        if(first && userData)
        {
            std::vector<std::string>* red;

            if(loadflag & LOADING_OBJECT_CLUTTER)
            {
                ClutterObjects* co = (ClutterObjects*)userData;
                red = &co->clutterobjects[ScenarioColors::RED];
            }
            else
                red = &((std::vector<std::string>*)userData)[ScenarioColors::RED];

            const char* s = xmlGetStringFromNode(node);

            if(*s)
            {
                char* p = strtok((char*)s, " ");

                while(p)
                {
                    red->push_back(p);
                    p = strtok(nullptr, " ");
                }
            }
        }

        return true;
    }

    XML_TAG_HANDLER(ReadGreenString)
    {
        if(first && userData)
        {
            std::vector<std::string>* grn;

            if(loadflag & LOADING_OBJECT_CLUTTER)
            {
                ClutterObjects* co = (ClutterObjects*)userData;
                grn = &co->clutterobjects[ScenarioColors::GREEN];
            }
            else
                grn = &((std::vector<std::string>*)userData)[ScenarioColors::GREEN];

            const char* s = xmlGetStringFromNode(node);

            if(*s)
            {
                char* p = strtok((char*)s, " ");

                while(p)
                {
                    grn->push_back(p);
                    p = strtok(nullptr, " ");
                }
            }
        }

        return true;
    }

    XML_TAG_HANDLER(ReadBlueString)
    {
        if(first && userData)
        {
            std::vector<std::string>* blu;

            if(loadflag & LOADING_OBJECT_CLUTTER)
            {
                ClutterObjects* co = (ClutterObjects*)userData;
                blu = &co->clutterobjects[ScenarioColors::BLUE];
            }
            else
                blu = &((std::vector<std::string>*)userData)[ScenarioColors::BLUE];

            const char* s = xmlGetStringFromNode(node);

            if(*s)
            {
                char* p = strtok((char*)s, " ");

                while(p)
                {
                    blu->push_back(p);
                    p = strtok(nullptr, " ");
                }
            }
        }

        return true;
    }

    XML_TAG_HANDLER(ReadRedInt)
    {
        if(first && userData)
        {
            std::vector<int>& red = ((std::vector<int>*)userData)[ScenarioColors::RED];

            const char* s = xmlGetStringFromNode(node);

            if(*s)
            {
                char* p = strtok((char*)s, " ");

                while(p)
                {
                    int i = atol(p);

                    if(loadflag & LOADING_GRASS)
                    {
                        GrassObjects* go = (GrassObjects*)userData;
                        go->types[ScenarioColors::RED].push_back(i);
                    }
                    else if(loadflag & LOADING_TREES)
                    {
                        TreeObjects* to = (TreeObjects*)userData;
                        to->types[ScenarioColors::RED].push_back(i);
                    }
                    else
                        red.push_back(i);

                    p = strtok(nullptr, " ");
                }
            }
        }

        return true;
    }

    XML_TAG_HANDLER(ReadGreenInt)
    {
        if(first && userData)
        {
            std::vector<int>& green = ((std::vector<int>*)userData)[ScenarioColors::GREEN];

            const char* s = xmlGetStringFromNode(node);

            if(*s)
            {
                char* p = strtok((char*)s, " ");

                while(p)
                {
                    int i = atol(p);

                    if(loadflag & LOADING_GRASS)
                    {
                        GrassObjects* go = (GrassObjects*)userData;
                        go->types[ScenarioColors::GREEN].push_back(i);
                    }
                    else if(loadflag & LOADING_TREES)
                    {
                        TreeObjects* to = (TreeObjects*)userData;
                        to->types[ScenarioColors::GREEN].push_back(i);
                    }
                    else
                        green.push_back(i);

                    p = strtok(nullptr, " ");
                }
            }
        }

        return true;
    }

    XML_TAG_HANDLER(ReadBlueInt)
    {
        if(first && userData)
        {
            std::vector<int>& blue = ((std::vector<int>*)userData)[ScenarioColors::BLUE];

            const char* s = xmlGetStringFromNode(node);

            if(*s)
            {
                char* p = strtok((char*)s, " ");

                while(p)
                {
                    int i = atol(p);

                    if(loadflag & LOADING_GRASS)
                    {
                        GrassObjects* go = (GrassObjects*)userData;
                        go->types[ScenarioColors::BLUE].push_back(i);
                    }
                    else if(loadflag & LOADING_TREES)
                    {
                        TreeObjects* to = (TreeObjects*)userData;
                        to->types[ScenarioColors::BLUE].push_back(i);
                    }
                    else
                        blue.push_back(i);

                    p = strtok(nullptr, " ");
                }
            }
        }

        return true;
    }

    XML_TAG_HANDLER(ReadBlueScale)
    {
        if(first && userData)
        {
            const char* s = xmlGetStringFromNode(node);

            if(*s)
            {
                float scale = (float)atof(s);

                if(loadflag & LOADING_GRASS)
                {
                    GrassObjects* go = (GrassObjects*)userData;
                    go->grassscales[ScenarioColors::BLUE] = scale;
                }
            }
        }

        return true;
    }

    XML_TAG_HANDLER(ReadRedScale)
    {
        if(first && userData)
        {
            const char* s = xmlGetStringFromNode(node);

            if(*s)
            {
                float scale = (float)atof(s);

                if(loadflag & LOADING_GRASS)
                {
                    GrassObjects* go = (GrassObjects*)userData;
                    go->grassscales[ScenarioColors::RED] = scale;
                }
            }
        }

        return true;
    }

    XML_TAG_HANDLER(ReadGreenScale)
    {
        if(first && userData)
        {
            const char* s = xmlGetStringFromNode(node);

            if(*s)
            {
                float scale = (float)atof(s);

                if(loadflag & LOADING_GRASS)
                {
                    GrassObjects* go = (GrassObjects*)userData;
                    go->grassscales[ScenarioColors::GREEN] = scale;
                }
            }
        }

        return true;
    }

    XML_TAG_HANDLER(ReadRedClamp)
    {
        if(first && userData)
        {
            const char* s = xmlGetStringFromNode(node);

            if(*s)
            {
                int clamp = atol(s);

                if(loadflag & LOADING_TREES)
                {
                    TreeObjects* to = (TreeObjects*)userData;
                    to->treeclamps[ScenarioColors::RED] = clamp;
                }
                else if(loadflag & LOADING_GRASS)
                {
                    GrassObjects* go = (GrassObjects*)userData;
                    go->grassclamps[ScenarioColors::RED] = clamp;
                }
                else if(loadflag & LOADING_OBJECT_CLUTTER)
                {
                    ClutterObjects* co = (ClutterObjects*)userData;
                    co->objclutterclamps[ScenarioColors::RED] = clamp;
                }
            }
        }

        return true;
    }

    XML_TAG_HANDLER(ReadGreenClamp)
    {
        if(first && userData)
        {
            const char* s = xmlGetStringFromNode(node);

            if(*s)
            {
                int clamp = atol(s);

                if(loadflag & LOADING_TREES)
                {
                    TreeObjects* to = (TreeObjects*)userData;
                    to->treeclamps[ScenarioColors::GREEN] = clamp;
                }
                else if(loadflag & LOADING_GRASS)
                {
                    GrassObjects* go = (GrassObjects*)userData;
                    go->grassclamps[ScenarioColors::GREEN] = clamp;
                }
                else if(loadflag & LOADING_OBJECT_CLUTTER)
                {
                    ClutterObjects* co = (ClutterObjects*)userData;
                    co->objclutterclamps[ScenarioColors::GREEN] = clamp;
                }
            }
        }

        return true;
    }

    XML_TAG_HANDLER(ReadBlueClamp)
    {
        if(first && userData)
        {
            const char* s = xmlGetStringFromNode(node);

            if(*s)
            {
                int clamp = atol(s);

                if(loadflag & LOADING_TREES)
                {
                    TreeObjects* to = (TreeObjects*)userData;
                    to->treeclamps[ScenarioColors::BLUE] = clamp;
                }
                else if(loadflag & LOADING_GRASS)
                {
                    GrassObjects* go = (GrassObjects*)userData;
                    go->grassclamps[ScenarioColors::BLUE] = clamp;
                }
                else if(loadflag & LOADING_OBJECT_CLUTTER)
                {
                    ClutterObjects* co = (ClutterObjects*)userData;
                    co->objclutterclamps[ScenarioColors::BLUE] = clamp;
                }
            }
        }

        return true;
    }

    XML_TAG_HANDLER(ReadBlueWeight)
    {
        if(first && userData)
        {
            const char* s = xmlGetStringFromNode(node);

            if(*s)
            {
                int weight = atol(s);

                if(loadflag & LOADING_TREES)
                {
                    TreeObjects* to = (TreeObjects*)userData;
                    to->treeweights[ScenarioColors::BLUE] = weight;
                }
                else if(loadflag & LOADING_GRASS)
                {
                    GrassObjects* go = (GrassObjects*)userData;
                    go->grassweights[ScenarioColors::BLUE] = weight;
                }
                else if(loadflag & LOADING_OBJECT_CLUTTER)
                {
                    ClutterObjects* co = (ClutterObjects*)userData;
                    co->objclutterweights[ScenarioColors::BLUE] = weight;
                }
            }
        }

        return true;
    }

    XML_TAG_HANDLER(ReadGreenWeight)
    {
        if(first && userData)
        {
            const char* s = xmlGetStringFromNode(node);

            if(*s)
            {
                int weight = atol(s);

                if(loadflag & LOADING_TREES)
                {
                    TreeObjects* to = (TreeObjects*)userData;
                    to->treeweights[ScenarioColors::GREEN] = weight;
                }
                else if(loadflag & LOADING_GRASS)
                {
                    GrassObjects* go = (GrassObjects*)userData;
                    go->grassweights[ScenarioColors::GREEN] = weight;
                }
                else if(loadflag & LOADING_OBJECT_CLUTTER)
                {
                    ClutterObjects* co = (ClutterObjects*)userData;
                    co->objclutterweights[ScenarioColors::GREEN] = weight;
                }
            }
        }

        return true;
    }

    XML_TAG_HANDLER(ReadRedWeight)
    {
        if(first && userData)
        {
            const char* s = xmlGetStringFromNode(node);

            if(*s)
            {
                int weight = atol(s);

                if(loadflag & LOADING_TREES)
                {
                    TreeObjects* to = (TreeObjects*)userData;
                    to->treeweights[ScenarioColors::RED] = weight;
                }
                else if(loadflag & LOADING_GRASS)
                {
                    GrassObjects* go = (GrassObjects*)userData;
                    go->grassweights[ScenarioColors::RED] = weight;
                }
                else if(loadflag & LOADING_OBJECT_CLUTTER)
                {
                    ClutterObjects* co = (ClutterObjects*)userData;
                    co->objclutterweights[ScenarioColors::RED] = weight;
                }
            }
        }

        return true;
    }

    bool IsLoaded() {return loaded;}
    void Reset()
    {
        loaded = false;

        fogstart = BAD_FLOAT;
        fogend = BAD_FLOAT;
        fogcolor = BAD_INT;

        whichmask = 0;
        layer = 0;

        TileMap::iterator itr = tilemap.begin();

        if(!tilemap.empty())
            itr->second->layercount = 0;

        for(; itr != tilemap.end(); ++itr)
        {
            for(unsigned int i = 0; i < itr->second->blends.size(); i++)
            {
                BlendMask* mask = itr->second->blends[i];
                delete mask;
            }

            delete itr->second;
        }

        GrassObjectMap::iterator itr2 = grassmap.begin();

        for(; itr2 != grassmap.end(); ++itr2)
        {
            delete itr2->second;
        }

        TreeObjectMap::iterator itr3 = treemap.begin();

        for(; itr3 != treemap.end(); ++itr3)
        {
            delete itr3->second;
        }

        ClutterObjectMap::iterator itr4 = cluttermap.begin();

        for(; itr4 != cluttermap.end(); ++itr4)
        {
            delete itr4->second;
        }

        tilemap.clear();
        grassmap.clear();
        treemap.clear();
        cluttermap.clear();
    }

    void GetFogParams(float& start, float& end, unsigned int& color)
    {
        if(fogstart == BAD_FLOAT)
            fogstart = piLongLikeFloat(FogStart);

        if(fogend == BAD_FLOAT)
            fogend = piLongLikeFloat(FogEnd);;

        if(fogcolor == BAD_INT)
            color = FogColor;

        start = fogstart;
        end = fogend;
        color = fogcolor;
    }

    const char* GetTerrainMaterial(unsigned int layer)
    {
        if(layer >= terrainmaterials.size())
        {
            return "default";
        }

        return terrainmaterials[layer].c_str();
    }

    void SetClutterWhichMask(unsigned int i)
    {
        whichmask = i;
    }

    unsigned int GetWhichMask()
    {
        return whichmask;
    }

private:
    bool loaded;
    unsigned int loadflag;

    std::vector<std::string> terrainmaterials;

    float fogstart;
    float fogend;
    unsigned int fogcolor;
    unsigned int whichmask;
    unsigned int layer;
};

static ExposedLoader exposedloader;

static colliderQuery* collider = nullptr;

/*--------------------------------------------------------------------
Function: InitColliderIfNecessary
Description:
---------------------------------------------------------------------*/
static void InitColliderIfNecessary()
{
    if(!collider)
    {
        collider = new colliderQuery(0, false);
        collider->addGeomToCollideWith((dGeomID)colliderTerrainTileSpace());
        collider->setMaxContactsPerHit(1);

		UINT32 col = COLLIDER_TILE;

        for(int i = 0; i < 1024; i++)
        {
            dGeomID ray = dCreateRay(collider->space(), 10000.0f);
            dGeomRaySet(ray, 0, 0, 0, 0, 0, -1);
            collider->addGeom(ray, 0, col);
        }
    }
}

/*--------------------------------------------------------------------
Function: Collide
Description:
---------------------------------------------------------------------*/
static void Collide(colliderQuery::CollisionList& list)
{
    collider->setDirty(true);

    dGeomID current = nullptr;

    if(collider->findCollisions(true))
    {
        colliderQuery::CollisionList& c = collider->getCollisions();

        for(unsigned int i = 0; i < c.size(); i++)
        {
            if(current == c[i].g1)
                continue;

            current = c[i].g1;

			// don't use blocks
			if(colliderIsType(c[i].g2, COLLIDER_BLOCK))
				continue;

            if(colliderIsType(c[i].g2, COLLIDER_TILE))
            {
                list.push_back(c[i]);
            }
        }
    }
}

/*--------------------------------------------------------------------
Function: ReadXMLFile
Description:
---------------------------------------------------------------------*/
static void ReadXMLFile()
{
    static int oldmod = 0;
    static int newmod = 0;

    std::string name = SCEN_MGR->MapPath("exposed.xml");
    static std::string current;

    fioModified(&newmod, SCEN_MGR->MapPath("exposed.xml"));

    if(newmod != oldmod)
    {
        oldmod = newmod;
        exposedloader.Reset();
    }

    if(current != name)
    {
        current = name;
        exposedloader.Reset();
    }

    if(!exposedloader.IsLoaded())
    {
        exposedloader.SetPath(SCEN_MGR->MapPath("exposed.xml"));
        fioXmlRegisterInputCallbacks();
        exposedloader.Load();
    }
}

/*--------------------------------------------------------------------
Function: TerrainScenario::GetInstance
Description:
---------------------------------------------------------------------*/
TerrainScenario& TerrainScenario::GetInstance()
{
    if(!terrainscenario.valid())
    {
        terrainscenario = new TerrainScenario;
    }

    return *terrainscenario.get();
}

/*--------------------------------------------------------------------
Function: TerrainScenario::GetFogParams
Description:
---------------------------------------------------------------------*/
void TerrainScenario::GetFogParams(float& start, float& end, unsigned int& fogcolor)
{
    exposedloader.GetFogParams(start, end, fogcolor);
}

/*--------------------------------------------------------------------
Function: GetShadowValue
Description: Retrieves alpha (shadow) value from the mask.
---------------------------------------------------------------------*/
inline static float GetShadowValue(int id, double x, double y)
{
   if(!gCustomTerrain)
        return 0.0f;

    // Translate to origin
    float xx = (float)(x - WorldOriginOffset.x + SDM_OCTET_SIZE_2);
    float yy = (float)(y - WorldOriginOffset.y + SDM_OCTET_SIZE_2);

    float u = xx / SDM_OCTET_SIZE;
    float v = 1.0f - (yy / SDM_OCTET_SIZE);

    unsigned int color;
    oglGetTextureUVColor(&color, id, 0, u, v);

    float alpha = 1.0f - piRGBA_GETALPHA(color) * colorONEOVER255;

    return alpha;
}

/*--------------------------------------------------------------------
Function: TerrainScenario::GetMaterial
Description:
---------------------------------------------------------------------*/
const char* TerrainScenario::GetMaterial(double x, double y, int id)
{
   if(!gCustomTerrain)
        return nullptr;

   TerrainTile* tt = (TerrainTile*)id;

   if(!tt)
   {
       TileMap::iterator itr = tilemap.find("maintile");

       if(itr != tilemap.end())
           tt = itr->second;
   }

   if(!tt)
       return "default";

    // Translate to origin
    float xx = (float)(x - WorldOriginOffset.x + SDM_OCTET_SIZE_2);
    float yy = (float)(y - WorldOriginOffset.y + SDM_OCTET_SIZE_2);

    for( int i = tt->blends.size() - 1; i >= 0; i--)
    {
        BlendMask& mask = *tt->blends[i];

        std::string str = SCEN_MGR->MapPath(mask.blendmask.c_str());

        if(!str.empty())
        {
            int pos = str.find(".dds");

            if(pos != -1)
            {
                str[pos] = 0;
            }

            int id = oglGetNamedTexture(str.c_str());

            if(id < 0)
                return nullptr;

            float u = xx / SDM_OCTET_SIZE;
            float v = 1.0f - (yy / SDM_OCTET_SIZE);

            unsigned int color;
            oglGetTextureUVColor(&color, id, 0, u, v);

            float r = piRGBA_GETRED(color) * colorONEOVER255;
            float g = piRGBA_GETGREEN(color) * colorONEOVER255;
            float b = piRGBA_GETBLUE(color) * colorONEOVER255;
            float a = piRGBA_GETALPHA(color) * colorONEOVER255;

            const char* mat = nullptr;

            if(b > 0.75f)
            {
                mat = mask.materials[ExposedLoader::LAYER3].c_str();
            }
            else if(g > 0.75f)
            {
                mat = mask.materials[ExposedLoader::LAYER2].c_str();
            }
            else if(r > 0.75f)
            {
                mat = mask.materials[ExposedLoader::LAYER1].c_str();
            }
            else
            {
                if(r > g && r > b && r > a)
                    mat = mask.materials[ExposedLoader::LAYER1].c_str();
                else if(g > r && g > b && g > a)
                    mat = mask.materials[ExposedLoader::LAYER2].c_str();
                else if(b > r && b > g && b > a)
                    mat = mask.materials[ExposedLoader::LAYER3].c_str();
                else if(a > r && a > g && a > b)
                    mat = mask.materials[ExposedLoader::LAYER0].c_str();
            }

            return mat;
        }
    }

    return "default";
}

/*--------------------------------------------------------------------
Function: TerrainScenario::MaskWalkBlock
Description:
---------------------------------------------------------------------*/
void TerrainScenario::MaskWalkBlock(Image* img,
                                    void* data,
                                    const Vec2f& center,
                                    int goodred,
                                    int goodgrn,
                                    int goodblu,
                                    int redweight,
                                    int grnweight,
                                    int bluweight,
                                    unsigned short redclamp,
                                    unsigned short grnclamp,
                                    unsigned short bluclamp,
                                    void (*callback)(const std::vector<Vec2f>& bucket, int color, void* data))
{
    struct DXT1TexelsBlock
    {
        unsigned short color_0;  // High color
        unsigned short color_1;  // Low color
        unsigned int texels4x4;  // Index table (2 bits per texel)
    };

    const DXT1TexelsBlock* texelsBlock = reinterpret_cast<const DXT1TexelsBlock*>(img->getMipmapData(0));
    unsigned int size = img->getTotalSizeInBytesIncludingMipmaps();

    // Only do the check on the first mipmap level
    unsigned int numBlocks = size / sizeof(DXT1TexelsBlock);

    unsigned short color[4];

    unsigned short transparent = 0;

    piVECTOR pos;
    pos.set(-SDM_OCTET_SIZE_2 + center.x(), SDM_OCTET_SIZE_2 + center.y(), 0);

    std::set<unsigned short> colormap;

    int col = 0;
    int limit = (int)sqrt((float)numBlocks);

    float xoffset = (img->s() / limit) * (SDM_OCTET_SIZE / img->s());
    float yoffset = (img->t() / limit) * (SDM_OCTET_SIZE / img->t());

    static int end = 0;

    std::vector<Vec2f> redbucket;
    std::vector<Vec2f> grnbucket;
    std::vector<Vec2f> blubucket;

    for(int i = numBlocks; i > end; --i, ++texelsBlock)
    {
        color[0] = texelsBlock->color_0;
        color[1] = texelsBlock->color_1;

        if(color[0] > color[1])
        {
            color[2] = (2 * color[0] + color[1] + 1) / 3;
            color[3] = (color[0] + 2 * color[1] + 1) / 3;
        }
        else
        {
            color[2] = (color[0] + color[1]) / 2;
            color[3] = transparent;
        }

        unsigned char index[4][4];

        unsigned int indextable = texelsBlock->texels4x4;

        for(int r = 0; r < 4; r++)
        {
            for(int c = 0; c < 4; c++)
            {
                index[r][c] = indextable & 0x03;
                indextable >>= 2;
            }
        }

        int r = PNRandom::random(0, 3);
        int c = PNRandom::random(0, 3);

        unsigned short cc = color[index[r][c]];

        unsigned short red = ((cc >> 10) & 0x1f) << 3;

        int good = PNRandom::random(0, redweight);

        if(red >= redclamp && good == goodred)
        {
            piVECTOR pp = pos;

            pp.x += PNRandom::random(0.0f, xoffset);
            pp.y -= PNRandom::random(0.0f, yoffset);

            if(pp.x >= -SDM_OCTET_SIZE_2 + center.x() && pp.x <= SDM_OCTET_SIZE_2 + center.x())
            {
                if(pp.y >= -SDM_OCTET_SIZE_2 + center.y() && pp.y <= SDM_OCTET_SIZE_2 + center.y())
                {
                    redbucket.push_back(Vec2f(pp.x, pp.y));
                }
            }
        }

        r = PNRandom::random(0, 3);
        c = PNRandom::random(0, 3);

        unsigned short green = ((cc >> 5) & 0x1f) << 3;

        good = PNRandom::random(0, grnweight);

        if(green >= grnclamp && good == goodgrn)
        {
            piVECTOR pp = pos;

            pp.x += PNRandom::random(0.0f, xoffset);
            pp.y -= PNRandom::random(0.0f, yoffset);

            if(pp.x >= -SDM_OCTET_SIZE_2 + center.x() && pp.x <= SDM_OCTET_SIZE_2 + center.x())
            {
                if(pp.y >= -SDM_OCTET_SIZE_2 + center.y() && pp.y <= SDM_OCTET_SIZE_2 + center.y())
                {
                    grnbucket.push_back(Vec2f(pp.x, pp.y));
                }
            }
        }

        r = PNRandom::random(0, 3);
        c = PNRandom::random(0, 3);

        cc = color[index[r][c]];

        unsigned short blue = (cc & 0x1f) << 3;

        good = PNRandom::random(0, bluweight);

        if(blue >= bluclamp && good == goodblu)
        {
            piVECTOR pp = pos;

            pp.x += PNRandom::random(0.0f, xoffset);
            pp.y -= PNRandom::random(0.0f, yoffset);

            if(pp.x >= -SDM_OCTET_SIZE_2 + center.x() && pp.x <= SDM_OCTET_SIZE_2 + center.x())
            {
                if(pp.y >= -SDM_OCTET_SIZE_2 + center.y() && pp.y <= SDM_OCTET_SIZE_2 + center.y())
                {
                    blubucket.push_back(Vec2f(pp.x, pp.y));
                }
            }
        }

        col++;

        pos.x += xoffset;

        if(col == limit)
        {
            col = 0;
            pos.x = (float)-SDM_OCTET_SIZE_2;
            pos.y -= yoffset;
        }
    }

    if(!redbucket.empty())
    {
        callback(redbucket, ScenarioColors::RED, data);
    }

    if(!grnbucket.empty())
    {
        callback(grnbucket, ScenarioColors::GREEN, data);
    }

    if(!blubucket.empty())
    {
        callback(blubucket, ScenarioColors::BLUE, data);
    }
}

/*--------------------------------------------------------------------
Function: TerrainScenario::MaskWalkTexel
Description:
---------------------------------------------------------------------*/
void TerrainScenario::MaskWalkTexel(Image* img,
                                    void* data,
                                    const Vec2f& center,
                                    int goodred,
                                    int goodgrn,
                                    int goodblu,
                                    int redweight,
                                    int grnweight,
                                    int bluweight,
                                    unsigned short redclamp,
                                    unsigned short grnclamp,
                                    unsigned short bluclamp,
                                    void (*callback)(const std::vector<Vec2f>& bucket, int color, void* data))
{
    struct DXT1TexelsBlock
    {
        unsigned short color_0;  // High color
        unsigned short color_1;  // Low color
        unsigned int texels4x4;  // Index table (2 bits per texel)
    };

    const DXT1TexelsBlock* texelsBlock = reinterpret_cast<const DXT1TexelsBlock*>(img->getMipmapData(0));
    unsigned int size = img->getTotalSizeInBytesIncludingMipmaps();

    // Only do the check on the first mipmap level
    unsigned int numBlocks = size / sizeof(DXT1TexelsBlock);

    unsigned short color[4];

    unsigned short transparent = 0;

    piVECTOR pos;
    pos.set(-SDM_OCTET_SIZE_2 + center.x(), SDM_OCTET_SIZE_2 + center.y(), 0);

    std::set<unsigned short> colormap;

    int col = 0;

    int limit = (int)sqrt((float)numBlocks);

    float xoffset = SDM_OCTET_SIZE / limit;
    float yoffset = SDM_OCTET_SIZE / limit;

    static int end = 0;

    std::vector<Vec2f> redbucket;
    std::vector<Vec2f> grnbucket;
    std::vector<Vec2f> blubucket;

    for(int i = numBlocks; i > end; --i, ++texelsBlock)
    {
        color[0] = texelsBlock->color_0;
        color[1] = texelsBlock->color_1;

        if(color[0] > color[1])
        {
            color[2] = (2 * color[0] + color[1] + 1) / 3;
            color[3] = (color[0] + 2 * color[1] + 1) / 3;
        }
        else
        {
            color[2] = (color[0] + color[1]) / 2;
            color[3] = transparent;
        }

        unsigned char index[4][4];

        unsigned int indextable = texelsBlock->texels4x4;

        for(int r = 0; r < 4; r++)
        {
            for(int c = 0; c < 4; c++)
            {
                index[r][c] = indextable & 0x03;
                indextable >>= 2;
            }
        }

        for(int r = 0; r < 4; r++)
        {
            for(int c = 0; c < 4; c++)
            {
                unsigned short cc = color[index[r][c]];

                unsigned short red = ((cc >> 10) & 0x1f) << 3;

                int good = PNRandom::random(0, redweight);

                if(red >= redclamp && good == goodred)
                {
                    piVECTOR pp = pos;

                    pp.x += PNRandom::random(0.0f, xoffset);
                    pp.y -= PNRandom::random(0.0f, yoffset);

                    if(pp.x >= -SDM_OCTET_SIZE_2 + center.x() && pp.x <= SDM_OCTET_SIZE_2 + center.x())
                    {
                        if(pp.y >= -SDM_OCTET_SIZE_2 + center.y() && pp.y <= SDM_OCTET_SIZE_2 + center.y())
                        {
                            redbucket.push_back(Vec2f(pp.x, pp.y));
                        }
                    }
                }

                unsigned short green = ((cc >> 5) & 0x1f) << 3;

                good = PNRandom::random(0, grnweight);

                if(green >= grnclamp && good == goodgrn)
                {
                    piVECTOR pp = pos;

                    pp.x += PNRandom::random(0.0f, xoffset);
                    pp.y -= PNRandom::random(0.0f, yoffset);

                    if(pp.x >= -SDM_OCTET_SIZE_2 + center.x() && pp.x <= SDM_OCTET_SIZE_2 + center.x())
                    {
                        if(pp.y >= -SDM_OCTET_SIZE_2 + center.y() && pp.y <= SDM_OCTET_SIZE_2 + center.y())
                        {
                            grnbucket.push_back(Vec2f(pp.x, pp.y));
                        }
                    }
                }

                unsigned short blue = (cc & 0x1f) << 3;

                good = PNRandom::random(0, bluweight);

                if(blue >= bluclamp && good == goodblu)
                {
                    piVECTOR pp = pos;

                    pp.x += PNRandom::random(0.0f, xoffset);
                    pp.y -= PNRandom::random(0.0f, yoffset);

                    if(pp.x >= -SDM_OCTET_SIZE_2 + center.x() && pp.x <= SDM_OCTET_SIZE_2 + center.x())
                    {
                        if(pp.y >= -SDM_OCTET_SIZE_2 + center.y() && pp.y <= SDM_OCTET_SIZE_2 + center.y())
                        {
                            blubucket.push_back(Vec2f(pp.x, pp.y));
                        }
                    }
                }
            }
        }

        col++;

        pos.x += xoffset;

        if(col == limit)
        {
            col = 0;
            pos.x = -SDM_OCTET_SIZE_2;
            pos.y -= yoffset;
        }
    }

    if(!redbucket.empty())
    {
        callback(redbucket, ScenarioColors::RED, data);
    }

    if(!grnbucket.empty())
    {
        callback(grnbucket, ScenarioColors::GREEN, data);
    }

    if(!blubucket.empty())
    {
        callback(blubucket, ScenarioColors::BLUE, data);
    }
}

/*--------------------------------------------------------------------
Function: HandleTreeObject
Description:
---------------------------------------------------------------------*/
static void HandleTreeObject(const std::vector<Vec2f>& bucket, int color, void* data)
{
    InitColliderIfNecessary();

    TerrainTile* tt = (TerrainTile*)data;
    TreeObjects* to = treemap.find(tt->treemask)->second;

    colliderQuery::dGeomList& geoms = collider->geoms();

    int count = 0;
    colliderQuery::CollisionList list;

    for(unsigned int i = 0; i < geoms.size(); i++)
        dGeomDisable(geoms[i]);

    for(unsigned int i = 0; i < bucket.size(); i++)
    {
        dGeomID g = geoms[count++];

        dGeomEnable(g);

        dGeomRaySet(g, bucket[i].x(), bucket[i].y(), TerrainScenario::GetInstance().GetBBoxHeight() + 10.0f, 0, 0, -1);

        if(count >= 1024)
        {
            count = 0;
            Collide(list);

            for(unsigned int j = 0; j < 1024; j++)
                dGeomDisable(geoms[j]);
        }
    }

    if(count)
    {
        Collide(list);
    }

    int type = -1;

    Vec3f pos;

    std::string str = SCEN_MGR->MapPath(tt->blends.back()->blendmask.c_str());

    int id = -1;

    if(!str.empty())
    {
        int strpos = str.find(".dds");

        if(strpos != -1)
        {
            str[strpos] = 0;
        }

        id = oglGetNamedTexture(str.c_str());
    }

    for(unsigned int i = 0; i < list.size(); i++)
    {
        int seed = (int)((SINT64)list[i].pos.x() * (SINT64)113510315245LL *
                         (SINT64)list[i].pos.y() * (SINT64)103515051525LL);

        pos.set((float)list[i].pos.x(), (float)list[i].pos.y(), (float)-list[i].pos.z());

        if(color == ScenarioColors::RED)
        {
            type = to->GetTreeType(ScenarioColors::RED, seed);
        }

        if(color == ScenarioColors::GREEN)
        {
            type = to->GetTreeType(ScenarioColors::GREEN, seed);
        }

        if(color == ScenarioColors::BLUE)
        {
            type = to->GetTreeType(ScenarioColors::BLUE, seed);
        }

        float shadow = GetShadowValue(id, list[i].pos.x(), list[i].pos.y());

        if(type > -1)
        {
            pos.x() -= (float)WorldOriginOffset.x;
            pos.y() -= (float)WorldOriginOffset.y;

            sptAddTreeCommand(type, pos, true, shadow);
        }
    }
}

/*--------------------------------------------------------------------
Function: HandleGrassObject
Description:
---------------------------------------------------------------------*/
static void HandleGrassObject(const std::vector<Vec2f>& bucket, int color, void* data)
{
    InitColliderIfNecessary();

    TerrainTile* tt = (TerrainTile*)data;
    GrassObjects* go = grassmap.find(tt->grassmask)->second;

    colliderQuery::dGeomList& geoms = collider->geoms();

    int count = 0;
    colliderQuery::CollisionList list;

    for(unsigned int i = 0; i < geoms.size(); i++)
        dGeomDisable(geoms[i]);

    for(unsigned int i = 0; i < bucket.size(); i++)
    {
        dGeomID g = geoms[count++];

        dGeomEnable(g);

        dGeomRaySet(g, bucket[i].x(), bucket[i].y(), TerrainScenario::GetInstance().GetBBoxHeight() + 10.0f, 0, 0, -1);

        if(count >= 1024)
        {
            count = 0;
            Collide(list);

            for(unsigned int j = 0; j < 1024; j++)
                dGeomDisable(geoms[j]);
        }
    }

    if(count)
    {
        Collide(list);
    }

    std::string str = SCEN_MGR->MapPath(tt->blends.back()->blendmask.c_str());

    int splatid = -1;

    if(!tt->splatmask.empty())
    {
        std::string splat = SCEN_MGR->MapPath(tt->splatmask.c_str());
        splatid = ldTextureLoad(splat.c_str(), LDPIX_CLAMP);
    }

    int id = -1;

    if(!str.empty())
    {
        int pos = str.find(".dds");

        if(pos != -1)
        {
            str[pos] = 0;
        }

        id = oglGetNamedTexture(str.c_str());
    }

    for(unsigned int i = 0; i < list.size(); i++)
    {
        list[i].pos.z() = -list[i].pos.z();

        int seed = (int)((SINT64)list[i].pos.x() * (SINT64)113510315245LL *
                         (SINT64)list[i].pos.y() * (SINT64)103515051525LL);

        float shadow = GetShadowValue(id, list[i].pos.x(), list[i].pos.y());

        if(color == ScenarioColors::RED)
        {
            speedgrassAddCoverGrass(list[i].pos,
                                    SpeedGrassDeprecated::type_userdefined,
                                    0,
                                    go->GetGrassScale(ScenarioColors::RED),
                                    go->GetGrassType(ScenarioColors::RED, seed),
                                    shadow,
                                    splatid);
        }

        if(color == ScenarioColors::GREEN )
        {
            speedgrassAddCoverGrass(list[i].pos,
                                    SpeedGrassDeprecated::type_userdefined,
                                    0,
                                    go->GetGrassScale(ScenarioColors::GREEN),
                                    go->GetGrassType(ScenarioColors::GREEN, seed),
                                    shadow,
                                    splatid);
        }

        if(color == ScenarioColors::BLUE)
        {
            speedgrassAddCoverGrass(list[i].pos,
                                    SpeedGrassDeprecated::type_userdefined,
                                    0,
                                    go->GetGrassScale(ScenarioColors::BLUE),
                                    go->GetGrassType(ScenarioColors::BLUE, seed),
                                    shadow,
                                    splatid);
        }
    }
}

/*--------------------------------------------------------------------
Function: HandleClutterObject
Description:
---------------------------------------------------------------------*/
static void HandleClutterObject(const std::vector<Vec2f>& bucket, int color, void* data)
{
    InitColliderIfNecessary();

    TerrainTile* tt = (TerrainTile*)data;
    ClutterObjects* co = cluttermap.find(tt->cluttermask[exposedloader.GetWhichMask()])->second;

    colliderQuery::dGeomList& geoms = collider->geoms();

    int count = 0;
    colliderQuery::CollisionList list;

    for(unsigned int i = 0; i < geoms.size(); i++)
        dGeomDisable(geoms[i]);

    for(unsigned int i = 0; i < bucket.size(); i++)
    {
        dGeomID g = geoms[count++];

        dGeomEnable(g);

        dGeomRaySet(g, bucket[i].x(), bucket[i].y(), TerrainScenario::GetInstance().GetBBoxHeight() + 10.0f, 0, 0, -1);

        if(count >= 1024)
        {
            count = 0;
            Collide(list);

            for(unsigned int j = 0; j < 1024; j++)
                dGeomDisable(geoms[j]);
        }
    }

    if(count)
    {
        Collide(list);
    }

    for(unsigned int i = 0; i < list.size(); i++)
    {
        int seed = (int)((SINT64)list[i].pos.x() * (SINT64)113510315245LL *
                         (SINT64)list[i].pos.y() * (SINT64)103515051525LL);

        char buff[256];
        const char* obj;
        float rot = 0;

        if(color == ScenarioColors::RED)
        {
            rot = PNRandom::random(-co->GetClutterRotation(ScenarioColors::RED), co->GetClutterRotation(ScenarioColors::RED));
            obj = co->GetClutterObject(ScenarioColors::RED, seed);
        }

        if(color == ScenarioColors::GREEN)
        {
            rot = PNRandom::random(-co->GetClutterRotation(ScenarioColors::GREEN), co->GetClutterRotation(ScenarioColors::GREEN));
            obj = co->GetClutterObject(ScenarioColors::GREEN, seed);
        }

        if(color == ScenarioColors::BLUE)
        {
            rot = PNRandom::random(-co->GetClutterRotation(ScenarioColors::BLUE), co->GetClutterRotation(ScenarioColors::BLUE));
            obj = co->GetClutterObject(ScenarioColors::BLUE, seed);
        }

        if(obj)
        {
            sprintf(buff, "clutter %s %f %f %f 0 0 %f 0 0 0 0", obj, (float)list[i].pos.x(), (float)list[i].pos.y(), (float)list[i].pos.z(), rot);
            SCEN_MGR->AddContent(buff);
        }
    }
}

/*--------------------------------------------------------------------
Function: TerrainScenario::LoadClutterObjects
Description:
---------------------------------------------------------------------*/
void TerrainScenario::LoadClutterObjects(const OBJECT* o, void* data)
{
    extern Image* ReadDDSFile(const std::string& name);

    TerrainTile* tt = (TerrainTile*)data;

    PNRandom::seedrandom(1309);

    for(unsigned int i = 0; i < tt->cluttermask.size(); i++)
    {
        exposedloader.SetClutterWhichMask(i);

        ClutterObjectMap::iterator itr = cluttermap.find(tt->cluttermask[i]);

        if(itr == cluttermap.end())
            continue;

        ClutterObjects* co = itr->second;

        Image* img = ReadDDSFile(SCEN_MGR->MapPath(itr->first.c_str()));

        if(!img)
            continue;

        int grnweight = co->GetClutterWeight(ScenarioColors::GREEN);
        int redweight = co->GetClutterWeight(ScenarioColors::RED);
        int bluweight = co->GetClutterWeight(ScenarioColors::BLUE);

        int goodgrn = PNRandom::random(0, grnweight);
        int goodred = PNRandom::random(0, redweight);
        int goodblu = PNRandom::random(0, bluweight);

        unsigned short redclamp = co->GetClutterClamp(ScenarioColors::RED);
        unsigned short greenclamp = co->GetClutterClamp(ScenarioColors::GREEN);
        unsigned short blueclamp = co->GetClutterClamp(ScenarioColors::BLUE);

        Vec2f center;
        center.x() = o->modelInstance->GetBounds().center().x();
        center.y() = o->modelInstance->GetBounds().center().y();

        MaskWalkBlock(img,
                      tt,
                      center,
                      goodred,
                      goodgrn,
                      goodblu,
                      redweight,
                      grnweight,
                      bluweight,
                      redclamp,
                      greenclamp,
                      blueclamp,
                      HandleClutterObject);

        img->unref();
    }
}

/*--------------------------------------------------------------------
Function: TerrainScenario::LoadGrass
Description:
---------------------------------------------------------------------*/
void TerrainScenario::LoadGrass(const OBJECT* o, void* data)
{
    extern Image* ReadDDSFile(const std::string& name);

    TerrainTile* tt = (TerrainTile*)data;

    if(tt->grassmask.empty())
        return;

    GrassObjects* go = grassmap.find(tt->grassmask)->second;

    if(!go)
        return;

    Image* img = ReadDDSFile(SCEN_MGR->MapPath(tt->grassmask.c_str()));

    if(!img)
        return;

    PNRandom::seedrandom(1309);

    int grnweight = go->GetGrassWeight(ScenarioColors::GREEN);
    int redweight = go->GetGrassWeight(ScenarioColors::RED);
    int bluweight = go->GetGrassWeight(ScenarioColors::BLUE);

    int goodgrn = PNRandom::random(0, grnweight);
    int goodred = PNRandom::random(0, redweight);
    int goodblu = PNRandom::random(0, bluweight);

    unsigned short redclamp = go->GetGrassClamp(ScenarioColors::RED);
    unsigned short greenclamp = go->GetGrassClamp(ScenarioColors::GREEN);
    unsigned short blueclamp = go->GetGrassClamp(ScenarioColors::BLUE);

    Vec2f center;
    center.x() = o->modelInstance->GetBounds().center().x();
    center.y() = o->modelInstance->GetBounds().center().y();

    MaskWalkTexel(img,
                  tt,
                  center,
                  goodred,
                  goodgrn,
                  goodblu,
                  redweight,
                  grnweight,
                  bluweight,
                  redclamp,
                  greenclamp,
                  blueclamp,
                  HandleGrassObject);

    img->unref();
}

/*--------------------------------------------------------------------
Function: TerrainScenario::LoadTrees
Description:
---------------------------------------------------------------------*/
void TerrainScenario::LoadTrees(const OBJECT* o, void* data)
{
    TerrainTile* tt = (TerrainTile*)data;

    if(tt->treemask.empty())
        return;

    extern Image* ReadDDSFile(const std::string& name);

    Image* img = ReadDDSFile(SCEN_MGR->MapPath(tt->treemask.c_str()));

    if(!img)
        return;

    PNRandom::seedrandom(1309);

    TreeObjects* to = treemap.find(tt->treemask)->second;

    if(!to)
    {
        img->unref();
        return;
    }

    int grnweight = to->GetTreeWeight(ScenarioColors::GREEN);
    int redweight = to->GetTreeWeight(ScenarioColors::RED);
    int bluweight = to->GetTreeWeight(ScenarioColors::BLUE);

    int goodgrn = PNRandom::random(0, grnweight);
    int goodred = PNRandom::random(0, redweight);
    int goodblu = PNRandom::random(0, bluweight);

    unsigned short redclamp = to->GetTreeClamp(ScenarioColors::RED);
    unsigned short greenclamp = to->GetTreeClamp(ScenarioColors::GREEN);
    unsigned short blueclamp = to->GetTreeClamp(ScenarioColors::BLUE);

    Vec2f center;
    center.x() = o->modelInstance->GetBounds().center().x();
    center.y() = o->modelInstance->GetBounds().center().y();

    MaskWalkBlock(img,
                  tt,
                  center,
                  goodred,
                  goodgrn,
                  goodblu,
                  redweight,
                  grnweight,
                  bluweight,
                  redclamp,
                  greenclamp,
                  blueclamp,
                  HandleTreeObject);

    img->unref();
}

/*--------------------------------------------------------------------
Function: TerrainScenario::LoadEnvironment
Description:
---------------------------------------------------------------------*/
void TerrainScenario::LoadEnvironment(unsigned int flags)
{
    LoadedTerrainMap::iterator itr = loadedmap.begin();

    piVECTOR dummy;
    sptAddTreeCommand(-1, dummy);

    speedgrassRemoveAll();

    ReadXMLFile();

    for(; itr != loadedmap.end(); ++itr)
    {
        const std::string& flt = itr->first;
        const std::string& key = itr->second;

        OBJECT* o = obmainFindTopmostObject(flt.c_str());

        if(!o)
        {
            o = obmgrCreateModelInstance(flt.c_str());
        }

        if(!o)
            continue;

        TileMap::iterator i = tilemap.find(key);

        if(i != tilemap.end())
        {
            LoadTrees(o, i->second);
            LoadClutterObjects(o, i->second);

            if(!(flags & TS_NO_GRASS))
            {
                LoadGrass(o, i->second);
            }
        }
    }

    loadedmap.clear();
}

#if USE_STATESET
/*--------------------------------------------------------------------
Function: AlphaMaskDraw
Description:
---------------------------------------------------------------------*/
static bool AlphaMaskDraw(OBJECT* o)
{
    oglSetWorldMatrix(&o->mat);

    Texture2D* tex = ShadowMap::GetInstance().GetShadowMapTexture();
    const Matrixf& mvp = ShadowMap::GetInstance().GetInverseMVP();

    TextureUnit* tu;
    tu = dynamic_cast<TextureUnit*>(o->material->GetRenderState(RS_TEXTURE_UNIT2));

    tu->SetTexture(tex);

    TextureMatrix* tm;
    tm = dynamic_cast<TextureMatrix*>(o->material->GetRenderState(RS_TEXTURE_MATRIX2));

    tm->SetMatrix(mvp);

    TerrainTile* tt = (TerrainTile*)o->soundID;

    if(tt)
    {
        Program* pgm = dynamic_cast<Program*>(o->material->GetRenderState(RS_GLSLPROGRAM));
        glslUniform* u = pgm->GetProgram()->GetUniform("scale");

        int idx = 0;

        for(unsigned int i = 0; i < tt->blends.size(); i++)
        {
            BlendMask& mask = *tt->blends[i];

            u->SetElement(idx++, mask.multitexturescale[0]);
            u->SetElement(idx++, mask.multitexturescale[1]);
            u->SetElement(idx++, mask.multitexturescale[2]);
            u->SetElement(idx++, mask.multitexturescale[3]);
            u->SetElement(idx++, mask.multitexturescale[4]);
        }
    }

    o->material->Apply();

    obdrawDrawMesh(o);

    return true;
}
#else
/*--------------------------------------------------------------------
Function: AlphaMaskDraw
Description:
---------------------------------------------------------------------*/
static bool AlphaMaskDraw(OBJECT* o)
{
    oglSetWorldMatrix(&o->mat);

    ShadowMap::GetInstance().SetShadowStuff(*o->material, 2);

    TerrainTile* tt = (TerrainTile*)o->soundID;

    if(tt)
    {
        glslUniform* u = o->material->GetProgram()->GetUniform("scale");

        int idx = 0;

        for(unsigned int i = 0; i < tt->blends.size(); i++)
        {
            BlendMask& mask = *tt->blends[i];

            u->SetElement(idx++, mask.multitexturescale[0]);
            u->SetElement(idx++, mask.multitexturescale[1]);
            u->SetElement(idx++, mask.multitexturescale[2]);
            u->SetElement(idx++, mask.multitexturescale[3]);
            u->SetElement(idx++, mask.multitexturescale[4]);
        }
    }

    o->material->Apply();

    oglSetRenderState(oglRENDERSTATE_LIGHTING, 0);
    obdrawDrawMesh(o);

    oglSetRenderState(oglRENDERSTATE_LIGHTING, piLIGHTING_DEFAULT);

    ShadowMap::GetInstance().UnApplyShadowMaps(2);

    return true;
}
#endif

#if USE_STATESET
struct AddMultiTexture
{
    OBJECT* o;
    int Mask;
    int layer0;
    int layer1;
    int layer2;
    int layer3;
    int layer4;
    TerrainTile* tt;
    ref_ptr<RenderStateSet> material;
    std::string key;

    void Apply(OBJECT* o)
    {
        if(o)
        {
            if(!(o->flags & OBJECT_NOTOBJECT) && o->numverts > 0)
            {
                o->material = material.get();
                o->material->ref();

                Material* material = dynamic_cast<Material*>(o->material->GetRenderState(RS_MATERIAL));
                material->SetMaterialID(o->matid);

                o->soundID = (int)tt; // Sound ID won't be used for terrain.

                o->drawcallback = AlphaMaskDraw;
            }

            Apply(o->child);
            Apply(o->sibling);
            Apply(o->attached);
        }
    }

    void Add()
    {
        TileMap::iterator itr = tilemap.find(key);

        tt = nullptr;

        if(itr != tilemap.end())
        {
            extern Texture2D* obdrawGetSurfaceTexture();

            material = new RenderStateSet;

            tt = itr->second;

            char num[16];
            sprintf(num, "%d", tt->layercount);

            material->SetRenderState(new Program(tt->shader + num));
            std::vector<std::string> textures;

            for(unsigned int i = 0; i < tt->blends.size(); i++)
            {
                BlendMask& mask = *tt->blends[i];

                if(!mask.layers[0].empty())
                    textures.push_back(SCEN_MGR->MapPath(mask.layers[0].c_str()));
                if(!mask.layers[1].empty())
                    textures.push_back(SCEN_MGR->MapPath(mask.layers[1].c_str()));
                if(!mask.layers[2].empty())
                    textures.push_back(SCEN_MGR->MapPath(mask.layers[2].c_str()));
                if(!mask.layers[3].empty())
                    textures.push_back(SCEN_MGR->MapPath(mask.layers[3].c_str()));
                if(!mask.layers[4].empty())
                    textures.push_back(SCEN_MGR->MapPath(mask.layers[4].c_str()));
            }

            layer0 = ldLoadTexture2DArray(SCEN_MGR->MapName(),
                                          textures,
                                          Texture::REPEAT,
                                          Texture::REPEAT);


            std::vector<int> ids;

            for(unsigned int i = 0; i < tt->blends.size(); i++)
            {
                BlendMask& mask = *tt->blends[i];
                ids.push_back(ldTextureLoad(SCEN_MGR->MapPath(mask.blendmask.c_str()), LDPIX_CREATE_SURFACE_DATA | LDPIX_CLAMP));
            }

            material->SetRenderState(new TextureUnit(0, layer0));  // Layer 0
            material->SetRenderState(new TextureUnit(1, obdrawGetSurfaceTexture())); // Surface layer
            material->SetRenderState(new TextureUnit(2, -1));          // Shadow layer
            material->SetRenderState(new Material);

            material->SetRenderState(new TextureMatrix(2));

            if(ids.size() > 0)
                material->SetRenderState(new TextureUnit(3, ids[0]));    // Mask
            if(ids.size() > 1)
                material->SetRenderState(new TextureUnit(4, ids[1]));    // Mask

            material->SetMode(GL_LIGHTING, 0);

            Apply(o);
        }
    }
};
#else
struct AddMultiTexture
{
    OBJECT* o;
    int Mask;
    int layer0;
    int layer1;
    int layer2;
    int layer3;
    int layer4;
    TerrainTile* tt;
    ref_ptr<oglMaterial> material;
    std::string key;

    void Apply(OBJECT* o)
    {
        if(o)
        {
            if(!(o->flags & OBJECT_NOTOBJECT) && o->numverts > 0)
            {
                o->material = material.get();
                o->material->ref();
                o->material->SetMaterial(o->matid);

                o->soundID = (int)tt; // Sound ID won't be used for terrain.

                o->drawcallback = AlphaMaskDraw;
            }

            Apply(o->child);
            Apply(o->sibling);
            Apply(o->attached);
        }
    }

    void Add()
    {
        TileMap::iterator itr = tilemap.find(key);

        tt = nullptr;

        if(itr != tilemap.end())
        {
            extern Texture2D* obdrawGetSurfaceTexture();

            material = new oglMaterial;

            tt = itr->second;

            char num[16];
            sprintf(num, "%d", tt->layercount);

            material->SetShader(tt->shader + num);
            std::vector<std::string> textures;

            for(unsigned int i = 0; i < tt->blends.size(); i++)
            {
                BlendMask& mask = *tt->blends[i];

                if(!mask.layers[0].empty())
                    textures.push_back(SCEN_MGR->MapPath(mask.layers[0].c_str()));
                if(!mask.layers[1].empty())
                    textures.push_back(SCEN_MGR->MapPath(mask.layers[1].c_str()));
                if(!mask.layers[2].empty())
                    textures.push_back(SCEN_MGR->MapPath(mask.layers[2].c_str()));
                if(!mask.layers[3].empty())
                    textures.push_back(SCEN_MGR->MapPath(mask.layers[3].c_str()));
                if(!mask.layers[4].empty())
                    textures.push_back(SCEN_MGR->MapPath(mask.layers[4].c_str()));
            }

            layer0 = ldLoadTexture2DArray(SCEN_MGR->MapName(),
                                          textures,
                                          Texture::REPEAT,
                                          Texture::REPEAT);


            std::vector<int> ids;

            for(unsigned int i = 0; i < tt->blends.size(); i++)
            {
                BlendMask& mask = *tt->blends[i];
                ids.push_back(ldTextureLoad(SCEN_MGR->MapPath(mask.blendmask.c_str()), LDPIX_CREATE_SURFACE_DATA));
            }

            material->AddTexture(0, layer0);  // Layer 0
            material->AddTexture(1, obdrawGetSurfaceTexture()); // Surface layer
            material->AddTexture(2, -1);          // Shadow layer

            if(ids.size() > 0)
                material->AddTexture(3, ids[0]);    // Mask
            if(ids.size() > 1)
                material->AddTexture(4, ids[1]);    // Mask

            Apply(o);
        }
    }
};
#endif

/*--------------------------------------------------------------------
Function: DeleteBuildings
Description: Delete buildings
---------------------------------------------------------------------*/
static void DeleteBuildings()
{
    std::string tspawn = ".tspawn -deletebuildings";
    uiDotCommand(tspawn.c_str());
}

struct AdjustMatrix
{
    float zoffset;
    OBJECT* o;

    void apply(OBJECT* o)
    {
        if(o)
        {
            if(!(o->flags & OBJECT_NOTOBJECT))
            {
                if(o->numverts > 0)
                {
                    o->mat._43 -= zoffset;
                }
            }

            apply(o->child);
            apply(o->sibling);
            apply(o->attached);
        }
    }

    void adjust()
    {
        apply(o);
    }
};

/*--------------------------------------------------------------------
Function: damageObject
Description:
---------------------------------------------------------------------*/
static void damageObject(OBJECT* o)
{
    if(o->modelInstance)
    {
        OBJECT* oo = o->modelInstance->rootObject->child;

        while(oo)
        {
            trnCDTerrainObject* TO = trncdFindTerrainObject(oo);

            if(TO)
            {
                if(TO->DamageLevel == 0)
                    trnSetDamageLevel(TO, 1);
                else
                {
                    trnSetDamageLevel(TO, 0);

                    if(TO->LastFXDamageLevel == 1)
                    {
                        TO->LastFXDamageLevel = 0;
                    }
                }
            }

            oo = oo->sibling;
        }

        char model[128];

        PoolModel* pm = obmgrGetPoolModel(o->modelInstance->GetModelName());

        if(pm)
        {
            for(unsigned int i = 0; i < pm->subModelReferences.size(); i++)
            {
                sprintf(model, "%s_%d", pm->subModelReferences[i].poolModel->modelname.c_str(), i + 1);

                OBJECT* ooo = obmainFindTopmostObject(model);

                if(!ooo)
                    continue;

                ooo = ooo->child;

                while(ooo)
                {
                    trnCDTerrainObject* TO = trncdFindTerrainObject(ooo);

                    if(TO)
                    {
                        if(TO->DamageLevel == 0)
                            trnSetDamageLevel(TO, 1);
                        else
                        {
                            trnSetDamageLevel(TO, 0);

                            if(TO->LastFXDamageLevel == 1)
                            {
                                TO->LastFXDamageLevel = 0;
                            }
                        }
                   }

                    ooo = ooo->sibling;
                }
            }
        }
    }
}

/*--------------------------------------------------------------------
Function: TerrainScenario::LoadBuildings
Description:
---------------------------------------------------------------------*/
void TerrainScenario::LoadBuildings()
{
    DeleteBuildings();

    // write buffer
    char buffer[1023+1];

    int wfd = fioOpenWrite("buildings.txt");
    int fd = fioOpen(SCEN_MGR->MapPath("Clip_TownLayout_V2.txt"));

    std::string tspawn;
    std::string name;

    float zoffset = 0.0f;
    bool damage = false;
    piVECTOR rot;

    if(fd > 0)
    {
        tspawn.clear();

        char buff[80];
        char str[64];

        char* p;
        int count = 0;

        while(fioGetString(fd, buff, 79))
        {
            if((p = strstr(buff, "zoffset")))
            {
                if(strchr(p, '='))
                {
                    sscanf(p, "%*s %*c %f", &zoffset);
                }
            }

            if((p = strstr(buff, "damage")))
            {
                damage = true;
            }

            if((p = strstr(buff, "filename")))
            {
                if(!tspawn.empty())
                {
                    uiDotCommand(tspawn.c_str());
                    tspawn.clear();

                    if(zoffset && !name.empty())
                    {
                        OBJECT* o = obmainFindTopmostObject(name.c_str());

                        if(o)
                        {
                            AdjustMatrix am;
                            am.o = o;
                            am.zoffset = zoffset;
                            am.adjust();
                        }
                    }

                    if(damage)
                    {
                        OBJECT* o = obmainFindTopmostObject(name.c_str());
                        assert(o);
                        damageObject(o);
                    }

                    piVECTOR& finalPos = uiDotGetPosXYZ();

                    snprintf(buffer, 1023, "\"place %s %.2f %.2f %.2f %d %d %d 0 0 0 %d\",\n",
                        name.c_str(),
                        finalPos.x,
                        finalPos.y,
                        -finalPos.z + zoffset,
                        (int)rot.x,
                        (int)rot.y,
                        (int)rot.z,
                        (damage) ? 1 : 0);

                    count++;

                    fioWrite(wfd, buffer, strlen(buffer));

                }

                zoffset = 0;
                name.clear();
                damage = false;
                rot.set(0,0,0);

                tspawn = ".tspawn ";

                p += 12;
                int len = strlen(p) - 1;
                strncpy(str, p, len);
                str[len] = 0;

                name = str;

                tspawn += str;

                continue;
            }

            if((p = strstr(buff, "translate")))
            {
                p += 13;
                int len = strlen(p) - 1;
                strncpy(str, p, len);
                str[len] = 0;
                tspawn += " -pos ";
                tspawn += str;

                continue;
            }

            if((p = strstr(buff, "rotate")))
            {
                p += 10;
                int len = strlen(p) - 1;
                strncpy(str, p, len);
                str[len] = 0;
                tspawn += " -rot ";
                tspawn += str;

                // z and y are backwards, yes
                sscanf(str, "%f %f %f", &rot.x, &rot.z, &rot.y);

                continue;
            }
        }

        if(!tspawn.empty())
        {
            uiDotCommand(tspawn.c_str());
            tspawn.clear();

            if(zoffset && !name.empty())
            {
                OBJECT* o = obmainFindTopmostObject(name.c_str());

                if(o)
                {
                    AdjustMatrix am;
                    am.o = o;
                    am.zoffset = zoffset;
                    am.adjust();
                }

                zoffset = 0;
                name.clear();
            }

            piVECTOR& finalPos = uiDotGetPosXYZ();

            snprintf(buffer, 1023, "\"place %s %.2f %.2f %.2f %d %d %d 0 0 0 %d\",\n",
                name.c_str(),
                finalPos.x,
                finalPos.y,
                -finalPos.z + zoffset,
                (int)rot.x,
                (int)rot.y,
                (int)rot.z,
                (damage) ? 1 : 0);

            count++;

            fioWrite(wfd, buffer, strlen(buffer));
        }

        fioClose(fd);
        fioClose(wfd);
    }
}

/*--------------------------------------------------------------------
Function: TerrainScenario::LoadTerrain
Description:
---------------------------------------------------------------------*/
void TerrainScenario::LoadTerrain(const std::string& fltname, const std::string& key)
{
    OBJECT* o = obmainFindTopmostObject(fltname.c_str());

    if(!o)
    {
        o = obmgrCreateModelInstance(fltname.c_str());
    }

    if(!o)
        return;

    max_bound_box_height = -o->modelInstance->GetBounds().zMin();

    if(o)
    {
        ReadXMLFile();

        AddMultiTexture add;
        add.key = key;
        add.o = o;
        add.Add();

        loadedmap[fltname] = key;
    }
}
