#include <syscfg.h> // Path is project specific.

#include "pi/pi.h"
#include "include/vars.h"
#include "fio/fio.h"
#include "ld/ldmdl.h"
#include "ld/ldflt.h"
#include "ob/obc.h"
#include "ob/obdraw.h"
#include "ob/obdlite.h"
#include "ob/obdraw.h"
#include "ob/obslite.h"
#include "ob/obebox.h"
#include "ob/obplyr.h"
#include "ob/obwld.h"
#include "ob/obcd.h"
#include "parse/parse.h"
#include "parse/parseIO.h"
#include "trn/trnmain.h"
#include "trn/trnop.h"
#include "ob/obpool.h"
#include "ld/ldpix.h"
#include "rn/rnmain.h"
#include <math.h>
#include <ctype.h>
#include "math/math3d.h"

#include "rn/rnlogo.h"

#include "gr2/gr2Anim.h"

#include "parse/parse.h"
#include "sn/sndsSoundCache.h"
#include "ob/obMgr.h"

static CONSTANT_VALUE constants[]={
								  {"OBJECT_VISIBLE",		0x00000001},
								  {"OBJECT_LITE",			0x00000002},
								  {"OBJECT_DYNALITE",		0x00000004},
								  {"OBJECT_COLLISION",		0x00000008},
								  {"OBJECT_CLIPPED",		0x00000010},
								  {"OBJECT_INDEXED",		0x00000020},
								  {"OBJECT_LINESTRIP",		0x00000040},
								  {"OBJECT_LINECLOSED",		0x00000080},
								  {"OBJECT_TRIANGLEFAN",	0x00000100},
								  {"OBJECT_TRIANGLELIST",	0x00000200},
								  {"OBJECT_NOBACKFACE",		0x00000400},
								  {"OBJECT_USELOD",			0x00000800},
								  {"OBJECT_ATTACHED",		0x00001000},
								  {"OBJECT_SKIPDRAWING",	0x00002000},
								  {"OBJECT_USEVERTEXBSP",	0x00004000},
								  {"OBJECT_RADIUSCOLLISION",0x00008000},
								  {"OBJECT_USEMATRIX",		0x00010000},
								  {"OBJECT_REFLECTIVE",		0x00020000},
								  {"OBJECT_NOFILTER",		0x00040000},
								  {"OBJECT_REVERSENORMALS",	0x00080000},
								  {"OBJECT_TESSELATION",	0x00100000},
                                  {"OBJECT_DYNASHADOW",		0x00200000},
                                  {"OBJECT_DYNAENVMAP",		0x00400000},
                                  {"OBJECT_DYNAFOG",		0x00800000},
                                  {"OBJECT_COPY",	        0x01000000},
                                  {"OBJECT_ZWRITEOFF",      0x02000000},
                                  {"OBJECT_NOSPECULAR",		0x04000000},
                                  {"OBJECT_TERRAIN",		0x08000000},
                                  {"OBJECT_USERDRAW",		0x10000000},
                                  {"OBJECT_DOF",			0x20000000},
                                  {"OBJECT_LOD",			0x40000000},
                                  {"OBJECT_GROUP",			0x80000000},
                                  {"OBJECT_NOTOBJECT",		(OBJECT_DOF|OBJECT_LOD|OBJECT_GROUP)},
								  // flag2
                                  {"OBJECT_DOF_USE_DOFPOSMATRIX",	0x00000001},
                                  {"OBJECT_POOL",					0x00000002},
                                  {"OBJECT_TOPMOST",				0x00000004},
                                  {"OBJECT_IGNORECENTER",			0x00000008},
                                  {"LIGHT_POINT",           1},
                                  {"LIGHT_SPOT",            2},
                                  {"LIGHT_DIRECTIONAL",     3},
                                  {"LIGHT_PARALLELPOINT",   4},
                                  {"LIGHT_GLSPOT",          5},
                                  {"LITE_ON",               0x00000001},
                                  {"LITE_LENSFLARES",       0x00000002},
                                  {"LITE_LFMEDIUM",         0x00000004},
                                  {"LITE_LFBRIGHT",         0x00000008},
								  {"",						0}
									};

static bool Backdrop(int numargs, FUNCARGS* args);
static bool Sphere(int numargs, FUNCARGS* args);
static bool SphereVoxel(int numargs, FUNCARGS* args);
static bool ChangeFlags(int numargs, FUNCARGS* args);
static bool ChangeFlag2(int numargs, FUNCARGS* args);
static bool ChangeLayer(int numargs, FUNCARGS* args);
static bool ChangeName(int numargs, FUNCARGS* args);
static bool LoadModel(int numargs, FUNCARGS* args);
static bool LoadCockpitModel(int numargs, FUNCARGS* args);
static bool LoadUIModel(int numargs, FUNCARGS* args);
static bool Offset(int numargs, FUNCARGS* args);
static bool OffsetGroup(int numargs, FUNCARGS* args);
static bool PlaceOnSphere(int numargs, FUNCARGS* args);
static bool RotatePlayer(int numargs, FUNCARGS* args);
static bool MovePlayer(int numargs, FUNCARGS* args);
static bool ResetPlayer(int numargs, FUNCARGS* args);
static bool StaticLights(int numargs, FUNCARGS* args);
static bool DynamicLights(int numargs, FUNCARGS* args);
static bool AddDynamicLight(int numargs, FUNCARGS* args);
static bool AddShadow(int numargs, FUNCARGS* args);
static bool ChangeLightFlag(int numargs, FUNCARGS* args);
static bool ReverseNormals(int numargs, FUNCARGS* args);
static bool LoadTerrain(int numargs, FUNCARGS* args);
static bool ReduceObject(int numargs, FUNCARGS* args);
//static bool LoadAnimation(int numargs, FUNCARGS* args);
static bool SetTerrainOrigin(int numargs, FUNCARGS* args);
static bool PlaceObjectOnTerrain(int numargs, FUNCARGS* args);
//static bool AttachAnimationToObject(int numargs, FUNCARGS* args);
static bool LoadSound(int numargs, FUNCARGS* args);
static bool MultiTexture(int numargs, FUNCARGS* args);


static PARSE_CALLBACK  world[]={{"backdrop",				Backdrop},
                               {"sphere",					Sphere},
                               {"spherevoxel",				SphereVoxel},
                               {"changeflags",				ChangeFlags},
                               {"changeflag2",				ChangeFlag2},
                               {"changelayer",				ChangeLayer},
                               {"changename",				ChangeName},
                               {"loadmodel",				LoadModel},
                               {"loadcockpitmodel",			LoadCockpitModel},
                               {"loaduimodel",				LoadUIModel},
                               {"offset",					Offset},
                               {"offsetgroup",				OffsetGroup},
                               {"placeonsphere",			PlaceOnSphere},
                               {"rotateplayer",				RotatePlayer},
                               {"moveplayer",				MovePlayer},
                               {"resetplayer",				ResetPlayer},
                               {"staticlights",				StaticLights},
                               {"dynamiclights",			DynamicLights},
                               {"adddynamiclight",			AddDynamicLight},
#ifdef USE_SHADOWS
								{"addshadow",				AddShadow},
#endif
                               {"changelightflag",			ChangeLightFlag},
                               {"reversenormals",			ReverseNormals},
                               {"loadterrain",				LoadTerrain},
                               {"reduceobject",				ReduceObject},
//								{"loadanimation",			LoadAnimation},
                               {"setterrainorigin",			SetTerrainOrigin},
                               {"placeobjectonterrain",		PlaceObjectOnTerrain},
//								{"attachanimationtoobject",	AttachAnimationToObject},
                               {"loadsound",				LoadSound},
							   {"multitexture",				MultiTexture},
                               {"",							nullptr},
                              };

static SINT8	worldfile[64];
static FLOAT32	xorigin, yorigin;

static UINT32 FindConstant(const char* string)
{
int		i;

	for (i=0; constants[i].flagval; i++)
	{
		if (lstrcmp(string, constants[i].flagname) == 0)
		{
			return(constants[i].flagval);
		}
	}

	return(0);
}

static bool Backdrop(int numargs, FUNCARGS* args)
{
	if (bBackdrop)
	{
		if (!obeboxCreate(args[0].svalue, args[1].fvalue))
		{
			return(FALSE);
		}
	}

	return(TRUE);
}

static bool Sphere(int numargs, FUNCARGS* args)
{
piVECTOR	loc;
float		x, y, z, scale;
UINT32		mesh, color;

	mesh  = (UINT32)args[2].fvalue;
	x     = args[3].fvalue;
	y     = args[4].fvalue;
	z     = args[5].fvalue;
	scale = args[6].fvalue;
	color = args[7].dvalue;

	loc.x = x;
	loc.y = y;
	loc.z = z;
	if (obcSphere(args[0].svalue, args[1].svalue, (int)mesh, &loc, scale, color) == nullptr)
	{
		return(FALSE);
	}

	return(TRUE);
}

static bool SphereVoxel(int numargs, FUNCARGS* args)
{
piVECTOR	loc;
float		x, y, z, scale, depth;
UINT32		mesh, color;

	mesh  = (UINT32)args[2].fvalue;
	x     = args[3].fvalue;
	y     = args[4].fvalue;
	z     = args[5].fvalue;
	scale = args[6].fvalue;
	depth = args[7].fvalue;
	color = args[8].dvalue;

	loc.x = x;
	loc.y = y;
	loc.z = z;
	if (obcSphereVoxel(args[0].svalue, args[1].svalue, (int)mesh, &loc, scale, depth, color) == nullptr)
	{
		return(FALSE);
	}

	return(TRUE);
}

static bool ChangeFlags(int numargs, FUNCARGS* args)
{
UINT32		flagval;
OBJECT*	lpobj;

	lpobj = obmainFindNamedObject(args[0].svalue);
	if (lpobj == nullptr)
	{
		return(FALSE);
	}

	flagval = FindConstant(args[1].svalue);
	if (!flagval)
	{
		return(FALSE);
	}

	/*
	if (flagval & OBJECT_REVERSENORMALS)
	{
		ldmdlReverseNormals(lpobj);
	}
	else
	*/
	if (args[2].dvalue)
	{
		obmainSetFlagsInObjectTree(lpobj, flagval);
	}
	else
	{
		obmainClearFlagsInObjectTree(lpobj, flagval);
	}

	return(TRUE);
}

static bool ChangeFlag2(int numargs, FUNCARGS* args)
{
UINT32		flagval;
OBJECT*	lpobj;

	lpobj = obmainFindTopmostObject(args[0].svalue);
	if (lpobj == nullptr)
	{
		return(FALSE);
	}

	flagval = FindConstant(args[1].svalue);
	if (!flagval)
	{
		return(FALSE);
	}

	if (args[2].dvalue)
	{
		obmainSetFlag2InObjectTree(lpobj, flagval);
	}
	else
	{
		obmainClearFlag2InObjectTree(lpobj, flagval);
	}

	return(TRUE);
}

static bool ChangeName(int numargs, FUNCARGS* args)
{
OBJECT*	lpobj;

	// no object by new name can exist
	lpobj = obmainFindNamedObject(args[1].svalue);
	if (lpobj != nullptr)
	{
		return(FALSE);
	}

	// object with old name must exist
	lpobj = obmainFindNamedObject(args[0].svalue);
	if (lpobj == nullptr)
	{
		return(FALSE);
	}

	// rename
	strcpy(lpobj->name, args[1].svalue);

	return(TRUE);
}

static bool ChangeLayer(int numargs, FUNCARGS* args)
{
OBJECT*	lpobj;

	lpobj = obmainFindNamedObject(args[0].svalue);
	if (lpobj == nullptr)
	{
		return(FALSE);
	}

	do
	{
		lpobj->layer = (short)args[1].dvalue;
		lpobj = lpobj->attached;
	}
	while (lpobj);

	return(TRUE);
}

static bool LoadModel(int numargs, FUNCARGS* args)
{
	rnlogoUpdateStatus("loading.vehicles");

    obmgrAddModelToPool(args[0].svalue, false, args[2].dvalue);

	return(TRUE);
}

static bool LoadCockpitModel(int numargs, FUNCARGS* args)
{
	rnlogoUpdateStatus("loading.vehicles");
	if(g_bBuildMDLS)	obmgrAddModelToPool(args[0].svalue, true);
	else	obmgrAddModelToPool(args[0].svalue, false);

	return TRUE;
}

static bool LoadUIModel(int numargs, FUNCARGS* args)
{
	rnlogoUpdateStatus("loading.ui");
	obmgrAddModelToPool(args[0].svalue, TRUE);

	return(TRUE);
}

static bool Offset(int numargs, FUNCARGS* args)
{
OBJECT*	lpobj;
piVECTOR	loc;

	lpobj = obmainFindNamedObject(args[0].svalue);
	if (lpobj == nullptr)
	{
		return(FALSE);
	}

	loc.x = args[1].fvalue * ScaleFactor;
	loc.y = args[2].fvalue * ScaleFactor;
	loc.z = args[3].fvalue * ScaleFactor;
	obplyrOffsetObject(lpobj, &loc);

	return(TRUE);
}

static bool OffsetTree(OBJECT* lpgrp, piVECTOR* loc)
{
OBJECT*	lpobj;

	if (lpgrp == nullptr)
	{
		return(FALSE);
	}

	lpobj = lpgrp;
	while (lpobj != nullptr)
	{
		if (!(lpobj->flags & OBJECT_NOTOBJECT))
		{
			obplyrOffsetObject(lpobj, loc);
			//Write_Debug("Offset: %s", lpobj->name);
		}

		OffsetTree(lpobj->sibling, loc);

		lpobj = lpobj->child;
	}

	return(TRUE);
}

static bool OffsetGroup(int numargs, FUNCARGS* args)
{
OBJECT*	lpgrp;
piVECTOR	loc;

	lpgrp = obmainFindGroupObject(args[0].svalue);
	if (lpgrp == nullptr)
	{
		return(FALSE);
	}

	loc.x = args[1].fvalue * ScaleFactor;
	loc.y = args[2].fvalue * ScaleFactor;
	loc.z = args[3].fvalue * ScaleFactor;

	if (!OffsetTree(lpgrp, &loc))
	{
		return(FALSE);
	}

	return(TRUE);
}

static void LocateObjectOnSphere(OBJECT* lpobj, float radius, float lat, float lng, piVECTOR* pos)
{
	pos->x = radius * (float)cos(lat) * (float)cos(lng);
	pos->y = radius * (float)sin(lat) * (float)cos(lng);
	pos->z = radius * (float)sin(lng);
}

static bool PlaceOnSphere(int numargs, FUNCARGS* args)
{
float		radius, lat, lng, roll;
piVECTOR	pos, rot;
COLLISION	collision;

	OBJECT* lpsph = obmainFindNamedObject(args[1].svalue);
	if (lpsph == nullptr)
	{
		return(FALSE);
	}

	OBJECT* lpobj = obmainFindNamedObject(args[0].svalue);
	if (lpobj == nullptr)
	{
		return(FALSE);
	}

	radius = (float)sqrt(lpsph->maxrad) - lpobj->obox[0].z;
	lat = PI2 / 4.0f * 3.0f + args[2].fvalue;
	lng = PI2 / 4.0f * 0.0f + args[3].fvalue;
	roll = args[4].fvalue;

	LocateObjectOnSphere(lpobj, radius, lat, lng, &pos);

	rot.x = lng + PI / 2.0f;
	rot.y = lat + PI / 2.0f;
	rot.z = roll;

	collision.object = lpobj;
	collision.oldpos = pos;
	collision.newpos = lpsph->loc;

	if (!obcdCheckObjectCollision(lpsph, &collision))
	{
		radius = (float)sqrt(lpobj->maxrad) - (float)sqrt(lpobj->minrad);
		radius = (float)sqrt(lpobj->minrad) + radius / 2.0f;
	}
	else
	{
		radius = (float)sqrt(collision.colpos.x * collision.colpos.x +
												 collision.colpos.y * collision.colpos.y +
												 collision.colpos.z * collision.colpos.z) -
												 lpobj->obox[0].z;
		pos.x = collision.colpos.x - lpobj->obox[0].z * collision.colnormal.x;
		pos.y = collision.colpos.y - lpobj->obox[0].z * collision.colnormal.y;
		pos.z = collision.colpos.z - lpobj->obox[0].z * collision.colnormal.z;
	}

	obplyrUpdateObject(lpobj, &pos, &rot);

	if (!obmainAddChildObject(lpsph, lpobj))
	{
		return(FALSE);
	}

	do
	{
		lpobj->loc.x = lpsph->loc.x;
		lpobj->loc.y = lpsph->loc.y;
		lpobj->loc.z = lpsph->loc.z;
		lpobj->mat = IdentityMatrix;
		lpobj = lpobj->attached;
	}
	while (lpobj != nullptr);

	return(TRUE);
}

static bool RotatePlayer(int numargs, FUNCARGS* args)
{
piVECTOR	rot;

	rot.x = args[0].fvalue;
	rot.y = args[1].fvalue;
	rot.z = args[2].fvalue;
	rnmainRotatePlayer(&rot);

	return(TRUE);
}

static bool MovePlayer(int numargs, FUNCARGS* args)
{
piVECTOR	pos;

	pos.x = xorigin = args[0].fvalue;
	pos.y = yorigin = args[1].fvalue;
	pos.z = args[2].fvalue;

//	rnmainMovePlayer(&pos);

	return(TRUE);
}

static bool ResetPlayer(int numargs, FUNCARGS* args)
{
	rnmainResetPlayer();

	return(TRUE);
}

static bool StaticLights(int numargs, FUNCARGS* args)
{
	StaticLighting = args[0].dvalue != 0;

	return(TRUE);
}

static bool DynamicLights(int numargs, FUNCARGS* args)
{
	DynamicLighting = args[0].dvalue != 0;

	return(TRUE);
}

static bool AddDynamicLight(int numargs, FUNCARGS* args)
{
UINT32   type;
LIGHT   light;

	type = FindConstant(args[1].svalue);
	if (type == 0)
	{
		return(FALSE);
	}

	// init the light
	memset(&light, 0, sizeof(LIGHT));
	strcpy(light.name, args[0].svalue);
	light.flags = LITE_ON;
	light.light.dltType = type;
	light.light.dcvColor.r = args[2].fvalue;
	light.light.dcvColor.g = args[3].fvalue;
	light.light.dcvColor.b = args[4].fvalue;
	light.light.dcvColor.a = 1.0f;
	light.light.dvPosition.x = args[5].fvalue * ScaleFactor;
	light.light.dvPosition.y = args[6].fvalue * ScaleFactor;
	light.light.dvPosition.z = args[7].fvalue * ScaleFactor;
	light.light.dvRange = args[8].fvalue * ScaleFactor;
	light.light.dvFalloff = 1.0f;
	light.light.dvAttenuation0 = 0.0f;
	light.light.dvAttenuation1 = 0.0f;
	light.light.dvAttenuation2 = 1.0f;
	light.light.dvTheta = PI / 8;
	light.light.dvPhi = PI / 16;

	light.light.dvDirection = light.light.dvPosition;
	math3dNormalize(&light.light.dvDirection);
	if (!obdliteAddDynaLite(&light, nullptr))
	{
		return(FALSE);
	}

	return(TRUE);
}

#ifdef USE_SHADOWS
static bool AddShadow(int numargs, FUNCARGS* args)
{
	OBJECT* lpgrp = obmainFindTopmostObject(args[0].svalue);
	if (lpgrp == nullptr)
	{
		return(FALSE);
	}

	OBJECT* lpobj = obmainFindChildFromObject(lpgrp, "shadow");
	if (lpobj == nullptr)
	{
		Write_Debug("Shadow object not found in model");
		return(FALSE);
	}

	if (!obdshadCreate(lpgrp, lpobj))
	{
		return(FALSE);
	}

	return(TRUE);
}
#endif // USE_SHADOWS

static bool ChangeLightFlag(int numargs, FUNCARGS* args)
{
UINT32			flagval;
LIGHT*	  lplite;

	lplite = obdliteFindDynaLight(args[0].svalue);
	if (lplite == nullptr)
	{
		return(FALSE);
	}

	flagval = FindConstant(args[1].svalue);
	if (!flagval)
	{
		return(FALSE);
	}

	if (args[2].dvalue)
	{
		lplite->flags |= flagval;
	}
	else
	{
		lplite->flags &= ~flagval;
	}

	return(TRUE);
}


static bool ReverseNormals(int numargs, FUNCARGS* args)
{
OBJECT*	lpobj;

	lpobj = obmainFindNamedObject(args[0].svalue);
	if (lpobj == nullptr)
	{
		return(FALSE);
	}

	if (!ldmdlReverseObjectNormals(lpobj))
	{
		return(FALSE);
	}

	return(TRUE);
}

static bool LoadTerrain(int numargs, FUNCARGS* args)
{
	strcpy(worldfile, args[0].svalue);

	return(TRUE);
}

static bool ReduceObject(int numargs, FUNCARGS* args)
{
	OBJECT*	lpobj;

	lpobj = obmainFindNamedObject(args[0].svalue);
	ldmdlReduceAttachments(lpobj);

	return(TRUE);
}

/*static bool LoadAnimation(int numargs, FUNCARGS* args)
{
	if (!anmainLoadAnimation(args[0].svalue, args[1].fvalue, args[2].dvalue))
	{
		return(FALSE);
	}

	return(TRUE);
}*/

static bool SetTerrainOrigin(int numargs, FUNCARGS* args)
{
	xorigin = args[0].fvalue * ScaleFactor;
	yorigin = args[1].fvalue * ScaleFactor;

	return(TRUE);
}

static bool PlaceObjectOnTerrain(int numargs, FUNCARGS* args)
{
piVECTOR	pos;
OBJECT*	lpobj;

	if(!obmgrCreateModelInstance(args[0].svalue))
	{
		return(FALSE);
	}

	lpobj = obmainFindTopmostObject(args[0].svalue);
	if (lpobj == nullptr)
	{
		return(FALSE);
	}

	pos.x = args[1].fvalue * ScaleFactor;
	pos.y = args[2].fvalue * ScaleFactor;
	pos.z = 0.0f;
	lpobj->att.x = 0.0f;
	lpobj->att.y = 0.0f;
	lpobj->att.z = args[3].fvalue * ScaleFactor;

    lpobj->flag2 |= OBJECT_IS_ROTATED;

	trnopOrientSubobjects(lpobj, &pos, false);

	return(TRUE);
}

static bool LoadSound(int numargs, FUNCARGS* args)
{
	rnlogoUpdateStatus("loading.sounds");

    // Not an error
    if(bSoundEnable == FALSE)
        return TRUE;

    if(numargs == 2)
    {
        if(!sndsPreloadSoundCache(args[0].svalue, args[1].fvalue))
            return FALSE;
    }
    else
    {
        if(!sndsPreloadSoundCache(args[0].svalue))
            return FALSE;
    }

	return(TRUE);
}

static bool MultiTexture(int numargs, FUNCARGS* args)
{
#if defined(BETA_RELEASE)
	//if(NoBlitzWorPreload) return TRUE;
#endif

	if (numargs >= 5)
	{
		rnlogoUpdateStatus("loading.multitextures");

		if (obdrawSetMultiTexture(args[0].svalue, args[1].svalue, (SINT32)args[2].dvalue, args[3].fvalue, args[4].fvalue, args[5].fvalue))
		{
			return TRUE;
		}
	}

	return FALSE;
}

#if(0)
static int TrimSpace(char* string)
{
int   i;

	for (i=0; string[i]; i++)
	{
		if (!isspace(string[i]))
		{
			break;
		}
	}

	if (i != 0)
	{
		strcpy(string, &string[i]);
	}

	for (i=lstrlen(string)-1; i >= 0 && isspace(string[i]); i--)
	{
		string[i] = 0;
	}

	return(lstrlen(string));
}
#endif

#if(0)
static int GetArg(char* string, char* arg)
{
int   i;

	for (i=0; string[i]; i++)
	{
		if (isspace(string[i]))
		{
			break;
		}
	}

	strncpy(arg, string, i+1);
	strcpy(string, &string[i]);
	TrimSpace(string);

	return(i);
}
#endif

LOGICAL obwldInit(char* levelfile)
{
UINT32	T1, T2;

#ifdef MAP_MATS
    materialsMapInit();
#endif

    if(!obcInit())
    {
        Write_Log(LOG_OLD, LOG_INFO, "[obwldInit] obcInit** FAILED**");
        return FALSE;
    }

#ifdef USE_SHADOWS
	if (!obdshadInit())
	{
		return(FALSE);
	}
#endif

	worldfile[0] = 0;
	T1 = timeGetTime();
logTimerBeginExtMacro(parseWor);
	if ( ! parseWor(levelfile,world) )
		return (FALSE);
logTimerEndExtMacro(parseWor);

	T2 = timeGetTime();
	Write_Debug ("*****************Total time loading blitzwor.csv file: %u milliseconds", T2-T1);

#ifdef ANDREWTEST
    {
        /*
        extern PoolMap _poolmap;

        PoolMap::iterator it = _poolmap.begin();
        while (it != _poolmap.end() )
        {
            printf("%s:", it->first.c_str() );
            PoolModel* model = it->second;
            PoolNode* node = it->second->node;
            size_t size = node->calcMemoryUsage();
            printf(" Size=%ld\n", size );
            it++;
        }
        */
    }
#endif

	// Granny2 system (needs materials loaded)


#if defined(BETA_RELEASE)
	if(!NoInfPreload){
#endif
	rnlogoUpdateStatus("loading.infantry");

	if (!gr2AnimInit())
	{
		return(FALSE);
	}

#if defined(BETA_RELEASE)
}
#endif

    rnlogoUpdateStatus("loading.objects");
	if (!trnmainInit(worldfile, xorigin, yorigin))
	{
		return(FALSE);
	}

	return(TRUE);

}

void obwldKill(void)
{
	trnmainKill();

	gr2AnimKill();

    Write_Log(LOG_OLD, LOG_INFO, "[obwldKill]");
}

LOGICAL obwldUpdate(void)
{
	return(TRUE);
}

#ifdef USE_SHADOWS
LOGICAL obwldPredrawUpdate(void)
{
	if (!obdshadUpdate())
	{
		return(FALSE);
	}

	return(TRUE);
}
#endif
