#include "syscfg.h" // Path is project specific.
//

#include <math.h>
#include "pi/pi.h"
#include "include/vars.h"
#include "ob/obmain.h"
#include "ob/obdraw.h"
#include "ob/obplyr.h"
#include "ob/obc.h"
#include "ob/obdlite.h"
#include "ob/obpool.h"
#include "ogrp/ogrptype.h"
#include "trn/trnmain.h"
#include "trn/trnsky.h"
#include "trn/trnwtr.h"
#include "ld/ldmdl.h"
#include "math/math3d.h"
#include "pers/persmain.h"
#include "ml/mlutility.h" // for deg/rad converters
#include "trn/trntod.h"
#include "trn/trnop.h"
#include "rn/rnmain.h"
#include "math/mathvect.h"
#include "math/matheul.h"
#include "fx/FXInterface.h"
#include "rn/rnvis.h"
#include "trn/trncloud.h"
#include "weap/weapSpec.h"
#include "ob/obMgr.h"
#include "fx/fxWeather.h"
#include "ogl/oglGlobalStates.h"
#include "ogl/oglMaterial.h"
#include "math/mathnum.h"
#include "ld/ldtexture.h"
#include "trn/trnScenario.h"
#include "ogl/States.h"

#include <vector>
#include "trn/trndata.h"

#include "draw/draw3d.h"

#include "scen/ScenarioManager.h"
static int skyboxtextures[5] = {-1, -1, -1, -1, -1};

#ifdef USE_LENS_FLARE
#include "trn/LensFlare.h"
static FLOAT32 g_distanceScale = 1.0f;
static FLOAT32 g_halfMaxBright = 0.45f;

#endif // USE_LENS_FLARE

static FLOAT32 _startdensity = 0.000289f;
static FLOAT32 _enddensity = 0.000194f;

#define NUM_STARS				512

static SINT32 _setting = -1;
static SINT32 _fogoption = -1;
static int skyboxtexture = -1;

static SINT32		meshes;
static OBJECT*		sky, *horiz, *horiztop;
static OBJECT*		sundisk, *moondisk;
static FLOAT32		sunelevation, moonelevation;//, suntilt;
static FLOAT32		_horizheight;
static FLOAT32		_SunAngle;
static UINT32		_FogColor;

static LIGHT*		_SunLight;
static LIGHT*		_MoonLight;
static FLOAT32		_SkyDomeScaleFactor;

static FLOAT32 g_afterbright;
static FLOAT32 g_beforebright;
static FLOAT32 _altitude = 0.0f;
static float rainfogclip = 1.0f;

static piVECTOR		_StarField[NUM_STARS];

static void _UpdateSunLight(LIGHT* light, const TERRAINSKY* s);
static void _UpdateMoonLight(LIGHT* light, const TERRAINSKY* s);
static UINT32 _InitSun();
static UINT32 _InitMoon();
static void _UpdateFogSphere();
static UINT32 _ColorObject(OBJECT * Object,UINT32 Color);
static UINT32 _ColorSubObject(OBJECT * Object,UINT32 Color);
static UINT32 _DrawStarsDeprecated();

static bool _moonfirsttime = true;
static bool _sunfirsttime = true;

static void _setStartEndDensity();
static void InterpolateIfNecessary(TERRAINSKY& c);

#if USE_STATESET
static ref_ptr<RenderStateSet> starmaterial;
static ref_ptr<RenderStateSet> skyboxmaterial;
#else
static ref_ptr<oglMaterial> starmaterial;
static ref_ptr<oglMaterial> skyboxmaterial;
#endif

static ref_ptr<oglVertexBuffer> vbo;

static void _setproperties()
{
    TERRAINSKY* s = trndataGetSkyData(_setting);

    if(!s)
    {
        return;
    }

    _setting = s->setting - 1;

#ifdef BETA_RELEASE
    rnmainListText("Light for setting %d", s->setting);

    FOGOPTION* f = trndataGetFogData(_setting, _fogoption);

    if(f)
    {
        rnmainListText("Fog for option %d", f->fogoption);
    }
#endif

    _setStartEndDensity();

#if(0)
    g_colors[IDE_SKY_COLOR_RED] = s->_skycolor.r;
    g_colors[IDE_SKY_COLOR_GREEN] = s->_skycolor.g;
    g_colors[IDE_SKY_COLOR_BLUE] = s->_skycolor.b;

    g_colors[IDE_SKY_COLOR_NIGHT_RED] = s->_skycolornight.r;
    g_colors[IDE_SKY_COLOR_NIGHT_GREEN] = s->_skycolornight.g;
    g_colors[IDE_SKY_COLOR_NIGHT_BLUE] = s->_skycolornight.b;

    g_colors[IDE_SKY_CLEAR_AMBIENT_RED] = s->_skyclearambient.r;
    g_colors[IDE_SKY_CLEAR_AMBIENT_GREEN] = s->_skyclearambient.g;
    g_colors[IDE_SKY_CLEAR_AMBIENT_BLUE] = s->_skyclearambient.b;

    g_colors[IDE_SKY_NIGHT_AMBIENT_RED] = s->_skynightambient.r;
    g_colors[IDE_SKY_NIGHT_AMBIENT_GREEN] = s->_skynightambient.g;
    g_colors[IDE_SKY_NIGHT_AMBIENT_BLUE] = s->_skynightambient.b;

    g_colors[IDE_SUN_NOON_RED_COLOR] = s->_sunnoon.r;
    g_colors[IDE_SUN_NOON_GREEN_COLOR] = s->_sunnoon.g;
    g_colors[IDE_SUN_NOON_BLUE_COLOR] = s->_sunnoon.b;

    g_colors[IDE_SUN_ITSELF_NOON_RED_COLOR] = s->_sunitselfnoon.r;
    g_colors[IDE_SUN_ITSELF_NOON_GREEN_COLOR] = s->_sunitselfnoon.g;
    g_colors[IDE_SUN_ITSELF_NOON_BLUE_COLOR] = s->_sunitselfnoon.b;

    g_colors[IDE_SUN_NIGHT_RED_COLOR] = s->_sunnight.r;
    g_colors[IDE_SUN_NIGHT_GREEN_COLOR] = s->_sunnight.g;
    g_colors[IDE_SUN_NIGHT_BLUE_COLOR] = s->_sunnight.b;

    g_colors[IDE_SUNSET_RED_COLOR] = s->_sunset.r;
    g_colors[IDE_SUNSET_GREEN_COLOR] = s->_sunset.g;
    g_colors[IDE_SUNSET_BLUE_COLOR] = s->_sunset.b;

    g_colors[IDE_HORIZ_COLOR_RED] = s->_horizon.r;
    g_colors[IDE_HORIZ_COLOR_GREEN] = s->_horizon.g;
    g_colors[IDE_HORIZ_COLOR_BLUE] = s->_horizon.b;

    g_colors[IDE_MOON_RED_COLOR] = s->_moon.r;
    g_colors[IDE_MOON_GREEN_COLOR] = s->_moon.g;
    g_colors[IDE_MOON_BLUE_COLOR] = s->_moon.b;

    g_colors[IDE_MOON_DAY_RED_COLOR] = s->_moonday.r;
    g_colors[IDE_MOON_DAY_GREEN_COLOR] = s->_moonday.g;
    g_colors[IDE_MOON_DAY_BLUE_COLOR] = s->_moonday.b;
#endif
}

void trnskyChangeSetting(SINT32 setting, SINT32 fogoption, bool Override)
{
    if(_setting != setting || _fogoption != fogoption || Override)
    {
        _setting = setting;
        _fogoption = fogoption;

        _setproperties();
    }
}

void trnskyInitSkyBox()
{
    if(gCustomTerrain)
    {
        if(ogl::UseDeprecatedPath())
        {
            oglFreeMipTextures(skyboxtextures[SKY_NORTH]);
            oglFreeMipTextures(skyboxtextures[SKY_EAST]);
            oglFreeMipTextures(skyboxtextures[SKY_SOUTH]);
            oglFreeMipTextures(skyboxtextures[SKY_WEST]);
            oglFreeMipTextures(skyboxtextures[SKY_TOP]);

            skyboxtextures[SKY_NORTH] = ldLoadTexture2D(SCEN_MGR->MapPath("skybox_north.tga"),
                                                        Texture::CLAMP_TO_EDGE,
                                                        Texture::CLAMP_TO_EDGE,
                                                        Texture::LINEAR,
                                                        Texture::LINEAR);
            skyboxtextures[SKY_EAST] = ldLoadTexture2D(SCEN_MGR->MapPath("skybox_east.tga"),
                                                        Texture::CLAMP_TO_EDGE,
                                                        Texture::CLAMP_TO_EDGE,
                                                        Texture::LINEAR,
                                                        Texture::LINEAR);
            skyboxtextures[SKY_SOUTH] = ldLoadTexture2D(SCEN_MGR->MapPath("skybox_south.tga"),
                                                        Texture::CLAMP_TO_EDGE,
                                                        Texture::CLAMP_TO_EDGE,
                                                        Texture::LINEAR,
                                                        Texture::LINEAR);
            skyboxtextures[SKY_WEST] = ldLoadTexture2D(SCEN_MGR->MapPath("skybox_west.tga"),
                                                        Texture::CLAMP_TO_EDGE,
                                                        Texture::CLAMP_TO_EDGE,
                                                        Texture::LINEAR,
                                                        Texture::LINEAR);
            skyboxtextures[SKY_TOP] = ldLoadTexture2D(SCEN_MGR->MapPath("skybox_top.tga"),
                                                        Texture::CLAMP_TO_EDGE,
                                                        Texture::CLAMP_TO_EDGE,
                                                        Texture::LINEAR,
                                                        Texture::LINEAR);
        }
        else
        {
            oglFreeMipTextures(skyboxtexture);

            std::vector<std::string> skyboxtex;

            skyboxtex.push_back(SCEN_MGR->MapPath("skybox_north.tga"));
            skyboxtex.push_back(SCEN_MGR->MapPath("skybox_east.tga"));
            skyboxtex.push_back(SCEN_MGR->MapPath("skybox_south.tga"));
            skyboxtex.push_back(SCEN_MGR->MapPath("skybox_west.tga"));
            skyboxtex.push_back(SCEN_MGR->MapPath("skybox_top.tga"));

            skyboxtexture = ldLoadTexture2DArray("skybox",
                                                 skyboxtex,
                                                 Texture::CLAMP_TO_EDGE,
                                                 Texture::CLAMP_TO_EDGE,
                                                 Texture::LINEAR,
                                                 Texture::LINEAR);

            if(!oglIsTexture(skyboxtexture, 0))
            {
                Write_Debug("Creating skybox texture failed");
                return;
            }

#if USE_STATESET
            skyboxmaterial = new RenderStateSet;
            skyboxmaterial->SetRenderState(new TextureUnit(0, GL_TEXTURE_2D_ARRAY_EXT, oglGetTextureData(skyboxtexture)->genlist));
            skyboxmaterial->SetRenderState(new Program("skybox"));
            skyboxmaterial->SetRenderState(new DepthMask(false));
#else
            skyboxmaterial = new oglMaterial;
            skyboxmaterial->AddTexture(0, GL_TEXTURE_2D_ARRAY_EXT, oglGetTextureData(skyboxtexture)->genlist);
            skyboxmaterial->SetShader("skybox");
#endif

            if(!vbo.valid())
            {
                std::vector<float> data;

                float xx = 1;
                float yy = 1;
                float zz = 1;
                float zoffset = 0.1f;

                // North
                data.push_back(-xx);
                data.push_back(yy);
                data.push_back(-zz - zoffset);
                data.push_back(0);
                data.push_back(0);

                data.push_back(xx);
                data.push_back(yy);
                data.push_back(-zz - zoffset);
                data.push_back(1);
                data.push_back(0);

                data.push_back(xx);
                data.push_back(yy);
                data.push_back(zz - zoffset);
                data.push_back(2);
                data.push_back(0);

                data.push_back(-xx);
                data.push_back(yy);
                data.push_back(zz - zoffset);
                data.push_back(3);
                data.push_back(0);

                // East
                data.push_back(xx);
                data.push_back(yy);
                data.push_back(-zz - zoffset);
                data.push_back(0);
                data.push_back(1);

                data.push_back(xx);
                data.push_back(-yy);
                data.push_back(-zz - zoffset);
                data.push_back(1);
                data.push_back(1);

                data.push_back(xx);
                data.push_back(-yy);
                data.push_back(zz - zoffset);
                data.push_back(2);
                data.push_back(1);

                data.push_back(xx);
                data.push_back(yy);
                data.push_back(zz - zoffset);
                data.push_back(3);
                data.push_back(1);

                // South
                data.push_back(xx);
                data.push_back(-yy);
                data.push_back(-zz - zoffset);
                data.push_back(0);
                data.push_back(2);

                data.push_back(-xx);
                data.push_back(-yy);
                data.push_back(-zz - zoffset);
                data.push_back(1);
                data.push_back(2);

                data.push_back(-xx);
                data.push_back(-yy);
                data.push_back(zz - zoffset);
                data.push_back(2);
                data.push_back(2);

                data.push_back(xx);
                data.push_back(-yy);
                data.push_back(zz - zoffset);
                data.push_back(3);
                data.push_back(2);

                // West
                data.push_back(-xx);
                data.push_back(-yy);
                data.push_back(-zz - zoffset);
                data.push_back(0);
                data.push_back(3);

                data.push_back(-xx);
                data.push_back(yy);
                data.push_back(-zz - zoffset);
                data.push_back(1);
                data.push_back(3);

                data.push_back(-xx);
                data.push_back(yy);
                data.push_back(zz - zoffset);
                data.push_back(2);
                data.push_back(3);

                data.push_back(-xx);
                data.push_back(-yy);
                data.push_back(zz - zoffset);
                data.push_back(3);
                data.push_back(3);

                // Top
                data.push_back(-xx);
                data.push_back(-yy);
                data.push_back(-zz - zoffset);
                data.push_back(2);
                data.push_back(4);

                data.push_back(xx);
                data.push_back(-yy);
                data.push_back(-zz - zoffset);
                data.push_back(3);
                data.push_back(4);

                data.push_back(xx);
                data.push_back(yy);
                data.push_back(-zz - zoffset);
                data.push_back(0);
                data.push_back(4);

                data.push_back(-xx);
                data.push_back(yy);
                data.push_back(-zz - zoffset);
                data.push_back(1);
                data.push_back(4);

                vbo = new oglVertexBuffer;

                int stride = 20;

                vbo->AddBuffer(&data[0], data.size() * sizeof(float), true, oglVertexBuffer::MODE_ARB_VAO);
                vbo->SetStride(oglVertexBuffer::ATTRIBUTE_POSITION, 4, GL_FLOAT, stride, 0, 0);
                vbo->SetStride(oglVertexBuffer::ATTRIBUTE_TEXCOORD0, 1, GL_FLOAT, stride, 16, 0);

                vbo->AddDrawArraysPrimitive(GL_QUADS, 0, data.size() / 5, 0);
                vbo->Buffer();
            }
        }
    }
}

LOGICAL trnskyInit(const SINT8* name, SINT32 mesh, FLOAT32 scale)
{
    piVECTOR loc;
    UINT32 color;
    FLOAT32 ellipse;
    UINT32 i;
    FLOAT32 horizon;

    trnskyChangeSetting(0, 0, false);

    TERRAINSKY* s = trndataGetSkyData(_setting);

	loc.x = -playpos.x;
	loc.y = -playpos.y;
	loc.z = 0.0f;	//-playpos.z;
    color = piRGBA_MAKE((UINT32)s->_skycolor.r,
                        (UINT32)s->_skycolor.g,
                        (UINT32)s->_skycolor.b,
                        255);

	// ellipse should be "shallow" to display correct fog.
	ellipse = 0.6f;

	sky = obcHemiSphere("sky1", nullptr, mesh, &loc, scale * 1.1f, ellipse, color);

	if (sky == nullptr)
	{
		return(FALSE);
	}

	sky->flags &= ~OBJECT_DYNALITE;
	sky->flags &= ~OBJECT_COLLISION;
	sky->flags |= OBJECT_NOSPECULAR;
	sky->flags &= ~OBJECT_VISIBLE;
	sky->flags |= OBJECT_ZWRITEOFF;
	sky->flags |= OBJECT_USEMATRIX;

	if (!ldmdlReverseObjectNormals(sky))
	{
		return(FALSE);
	}

	loc.z = -playpos.z;
    color = piRGBA_MAKE((UINT32)s->_horizon.r,
                        (UINT32)s->_horizon.g,
                        (UINT32)s->_horizon.b,
                        255);
	ellipse = 1.5;
	_horizheight = ellipse * scale * 1.2f;
	horiz = obcHemiSphere("horiz", "horizon1", mesh, &loc, scale, 0.6f, color);
	horiztop = obcCylinder("horiztop", "horizon1", mesh, &loc, scale, ellipse, color);
	if (horiz == nullptr ||
		horiztop == nullptr)
	{
		return(FALSE);
	}

	piVectorPtr pos = horiz->StaticPositions();
	for (i = 0; i < horiz->numverts; i++, ++pos)
	{
		pos->z = -pos->z;
	}
	horiz->SyncMutableData();

	horiz->flags &= ~OBJECT_DYNALITE;
	horiz->flags &= ~OBJECT_COLLISION;
	horiz->flags |= OBJECT_NOSPECULAR;
	horiz->flags &= ~OBJECT_VISIBLE;
	horiz->flags |= OBJECT_ZWRITEOFF;
	horiz->flags |= OBJECT_USEMATRIX;

	horiztop->flags = horiz->flags;

	if (!ldmdlReverseObjectNormals(horiz))
	{
		return(FALSE);
	}

	if (!ldmdlReverseObjectNormals(horiztop))
	{
		return(FALSE);
	}

	meshes = mesh;

	trnmainGetHorizon(&horizon);

	// now init the sun objects
	if(!obmgrCreateModelInstance("sunglare1"))
	{
		Write_Debug("Can't find sunglare1.flt object!");
		return(FALSE);
	}

	sundisk = obmainFindNamedObject("sunglare1");
	if (sundisk == nullptr)
	{
		Write_Debug("Can't find sunglare1.flt object!");
		return(FALSE);
	}

	if (sundisk->child == nullptr)
	{
		Write_Debug("no child object in sunglare1.flt object!");
		return(FALSE);
	}

	strcpy(sundisk->name, "sundisk");
	sundisk->child->flags &= ~OBJECT_DYNALITE;
	sundisk->child->flags &= ~OBJECT_COLLISION;
	sundisk->child->flags |= OBJECT_NOSPECULAR;
	sundisk->child->flags &= ~OBJECT_VISIBLE;
	sundisk->child->flags |= OBJECT_ZWRITEOFF;
	sundisk->child->flags |= OBJECT_USEMATRIX;
	sundisk->child->flag2 |= OBJECT_NOFOG;

	_InitSun();

	// now init the moon objects
	if(!obmgrCreateModelInstance("fullmoon1"))
	{
		Write_Debug("Can't find fullmoon1.flt object!");
		return(FALSE);
	}

	moondisk = obmainFindNamedObject("fullmoon1");
	if (moondisk == nullptr)
	{
		Write_Debug("Can't find fullmoon1.flt object!");
		return(FALSE);
	}

	if (moondisk->child == nullptr)
	{
		Write_Debug("no child object in fullmoon1.flt object!");
		return(FALSE);
	}

	strcpy(moondisk->name, "moondisk");
//	moondisk->child->flags &= ~OBJECT_DYNALITE;
	moondisk->child->flags &= ~OBJECT_COLLISION;
	moondisk->child->flags |= OBJECT_NOSPECULAR;
	moondisk->child->flags &= ~OBJECT_VISIBLE;
	moondisk->child->flags |= OBJECT_ZWRITEOFF;
	moondisk->child->flags |= OBJECT_USEMATRIX;
	moondisk->child->flag2 |= OBJECT_NOFOG;
	moondisk->child->flag2 |= OBJECT_BLENDADDITIVE;

	_InitMoon();

    PNRandom::seedrandom(6547);

    float deg;
    float x, y;

	for (i=0 ; i < NUM_STARS ; i++)
	{
        deg = PNRandom::random(0.0f, PI2);
        x = (PNRandom::random(0, RAND_MAX) - RAND_MAX/2) / (float)(RAND_MAX/2) * 10000;
        y = (PNRandom::random(0, RAND_MAX) - RAND_MAX/2) / (float)(RAND_MAX/2) * 10000;

		_StarField[i].x = cos(deg) * x + sin(deg) * y;
		_StarField[i].y = -sin(deg) * x + cos(deg) * y;
		_StarField[i].z = (PNRandom::random(0, RAND_MAX) - RAND_MAX/2) / (float)(RAND_MAX/2) * 10000;
	}

#ifdef USE_LENS_FLARE
    FLOAT32 flareColor[4];

    flareColor[0] = _SunLight->light.dcvColor.r;
    flareColor[1] = _SunLight->light.dcvColor.g;
    flareColor[2] = _SunLight->light.dcvColor.b;
    flareColor[3] = 1.0f;

    g_lensFlare.CharacterizeFlare(true, false, 0.50f, flareColor);//0.75f, flareColor);
    g_lensFlare.SetMainFlare("fx\\sunflare\\sunflare.dds", 0.2f);
    g_lensFlare.Initialize("fx\\sunflare");
#endif

    if(gCustomTerrain)
    {
        trnskyInitSkyBox();
    }
    else if(ogl::UseDeprecatedPath())
    {
#if USE_STATESET
        DepthMask* depth = new DepthMask;
        depth->Set(false);

        sky->material = new RenderStateSet;
        sky->material->SetRenderState(new Material(sky->matid));
        sky->material->SetRenderState(depth);
        sky->material->SetMode(GL_LIGHTING, 0);
        sky->material->SetMode(GL_BLEND, 1);

        moondisk->material = new RenderStateSet;
        moondisk->material->SetRenderState(new TextureUnit(0, moondisk->child->texid));
        moondisk->material->SetRenderState(new Material(moondisk->child->matid));
        moondisk->material->SetRenderState(depth);
        moondisk->material->SetMode(GL_FOG, 0);
        moondisk->material->SetMode(GL_BLEND, 1);

        horiztop->material = new RenderStateSet;
        horiztop->material->SetRenderState(new TextureUnit(0, horiztop->texid));
        horiztop->material->SetRenderState(new Material(horiztop->matid));
        horiztop->material->SetRenderState(depth);
        horiztop->material->SetMode(GL_LIGHTING, 0);
        horiztop->material->SetMode(GL_BLEND, 1);

        horiz->material = new RenderStateSet;
        horiz->material->SetRenderState(new TextureUnit(0, horiz->texid));
        horiz->material->SetRenderState(new Material(horiz->matid));
        horiz->material->SetRenderState(depth);
        horiz->material->SetMode(GL_LIGHTING, 0);
        horiz->material->SetMode(GL_BLEND, 1);
#else
        sky->material = new oglMaterial;
        sky->material->AddTexture(0, sky->texid);
        sky->material->SetMaterial(sky->matid);

        moondisk->child->material = new oglMaterial;
        moondisk->child->material->AddTexture(0, moondisk->child->texid);
        moondisk->child->material->SetMaterial(moondisk->child->matid);

        horiztop->material = new oglMaterial;
        horiztop->material->AddTexture(0, horiztop->texid);
        horiztop->material->SetMaterial(horiztop->matid);

        horiz->material = new oglMaterial;
        horiz->material->AddTexture(0, horiz->texid);
        horiz->material->SetMaterial(horiz->matid);
#endif
    }
    else
    {
#if USE_STATESET
        DepthMask* depth = new DepthMask;
        depth->Set(false);

        horiztop->material = new RenderStateSet;

        horiztop->material->SetRenderState(new TextureUnit(0, horiztop->texid));
        horiztop->material->SetRenderState(new Material(horiztop->matid));
        horiztop->material->SetRenderState(new Program("horiz"));
        horiztop->material->SetRenderState(depth);
        horiztop->material->SetMode(GL_BLEND, 1);
        horiztop->material->SetMode(GL_LIGHTING, 0);

        sky->material = new RenderStateSet;
        sky->material->SetRenderState(new TextureUnit(0, sky->texid));
        sky->material->SetRenderState(new Material(sky->matid));
        sky->material->SetRenderState(new Program("sky"));

        moondisk->child->material = new RenderStateSet;
        moondisk->child->material->SetRenderState(new TextureUnit(0, moondisk->child->texid));
        moondisk->child->material->SetRenderState(new Material(moondisk->child->matid));
        moondisk->child->material->SetRenderState(new Program("moon"));

        horiz->material = new RenderStateSet;
        horiz->material->SetRenderState(new TextureUnit(0, horiz->texid));
        horiz->material->SetRenderState(new Material(horiz->matid));
        horiz->material->SetRenderState(new Program("horiz"));
#else
        horiztop->material = new oglMaterial;
        horiztop->material->AddTexture(0, horiztop->texid);
        horiztop->material->SetMaterial(horiztop->matid);
        horiztop->material->SetShader("horiz");

        sky->material = new oglMaterial;
        sky->material->AddTexture(0, sky->texid);
        sky->material->SetMaterial(sky->matid);
        sky->material->SetShader("sky");

        moondisk->child->material = new oglMaterial;
        moondisk->child->material->AddTexture(0, moondisk->child->texid);
        moondisk->child->material->SetMaterial(moondisk->child->matid);
        moondisk->child->material->SetShader("moon");

        horiz->material = new oglMaterial;
        horiz->material->AddTexture(0, horiz->texid);
        horiz->material->SetMaterial(horiz->matid);
        horiz->material->SetShader("horiz");
#endif

#if USE_STATESET
        starmaterial = new RenderStateSet;
        starmaterial->SetRenderState(new Program("stars"));
        starmaterial->SetRenderState(new DepthMask(false));
#else
        starmaterial = new oglMaterial;
        starmaterial->SetMaterial(-1);
        starmaterial->AddTexture(0, -1);
        starmaterial->SetShader("stars");
#endif
    }

	return(TRUE);
}

void* trnskyGetSunObject()
{
    return (void*)sundisk;
}

#ifdef USE_LENS_FLARE
static inline float Interpolate(float fStart, float fEnd, float fPercent)
{
	return fStart + (fEnd - fStart) * fPercent;
}

void computeFlareParameters()
{
    // project the sun to the window
    Vec4f pos;
    piVECTOR dir = _SunLight->light.dvDirection;

    dir.x = -dir.x;
    dir.y = -dir.y;
    dir.z = -dir.z;

    pos.x() = dir.x * 100000.0f;
    pos.y() = dir.y * 100000.0f;
    pos.z() = dir.z * 100000.0f;
    pos.w() = 1.0f;

    Matrixf mvp;
    oglGetModelViewProjectionMatrix(mvp);

    pos = pos * mvp;

    pos.x() /= pos.w();
    pos.y() /= pos.w();
    pos.z() /= pos.w();
    pos.w() = 1.0f;

    float halfw = (float)ddpGetWidth() * 0.5f;
    float halfh = (float)ddpGetHeight() * 0.5f;

   // halfw *= (FieldOfView / 90.0f);
   // halfh *= (FieldOfView / 90.0f);

    float x = pos.x() * halfw + halfw;
    float y = pos.y() * halfh + halfh;

    // set flare location
    g_lensFlare.SetFlareLocation(x, y);

    FLOAT32 flareColor[4];

    flareColor[0] = _SunLight->light.dcvColor.r;
    flareColor[1] = _SunLight->light.dcvColor.g;
    flareColor[2] = _SunLight->light.dcvColor.b;
    flareColor[3] = 1.0f;

    g_lensFlare.setFlareColor(flareColor);

    FLOAT32 lookAway = math3dDotProduct(&dir, &playdir);

    // determine visibility
    if(_SunLight->flags & LITE_ON && lookAway > 0)
    {
        g_lensFlare.SetVisible(true);
    }
    else
    {
        g_lensFlare.SetVisible(false);
    }

    // set flare brightness
    x /= (float)ddpGetWidth();
    y /= (float)ddpGetHeight();

    float distance = math3dFastSqrt((FLOAT32)(((0.5f - x) * (0.5f - x)) + ((0.5f - y) * (0.5f - y))));
	g_beforebright = Interpolate(0.0f, g_halfMaxBright, 1.0f - (distance * g_distanceScale));
	g_afterbright = Interpolate(0.0f, 1.0f, 1.0f - (distance * g_distanceScale));

    g_beforebright = lookAway < 0 ? 0 : g_beforebright;
    g_afterbright = lookAway < 0 ? 0 : g_afterbright;

    g_lensFlare.SetBrightnesses(g_beforebright, g_afterbright);
}

FLOAT32 trnskyGetAfterBright()
{
    if(g_lensFlare.isVisible())
    {
        return g_afterbright + g_beforebright;
    }
    else
    {
        return -1.0f;
    }
}
#endif

static void _FogUpdate()
{
	int j, num;

	num = (meshes * 2 + 1) * meshes;

	piVertexColorPtr skyColor = sky->StaticColors();
	skyColor += num;

	for (j=0 ; j <= meshes * 2 ; j++, ++skyColor)
	{
		skyColor->color = piRGBA_SETALPHA(skyColor->color, 0);
	}

	num = sky->numverts;
}

LOGICAL trnskyUpdate()
{
SINT32		paints;
piVECTOR	pos, rot;
piVECTOR	tempvec;//,tempvec1;
piMATRIX	mat,mat1,ScaleMat;
FLOAT32		horizon;

static FLOAT32 _LastAlt;

	if (sky == nullptr ||
		horiz == nullptr ||
		sundisk == nullptr ||
		moondisk == nullptr ||
		horiztop == nullptr)
	{
		return(TRUE);
	}

#if(0)
    // Previously, obdlite update func always called the update light functions per frame, so it does
    // not make sense to do this....Just call once per frame as before (below)
	if(trntodTimeOfDayHasChanged != 0 ||
		playpos.z - _LastAlt > 100 ||
		playpos.z - _LastAlt < -100)
#else
    if(1)
#endif
	{
        TERRAINSKY interpolate;
        TERRAINSKY* s;

        if(fxWeatherIsCloudy())
        {
            // Interpolate at certain altitudes so color
            // becomes clear as we move above storm clouds
            InterpolateIfNecessary(interpolate);
            s = &interpolate;
        }
        else
        {
            s = trndataGetSkyData(_setting);
            rainfogclip = 1.0f;
        }

		_UpdateSunLight(_SunLight, s);
		_UpdateMoonLight(_MoonLight, s);
		_LastAlt = playpos.z;
	}

	paints = piGetTotalPaints();

	if (ft200msticks ||
		paints == 1 ||
		trnmainWorldOriginMoved() == TRUE)
	{
		// Sky
		pos.x = -playpos.x;
		pos.y = -playpos.y;
		pos.z = -playpos.z;

		rot.x = 0.0f;
		rot.y = 0.0f;
		rot.z = 0.0f;
		if (!obplyrUpdateThisObject(sky, &pos, &rot))
		{
			return(FALSE);
		}

		ScaleMat = IdentityMatrix;
		ScaleMat._11 = _SkyDomeScaleFactor * persGetFogScaleMultiplier();
		ScaleMat._22 = _SkyDomeScaleFactor * persGetFogScaleMultiplier();
		ScaleMat._33 = _SkyDomeScaleFactor * persGetFogScaleMultiplier();

		// scale sky for fog
		math3dMatrixMultiply(&sky->mat, &ScaleMat, &sky->mat);

		// Horizon
		pos.z = -playpos.z;// / 2.0f;
		pos.z += _horizheight;
		if (!obplyrUpdateThisObject(horiz, &pos, &rot))
		{
			return(FALSE);
		}

		math3dMatrixMultiply(&horiz->mat, &ScaleMat, &horiz->mat);
		pos.z -= _horizheight;
		if (!obplyrUpdateThisObject(horiztop, &pos, &rot))
		{
			return(FALSE);
		}

		math3dMatrixMultiply(&horiztop->mat, &ScaleMat, &horiztop->mat);

		// Sun
		pos.x = -playpos.x;
		pos.y = -playpos.y;
		pos.z = -playpos.z;

		trnmainGetHorizon(&horizon);

		tempvec.x = (FLOAT32)(horizon * sin(sunelevation) * 0.88f);
		tempvec.y = (FLOAT32)(horizon * 0.2 + -cos(sunelevation) * horizon * 0.2);
		tempvec.z = (FLOAT32)(horizon * cos(sunelevation) * 0.88f);

		pos.x += tempvec.x;
		pos.y += tempvec.y;
		pos.z += tempvec.z;

		rot.x = PI / 2;
		rot.y = 0;
		rot.z = 0;
		mathEul_ToMatrix(&mat, EulOrdXYZs, &rot);

		rot.x = 0;
		rot.y = -sunelevation;
		rot.z = 0;
		mathEul_ToMatrix(&mat1, EulOrdXYZs, &rot);
		math3dMatrixMultiply(&mat, &mat, &mat1);

		mat._41 = pos.x;
		mat._42 = pos.y;
		mat._43 = pos.z;

		sundisk->child->mat = mat;

		pos.x = -playpos.x;
		pos.y = -playpos.y;
		pos.z = -playpos.z;

		trnmainGetHorizon(&horizon);

		tempvec.x = -(FLOAT32)(horizon * sin(moonelevation) * 0.88f);
		tempvec.y = (FLOAT32)(horizon * 0.2 + cos(moonelevation) * horizon * 0.2);
		tempvec.z = -(FLOAT32)(horizon * cos(moonelevation) * 0.88f);

		pos.x += tempvec.x;
		pos.y += tempvec.y;
		pos.z += tempvec.z;

		rot.x = -PI / 2;
		rot.y = 0;
		rot.z = 0;
		mathEul_ToMatrix(&mat, EulOrdXYZs, &rot);

		rot.x = 0;
		rot.y = -moonelevation;
		rot.z = 0;
		mathEul_ToMatrix(&mat1, EulOrdXYZs, &rot);
		math3dMatrixMultiply(&mat, &mat, &mat1);

		mat._41 = pos.x;
		mat._42 = pos.y;
		mat._43 = pos.z;

		moondisk->child->mat = mat;
	}

	_UpdateFogSphere();

    return(TRUE);
}

#ifdef USE_LENS_FLARE
void trnskyReadObstruction()
{
    g_lensFlare.ReadObstructionPixels();
}

void trnskyAdjustBrightness()
{
    // adjust and draw flare
    g_lensFlare.AdjustBrightness();
}

void trnskyDrawFlare()
{
    g_lensFlare.DrawFlare();
}

void trnskyDrawAfterFlare()
{
 //   g_lensFlare.DrawAfterFlare(viewport);
}

void trnskyReadControlPixels()
{
    g_lensFlare.ReadControlPixels();
}

void trnskyPreDrawFlare()
{
    // draw main flare
    if(_SunLight->flags & LITE_ON)
    {
        computeFlareParameters();
        g_lensFlare.DrawBeforeFlare();
    }
    else
    {
        g_lensFlare.SetBrightnesses(0, 0);
    }
}
#endif // USE_LENS_FLARE

static void DrawStars()
{
    static std::vector<float> coords;

    piVECTOR Pos;
    piMATRIX Mat;
    piVECTOR Att;

    oglSetWorldMatrix(&IdentityMatrix);

    piVECTOR up;
    up.set(0, 0, 1);

    float alpha = 0.0f;

	if(_SunAngle <= 5.0)
	{
		if(_SunAngle < -10.0)
			alpha = 1.0;
		else
			alpha = 1.0f - (_SunAngle + 10.0f) / 15.0f;

		Att.x = 0;
		Att.y = -sunelevation;
		Att.z = 0;

		mathEul_ToMatrix(&Mat, EulOrdXYZs, &Att);

		for(int i = NUM_STARS; i != 0; --i)
		{
			math3dVectorMultiply(&Pos, &_StarField[i - 1], &Mat);

			if(mathVectDotProduct(&Pos, &up) > 0.0)
			{
				continue;
			}

            coords.push_back(Pos.x - playpos.x);
            coords.push_back(Pos.y - playpos.y);
            coords.push_back(Pos.z - playpos.z);
            coords.push_back(alpha);
		}

        if(!coords.empty())
        {
#if USE_STATESET
#else
            oglSetRenderState(oglRENDERSTATE_ZWRITEENABLE, FALSE);
#endif
            glPointSize(2.0f);

            starmaterial->Apply();

            ogl::SetVertexAttribPointer(oglVertexBuffer::ATTRIBUTE_POSITION, 4, GL_FLOAT, 0, 0, &coords[0]);
            ogl::DrawArrays(GL_POINTS, 0, coords.size() / 4);

#if USE_STATESET
#else
            oglSetRenderState(oglRENDERSTATE_ZWRITEENABLE, bZWrite);
#endif
            glPointSize(1.0f);
        }
	}

    coords.clear();
}

void trnskyDraw()
{
    if(gCustomTerrain)
    {
#if USE_STATESET
#else
        oglSetRenderState(oglRENDERSTATE_CULLMODE, piCULLMODE_DEFAULT);
        oglSetRenderState(oglRENDERSTATE_ZWRITEENABLE, 0);
#endif
        oglSetWorldMatrix(&IdentityMatrix);

        Vec3f pos = -Vec3f(playpos);

#if USE_STATESET
        glslProgram* pgm = dynamic_cast<Program*>(skyboxmaterial->GetRenderState(RS_GLSLPROGRAM))->GetProgram();
        pgm->GetUniform("campos")->Set(pos);
#else
        skyboxmaterial->GetProgram()->GetUniform("campos")->Set(pos);
#endif

        skyboxmaterial->Apply();
        vbo->DrawPrimitive();

#if USE_STATESET
#else
        oglSetRenderState(oglRENDERSTATE_ZWRITEENABLE, bZWrite);
#endif
    }
    else
    {
#if USE_STATESET
#else
        piSetRenderState(piRENDERSTATE_ALPHABLENDENABLE, piALPHABLENDENABLE_DEFAULT);
        oglSetRenderState(oglRENDERSTATE_ZWRITEENABLE, 0);
#endif
        piSTRIDEDDATA sd;

        horiztop->StridedRenderData(&sd);

        oglSetWorldMatrix(&horiztop->mat);
        horiztop->material->Apply();

        ogl::SetVertexAttribPointer(oglVertexBuffer::ATTRIBUTE_POSITION, 3, GL_FLOAT, 0, sd.position.stride, sd.position.pdata);
        ogl::SetVertexAttribPointer(oglVertexBuffer::ATTRIBUTE_TEXCOORD0, 2, GL_FLOAT, 0, sd.textureCoords[0].stride, sd.textureCoords[0].pdata);
        ogl::SetVertexAttribPointer(oglVertexBuffer::ATTRIBUTE_COLOR, 4, GL_UNSIGNED_BYTE, 0, sd.diffuse.stride, sd.diffuse.pdata);

        ogl::DrawElements(GL_TRIANGLES, horiztop->numtris * 3, GL_UNSIGNED_SHORT, horiztop->ptris);

        sky->StridedRenderData(&sd);

        oglSetWorldMatrix(&sky->mat);
        sky->material->Apply();

        ogl::SetVertexAttribPointer(oglVertexBuffer::ATTRIBUTE_POSITION, 3, GL_FLOAT, 0, sd.position.stride, sd.position.pdata);
        ogl::SetVertexAttribPointer(oglVertexBuffer::ATTRIBUTE_COLOR, 4, GL_UNSIGNED_BYTE, 0, sd.diffuse.stride, sd.diffuse.pdata);

        ogl::DrawElements(GL_TRIANGLES, sky->numtris * 3, GL_UNSIGNED_SHORT, sky->ptris);

        if(!(moondisk->child->flag2 & OBJECT_HIDDEN))
        {
            moondisk->child->StridedRenderData(&sd);

            oglSetWorldMatrix(&moondisk->child->mat);
            moondisk->child->material->Apply();

            ogl::SetVertexAttribPointer(oglVertexBuffer::ATTRIBUTE_POSITION, 3, GL_FLOAT, 0, sd.position.stride, sd.position.pdata);
            ogl::SetVertexAttribPointer(oglVertexBuffer::ATTRIBUTE_TEXCOORD0, 2, GL_FLOAT, 0, sd.textureCoords[0].stride, sd.textureCoords[0].pdata);
            ogl::SetVertexAttribPointer(oglVertexBuffer::ATTRIBUTE_COLOR, 4, GL_UNSIGNED_BYTE, 0, sd.diffuse.stride, sd.diffuse.pdata);

            ogl::DrawElements(GL_TRIANGLES, moondisk->child->numtris * 3, GL_UNSIGNED_SHORT, moondisk->child->ptris);
        }

        DrawStars();

        horiz->StridedRenderData(&sd);

        oglSetWorldMatrix(&horiz->mat);
        horiz->material->Apply();

        ogl::SetVertexAttribPointer(oglVertexBuffer::ATTRIBUTE_POSITION, 3, GL_FLOAT, 0, sd.position.stride, sd.position.pdata);
        ogl::SetVertexAttribPointer(oglVertexBuffer::ATTRIBUTE_TEXCOORD0, 2, GL_FLOAT, 0, sd.textureCoords[0].stride, sd.textureCoords[0].pdata);
        ogl::SetVertexAttribPointer(oglVertexBuffer::ATTRIBUTE_COLOR, 4, GL_UNSIGNED_BYTE, 0, sd.diffuse.stride, sd.diffuse.pdata);

        ogl::DrawElements(GL_TRIANGLES, horiz->numtris * 3, GL_UNSIGNED_SHORT, horiz->ptris);

#if USE_STATESET
#else
        oglSetRenderState(oglRENDERSTATE_ZWRITEENABLE, bZWrite);
        ProgramState::Bind(0);
#endif
    }
}

LOGICAL trnskyDrawDeprecated()
{
    if(gCustomTerrain)
    {
#if USE_STATESET
        static ref_ptr<RenderStateSet> stateset;

        if(!stateset.valid())
        {
            stateset = new RenderStateSet;

            stateset->SetMode(GL_LIGHTING, 0);
            stateset->SetMode(GL_BLEND, 0);
            stateset->SetRenderState(new DepthMask(false));
            stateset->SetRenderState(new TextureUnit(0));
            stateset->SetRenderState(new Material);
        }

        TextureUnit* tu = dynamic_cast<TextureUnit*>(stateset->GetRenderState(RS_TEXTURE_UNIT0));
#else
        ogl::Disable();

        oglSetRenderState(oglRENDERSTATE_LIGHTING, 0);
        oglSetRenderState(oglRENDERSTATE_ALPHABLENDENABLE, 0);
        oglSetRenderState(oglRENDERSTATE_CULLMODE, piCULLMODE_DEFAULT);
        oglSetRenderState(oglRENDERSTATE_ZWRITEENABLE, 0);
#endif
        oglSetWorldMatrix(&IdentityMatrix);

        static bool first = true;
        static std::vector<float> data;

        if(first)
        {
            first = false;
            data.resize(20);
        }

#if USE_STATESET
#else
        oglDisableMaterial();
#endif

        Matrixf mat;
        mat(3, 0) = -playpos.x;
        mat(3, 1) = -playpos.y;
        mat(3, 2) = -playpos.z;

        oglSetWorldMatrix(mat);

#if USE_STATESET
        tu->SetTexture(skyboxtextures[SKY_NORTH]);
#else
        oglSetActiveTexture(0, skyboxtextures[SKY_NORTH]);
#endif

        static float xx = 600;
        static float yy = 600;
        static float zz = 600;
        static float zoffset = 80.0f;

        data[0] = -xx;
        data[1] = yy;
        data[2] = -zz - zoffset;
        data[3] = 0;
        data[4] = 1;

        data[5] = xx;
        data[6] = yy;
        data[7] = -zz - zoffset;
        data[8] = 1;
        data[9] = 1;

        data[10] = xx;
        data[11] = yy;
        data[12] = zz - zoffset;
        data[13] = 1;
        data[14] = 0;

        data[15] = -xx;
        data[16] = yy;
        data[17] = zz - zoffset;
        data[18] = 0;
        data[19] = 0;

#if USE_STATESET
        stateset->Apply();
#endif

        ogl::SetVertexPointer(3, GL_FLOAT, 20, &data[0]);
        ogl::SetTextureUnitCoordPointer(GL_TEXTURE0, 2, GL_FLOAT, 20, &data[3]);
        ogl::DrawArrays(GL_QUADS, 0, 4);

#if USE_STATESET
        tu->SetTexture(skyboxtextures[SKY_EAST]);
#else
        oglSetActiveTexture(0, skyboxtextures[SKY_EAST]);
#endif

        data[0] = xx;
        data[1] = yy;
        data[2] = -zz - zoffset;
        data[3] = 0;
        data[4] = 1;

        data[5] = xx;
        data[6] = -yy;
        data[7] = -zz - zoffset;
        data[8] = 1;
        data[9] = 1;

        data[10] = xx;
        data[11] = -yy;
        data[12] = zz - zoffset;
        data[13] = 1;
        data[14] = 0;

        data[15] = xx;
        data[16] = yy;
        data[17] = zz - zoffset;
        data[18] = 0;
        data[19] = 0;

#if USE_STATESET
        tu->apply();
#endif

        ogl::SetVertexPointer(3, GL_FLOAT, 20, &data[0]);
        ogl::SetTextureUnitCoordPointer(GL_TEXTURE0, 2, GL_FLOAT, 20, &data[3]);
        ogl::DrawArrays(GL_QUADS, 0, 4);

#if USE_STATESET
        tu->SetTexture(skyboxtextures[SKY_SOUTH]);
#else
        oglSetActiveTexture(0, skyboxtextures[SKY_SOUTH]);
#endif

        data[0] = xx;
        data[1] = -yy;
        data[2] = -zz - zoffset;
        data[3] = 0;
        data[4] = 1;

        data[5] = -xx;
        data[6] = -yy;
        data[7] = -zz - zoffset;
        data[8] = 1;
        data[9] = 1;

        data[10] = -xx;
        data[11] = -yy;
        data[12] = zz - zoffset;
        data[13] = 1;
        data[14] = 0;

        data[15] = xx;
        data[16] = -yy;
        data[17] = zz - zoffset;
        data[18] = 0;
        data[19] = 0;

#if USE_STATESET
        tu->apply();
#endif

        ogl::SetVertexPointer(3, GL_FLOAT, 20, &data[0]);
        ogl::SetTextureUnitCoordPointer(GL_TEXTURE0, 2, GL_FLOAT, 20, &data[3]);
        ogl::DrawArrays(GL_QUADS, 0, 4);

#if USE_STATESET
        tu->SetTexture(skyboxtextures[SKY_WEST]);
#else
        oglSetActiveTexture(0, skyboxtextures[SKY_WEST]);
#endif

        data[0] = -xx;
        data[1] = -yy;
        data[2] = -zz - zoffset;
        data[3] = 0;
        data[4] = 1;

        data[5] = -xx;
        data[6] = yy;
        data[7] = -zz - zoffset;
        data[8] = 1;
        data[9] = 1;

        data[10] = -xx;
        data[11] = yy;
        data[12] = zz - zoffset;
        data[13] = 1;
        data[14] = 0;

        data[15] = -xx;
        data[16] = -yy;
        data[17] = zz - zoffset;
        data[18] = 0;
        data[19] = 0;

 #if USE_STATESET
        tu->apply();
#endif

        ogl::SetVertexPointer(3, GL_FLOAT, 20, &data[0]);
        ogl::SetTextureUnitCoordPointer(GL_TEXTURE0, 2, GL_FLOAT, 20, &data[3]);
        ogl::DrawArrays(GL_QUADS, 0, 4);

#if USE_STATESET
        tu->SetTexture(skyboxtextures[SKY_TOP]);
#else
        oglSetActiveTexture(0, skyboxtextures[SKY_TOP]);
#endif

        data[0] = xx;
        data[1] = yy;
        data[2] = -zz - zoffset;
        data[3] = 0;
        data[4] = 1;

        data[5] = -xx;
        data[6] = yy;
        data[7] = -zz - zoffset;
        data[8] = 1;
        data[9] = 1;

        data[10] = -xx;
        data[11] = -yy;
        data[12] = -zz - zoffset;
        data[13] = 1;
        data[14] = 0;

        data[15] = xx;
        data[16] = -yy;
        data[17] = -zz - zoffset;
        data[18] = 0;
        data[19] = 0;

#if USE_STATESET
        tu->apply();
#endif

        ogl::SetVertexPointer(3, GL_FLOAT, 20, &data[0]);
        ogl::SetTextureUnitCoordPointer(GL_TEXTURE0, 2, GL_FLOAT, 20, &data[3]);
        ogl::DrawArrays(GL_QUADS, 0, 4);

#if USE_STATESET
        States::ApplyRenderStates(nullptr);
        States::ApplyRenderModes(nullptr);
#else
        oglSetRenderState(oglRENDERSTATE_ZWRITEENABLE, bZWrite);
        oglSetRenderState(oglRENDERSTATE_ALPHABLENDENABLE, piALPHABLENDENABLE_DEFAULT);
        oglSetRenderState(oglRENDERSTATE_LIGHTING, piLIGHTING_DEFAULT);
#endif
    }
    else
    {
#if USE_STATESET
#else
        // Prevent sun and moon from culling
        FLOAT32 ZBufferEnd = 0;
        FLOAT32 ZBufferStart = 0;

        rnmainGetZBufferNearVal(&ZBufferStart);

        trnmainGetHorizon(&ZBufferEnd);

        piSetRenderState(piRENDERSTATE_ALPHABLENDENABLE, piALPHABLENDENABLE_DEFAULT);
#endif

        if(horiztop != nullptr)
	    {
#if USE_STATESET
            horiztop->material->Apply();
            oglSetWorldMatrix(&horiztop->mat);
            obdrawDrawMesh(horiztop);
#else
            obdrawDrawSingleObject(horiztop);
#endif
	    }

	    if(sky != nullptr)
	    {
#if USE_STATESET
            sky->material->Apply();
            oglSetWorldMatrix(&sky->mat);
            obdrawDrawMesh(sky);
#else
            obdrawDrawSingleObject(sky);
#endif
	    }

	    if(sundisk != nullptr)
	    {
		    if (!(sundisk->child->flag2 & OBJECT_HIDDEN))
		    {
// Lens glare will cover for the sun. Otherwise, you get a full screen nuke
// effect, especially with post render on.
#ifndef USE_LENS_FLARE
                // Z buffer fix makes moon fail the cull check.
                if(1)//rnvisFOVCheck(sundisk->child))
                {
                    obdrawDrawSingleObject(sundisk->child);
                }
#endif
		    }
	    }

	    if(moondisk != nullptr)
	    {
		    if(!(moondisk->child->flag2 & OBJECT_HIDDEN))
		    {
#if USE_STATESET
                moondisk->material->Apply();
                oglSetWorldMatrix(&moondisk->child->mat);
                obdrawDrawMesh(moondisk->child);
#else
                obdrawDrawSingleObject(moondisk->child);
#endif
		    }
	    }

        _DrawStarsDeprecated();

	    if(horiz != nullptr)
	    {
#if USE_STATESET
            horiz->material->Apply();
            oglSetWorldMatrix(&horiz->mat);
            obdrawDrawMesh(horiz);
#else
            obdrawDrawSingleObject(horiz);
#endif
	    }
    }

    return(TRUE);
}

#if USE_STATESET
static UINT32 _DrawStarsDeprecated()
{
	static piLVERTEX Vertex[NUM_STARS];
	UINT32		i, num;
	piVECTOR	Pos;
	piVECTOR	StraightUp;
	piMATRIX	Mat;
	piVECTOR	Att;
	UINT32		Color;
	FLOAT32		Brightness;

    static ref_ptr<RenderStateSet> stateset;

    if(!stateset.valid())
    {
        stateset = new RenderStateSet;
        stateset->SetMode(GL_FOG, 0);
        stateset->SetMode(GL_LIGHTING, 0);
        stateset->SetRenderState(new DepthMask(false));
    }

	if (_SunAngle <= 5.0)
	{
		StraightUp.z = 1.0;
		StraightUp.x = StraightUp.y = 0;

		if (_SunAngle < -10.0)
		{
			Brightness = 1.0;
		}
		else
		{
			Brightness = (FLOAT32)(1.0 - (_SunAngle + 10) / 15.0);
		}

		Color = colorWHITE;
		Color = piRGBA_SETALPHA(Color,(UINT32)(Brightness * 255.0));

        Att.x = 0;
		Att.y = -sunelevation;
		Att.z = 0;

		mathEul_ToMatrix(&Mat, EulOrdXYZs, &Att);

		for (i=0, num=0;i<NUM_STARS;i++)
		{
			math3dVectorMultiply(&Pos, &_StarField[i], &Mat);

			if (mathVectDotProduct(&Pos, &StraightUp) > 0.0)
			{
				continue;
			}

			Pos.x -= playpos.x;
			Pos.y -= playpos.y;
			Pos.z -= playpos.z;

			Vertex[num].x = Pos.x;
			Vertex[num].y = Pos.y;
			Vertex[num].z = Pos.z;
			Vertex[num].color = Color;
			Vertex[num].specular = colorALPHA100;
			Vertex[num].tu = 0.0f;
			Vertex[num].tv = 0.0f;
			num += 1;
		}

		if (num > 0)
		{
            stateset->Apply();
			piRenderScene(piPT_POINTLIST, piVT_LVERTEX, &Vertex, num);
		}
	}

	return 0;
}
#else
static UINT32 _DrawStarsDeprecated()
{
	static piLVERTEX	Vertex[NUM_STARS];
	UINT32		i, num;
	piVECTOR	Pos;
	piVECTOR	StraightUp;
	piMATRIX	Mat;
	piVECTOR	Att;
	UINT32		Color;
	FLOAT32		Brightness;
	FLOAT32		FogStartSave,FogEndSave;

	if (_SunAngle <= 5.0)
	{
		FogStartSave = piLongLikeFloat(FogStart);
		FogEndSave = piLongLikeFloat(FogEnd);

		rnmainAdjustFogAttributes(colorGRAY50|colorALPHA100,5000,10000);
		piSetRenderState(piRENDERSTATE_ZWRITEENABLE, FALSE);

		StraightUp.z = 1.0;
		StraightUp.x = StraightUp.y = 0;

		if (_SunAngle < -10.0)
		{
			Brightness = 1.0;
		}
		else
		{
			Brightness = (FLOAT32)(1.0 - (_SunAngle + 10) / 15.0);
		}

		Color = colorWHITE;
		Color = piRGBA_SETALPHA(Color,(UINT32)(Brightness * 255.0));

        Att.x = 0;
		Att.y = -sunelevation;
		Att.z = 0;

		mathEul_ToMatrix(&Mat, EulOrdXYZs, &Att);

		for (i=0, num=0;i<NUM_STARS;i++)
		{
			math3dVectorMultiply(&Pos, &_StarField[i], &Mat);

			if (mathVectDotProduct(&Pos, &StraightUp) > 0.0)
			{
				continue;
			}

			Pos.x -= playpos.x;
			Pos.y -= playpos.y;
			Pos.z -= playpos.z;

			Vertex[num].x = Pos.x;
			Vertex[num].y = Pos.y;
			Vertex[num].z = Pos.z;
			Vertex[num].color = Color;
			Vertex[num].specular = colorALPHA100;
			Vertex[num].tu = 0.0f;
			Vertex[num].tv = 0.0f;
			num += 1;
		}

		if (num > 0)
		{
			piDisableTexture();
			piDisableMaterial();

            UINT32 lightOn = piGetEngineRenderState(piRENDERSTATE_LIGHTING);

		    piSetRenderState(piRENDERSTATE_LIGHTING, FALSE);

			if (!piRenderScene(piPT_POINTLIST, piVT_LVERTEX, &Vertex, num))
			{
				piSetRenderState(piRENDERSTATE_ZWRITEENABLE, bZWrite);
		        piSetRenderState(piRENDERSTATE_LIGHTING, lightOn);
				return(FALSE);
			}

            piSetRenderState(piRENDERSTATE_LIGHTING, lightOn);
		}

        piSetRenderState(piRENDERSTATE_ZWRITEENABLE, bZWrite);
		rnmainAdjustFogAttributes(_FogColor,FogStartSave,FogEndSave);
	}

	return 0;
}
#endif

extern void LensFlareKill();

void trnskyKill()
{
    LensFlareKill();

	if (sky != nullptr)
	{
		obmainFreeObject(sky);
		sky = nullptr;
	}

	if (horiz != nullptr)
	{
		obmainFreeObject(horiz);
		horiz = nullptr;
	}

	if (horiztop != nullptr)
	{
		obmainFreeObject(horiztop);
		horiztop = nullptr;
	}

	if (sundisk != nullptr)
	{
		obpoolFreeObjectCallback(sundisk);
		sundisk = nullptr;
	}

	if (moondisk != nullptr)
	{
		obpoolFreeObjectCallback(moondisk);
		moondisk = nullptr;
	}

    oglFreeMipTextures(skyboxtexture);
    oglFreeMipTextures(skyboxtextures[SKY_NORTH]);
    oglFreeMipTextures(skyboxtextures[SKY_EAST]);
    oglFreeMipTextures(skyboxtextures[SKY_SOUTH]);
    oglFreeMipTextures(skyboxtextures[SKY_WEST]);
    oglFreeMipTextures(skyboxtextures[SKY_TOP]);
}

static UINT32 _InitSun()
{
LIGHT Light;

	sunelevation = PI; // until otherwise told, we'll put the sun at noon

    TERRAINSKY* s = trndataGetSkyData(_setting);

	// init the light
	memset(&Light, 0, sizeof(LIGHT));
	strcpy(Light.name, "sun");
	Light.flags = LITE_ON;
	Light.light.dltType = piLIGHT_DIRECTIONAL;
    Light.light.dcvColor.r = s->_sunnoon.r;
	Light.light.dcvColor.g = s->_sunnoon.g;
	Light.light.dcvColor.b = s->_sunnoon.b;
	Light.light.dcvColor.a = 0.0f;
	Light.light.dcvSpecular.r = s->_sunnoon.r;
	Light.light.dcvSpecular.g = s->_sunnoon.g;
	Light.light.dcvSpecular.b = s->_sunnoon.b;
	Light.light.dcvSpecular.a = 0.0f;
    Light.light.dcvAmbient.r = s->_sunnight.r;
    Light.light.dcvAmbient.g = s->_sunnight.g;
    Light.light.dcvAmbient.b = s->_sunnight.b;
	Light.light.dcvAmbient.a = 0.0f;
	Light.light.dvPosition.x = 0;
	Light.light.dvPosition.y = 0;
	Light.light.dvPosition.z = -1.0;
	Light.light.dvRange = 0.0f;//((float)sqrt(FLT_MAX));
	Light.light.dvFalloff = 1.0f;
	Light.light.dvAttenuation0 = 0.0f;
	Light.light.dvAttenuation1 = 0.0f;
	Light.light.dvAttenuation2 = 0.0f;
	Light.light.dvTheta = 0.0f;//PI / 8;
	Light.light.dvPhi = 0.0f;//PI / 16;

	if (!obdliteAddDynaLite(&Light, nullptr))
	{
		return(FALSE);
	}

	_SunLight = obdliteFindDynaLight("sun");

	return 0;
}

LIGHT* trnskyGetSunSource()
{
	return _SunLight;
}

static UINT32 _InitMoon()
{
    TERRAINSKY* s = trndataGetSkyData(_setting);

    LIGHT Light;

	moonelevation = PI; // until otherwise told, we'll put the moon at midnight

	// init the light
	memset(&Light, 0, sizeof(LIGHT));
	strcpy(Light.name, "moon");
	Light.flags = LITE_ON;
	Light.light.dltType = piLIGHT_DIRECTIONAL;
    Light.light.dcvColor.r = s->_moon.r;
	Light.light.dcvColor.g = s->_moon.g;
	Light.light.dcvColor.b = s->_moon.b;
	Light.light.dcvColor.a = 0.0f;
	Light.light.dcvSpecular.r = s->_moon.r;
	Light.light.dcvSpecular.g = s->_moon.g;
	Light.light.dcvSpecular.b = s->_moon.b;
	Light.light.dcvSpecular.a = 0.0f;
	Light.light.dcvAmbient.r = s->_moon.r;
	Light.light.dcvAmbient.g = s->_moon.g;
	Light.light.dcvAmbient.b = s->_moon.b;
	Light.light.dcvAmbient.a = 0.0f;
	Light.light.dvPosition.x = 0;
	Light.light.dvPosition.y = 0;
	Light.light.dvPosition.z = -1.0;
	Light.light.dvRange = 0.0f;//((float)sqrt(FLT_MAX));
	Light.light.dvFalloff = 1.0f;
	Light.light.dvAttenuation0 = 0.0f;
	Light.light.dvAttenuation1 = 0.0f;
	Light.light.dvAttenuation2 = 0.0f;
	Light.light.dvTheta = 0.0f;//PI / 8;
	Light.light.dvPhi = 0.0f;//PI / 16;

	if (!obdliteAddDynaLite(&Light, nullptr))
	{
		return(FALSE);
	}

	_MoonLight = obdliteFindDynaLight("moon");

	return 0;
}

LIGHT* trnskyGetMoonSource()
{
	return(_MoonLight);
}

static void _resetsun(LIGHT* light)
{
    TERRAINSKY* s = trndataGetSkyData(_setting);

    light->light.dcvAmbient.r = s->_skyclearambient.r;
    light->light.dcvAmbient.g = s->_skyclearambient.g;
    light->light.dcvAmbient.b = s->_skyclearambient.b;

    light->light.dcvSpecular.r = s->_sunnoon.r;
    light->light.dcvSpecular.g = s->_sunnoon.g;
    light->light.dcvSpecular.b = s->_sunnoon.b;

    light->light.dvFalloff = 1.0f;
}

static void InterpolateRGB(piCOLORVALUE& tgt, piCOLORVALUE& start, piCOLORVALUE& end, float t)
{
    tgt.r = start.r + t * (end.r - start.r);
    tgt.g = start.g + t * (end.g - start.g);
    tgt.b = start.b + t * (end.b - start.b);
}

static void InterpolateIfNecessary(TERRAINSKY& c)
{
    if(playpos.z < RAIN_CLOUD_HEIGHT)
    {
        // Stormy
        c = *trndataGetSkyData(2);

        if(fxWeatherIsRaining())
            rainfogclip = MIN_FOG_SCALE;
    }
    else if(playpos.z >= START_SUNSHINE_HEIGHT)
    {
        // Sunshine
        c = *trndataGetSkyData(0);
        rainfogclip = MAX_FOG_SCALE;
    }
    else
    {
        // Interpolate
        float t = (playpos.z - RAIN_CLOUD_HEIGHT) / INTEROPLATE_DIST;
        t = 1.0f - t;
        t = t < 0 ? 0 : t > 1 ? 1 : t;

        if(fxWeatherIsRaining())
        {
            rainfogclip = MIN_FOG_SCALE + (1.0f - t) * (MAX_FOG_SCALE - MIN_FOG_SCALE);
        }

        TERRAINSKY* s1 = trndataGetSkyData(0);
        TERRAINSKY* s2 = trndataGetSkyData(2);

        InterpolateRGB(c._horizon, s1->_horizon, s2->_horizon, t);
        InterpolateRGB(c._moon, s1->_moon, s2->_moon, t);
        InterpolateRGB(c._moonday, s1->_moonday, s2->_moonday, t);
        InterpolateRGB(c._skyclearambient, s1->_skyclearambient, s2->_skyclearambient, t);
        InterpolateRGB(c._skycolor, s1->_skycolor, s2->_skycolor, t);
        InterpolateRGB(c._skycolornight, s1->_skycolornight, s2->_skycolornight, t);
        InterpolateRGB(c._skynightambient, s1->_skynightambient, s2->_skynightambient, t);
        InterpolateRGB(c._sunitselfnoon, s1->_sunitselfnoon, s2->_sunitselfnoon, t);
        InterpolateRGB(c._sunnight, s1->_sunnight, s2->_sunnight, t);
        InterpolateRGB(c._sunnoon, s1->_sunnoon, s2->_sunnoon, t);
        InterpolateRGB(c._sunset, s1->_sunset, s2->_sunset, t);
    }
}

static void _UpdateSunLight(LIGHT* light, const TERRAINSKY* s)
{
	UINT32 TimeOfDay;
	FLOAT32 Angle;
	piVECTOR Vec, Dir;
	FLOAT32 TempNum;
	FLOAT32 Red,Green,Blue,Alpha;
	UINT32 Color;
	FLOAT64VECTOR WorldOriginOffset;
	FLOAT32 PlayerSunDotProduct;

	trnTODGetTimeOfDay(&TimeOfDay);
	trnGetWorldOriginOffset(&WorldOriginOffset);

	sunelevation = (FLOAT32)(fmod(2*PI*TimeOfDay/86400,2*PI));

	light->light.dvPosition.x = -(FLOAT32)sin(sunelevation);
	light->light.dvPosition.y = -(FLOAT32)(0.2 + -cos(sunelevation) * 0.2);
	light->light.dvPosition.z = -(FLOAT32)cos(sunelevation);

	Dir.x = (FLOAT32)sin(sunelevation);
	Dir.y = 0.0f;
	Dir.z = (FLOAT32)cos(sunelevation);

	PlayerSunDotProduct = mathVectDotProduct(&Dir,&playdir);

	Vec.x = 0;
	Vec.y = 0;
	Vec.z = 1.0;

	TempNum = mathVectDotProduct(&Vec,&Dir);

	Angle = -(FLOAT32)mlRAD_TO_DEG(asin(TempNum));
	_SunAngle = Angle;

	if (_sunfirsttime)
	{
		obdliteSetDynaState(light, FALSE);
		_sunfirsttime = false;
		_resetsun(light);
	}

	// Night time, the sun is disabled, the moon lights the world.
	if(Angle < -10.0)
	{
		obdliteSetDynaState(light, FALSE);

		// Hide the sun
		sundisk->child->flag2 &= ~OBJECT_HIDDEN;
	}

// Sunrise and sunset
	// As the sun rises it's light gets faded in
	// and as it falls it's light gets faded out.
	else if (Angle >= -10.0 && Angle < 0.0)
	{
		obdliteSetDynaState(light, TRUE);

		FLOAT32 t = (FLOAT32)((Angle) / -10.0);

		t = 1.0f - std::max(std::min(t, 1.0f), 0.8f);

		Red = t * s->_skyclearambient.r;
		Green = t * s->_skyclearambient.g;
		Blue = t * s->_skyclearambient.b;
		//light->light.dcvAmbient.set(Red, Green, Blue, 1.0);

		light->light.dcvSpecular.r = t * s->_sunset.r;
		light->light.dcvSpecular.g = t * s->_sunset.g;
		light->light.dcvSpecular.b = t * s->_sunset.b;

		light->light.dcvColor.r = t * s->_sunset.r;
		light->light.dcvColor.g = t * s->_sunset.g;
		light->light.dcvColor.b = t * s->_sunset.b;
		light->light.dcvColor.a = t;

		// The Falloff value controls how light intensity decreases
		// between the outer edge of the inner cone and the inner edge
		// of the outer cone. This is not used, so using it for interpolating
		// between sun on/off states. See obShader.
		light->light.dvFalloff = t;
		// Make the sun visible
		sundisk->child->flag2 |= OBJECT_HIDDEN;
	}
	// Sunrise and sunset
	// Fades the colour of the sun from daylight to a nice orange sunset
	// and vice versa in the morning.
	else if (Angle >= 0.0 && Angle < 15.0)
	{
		obdliteSetDynaState(light, TRUE);

		FLOAT32 t = (FLOAT32)((Angle) / 15.0);

		t = std::max(std::min(t, 1.0f), 0.8f);

		Red = t * s->_skyclearambient.r;
		Green = t * s->_skyclearambient.g;
		Blue = t * s->_skyclearambient.b;
		light->light.dcvAmbient.set(Red, Green, Blue, 1.0);

		light->light.dcvSpecular.r = (FLOAT32)(t * s->_sunnoon.r + (1.0 - t) * s->_sunset.r);
		light->light.dcvSpecular.g = (FLOAT32)(t * s->_sunnoon.g + (1.0 - t) * s->_sunset.g);
		light->light.dcvSpecular.b = (FLOAT32)(t * s->_sunnoon.b + (1.0 - t) * s->_sunset.b);

		light->light.dcvColor.r = (FLOAT32)(t * s->_sunnoon.r + (1.0 - t) * s->_sunset.r);
		light->light.dcvColor.g = (FLOAT32)(t * s->_sunnoon.g + (1.0 - t) * s->_sunset.g);
		light->light.dcvColor.b = (FLOAT32)(t * s->_sunnoon.b + (1.0 - t) * s->_sunset.b);
		light->light.dcvColor.a = 1.0f;

		// The Falloff value controls how light intensity decreases
		// between the outer edge of the inner cone and the inner edge
		// of the outer cone. This is not used, so using it for interpolating
		// between sun on/off states. See obShader.
		light->light.dvFalloff = t;
		// Show the sun
		sundisk->child->flag2 |= OBJECT_HIDDEN;
	}
	// Daytime.
	else
	{
		obdliteSetDynaState(light, TRUE);

		Red = s->_skyclearambient.r;
		Green = s->_skyclearambient.g;
		Blue = s->_skyclearambient.b;
		light->light.dcvAmbient.set(Red, Green, Blue, 1.0);

		light->light.dcvSpecular.r = s->_sunnoon.r;
		light->light.dcvSpecular.g = s->_sunnoon.g;
		light->light.dcvSpecular.b = s->_sunnoon.b;

		light->light.dcvColor.r = s->_sunnoon.r;
		light->light.dcvColor.g = s->_sunnoon.g;
		light->light.dcvColor.b = s->_sunnoon.b;
		light->light.dcvColor.a = 1.0f;
		light->light.dvFalloff = 1.0f;

		sundisk->child->flag2 |= OBJECT_HIDDEN;
	}
	// Apply the above changes to the light.
    obdliteSetDynaLite(light);

 // Use the colours calculated above to change the sundisk colour.
	Color = piRGBA_MAKE((UINT32)(Red*255.0), (UINT32)(Green*255.0), (UINT32)(Blue*255.0), (UINT32)Alpha);
	_ColorObject(sundisk, Color);



// horizon color

	// After the sun has set
	if (Angle < -10.0)
	{
        Red = s->_skynightambient.r;
        Green = s->_skynightambient.g;
        Blue = s->_skynightambient.b;
	}
	// Sunset and sunrise
	else if(Angle >= -10.0 && Angle <= 15.0)
	{
		TempNum = (FLOAT32)((Angle + 10.0) / 25.0);

        Red = (FLOAT32)(s->_horizon.r * TempNum + s->_skynightambient.r * (1.0 - TempNum));
		Green = (FLOAT32)(s->_horizon.g * TempNum + s->_skynightambient.g * (1.0 - TempNum));
		Blue = (FLOAT32)(s->_horizon.b * TempNum + s->_skynightambient.b * (1.0 - TempNum));
	}
	// Daytime.
	else
	{
        Red = s->_horizon.r;
        Green = s->_horizon.g;
        Blue = s->_horizon.b;
	}

	_FogColor = FogColor = piRGBA_MAKE((UINT32)(Red*255.0), (UINT32)(Green*255.0), (UINT32)(Blue*255.0), 255);

	_ColorObject(horiz, FogColor);
	_ColorObject(horiztop, FogColor);




// sky color

	// After the sun has set. 
	if (Angle < -10.0)
	{
        Red = s->_skycolornight.r;
        Green = s->_skycolornight.g;
        Blue = s->_skycolornight.b;
	}
	// sunset and sunrise.
	else
	if (Angle >= -10.0 && Angle <= 15.0)
	{
		TempNum = (FLOAT32)((Angle + 10.0) / 25.0);

        Red = (FLOAT32)((s->_skycolor.r * TempNum) + (s->_skycolornight.r * (1.0 - TempNum)));
        Green = (FLOAT32)((s->_skycolor.g * TempNum) + (s->_skycolornight.g * (1.0 - TempNum)));
        Blue = (FLOAT32)((s->_skycolor.b * TempNum) + (s->_skycolornight.b * (1.0 - TempNum)));
	}
	// Daytime.
	else
	{
        Red = s->_skycolor.r;
        Green = s->_skycolor.g;
        Blue = s->_skycolor.b;
	}

	Color = piRGBA_MAKE((UINT32)(Red*255.0), (UINT32)(Green*255.0), (UINT32)(Blue*255.0), 255);

	_ColorObject(sky,Color);

	_FogUpdate();
}

static void _resetmoon(LIGHT* light)
{
    TERRAINSKY* s = trndataGetSkyData(_setting);

    light->light.dcvAmbient.r = s->_moon.r;
    light->light.dcvAmbient.g = s->_moon.g;
    light->light.dcvAmbient.b = s->_moon.b;

    light->light.dcvSpecular.r = s->_moon.r;
    light->light.dcvSpecular.g = s->_moon.g;
    light->light.dcvSpecular.b = s->_moon.b;

    light->light.dvFalloff = 1.0f;
}

static void _UpdateMoonLight(LIGHT* light, const TERRAINSKY* s)
{
	UINT32 TimeOfDay;
	FLOAT32 Angle;
	piVECTOR Vec, Dir;
	FLOAT32 TempNum;
	//FLOAT32 Red, Green, Blue, Alpha;

	trnTODGetTimeOfDay(&TimeOfDay);

	moonelevation = (FLOAT32)(fmod(2*PI*TimeOfDay/86400,2*PI));

	light->light.dvPosition.x = (FLOAT32)sin(moonelevation);
	light->light.dvPosition.y = -(FLOAT32)(0.2 + cos(moonelevation) * 0.2);
	light->light.dvPosition.z = (FLOAT32)cos(moonelevation);

	Dir.x = (FLOAT32)sin(moonelevation);
	Dir.y = 0.0f;
	Dir.z = (FLOAT32)cos(moonelevation);

	// now determine lighting
	Vec.x = 0;
	Vec.y = 0;
	Vec.z = 1.0;

	TempNum = mathVectDotProduct(&Vec,&Dir);
	Angle = -(FLOAT32)mlRAD_TO_DEG(asin(TempNum));

	if (_moonfirsttime)
	{
		obdliteSetDynaState(light, FALSE);
		_moonfirsttime = false;
		_resetmoon(light);
	}


	// Daytime, the moon is disabled, the sun lights the world.
	if (Angle > 10.0)
	{
		obdliteSetDynaState(light, FALSE);

		// Hide the moon
		moondisk->child->flag2 |= OBJECT_HIDDEN;
		
	}
	// As the moon rises it's light gets faded in
	// and as it falls it's light gets faded out.
	else if (Angle <= 10.0 && Angle > -15.0)
	{
		obdliteSetDynaState(light, TRUE);

		FLOAT32 t = (FLOAT32)((Angle - 10.0) / -20.0);

		t = std::max(std::min(t, 1.0f), 0.8f);

		light->light.dcvAmbient.r = t * s->_skynightambient.r;
		light->light.dcvAmbient.g = t * s->_skynightambient.g;
		light->light.dcvAmbient.b = t * s->_skynightambient.b;

		light->light.dcvSpecular.r = t * s->_moon.r;
		light->light.dcvSpecular.g = t * s->_moon.g;
		light->light.dcvSpecular.b = t * s->_moon.b;

		light->light.dcvColor.r = t * s->_skynightambient.r;
		light->light.dcvColor.g = t * s->_skynightambient.g;
		light->light.dcvColor.b = t * s->_skynightambient.b;
		light->light.dcvColor.a = 1.0f;

		// The Falloff value controls how light intensity decreases
		// between the outer edge of the inner cone and the inner edge
		// of the outer cone. This is not used, so using it for interpolating
		// between sun on/off states. See obShader.
		light->light.dvFalloff = t;
		// Show the moon
		moondisk->child->flag2 &= ~OBJECT_HIDDEN;
	}
	// Night time.
	else
	{
		obdliteSetDynaState(light, TRUE);

		light->light.dcvAmbient.r = s->_skynightambient.r;
		light->light.dcvAmbient.g = s->_skynightambient.g;
		light->light.dcvAmbient.b = s->_skynightambient.b;

		light->light.dcvSpecular.r = s->_moon.r;
		light->light.dcvSpecular.g = s->_moon.g;
		light->light.dcvSpecular.b = s->_moon.b;

		light->light.dcvColor.r = s->_skynightambient.r;
		light->light.dcvColor.g = s->_skynightambient.g;
		light->light.dcvColor.b = s->_skynightambient.b;
		light->light.dcvColor.a = 1.0f;
		light->light.dvFalloff = 1.0f;

		moondisk->child->flag2 &= ~OBJECT_HIDDEN;
	}
	
	// Apply the above changes to the light.
	obdliteSetDynaLite(light);
}

static UINT32 _ColorObject(OBJECT * Object,UINT32 Color)
{
	OBJECT * ObjAtt;
	UINT32 i;

	if (Object == nullptr)
	{
		return 0;
	}

	ObjAtt = Object;

	if (!(ObjAtt->flags & OBJECT_NOTOBJECT))
	{
		do
		{
			piVertexColorPtr sc = ObjAtt->MutableColors();
			// ugh, actually modifying pool color for sun, but that's what it was doing before.
			piUNIFIED_VERTEX* uc = ObjAtt->puverts;

			for (i = 0; i < ObjAtt->numverts; i++, ++sc, ++uc)
			{
				sc->color = uc->color = Color;
			}

			ObjAtt = ObjAtt->attached;
		}
		while (ObjAtt != nullptr);
	}

	if (Object->child != nullptr)
	{
		_ColorSubObject(Object->child,Color);
	}

	return 0;
}

static UINT32 _ColorSubObject(OBJECT * Object,UINT32 Color)
{
	OBJECT * ObjAtt,*ObjSib;
	UINT32 i;

	if (Object == nullptr)
	{
		return 0;
	}

	ObjSib = Object;

	do
	{

		ObjAtt = ObjSib;

		if (!(ObjAtt->flags & OBJECT_NOTOBJECT))
		{
			do
			{
				piVertexColorPtr sc = ObjAtt->MutableColors();
				// ugh, actually modifying pool color for sun, but that's what it was doing before.
				piUNIFIED_VERTEX* uc = ObjAtt->puverts;

				for (i = 0; i < ObjAtt->numverts; i++, ++sc, ++uc)
				{
					sc->color = uc->color = Color;
				}

				ObjAtt = ObjAtt->attached;
			}
			while (ObjAtt != nullptr);

			if (ObjSib->child != nullptr)
			{
				_ColorSubObject(ObjSib->child,Color);
			}
		}

		ObjSib = ObjSib->sibling;
	}
	while (ObjSib != nullptr);

	return 0;
}

#define FOG_CLOSURE_ALTITUDE	400.0f

#define MAXIMUM_SCALEFACTOR		2.0f

void _setStartEndDensity()
{
    // Adjust start/end densities based on timeofday
    FOGOPTION* f = trndataGetFogData(_setting, _fogoption);

    if(!f || f->time.empty())
    {
        return;
    }

    _fogoption = f->fogoption - 1;

    UINT32 timeofday;

    trnTODGetTimeOfDay(&timeofday);

    FLOAT32 t = 0.0f;

    // No interpolation...set to default
    if(f->time.size() == 1)
    {
        _startdensity = f->start[0];
        _enddensity = f->end[0];

        return;
    }

    for(UINT32 i = 0; i < f->time.size() - 1; i++)
    {
        if((SINT32)timeofday >= f->time[i] && (SINT32)timeofday < f->time[i + 1])
        {
            SINT32 t1 = f->time[i];
            SINT32 t2 = f->time[i + 1];

            t = (FLOAT32)(timeofday - t1) / (FLOAT32)(t2 - t1);

            FLOAT32 s1 = f->start[i];
            FLOAT32 s2 = f->start[i + 1];

            FLOAT32 e1 = f->end[i];
            FLOAT32 e2 = f->end[i + 1];

            _startdensity = s1 + t * (s2 - s1);
            _enddensity = e1 + t * (e2 - e1);

            break;
        }
    }
}

FLOAT32 trnskyGetAltitude()
{
    return _altitude;
}

SINT32 trnskyGetSetting()
{
    return _setting;
}

SINT32 trnskyGetFogOption()
{
    return _fogoption;
}

void _UpdateFogSphere()
{
	FLOAT32 Alt, AltStep, InvFactor;
	UINT32 Poly;
	piVECTOR Pos;
	FLOAT32 FogStart,FogEnd;
	FLOAT32 ScaleFactor;
	FLOAT32 ZBufferNearVal;
	FLOAT32 FogScaleFactor;
	UINT32 TempNum;

	Alt = -playpos.z;

	Pos.x = -playpos.x;
	Pos.y = -playpos.y;
	Pos.z = -playpos.z;
	trnopCollideDown(&Pos, (SINT32*)&Poly, &Pos);

	Alt = -(Alt - Pos.z);

    _altitude = Alt;

    // [DCF] HACK! Make ship fog near/far 'special cased'.
    if(persIsPersonaVehicleType(VEH_TYPE_SEA))
    {
        vehVehicle*v = persGetPersonaVehiclePtr();

        if(v)
        {
            static WEAPON_SPEC spec = NULL_WEAP_SPEC;
            static bool firsttime = 1;

            if(firsttime)
            {
                firsttime = false;
                spec = weapFindClass("sea");
            }

            if(spec.wClass == v->VehicleType->WeaponSpec.wClass)
            {
                Alt = 500.0f;
            }
        }
    }

	AltStep = FOG_CLOSURE_ALTITUDE / MAXIMUM_SCALEFACTOR;
	InvFactor = 1.0f / MAXIMUM_SCALEFACTOR;

#ifdef USE_LINEAR_FOG
	if (Alt < 0)
	{
		ScaleFactor = 10.0;
	}
	else
#endif
	{
#ifdef USE_LINEAR_FOG
		if (Alt <= FOG_CLOSURE_ALTITUDE - AltStep)
		{
			ScaleFactor = (FLOAT32)((FOG_CLOSURE_ALTITUDE - Alt) / AltStep);
		}
		else
		{
			ScaleFactor = 1.0f;
		}
#else
        ScaleFactor = 1.0f;
#endif
	}

	if (ScaleFactor > MAXIMUM_SCALEFACTOR)
	{
		ScaleFactor = MAXIMUM_SCALEFACTOR;
	}

	if (Alt < 0.5f)
	{
		ZBufferNearVal = 0.2f;
	}
	else if (Alt >= 0.5f && Alt < 1.0f)
	{
		ZBufferNearVal = 0.3f * (Alt - 0.5f) / 0.5f + 0.2f;
	}
	else //(Alt > 1.0f)
	{
		ZBufferNearVal = 0.5f;
	}

	if (ZBufferNearVal > 0.25)
	{
		ZBufferNearVal = 0.25;
	}

	rnmainSetZBufferNearVal(ZBufferNearVal);

	rnmainGetDefaultFogAttributes(&TempNum,&FogStart,&FogEnd);

	FogFactor = ((FLOAT32)(math3dFastSqrt(FogEnd * FogEnd + Alt * Alt) / FogEnd) / ScaleFactor);
	if (FogFactor < InvFactor)
	{
		FogFactor = InvFactor;
	}

	_SkyDomeScaleFactor = 0.95f / ScaleFactor;//InvFactor / FogFactor;

	FogScaleFactor = persGetFogScaleMultiplier();
	FogFactor *= FogScaleFactor;

    // Adjust fog for rain days
    FogFactor *= rainfogclip;

#ifndef USE_LINEAR_FOG
    _setStartEndDensity();

    FLOAT32 altitude = Alt <  FOG_CLOSURE_ALTITUDE ? FOG_CLOSURE_ALTITUDE : Alt;

    FLOAT32 t = FOG_CLOSURE_ALTITUDE / altitude;

    t = t > 1.0f ? 1.0f : t;

    FLOAT32 factor = t * (_startdensity - _enddensity);
    factor = factor < 0 ? 0 : factor;

    // Fog decreases as altitude increases
    FLOAT32 density = _enddensity + factor;

    FogDensity = piFloatLikeLong(density);

    piSetRenderState(piRENDERSTATE_FOGDENSITY, FogDensity);
#endif

	if (Alt < 0.0)
	{
        if(gCustomTerrain)
        {
            float start, end;
            unsigned int fogcolor;

            TerrainScenario::GetInstance().GetFogParams(start, end, fogcolor);
		    rnmainAdjustFogAttributes(fogcolor, start, end);
        }
        else
		    rnmainAdjustFogAttributes(_FogColor, FogStart * FogFactor, FogEnd * FogFactor);
	}
	else
	{
        if(gCustomTerrain)
        {
            float start, end;
            unsigned int fogcolor;

            TerrainScenario::GetInstance().GetFogParams(start, end, fogcolor);
		    rnmainAdjustFogAttributes(fogcolor, start, end);
        }
        else
    		rnmainAdjustFogAttributes(_FogColor, FogStart * FogFactor, FogEnd * FogFactor);
	}


	// get the lower part of the sky hemisphere to fade into fog color
	_FogUpdate();
}

void trnskyGetFogDensity(FLOAT32& start, FLOAT32& end)
{
    start = _startdensity;
    end = _enddensity;
}

void trnskySetFogStartDensity(FLOAT32 start)
{
    _startdensity = start;
}

void trnskySetFogEndDensity(FLOAT32 end)
{
    _enddensity = end;
}

void trnskyResetFade()
{
    _moonfirsttime = true;
    _sunfirsttime = true;
}
