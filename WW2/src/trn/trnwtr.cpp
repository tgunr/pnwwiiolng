#include <syscfg.h> // Path is project specific.
//
#include <string>

#include <math.h>
#include "pi/pi.h"
#include "include/vars.h"
#include "ob/obmain.h"
#include "ob/obdraw.h"
#include "ob/obplyr.h"
#include "ob/obc.h"
#include "trn/trnmain.h"
#include "trn/trnwtr.h"
#include "ld/ldmdl.h"
#include "math/math3d.h"
#include "ld/ldpix.h"
#include "str/strpool.h"
#include "ogl/ogl.h"
#include "wwii/wwiiCovert.h"
#include "shader/Shader.h"
#include "ob/obdlite.h"
#include "fio/fio.h"
#include "math/mathUnitConversion.h"
#include "ld/lddds.h"
#include "rn/rnmain.h"
#include <set>
#include "ogl/oglGlobalStates.h"
#include "ui/uiChat.h"
#include "ogl/oglMaterial.h"

static std::set<SINT32> _waterids;

struct WATERFBO
{
    SINT32 id;
    SINT32 depth;
    SINT32 w;
    SINT32 h;
};

// Defines
#define NUM_WATER_TEXTURES		8

#undef REVERSE_OPENGL_COLORS

#if platOS == OS_WIN32
#define REVERSE_OPENGL_COLORS
#endif

// Statics

// These are all the water textures in the game that we want to animate (or manipulate in some way)
static const SCHAR* waternames[NUM_WATER_TEXTURES] =
{
    "water1.bmp",
    "to\\water1.bmp",
    "to\\rivers1.bmp",
    "to\\rivers2.bmp",
    "to\\rivers3.bmp",
    "to\\rivers4.bmp",
    "to\\rivermouth1r.bmp",
    "to\\coast1-4r3w.bmp"
};

#define NUM_WATER_NAMES NUM_WATER_TEXTURES - 1

static piMATRIX TextureMatrix, RepeatMatrix;
static UINT32 curtime, lastime;

//static SINT32 caustics[32];
static SINT32 caustic_bumps[32];
static SINT32 caustic;
//static SINT32 reflectid;

#if(0)
static void _loadShaderWater();
#endif

LOGICAL trnwtrInit( const SINT8* name)
{
    char filename[_MAX_PATH];
    int id;

	for(int i = 0; i < NUM_WATER_TEXTURES; i++)
	{
		strcpy(filename, TERRAIN_PREPATH);
		strcat(filename, waternames[i]);
		id = ldpixLoadImageFileToSurface(filename, colorMAGENTA, LDPIX_MIPMAP);

		if(!piIsTexture(id, 0))
		{
			return(FALSE);
		}

        _waterids.insert(id);
	}

	return(TRUE);
}

void _loadShaderWater()
{
}

void trnwtrSetRenderPass()
{
}

void trnwtrKill()
{
#ifdef USE_WATER_EFFECT

    std::set<SINT32>::iterator b = _waterids.begin();
    std::set<SINT32>::iterator e = _waterids.end();

    for(; b != e; ++b)
    {
        piFreeMipTextures(*b);
    }

    _waterids.clear();
#endif

    WriteLog(LOG_OLD, LOG_INFO, "[trnwtrKill]");
}

LOGICAL trnwtrIsWaterTexture(SINT32 texid)
{
#ifdef USE_WATER_EFFECT

    if(_waterids.find(texid) != _waterids.end())
    {
        return TRUE;
    }

#endif

	return FALSE;
}

LOGICAL trnwtrUpdate()
{
#ifdef USE_WATER_EFFECT
	SINT32		i;
	UINT32		deltime;
	static FLOAT32		effect_cos=0.0f, effect_sin=0.0f;

	curtime = frametime;
	deltime = curtime - lastime;
	lastime = curtime;

	effect_cos += 0.0f;
	if (effect_cos >= PI2)
	{
		effect_cos -= PI2;
	}

	effect_sin += 0.0f;
	if (effect_sin >= PI2)
	{
		effect_sin -= PI2;
	}

	TextureMatrix = IdentityMatrix;
	TextureMatrix._41 = effect_cos;
	TextureMatrix._42 = effect_sin;

	RepeatMatrix._11 = 6.0f;
	RepeatMatrix._22 = 6.0f;

	for (i=0 ; i < ft50msticks ; i++)
	{
		caustic += 1;
		if (caustic >= 32)
		{
			caustic = 0;
		}
	}
#endif

	return(TRUE);
}

#if(0)
static SINT32		gte[8][24];

static void SaveTexEnv(int stage)
{
	SINT32		setstage;

	glGetIntegerv(GL_ACTIVE_TEXTURE_ARB,  (GLint*)&setstage);
	if (GL_TEXTURE0_ARB + stage == setstage)
	{
		glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE_ARB, (GLint*)&gte[stage][0]);
		glGetTexEnviv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, (GLint*)&gte[stage][1]);
		glGetTexEnviv(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, (GLint*)&gte[stage][2]);
		glGetTexEnviv(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB,  (GLint*)&gte[stage][3]);
		glGetTexEnviv(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB,  (GLint*)&gte[stage][4]);
		glGetTexEnviv(GL_TEXTURE_ENV, GL_SOURCE2_RGB_ARB,  (GLint*)&gte[stage][5]);
		glGetTexEnviv(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB,  (GLint*)&gte[stage][6]);
		glGetTexEnviv(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB,  (GLint*)&gte[stage][7]);
		glGetTexEnviv(GL_TEXTURE_ENV, GL_OPERAND2_RGB_ARB,  (GLint*)&gte[stage][8]);
		glGetTexEnviv(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB,  (GLint*)&gte[stage][9]);
		glGetTexEnviv(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB,  (GLint*)&gte[stage][10]);
		glGetTexEnviv(GL_TEXTURE_ENV, GL_SOURCE1_ALPHA_ARB,  (GLint*)&gte[stage][11]);
		glGetTexEnviv(GL_TEXTURE_ENV, GL_SOURCE2_ALPHA_ARB,  (GLint*)&gte[stage][12]);
		glGetTexEnviv(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB,  (GLint*)&gte[stage][13]);
		glGetTexEnviv(GL_TEXTURE_ENV, GL_OPERAND1_ALPHA_ARB,  (GLint*)&gte[stage][14]);
		glGetTexEnviv(GL_TEXTURE_ENV, GL_OPERAND2_ALPHA_ARB,  (GLint*)&gte[stage][15]);
		glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,  (GLint*)&gte[stage][16]);
		glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,  (GLint*)&gte[stage][17]);
		glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R,  (GLint*)&gte[stage][18]);
		glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,  (GLint*)&gte[stage][19]);
		glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,  (GLint*)&gte[stage][20]);
		glGetTexGeniv(GL_S, GL_TEXTURE_GEN_MODE,  (GLint*)&gte[stage][21]);
		glGetTexGeniv(GL_T, GL_TEXTURE_GEN_MODE,  (GLint*)&gte[stage][22]);
		glGetTexGeniv(GL_R, GL_TEXTURE_GEN_MODE,  (GLint*)&gte[stage][23]);
	}
}
#endif

#if(0)
static void RestoreTexEnv(int stage)
{
	SINT32		setstage;

	glGetIntegerv(GL_ACTIVE_TEXTURE_ARB,  (GLint*)&setstage);
	if (GL_TEXTURE0_ARB + stage == setstage)
	{
		oglSetActiveTextureStage(stage);

		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, gte[stage][1]);
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, gte[stage][2]);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, gte[stage][3]);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, gte[stage][4]);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE2_RGB_ARB, gte[stage][5]);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, gte[stage][6]);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, gte[stage][7]);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND2_RGB_ARB, gte[stage][8]);
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, gte[stage][9]);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, gte[stage][10]);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_ALPHA_ARB, gte[stage][11]);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE2_ALPHA_ARB, gte[stage][12]);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, gte[stage][13]);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_ALPHA_ARB, gte[stage][14]);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND2_ALPHA_ARB, gte[stage][15]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, gte[stage][16]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, gte[stage][17]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, gte[stage][18]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gte[stage][19]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gte[stage][20]);
		glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, gte[stage][21]);
		glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, gte[stage][22]);
		glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, gte[stage][23]);
	}
}
#endif

static LOGICAL ApplyBumpmapTextureStage(OBJECT* lpobj, piMATRIX* mat, SINT32 texid1, SINT32 texid2, const FLOAT32* ptu)
{
#ifdef USE_WATER_EFFECT
//	SINT32				tgen;

	if (bTexturesDisabled)
	{
		return TRUE;
	}

	piSetRenderState(piRENDERSTATE_ALPHABLENDENABLE, TRUE);

	// Update the color elements with a direction vector
	obdrawUpdateDOT3ColorDirectional(lpobj);

	oglSetActiveTextureStage(0);

	//SaveTexEnv(0);

	piSetActiveTexture(texid2, 0);

	piSetTextureMatrix(&RepeatMatrix);

	piSetRenderState(piRENDERSTATE_SRCBLEND, piBLEND_SRCCOLOR);
	piSetRenderState(piRENDERSTATE_DESTBLEND, piBLEND_INVSRCCOLOR);

    piSetRenderStageState(0, piTSS_TEXTURE_ENV_MODE, piTSS_COMBINE);
    piSetRenderStageState(0, piTSS_MINFILTER, piTFN_LINEAR_MIPMAP_LINEAR);
	piSetRenderStageState(0, piTSS_COMBINE_RGB, piTSS_DOT3_RGB);
	piSetRenderStageState(0, piTSS_SOURCE0_RGB, piTSS_TEXTURE);
	piSetRenderStageState(0, piTSS_SOURCE1_RGB, piTSS_PRIMARY_COLOR);
	piSetRenderStageState(0, piTSS_OPERAND0_RGB, piTSS_SRC_COLOR);
	piSetRenderStageState(0, piTSS_OPERAND1_RGB, piTSS_SRC_COLOR);

	ogl::SetTextureUnitCoordPointer(GL_TEXTURE1, 2, GL_FLOAT, sizeof(piUNIFIED_VERTEX), ptu);

	oglSetActiveTextureStage(1);

	//SaveTexEnv(1);

	piSetActiveTexture(texid1, 1);

	piSetTextureMatrix(mat);
	piSetRenderStageState(1, piTSS_MINFILTER, piTFN_LINEAR_MIPMAP_LINEAR);

	piSetRenderStageState(1, piTSS_TEXTURE_ENV_MODE, piTSS_COMBINE);

	piSetRenderStageState(1, piTSS_COMBINE_RGB, piTSS_MODULATE);
	piSetRenderStageState(1, piTSS_SOURCE0_RGB, piTSS_TEXTURE);
	piSetRenderStageState(1, piTSS_SOURCE1_RGB, piTSS_PREVIOUS);
	piSetRenderStageState(1, piTSS_OPERAND0_RGB, piTSS_SRC_COLOR);
	piSetRenderStageState(1, piTSS_OPERAND1_RGB, piTSS_SRC_COLOR);

	oglSetActiveTextureStage(0);
#endif

	return(TRUE);
}

static LOGICAL ReleaseBumpmapTextureStage()
{
#ifdef USE_WATER_EFFECT
	if (bTexturesDisabled)
	{
		return TRUE;
	}

	piSetRenderState(piRENDERSTATE_ALPHABLENDENABLE, piALPHABLENDENABLE_DEFAULT);
	piSetRenderState(piRENDERSTATE_SRCBLEND, SrcBlend);
	piSetRenderState(piRENDERSTATE_DESTBLEND, DestBlend);

	piSetRenderStageState(0, piTSS_TEXTURE_ENV_MODE, piTSS_MODULATE);

	oglSetActiveTextureStage(0);

	//RestoreTexEnv(0);

	piDisableTexture();
	piSetTextureMatrix(&IdentityMatrix);

	oglSetActiveTextureStage(1);

	//RestoreTexEnv(1);

	piDisableTexture();
	piSetTextureMatrix(&IdentityMatrix);

	oglSetActiveTextureStage(0);

#endif

	return(TRUE);
}

LOGICAL trnwtrPreDraw(OBJECT* lpobj)
{
#ifdef USE_WATER_EFFECT
	piMATRIX			mat;
	piTexCoordConstPtr	ptu;

	mat = IdentityMatrix;
//	math3dMatrixRotate(&mat, &lpobj->att);
	math3dMatrixMultiply(&mat, &TextureMatrix, &mat);

	ptu = lpobj->ImmutableTexCoords();

	//ApplyBumpmapTextureStage(lpobj, caustics[caustic], caustic_bumps[caustic], &ptu->u);
	ApplyBumpmapTextureStage(lpobj, &mat, lpobj->texid, caustic_bumps[caustic], &ptu->u);
#endif

	return(TRUE);
}

LOGICAL trnwtrPreDraw2(OBJECT* lpobj)
{
#ifdef USE_WATER_EFFECT
	piMATRIX		mat;

	mat = IdentityMatrix;
//	math3dMatrixRotate(&mat, &lpobj->att);
	math3dMatrixMultiply(&mat, &TextureMatrix, &mat);

	//ApplyBumpmapTextureStage(lpobj, caustics[caustic], caustic_bumps[caustic], &lpobj->ptverts->tu);
	ApplyBumpmapTextureStage(lpobj, &mat, lpobj->texid, caustic_bumps[caustic], &lpobj->ptverts->tu);
#endif

	return(TRUE);
}

LOGICAL trnwtrPreDrawMultitexture(OBJECT* lpobj, piMATRIX* mtmat)
{
#ifdef USE_WATER_EFFECT
	piMATRIX		mat;

	mat = IdentityMatrix;
	math3dMatrixRotate(&mat, &lpobj->att);
	math3dMatrixMultiply(&mat, &TextureMatrix, &mat);
	math3dMatrixMultiply(&mat, &mat, mtmat);
	piSetTextureMatrix(&mat);
#endif

	return(TRUE);
}

LOGICAL trnwtrPostDraw()
{
#ifdef USE_WATER_EFFECT
	ReleaseBumpmapTextureStage();

	piSetTextureMatrix(&IdentityMatrix);
#endif

	return(TRUE);
}































































































































































































































































































































#if(0)
static void _drawWater(OBJECT* o)
{
}
#endif

void _resetObjectUV(OBJECT* o)
{
    piTexCoordPtr tex = o->MutableTexCoords();
    piVectorConstPtr pos = o->RenderPositions();
    piINDICES* ind = o->ptris;

    for(SINT32 i = 0; i < o->numtris; i++)
    {
        // Octet for replacement tile between -400, 400 for x and y
        tex[ind[i].v1].u = (pos[ind[i].v1].x + 400.0f) / 800.0f;
        tex[ind[i].v1].v = (pos[ind[i].v1].y + 400.0f) / 800.0f;

        tex[ind[i].v2].u = (pos[ind[i].v2].x + 400.0f) / 800.0f;
        tex[ind[i].v2].v = (pos[ind[i].v2].y + 400.0f) / 800.0f;

        tex[ind[i].v3].u = (pos[ind[i].v3].x + 400.0f) / 800.0f;
        tex[ind[i].v3].v = (pos[ind[i].v3].y + 400.0f) / 800.0f;
    }
}

static void _drawScene(OBJECT* o)
{
    // Non replacement tiles
    if(o->flags & OBJECT_TERRAIN)
    {
        if(o->numtverts == 0)
        {
            return;
        }
    }
    // Replacement tiles
    else if(o->flag2 & OBJECT_TERRAIN_OBJECT)
    {
        if(o->numverts == 0)
        {
            return;
        }
    }

    SINT32 texType = obdrawGetMultiTextureOpcode(o->texid, nullptr);

    if(texType == MT_SINGLEPASS)
    {
        obdrawApplyMultiTextureStage(o);
    }

    if(o->flags & OBJECT_TERRAIN)
    {
        piRenderScene(piPT_TRIANGLELIST, piVT_CVERTEX, o->ptverts, o->numtverts);
    }
    else if(o->flag2 & OBJECT_TERRAIN_OBJECT)
    {
        if(o->flags & OBJECT_INDEXED)
        {
            piSTRIDEDDATA sd;
            o->StridedRenderData(&sd);

            piRenderIndexStridedScene(piPT_TRIANGLELIST,
                                      piVT_VERTEX,
                                      &sd,
                                      o->numverts,
                                      (UINT16*)o->ptris,
                                      o->numtris * 3);
        }
        else
        {
            piSTRIDEDDATA sd;
            o->StridedRenderData(&sd);

            piRenderStridedScene(piPT_TRIANGLELIST,
                                 piVT_VERTEX,
                                 &sd,
                                 o->numverts);
        }
    }

    if(texType == MT_SINGLEPASS)
    {
        obdrawReleaseMultiTextureStage(o);
    }
    else if(texType == MT_BUMPMAP)
    {
        obdrawApplyBumpmapTextureStage(o);

        piRenderScene(piPT_TRIANGLELIST, piVT_CVERTEX, o->ptverts, o->numtverts);

        obdrawReleaseBumpmapTextureStage(o);
    }
    else if(texType == MT_MULTIPASS)
    {
        UINT32 flags = o->flags;

        o->flags &= ~OBJECT_INDEXED;

        // Apply any required multitexturing
        obdrawApplyMultiTexture(o);

        o->flags = flags;
    }

    if(trnwtrIsWaterTexture(o->texid))
    {
        if(o->flags & OBJECT_TERRAIN)
        {
            trnwtrPreDraw2(o);
        }
        else
        {
            trnwtrPreDraw(o);
        }

        if(o->flags & OBJECT_TERRAIN)
        {
            piSetRenderState(piRENDERSTATE_ZFUNC, piCMP_LESSEQUAL);
            piRenderScene(piPT_TRIANGLELIST, piVT_CVERTEX, o->ptverts, o->numtverts);
            piSetRenderState(piRENDERSTATE_ZFUNC, ZFunction);
        }
        else if(o->flag2 & OBJECT_TERRAIN_OBJECT)
        {
            if(o->flags & OBJECT_INDEXED)
            {
                piSTRIDEDDATA sd;
                o->StridedRenderData(&sd);

                piRenderIndexStridedScene(piPT_TRIANGLELIST,
                                          piVT_VERTEX,
                                          &sd,
                                          o->numverts,
                                          (UINT16*)o->ptris,
                                          o->numtris * 3);
            }
            else
            {
                piSTRIDEDDATA sd;
                o->StridedRenderData(&sd);

                piRenderStridedScene(piPT_TRIANGLELIST,
                                    piVT_VERTEX,
                                    &sd,
                                    o->numverts);
            }
        }

        trnwtrPostDraw();
	}

#ifdef VEHICLE_OBJECT_ANALYSIS

    for(SINT32i = 1; i < 4; i++)
	{
		for(SINT32 j = 1; j < 4; j++)
        {
            OutlineSupercell(&trndtl[i * 5 + j], piRGBA_MAKE(244,184,54,255));
        }
    }

#endif
}

void trnwtrDrawReplacementWater(OBJECT* o)
{
    piSetWorldMatrix(&o->mat);

    piSetActiveMaterial(o->matid);
    piSetActiveTexture(o->texid, 0);

    _drawScene(o);
}

void trnwtrDrawNonReplacementWater(OBJECT* o)
{
    piSetWorldMatrix(&o->mat);

    piSetActiveMaterial(o->matid);
    piSetActiveTexture(o->texid, 0);

    _drawScene(o);
}

void trnwtrSetShaderFloat(WATER_PARAMS param, FLOAT32 value)
{
}

void trnwtrSetShaderFloatArray(WATER_PARAMS param, SINT32 index, FLOAT32 value)
{
}

void trnwtrSetShaderColor(WATER_PARAMS param, SINT32 index, FLOAT32 value)
{
}

FLOAT32 trnwtrGetShaderFloat(WATER_PARAMS type)
{
    return 0;
}

FLOAT32* trnwtrGetShaderFloatArray(WATER_PARAMS type)
{
    return nullptr;
}

UINT32 trnwtrGetShaderColor(WATER_PARAMS type)
{
    return 0;
}

SINT32 trnwtrGetTexture(SINT32& id, SINT32& depth)
{
    return -1;
}

LOGICAL trnwtrIsWaterDrawn(FLOAT32& maxheight, FLOAT32& distance)
{
    return FALSE;
}

void trnwtrToggle()
{
    gWaterShadersEnabled = !gWaterShadersEnabled;
    uiChatPrintLine("Water Shaders %s ", gWaterShadersEnabled ? "ENABLED" : "DISABLED");
}
