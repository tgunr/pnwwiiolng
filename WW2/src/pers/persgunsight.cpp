#include <syscfg.h> // Path is project specific.
#include "pi/pi.h"
#include "include/vars.h"

#include <math.h>

#include "weap/weapammo.h"
#include "ml/mlAtmo.h"
#include "ml/mlutility.h"
#include "mem/mem.h"
#include "fio/fio.h"
#include "gmlp/gmlpmain.h"
#include "ld/ldpix.h"
#include "draw/draw3d.h"
#include "math/mathvect.h"
#include "math/math3d.h"
#include "math/mathUnitConversion.h"
#include "math/matheul.h"
#include "rn/rntext.h"
#include "veh/vehmain.h"
#include "pers/persmain.h"
#include "fp/fpFPS.h"
#include "fp/fpFPSdata.h"
#include "ld/lddds.h"
#include "ogl/ogl.h"
#include "ui/uiMain.h"
#include "ogl/oglMaterial.h"

#include "pers/persgunsight.h"
#include "pers/persbombsight.h"

#if USE_STATESET
#include "ogl/States.h"
#endif

/************************************************************************************************
	persGunsight
*************************************************************************************************/
// defines
#define RANGETABLE_MAXRANGE 8000 // meters
#define RANGETABLE_INCREMENT 10  // meters
#define RANGETABLE_NUMELEMENTS ((RANGETABLE_MAXRANGE/RANGETABLE_INCREMENT)+1)

#define TIME_INCREMENT .008

#define GUNSIGHT_RANGE_MOVEMENT_RATE	0.2f
#define GUNSIGHT_SPEED_MOVEMENT_RATE 	0.2f
#define GUNSIGHT_ANGLE_MOVEMENT_RATE 	0.2f
#define GUNSIGHT_DEPTH_MOVEMENT_RATE 	0.4f
#define GUNSIGHT_CONV_MOVEMENT_RATE 	0.2f

#define GUNSIGHT_MAX_TORP_DEPTH 	10.0f
#define GUNGISHT_MAX_TARGET_SPEED 	KTS_TO_MPS(40.0f)
#define GUNGISHT_MAX_TARGET_RANGE 	8000.0f
#define GUNSIGHT_MIN_CONV_RANGE		200.0f
#define GUNSIGHT_MAX_CONV_RANGE		1000.0f

#define MAX_GUNFILE_ELEMENTS 512
#define MAX_GUNFILE_COMMANDS 128

// use 1 to distinguish from iconfnt
#define GUNSIGHT_FONT 1

// the gunsight uses a mils system to render
// the gunsight is placed at 1000 meters away effectively
// so the coordinate system is 1 meter per mil
enum
{
	GS_LADDER_VERTICAL = 1,
	GS_LADDER_HORIZONTAL,
	GL_LADDER_TYPE_MAX
};

enum
{
	GS_LADDER_INPUT_BOMBSIGHT_ALT = 1,
	GS_LADDER_INPUT_BOMBSIGHT_SPEED,
	GS_LADDER_INPUT_MAX
};

static ref_ptr<oglMaterial> basictexture;
static ref_ptr<oglMaterial> basiccolor;

struct tagGSCommandList;
typedef LOGICAL (GunFileReadFunc)(struct tagGSCommandList * GSCL,SINT8* Params);
typedef LOGICAL (GunFileDisplayFunc)(void*  Data);
typedef LOGICAL (GunFileKillFunc)(void*  Data);

// structs
typedef struct tagpersGunsightRangeTable
{
	FLOAT32 TimeToDist[RANGETABLE_NUMELEMENTS];
	FLOAT32 ProjectileDrop[RANGETABLE_NUMELEMENTS];
	FLOAT32 AimAngle[RANGETABLE_NUMELEMENTS];
} persGunsightRangeTable;

typedef struct tagGunfileLookup
{
	SINT8 CommandName[32];
	GunFileReadFunc* ReadFunction;
	GunFileDisplayFunc* DisplayFunction;
	GunFileKillFunc* KillFunction;
} GunfileLookup;

typedef struct tagGunfileElement
{
	SINT32 CommandCode;
	void*  Data;

} GunfileElement;

typedef struct tagGSCommandList
{
	GunfileElement GE[MAX_GUNFILE_ELEMENTS];
	SINT32 GECounter;

	persGunsightRangeTable* AlignRT;

	piMATRIX AlignmentMat;
	piMATRIX TiltMat;
	piMATRIX ViewUseMat;

	LOGICAL GunSightTiltsToAim;

	OBJECT * ViewDOF;
} GSCommandList;

typedef struct
{
	UINT32 type;
	SINT32 dir;
	UINT32 input;
	FLOAT32 inputMin;
	FLOAT32 inputMax;
	FLOAT32 len;
} GSLadderInfo;

// variables
static UCHAR _LadderTypeNames[GL_LADDER_TYPE_MAX][32] = { "none", "vertical", "horizontal" };
static UCHAR _LadderInputNames[GS_LADDER_INPUT_MAX][32] = { "none", "bombsightalt", "bombsightspeed" };
static GSCommandList GunSightView, InstView;

static FLOAT32 	_BaseFOV = 90.0f;
static SINT32 	_GunsightFOVTexId;

static LOGICAL	_DrawStats;
static FLOAT32 	_StatsRange;

// variables used to help draw gunsight
static FLOAT32 	_PLPos; 					// pitch ladder baseline position, in mils, left/right
static persGunsightRangeTable* _CurrentRT; 	// pointer to current range table
static FLOAT32 	_CurrentRange; 				// current range
static LOGICAL 	_UseAngleRange; 			// uses the curved angle range system
static FLOAT32 	_PitchScale = 1.0f; 		// sets the pitch ladder scale
static FLOAT32 	_TextScaleX = 1.0f;
static FLOAT32 	_TextScaleY = 1.0f;
static FLOAT32 	_AngleOffset;
static FLOAT32 	_ladderOffsetX, _ladderOffsetY;

static FLOAT32 	_ConvergenceRange = 200.0;
static FLOAT32 	_AdjustGunsight = 0.0f; 	// gunsight range adjuster
static FLOAT32 	_AdjustMin;
static FLOAT32 	_AdjustMax;
static SINT32	_AdjustGunsightDir;
static SINT32	_AdjustSpeedDir;
static SINT32	_AdjustAngleDir;
static SINT32	_AdjustTorpDepthDir;
static FLOAT32	_AdjustSpeed;
static FLOAT32	_AdjustAngle;
static FLOAT32	_AdjustTorpDepth;
static FLOAT32	_AdjustConvergenceDir;
static FLOAT32	_AdjustConvergence;
static FLOAT32	_TargetBearing;
static FLOAT32	_LeadAngle;
static FLOAT32 	_TravelTime;
static LOGICAL 	_CalcSolution;

static char 	_AmmoTypeName[64];

static piLVERTEX VertexList[2048];
static SINT32 	 VertexCount;

static piLVERTEX LineList[2048];
static SINT32 	 LineVertexCount;

static SINT32 _bazookascope = -1;
static SINT32 _yankscope = -1;
static SINT32 _britscope = -1;
static SINT32 _frenchscope = -1;
static SINT32 _germanscope = -1;
static SINT32 _binocs = -1;

// local function prototype
static LOGICAL _ClearGunfileData(GSCommandList * GSCL);
static LOGICAL _ReadGunfileData(GSCommandList * GSCL,SINT8 * FileName);

static LOGICAL _ReadGenericFloat(GSCommandList * GSCL,SINT8 * Params);
static LOGICAL _ReadThinLine(GSCommandList * GSCL,SINT8 * Params);
static LOGICAL _ReadThickLine(GSCommandList * GSCL,SINT8 * Params);
static LOGICAL _ReadTriangleLine(GSCommandList * GSCL,SINT8 * Params);
static LOGICAL _ReadAmmoTypeLine(GSCommandList * GSCL,SINT8 * Params);
static LOGICAL _ReadPLLine(GSCommandList * GSCL,SINT8 * Params);
static LOGICAL _ReadRangeLine(GSCommandList * GSCL,SINT8 * Params);
static LOGICAL _ReadTextLine(GSCommandList * GSCL,SINT8 * Params);
static LOGICAL _SetTilt(GSCommandList * GSCL,SINT8 * Params);

static LOGICAL _DisplayMagLine(void*  Data);
static LOGICAL _DisplayFovLine(void*  Data);
static LOGICAL _DisplayThinLine(void*  Data);
static LOGICAL _DisplayThickLine(void*  Data);
static LOGICAL _DisplayTriangle(void*  Data);
static LOGICAL _DisplayTriangleFill(void*  Data);
static LOGICAL _NewPitchLadder(void*  Data);
static LOGICAL _SetAmmoType(void*  Data);
static LOGICAL _SetPLPos(void*  Data);
static LOGICAL _SetPLRange(void*  Data);
static LOGICAL _DrawPLLine(void*  Data);
static LOGICAL _DrawRangeLine(void*  Data);
static LOGICAL _DisplayTextLine(void*  Data);
static LOGICAL _SetAngleRange(void*  Data);
static LOGICAL _ResetAngleRange(void*  Data);
static LOGICAL _SetPitchScale(void*  Data);
static LOGICAL _SetTextScaleX(void*  Data);
static LOGICAL _SetTextScaleY(void*  Data);
static LOGICAL _SetAngleOffset(void*  Data);
static LOGICAL _SetRangeAngleMin(void*  Data);
static LOGICAL _SetRangeAngleMax(void*  Data);
static LOGICAL _SetRangeReadout(void*  Data);

static LOGICAL _RenderTextVertices(void);
static LOGICAL _RenderLineVertices(void);

static LOGICAL _KillGeneric(void*  Data);

static FLOAT32 _ComputeShotAngle(void);
#if(0)
static FLOAT32 _ComputeShotDrop(FLOAT32 Range,persGunsightRangeTable * RT);
#endif
static LOGICAL _DrawGunsightText(FLOAT32 X,  FLOAT32 Y,  FLOAT32 Size, FLOAT32 Rotation,  const WCHAR* TextString);
static LOGICAL _TranslateCoordinatesByAngle(FLOAT32 * X,FLOAT32 * Y);
static LOGICAL _AlignGS(GSCommandList * GSCL,OBJECT * AlignObj,OBJECT * ViewDOF,FLOAT32 AlignmentRange);
static LOGICAL _UpdateView(GSCommandList * GSCL);

static LOGICAL _draw3dLine(piVECTOR* point1, piVECTOR* point2, UINT32 Color);

static LOGICAL _ReadLadder(GSCommandList* GSCL,SINT8* Params);
static LOGICAL _SetLadder(void* Data);
static LOGICAL _KillLadder(void* Data);
static LOGICAL _DisplayBearingLadder(void* Data);
static LOGICAL _DisplayTorpedoSolution(void* Data);
static LOGICAL _ComputeFireControlSolution(FLOAT32* LeadAngle, FLOAT32* TravelTime, FLOAT32 TargetRelBearing, FLOAT32 TargetAngleOffBow,
										 	FLOAT32 TargetRange, FLOAT32 TargetSpeed, FLOAT32 ProjSpeed);

static FLOAT32 _GetCurrentViewBearing();

static LOGICAL persComputeRangeTable(persGunsightRangeTable* RT,  weapAmmoType * AmmoType,  FLOAT64 MaxRange);
void _drawscope(SINT32 texid, float scale, bool hasAlpha = false);

// commmand look up
static GunfileLookup Lookup[MAX_GUNFILE_COMMANDS] =
{
	{"none",			nullptr,				nullptr,					nullptr},
	{"mag",				_ReadGenericFloat,	_DisplayMagLine,		nullptr},
	{"fov",				_ReadGenericFloat,	_DisplayFovLine,		nullptr},
	{"thinline",		_ReadThinLine,		_DisplayThinLine,		_KillGeneric},
	{"thickline",		_ReadThickLine,		_DisplayThickLine,		_KillGeneric},
	{"triangle",		_ReadTriangleLine,	_DisplayTriangle,		_KillGeneric},
	{"trianglefill",	_ReadTriangleLine,	_DisplayTriangleFill,	_KillGeneric},
	{"text",			_ReadTextLine,		_DisplayTextLine,		_KillGeneric},
	{"textscalex",		_ReadGenericFloat,	_SetTextScaleX,			nullptr},
	{"textscaley",		_ReadGenericFloat,	_SetTextScaleY,			nullptr},
	{"anglerangeon",	nullptr,				_SetAngleRange,			nullptr},
	{"anglerangeoff",	nullptr,				_ResetAngleRange,		nullptr},
	{"pitchladder",		nullptr,				_NewPitchLadder,		nullptr},
	{"ammotype",		_ReadAmmoTypeLine,	_SetAmmoType,			_KillGeneric},
	{"pos",				_ReadGenericFloat,	_SetPLPos,				nullptr},
	{"range",			_ReadGenericFloat,	_SetPLRange,			nullptr},
	{"line",			_ReadPLLine,		_DrawPLLine,			_KillGeneric},
	{"rangeline",		_ReadRangeLine,		_DrawRangeLine,			_KillGeneric},
	{"pitchscale",		_ReadGenericFloat,	_SetPitchScale,			nullptr},
	{"endpitchladder",	nullptr,				_NewPitchLadder,		nullptr},
	{"angleoffset",		_ReadGenericFloat,	_SetAngleOffset,		nullptr},
	{"setrangeanglemin",_ReadGenericFloat,	_SetRangeAngleMin,		nullptr},
	{"setrangeanglemax",_ReadGenericFloat,	_SetRangeAngleMax,		nullptr},
	{"usetilt",			_SetTilt,			nullptr,					nullptr},
	{"setrangereadout",	nullptr,				_SetRangeReadout,		nullptr},
	{"ladder",			_ReadLadder,		_SetLadder,				_KillLadder},
	{"bearingladder",	_ReadGenericFloat,	_DisplayBearingLadder,	nullptr},
	{"torpedosolution",	_ReadGenericFloat,	_DisplayTorpedoSolution,nullptr},
	{"max",				nullptr,				nullptr,					nullptr},
};
/************************************************************************************************
	persGunsightInit
*************************************************************************************************/

LOGICAL persGunsightInit(void)
{
	memset(&GunSightView, 0x0, sizeof(GSCommandList));
	memset(&InstView, 0x0, sizeof(GSCommandList));

	_ClearGunfileData(&GunSightView);
	_ClearGunfileData(&InstView);

	_GunsightFOVTexId = ldpixLoadImageFileToSurface("vehicles\\vo\\sightfov1.bmp", colorBLACK, LDPIX_NONE);
	if (!piIsTexture(_GunsightFOVTexId, 0))
	{
		Write_Debug("Unable to load up the basic gunsight texture map! (vehicles\\vo\\sightfov1.bmp)");
	}
	piAddTexRef(_GunsightFOVTexId);

	_bazookascope = ldddsLoadDDSTexture2D("vehicles/vo/m9optic.dds", colorBLACK, 0);
	_yankscope = ldddsLoadDDSTexture2D("vehicles/vo/amerscope.dds", colorBLACK, 0);
    _britscope = ldddsLoadDDSTexture2D("vehicles/vo/britscope.dds", colorBLACK, 0);
    _frenchscope = ldddsLoadDDSTexture2D("vehicles/vo/frenscope.dds", colorBLACK, 0);
    _germanscope = ldddsLoadDDSTexture2D("vehicles/vo/germscope.dds", colorBLACK, 0);
    _binocs = ldddsLoadDDSTexture2D("vehicles/vo/binos.dds", colorBLACK, 0);

    if(!piIsTexture(_bazookascope, 0))
    {
        Write_Log(LOG_OLD, LOG_INFO, "%s Failed To Load", "vehicles/vo/m9optic.dds");
        return FALSE;
    }

    if(!piIsTexture(_yankscope, 0))
    {
        Write_Log(LOG_OLD, LOG_INFO, "%s Failed To Load", "vehicles/vo/springfieldoptic.dds");
        return FALSE;
    }

    if(!piIsTexture(_britscope, 0))
    {
        Write_Log(LOG_OLD, LOG_INFO, "%s Failed To Load", "vehicles/vo/britscope.dds");
        return FALSE;
    }

    if(!piIsTexture(_frenchscope, 0))
    {
        Write_Log(LOG_OLD, LOG_INFO, "%s Failed To Load", "vehicles/vo/frenscope.dds");
        return FALSE;
    }

    if(!piIsTexture(_germanscope, 0))
    {
        Write_Log(LOG_OLD, LOG_INFO, "%s Failed To Load", "vehicles/vo/germscope.dds");
        return FALSE;
    }

    if(!piIsTexture(_binocs, 0))
    {
        Write_Log(LOG_OLD, LOG_INFO, "%s Failed To Load", "vehicles/vo/binos.dds");
        return FALSE;
    }

    if(!ogl::UseDeprecatedPath())
    {
        basictexture = new oglMaterial;
        basictexture->SetMaterial(-1);
        basictexture->AddTexture(0, rntextGetTexId(GUNSIGHT_FONT));
        basictexture->SetShader("basictexture");

        basiccolor = new oglMaterial;
        basiccolor->SetMaterial(-1);
        basiccolor->SetShader("basiccolor");
    }

	return TRUE;
}

/************************************************************************************************
	persGunsightKill
*************************************************************************************************/

LOGICAL persGunsightKill(void)
{
	_ClearGunfileData(&GunSightView);
	_ClearGunfileData(&InstView);
	piFreeMipTextures(_GunsightFOVTexId);

    piFreeMipTextures(_germanscope);
    piFreeMipTextures(_frenchscope);
	piFreeMipTextures(_yankscope);
    piFreeMipTextures(_britscope);
	piFreeMipTextures(_bazookascope);
    piFreeMipTextures(_binocs);

	return 0;
}

/************************************************************************************************
	persGunsightUpdate
*************************************************************************************************/

LOGICAL persGunsightUpdate(void)
{
	static SINT32 _LastUpdateTime;
	FLOAT32 DeltaTime;
	piMATRIX Mat1;
	piVECTOR Att;

	DeltaTime = (frametime - _LastUpdateTime) / 1000.0f;

// adjust all the little imap gunsight thingies
	_AdjustGunsight += (_AdjustGunsightDir * DeltaTime * GUNSIGHT_RANGE_MOVEMENT_RATE);
	if (_AdjustGunsight < 0.0f) _AdjustGunsight = 0.0f;
	if (_AdjustGunsight > 1.0f) _AdjustGunsight = 1.0f;

	_AdjustSpeed += (_AdjustSpeedDir * DeltaTime * GUNSIGHT_SPEED_MOVEMENT_RATE);
	if (_AdjustSpeed < 0.0f) _AdjustSpeed = 0.0f;
	if (_AdjustSpeed > 1.0f) _AdjustSpeed = 1.0f;

	_AdjustAngle += (_AdjustAngleDir * DeltaTime * GUNSIGHT_ANGLE_MOVEMENT_RATE);
	if (_AdjustAngle < -1.0f) _AdjustAngle = -1.0f;
	if (_AdjustAngle > 1.0f) _AdjustAngle = 1.0f;

	_AdjustTorpDepth += (_AdjustTorpDepthDir * DeltaTime * GUNSIGHT_DEPTH_MOVEMENT_RATE);
	if (_AdjustTorpDepth < 0.0f) _AdjustTorpDepth = 0.0f;
	if (_AdjustTorpDepth > 1.0f) _AdjustTorpDepth = 1.0f;

	_AdjustConvergence += (_AdjustConvergenceDir * DeltaTime * GUNSIGHT_CONV_MOVEMENT_RATE);
	if (_AdjustConvergence < 0.0f) _AdjustConvergence = 0.0f;
	if (_AdjustConvergence > 1.0f) _AdjustConvergence = 1.0f;

	weapAmmoSetTorpedoRunDepth(_AdjustTorpDepth * (GUNSIGHT_MAX_TORP_DEPTH - 1) + 1);

	if (GunSightView.GunSightTiltsToAim == FALSE)
	{
		GunSightView.ViewUseMat = GunSightView.AlignmentMat;
	}
	else
	{
		Att.y = Att.z = 0.0f;
		Att.x = DEG_TO_RAD(_AdjustGunsight * (_AdjustMax - _AdjustMin));
		mathEul_ToMatrix(&Mat1,EulOrdXYZr,&Att);

		math3dMatrixMultiply(&GunSightView.ViewUseMat,
								&Mat1,
								&GunSightView.AlignmentMat);
	}
	if (InstView.GunSightTiltsToAim == FALSE)
	{
		InstView.ViewUseMat = InstView.AlignmentMat;
	}
	else
	{
		Att.y = Att.z = 0.0f;
		Att.x = DEG_TO_RAD(_AdjustGunsight * (_AdjustMax - _AdjustMin));
		mathEul_ToMatrix(&Mat1,EulOrdXYZr,&Att);

		math3dMatrixMultiply(&InstView.ViewUseMat,
								&Mat1,
								&InstView.AlignmentMat);
	}

	if (_DrawStats == TRUE)
	{
		mlSetRangeReadout(_StatsRange);
	}

	_LastUpdateTime = frametime;

	return TRUE;
}

/************************************************************************************************
	persGunsightRangeInc
*************************************************************************************************/

UINT32 persGunsightRangeInc(SINT32 Param,UINT32 Id,UINT32 Detach)
{
	OBJECT*	pObjectData = persGetPersonaVehicleObjectDataPtr();
	if (!pObjectData)
		return 0;

	if (pObjectData->flag2 & OBJECT_ISFPS)
	{
		return (fpFPSControlInput(Param, Id, Detach));
	}
	else
	{
		_AdjustGunsightDir = (Param) ? 1 : 0;
		_TravelTime = 0;
	}
	return 0;
}

/************************************************************************************************
	persGunsightRangeDec
*************************************************************************************************/

UINT32 persGunsightRangeDec(SINT32 Param,UINT32 Id,UINT32 Detach)
{
	OBJECT*	pObjectData = persGetPersonaVehicleObjectDataPtr();
	if (!pObjectData)
		return 0;

	if (pObjectData->flag2 & OBJECT_ISFPS)
	{
		return (fpFPSControlInput(Param, Id, Detach));
	}
	else
	{
		_AdjustGunsightDir = (Param) ? -1 : 0;
		_TravelTime = 0;
	}
	return 0;
}

/************************************************************************************************
	persGunsightTargetSpeedInc
*************************************************************************************************/

UINT32 persGunsightTargetSpeedInc(SINT32 Param,UINT32 Id,UINT32 Detach)
{
	_AdjustSpeedDir = (Param) ? 1 : 0;
	_TravelTime = 0; // clear fire control solution
	return 0;
}

/************************************************************************************************
	persGunsightTargetSpeedDec
*************************************************************************************************/

UINT32 persGunsightTargetSpeedDec(SINT32 Param,UINT32 Id,UINT32 Detach)
{
	_AdjustSpeedDir = (Param) ? -1 : 0;
	_TravelTime = 0;
	return 0;
}

/************************************************************************************************
	persGunsightTargetAngleInc
*************************************************************************************************/

UINT32 persGunsightTargetAngleInc(SINT32 Param,UINT32 Id,UINT32 Detach)
{
	_AdjustAngleDir = (Param) ? 1 : 0;
	_TravelTime = 0;
	return 0;
}

/************************************************************************************************
	persGunsightTargetAngleDec
*************************************************************************************************/

UINT32 persGunsightTargetAngleDec(SINT32 Param,UINT32 Id,UINT32 Detach)
{
	_AdjustAngleDir = (Param) ? -1 : 0;
	_TravelTime = 0;
	return 0;
}

/************************************************************************************************
	persGunsightTargetMarkBearing
*************************************************************************************************/

UINT32 persGunsightTargetMarkBearing(SINT32 Param,UINT32 Id,UINT32 Detach)
{
	_TargetBearing = _GetCurrentViewBearing();
	_TravelTime = 0;

	return 0;
}

/************************************************************************************************
	persGunsightTorpedoDepthInc
*************************************************************************************************/

UINT32 persGunsightTorpedoDepthInc(SINT32 Param,UINT32 Id,UINT32 Detach)
{
	_AdjustTorpDepthDir = (Param) ? 1 : 0;
	_TravelTime = 0;
	return 0;
}

/************************************************************************************************
	persGunsightTorpedoDepthDec
*************************************************************************************************/

UINT32 persGunsightTorpedoDepthDec(SINT32 Param,UINT32 Id,UINT32 Detach)
{
	_AdjustTorpDepthDir = (Param) ? -1 : 0;
	_TravelTime = 0;
	return 0;
}

/************************************************************************************************
	persGunsightConvergenceInc
*************************************************************************************************/

UINT32 persGunsightConvergenceInc(SINT32 Param,UINT32 Id,UINT32 Detach)
{
	_AdjustConvergenceDir = (Param) ? 1.0f : 0.0f;
	return 0;
}

/************************************************************************************************
	persGunsightConvergenceDec
*************************************************************************************************/

UINT32 persGunsightConvergenceDec(SINT32 Param,UINT32 Id,UINT32 Detach)
{
	_AdjustConvergenceDir = (Param) ? -1.0f : 0.0f;
	return 0;
}

/************************************************************************************************
	persGunsightCalcTargetSolution
*************************************************************************************************/

UINT32 persGunsightCalcTargetSolution(SINT32 Param,UINT32 Id,UINT32 Detach)
{
	if(Param)
		_CalcSolution = TRUE;

	return 0;
}

/************************************************************************************************
	persGunsightSetConvergenceRange
*************************************************************************************************/

void persGunsightSetConvergenceRange(FLOAT32 Range)
{
	// This range check might be redundant since we are now calling from cfgPrefs
	if(Range > 800)
		Range = 800;
	else if(Range < 0)
		Range = 0;

	_ConvergenceRange = Range;
}

/************************************************************************************************
	persGunsightConvergenceRange
*************************************************************************************************/

FLOAT32 persGunsightConvergenceRange()
{
	return (float)_ConvergenceRange;
}

static LOGICAL _ClearGunfileData(GSCommandList * GSCL)
{
SINT32 i;

	for (i=0;i<MAX_GUNFILE_ELEMENTS;i++)
	{
		if (Lookup[GSCL->GE[i].CommandCode].KillFunction != nullptr)
		{
			Lookup[GSCL->GE[i].CommandCode].KillFunction(GSCL->GE[i].Data);
		}

		GSCL->GE[i].Data = nullptr;

		if (GSCL->GE[i].CommandCode == 0)
		{
			break;
		}
		GSCL->GE[i].CommandCode = 0;
	}
	GSCL->GECounter = 0;

	memset(GSCL, 0x0, sizeof(GSCommandList));

	GSCL->AlignmentMat = GSCL->TiltMat = GSCL->ViewUseMat = IdentityMatrix;

	return TRUE;
}

static LOGICAL _ReadGunfileData(GSCommandList * GSCL,SINT8 * FileName)
{
	SINT32 GunFile;
	static UCHAR Buffer[256];
	UCHAR * VarName;
	UCHAR * VarContents;
	SINT32 VarNum;
	SINT32 i;
	SINT32 LineCount;

	GSCL->AlignRT = nullptr;

	GSCL->ViewDOF = nullptr;

    std::string str = FileName;
    std::replace(str.begin(), str.end(), '\\', '/');

    bool usedirectory = false;
    int state = fioQuery();

#ifdef BETA_RELEASE

#define PATH "gunsights/"

    int pos =  str.find_last_of('/'); // filename
    pos = str.find_last_of('/', pos - 1); // directory

    std::string name;

    if(pos != std::string::npos)
    {
        name = str.substr(pos + 1, str.length() - pos);
    }

    if(!name.empty())
    {
        std::string path = PATH;
        path += name;

        // Force directory reading
        fioSet(ISDIRECTORY);

        // Test if local file is present for beta testers
        if(fioAccess(path.c_str()))
        {
            str = path;
            usedirectory = true;
        }

        fioSet(state);
    }

#endif

    if(usedirectory)
        fioSet(ISDIRECTORY);

    GunFile = fioOpen(str.c_str());

    fioSet(state);

	if (GunFile == -1)
	{
		Write_Debug("Failed attempt to read gunsight file %s",FileName);
		return FALSE;
	}

	Write_Debug("Reading gunsight file %s",FileName);

	_AdjustMin = 0.0;
	_AdjustMax = 0.0;
	_AmmoTypeName[0] = '\0';

	LineCount = 1;

	while (fioGetString(GunFile,(char*)Buffer,255) != 0)
	{
		if (Buffer[0] == '#' ||
			strlen((SCHAR*)Buffer) < 2)
		{
			continue;
		}

		VarName = nullptr;
		VarContents = nullptr;
		mlUtilityGetVarnameAndContents1(&VarName,&VarContents,Buffer);

		if (VarName == nullptr)
		{
			continue;
		}

		VarNum = 0;

		for (i=0;i<MAX_GUNFILE_COMMANDS;i++)
		{
			if (Lookup[i].CommandName[0] == 0)
			{
				Write_Debug("Unidentified gunfile command %s %s",VarName,VarContents);
				// at end of list, therefore command not recognized
				break;
			}

			if (strcmp(Lookup[i].CommandName,(char*)VarName) == 0)
			{
				VarNum = i;
				break;
			}
		}

		if (VarNum != 0)
		{
			if (Lookup[VarNum].ReadFunction != nullptr)
			{
				if (Lookup[VarNum].ReadFunction(GSCL,(char*)VarContents) != TRUE)
				{
					Write_Debug("Error function %s, line %d",VarName,LineCount);
				}
				else
				{
					GSCL->GE[GSCL->GECounter++].CommandCode = VarNum;
				}
			}
			else
			{
				GSCL->GE[GSCL->GECounter++].CommandCode = VarNum;
			}
		}

		LineCount++;
	}

	fioClose(GunFile);

	return TRUE;
}


static LOGICAL _ReadGenericFloat(GSCommandList * GSCL,SINT8 * Params)
{
FLOAT32 Temp;

	Temp = (FLOAT32)atof(Params);

	memcpy(&(GSCL->GE[GSCL->GECounter].Data),&Temp,sizeof(FLOAT32));

	return TRUE;
}


static LOGICAL _ReadThinLine(GSCommandList * GSCL,SINT8 * Params)
{
FLOAT32* Ptr;
FLOAT32 X1,Y1,X2,Y2;
SINT8* Ptr1,*Ptr2;

	mlUtilityGetVarnameAndContents1((UCHAR**)&Ptr1,(UCHAR**)&Ptr2,(UCHAR*)Params);
	if (Ptr1 == nullptr || Ptr2 == nullptr)
	{
		return FALSE;
	}
	X1 = (FLOAT32)(atof(Ptr1));

	mlUtilityGetVarnameAndContents1((UCHAR**)&Ptr1,(UCHAR**)&Ptr2,(UCHAR*)Ptr2);
	if (Ptr2 == nullptr)
	{
		return FALSE;
	}
	Y1 = (FLOAT32)(atof(Ptr1));

	mlUtilityGetVarnameAndContents1((UCHAR**)&Ptr1,(UCHAR**)&Ptr2,(UCHAR*)Ptr2);
	if (Ptr2 == nullptr)
	{
		return FALSE;
	}
	X2 = (FLOAT32)(atof(Ptr1));
	Y2 = (FLOAT32)(atof(Ptr2));

	Ptr = (float*)memAlloc(sizeof(FLOAT32)*4);

	if (Ptr == nullptr)
	{
		Write_Debug("Memory Allocation Error!");
		return FALSE;
	}

	Ptr[0] = X1;
	Ptr[1] = Y1;
	Ptr[2] = X2;
	Ptr[3] = Y2;

	GSCL->GE[GSCL->GECounter].Data = Ptr;

	return TRUE;
}

static LOGICAL _ReadThickLine(GSCommandList * GSCL,SINT8 * Params)
{
FLOAT32* Ptr;
FLOAT32 X1,Y1,X2,Y2;
FLOAT32 Thickness;
SINT8* Ptr1,*Ptr2;

	mlUtilityGetVarnameAndContents1((UCHAR**)&Ptr1,(UCHAR**)&Ptr2,(UCHAR*)Params);
	if (Ptr1 == nullptr || Ptr2 == nullptr)
	{
		return FALSE;
	}
	X1 = (FLOAT32)(atof(Ptr1));

	mlUtilityGetVarnameAndContents1((UCHAR**)&Ptr1,(UCHAR**)&Ptr2,(UCHAR*)Ptr2);
	if (Ptr2 == nullptr)
	{
		return FALSE;
	}
	Y1 = (FLOAT32)(atof(Ptr1));

	mlUtilityGetVarnameAndContents1((UCHAR**)&Ptr1,(UCHAR**)&Ptr2,(UCHAR*)Ptr2);
	if (Ptr2 == nullptr)
	{
		return FALSE;
	}
	X2 = (FLOAT32)(atof(Ptr1));

	mlUtilityGetVarnameAndContents1((UCHAR**)&Ptr1,(UCHAR**)&Ptr2,(UCHAR*)Ptr2);
	if (Ptr2 == nullptr)
	{
		return FALSE;
	}
	Y2 = (FLOAT32)(atof(Ptr1));

	Thickness = (FLOAT32)(atof(Ptr2));

	Ptr = (float*)memAlloc(sizeof(FLOAT32)*5);

	if (Ptr == nullptr)
	{
		Write_Debug("Memory Allocation Error!");
		return FALSE;
	}

	Ptr[0] = X1;
	Ptr[1] = Y1;
	Ptr[2] = X2;
	Ptr[3] = Y2;
	Ptr[4] = Thickness;

	GSCL->GE[GSCL->GECounter].Data = Ptr;

	return TRUE;
}

static LOGICAL _ReadTriangleLine(GSCommandList * GSCL,SINT8 * Params)
{
FLOAT32* Ptr;
FLOAT32 X1,Y1,X2,Y2,X3,Y3;
SINT8* Ptr1,*Ptr2;

	mlUtilityGetVarnameAndContents1((UCHAR**)&Ptr1,(UCHAR**)&Ptr2,(UCHAR*)Params);
	if (Ptr1 == nullptr || Ptr2 == nullptr)
	{
		return FALSE;
	}
	X1 = (FLOAT32)(atof(Ptr1));

	mlUtilityGetVarnameAndContents1((UCHAR**)&Ptr1,(UCHAR**)&Ptr2,(UCHAR*)Ptr2);
	if (Ptr2 == nullptr)
	{
		return FALSE;
	}
	Y1 = (FLOAT32)(atof(Ptr1));

	mlUtilityGetVarnameAndContents1((UCHAR**)&Ptr1,(UCHAR**)&Ptr2,(UCHAR*)Ptr2);
	if (Ptr2 == nullptr)
	{
		return FALSE;
	}
	X2 = (FLOAT32)(atof(Ptr1));

	mlUtilityGetVarnameAndContents1((UCHAR**)&Ptr1,(UCHAR**)&Ptr2,(UCHAR*)Ptr2);
	if (Ptr2 == nullptr)
	{
		return FALSE;
	}
	Y2 = (FLOAT32)(atof(Ptr1));

	mlUtilityGetVarnameAndContents1((UCHAR**)&Ptr1,(UCHAR**)&Ptr2,(UCHAR*)Ptr2);
	if (Ptr2 == nullptr)
	{
		return FALSE;
	}
	X3 = (FLOAT32)(atof(Ptr1));
	Y3 = (FLOAT32)(atof(Ptr2));

	Ptr = (float*)memAlloc(sizeof(FLOAT32)*6);

	if (Ptr == nullptr)
	{
		Write_Debug("Memory Allocation Error!");
		return FALSE;
	}

	Ptr[0] = X1;
	Ptr[1] = Y1;
	Ptr[2] = X2;
	Ptr[3] = Y2;
	Ptr[4] = X3;
	Ptr[5] = Y3;

	GSCL->GE[GSCL->GECounter].Data = Ptr;

	return TRUE;
}

// text size is in mils
// x and y are centerpoints
static LOGICAL _ReadTextLine(GSCommandList * GSCL,SINT8 * Params)
{
	SINT8* Ptr1,*Ptr2;
	static SINT8 TextString[128];
	FLOAT32 X,Y,Size,Rotation;
	FLOAT32* Ptr;

	mlUtilityGetVarnameAndContents1((UCHAR**)&Ptr1,(UCHAR**)&Ptr2,(UCHAR*)Params);
	if (Ptr1 == nullptr || Ptr2 == nullptr)
	{
		return FALSE;
	}
	X = (FLOAT32)(atof(Ptr1));

	mlUtilityGetVarnameAndContents1((UCHAR**)&Ptr1,(UCHAR**)&Ptr2,(UCHAR*)Ptr2);
	if (Ptr2 == nullptr)
	{
		return FALSE;
	}
	Y = (FLOAT32)(atof(Ptr1));

	mlUtilityGetVarnameAndContents1((UCHAR**)&Ptr1,(UCHAR**)&Ptr2,(UCHAR*)Ptr2);
	if (Ptr2 == nullptr)
	{
		return FALSE;
	}
	Size = (FLOAT32)(atof(Ptr1));

	mlUtilityGetVarnameAndContents1((UCHAR**)&Ptr1,(UCHAR**)&Ptr2,(UCHAR*)Ptr2);
	if (Ptr2 == nullptr)
	{
		return FALSE;
	}
	Rotation = (FLOAT32)(atof(Ptr1));

	strcpy(TextString,Ptr2);

	Ptr = (float*)memAlloc(sizeof(FLOAT32)*16 + 128);
	if (Ptr == nullptr)
	{
		Write_Debug("Memory Allocation Error!");
		return FALSE;
	}

	Ptr[0] = X;
	Ptr[1] = Y;
	Ptr[2] = Size;
	Ptr[3] = DEG_TO_RAD(Rotation);
	strcpy((char*)(&Ptr[4]),TextString);

	GSCL->GE[GSCL->GECounter].Data = Ptr;

	return TRUE;
}

static LOGICAL _ReadPLLine(GSCommandList * GSCL,SINT8 * Params)
{
FLOAT32* Ptr;
FLOAT32 X1,X2;
SINT8* Ptr1,*Ptr2;

	mlUtilityGetVarnameAndContents1((UCHAR**)&Ptr1,(UCHAR**)&Ptr2,(UCHAR*)Params);
	if (Ptr1 == nullptr || Ptr2 == nullptr)
	{
		return FALSE;
	}
	X1 = (FLOAT32)(atof(Ptr1));
	X2 = (FLOAT32)(atof(Ptr2));

	Ptr = (float*)memAlloc(sizeof(FLOAT32)*2);

	if (Ptr == nullptr)
	{
		Write_Debug("Memory Allocation Error!");
		return FALSE;
	}

	Ptr[0] = X1;
	Ptr[1] = X2;

	GSCL->GE[GSCL->GECounter].Data = Ptr;

	return TRUE;
}

static LOGICAL _ReadRangeLine(GSCommandList * GSCL,SINT8 * Params)
{
FLOAT32* Ptr;
FLOAT32 X1,X2,Range1,Range2;
SINT8* Ptr1,*Ptr2;

	mlUtilityGetVarnameAndContents1((UCHAR**)&Ptr1,(UCHAR**)&Ptr2,(UCHAR*)Params);
	if (Ptr1 == nullptr || Ptr2 == nullptr)
	{
		return FALSE;
	}
	X1 = (FLOAT32)(atof(Ptr1));

	mlUtilityGetVarnameAndContents1((UCHAR**)&Ptr1,(UCHAR**)&Ptr2,(UCHAR*)Ptr2);
	if (Ptr2 == nullptr)
	{
		return FALSE;
	}
	Range1 = (FLOAT32)(atof(Ptr1));


	mlUtilityGetVarnameAndContents1((UCHAR**)&Ptr1,(UCHAR**)&Ptr2,(UCHAR*)Ptr2);
	if (Ptr1 == nullptr || Ptr2 == nullptr)
	{
		return FALSE;
	}
	X2 = (FLOAT32)(atof(Ptr1));
	Range2 = (FLOAT32)(atof(Ptr2));

	Ptr = (float*)memAlloc(sizeof(FLOAT32)*4);

	if (Ptr == nullptr)
	{
		Write_Debug("Memory Allocation Error!");
		return FALSE;
	}

	Ptr[0] = X1;
	Ptr[1] = X2;
	Ptr[2] = Range1;
	Ptr[3] = Range2;

	GSCL->GE[GSCL->GECounter].Data = Ptr;

	return TRUE;
}


static LOGICAL _ReadAmmoTypeLine(GSCommandList * GSCL,SINT8 * Params)
{
persGunsightRangeTable * RT;
FLOAT32 MaxRange;
SINT8* Ptr1,*Ptr2;
weapAmmoType * AmmoType;

	mlUtilityGetVarnameAndContents1((UCHAR**)&Ptr1,(UCHAR**)&Ptr2,(UCHAR*)Params);
	if (Ptr1 == nullptr || Ptr2 == nullptr)
	{
		return FALSE;
	}

	MaxRange = (FLOAT32)(atof(Ptr2));

	RT = (persGunsightRangeTable*)memAlloc(sizeof(persGunsightRangeTable));
	if (RT == nullptr)
	{
		return FALSE;
	}

	AmmoType = nullptr;
	weapFindAmmoTypeByName(&AmmoType,(UCHAR*)Ptr1);
	if (AmmoType == nullptr)
	{
		return FALSE;
	}

	strncpy(_AmmoTypeName, Ptr1, 63);

	if(AmmoType->SubType == WEAPAMMO_SUBTYPE_TORPEDO)
		return TRUE;

	if (!persComputeRangeTable(RT,AmmoType,MaxRange))
	{
		memFree(RT);
		return FALSE;
	}

	GSCL->GE[GSCL->GECounter].Data = RT;

	if (GSCL->AlignRT == nullptr)
	{
		GSCL->AlignRT = RT;
	}

	return TRUE;
}

static LOGICAL _SetTilt(GSCommandList * GSCL,SINT8 * Params)
{
	GSCL->GunSightTiltsToAim = TRUE;

	return TRUE;
}

static LOGICAL _DisplayMagLine(void*  Data)
{
	vehVehicle* vehicle = persGetPersonaVehiclePtr();
	if ( vehicle == nullptr || vehicle->VehicleType == nullptr ) return TRUE;

	FLOAT32 horizon = piLongLikeFloat(FogEnd);
    // HACK!
    // Z value is 1000 meters. horizon less than 1000 will make the
    // gunsight disappear on near fog (user adjusted fog distance)
	if ( horizon < 1000.0f ) horizon = 1000.0f;

	FLOAT32* Mag = (FLOAT32*)&Data;
	FLOAT32 FieldOfView = _BaseFOV / (*Mag);

	if (vehicle->VehicleType->Type == VEH_TYPE_PERSON)
	{
		piSetFieldOfView(FieldOfView, 0.1f, horizon);
	}
	else
	{
		piSetFieldOfView(FieldOfView, 1.0f, horizon);
	}

	return TRUE;
}

// note: Gunsight texture has hole 1/2 its size, so we double
// the field of view to get an accurate cutout circle
//
// *MUST BE LAST FUNCTION DRAWN*
static LOGICAL _DisplayFovLine(void*  Data)
{
    _RenderLineVertices();
	_RenderTextVertices();

#if(1)
piVECTOR V1,V2,V3,V4;
FLOAT32* FOV,FOVMils;

	FOV = (FLOAT32*)&Data;

	FOVMils = DEG_TO_MIL(*FOV);

	V1.z = V2.z = V3.z = V4.z = 1000.0f;

	V1.x = -FOVMils;
	V2.x = FOVMils;
	V3.x = FOVMils;
	V4.x = -FOVMils;

	V1.y = -FOVMils;
	V2.y = -FOVMils;
	V3.y = FOVMils;
	V4.y = FOVMils;

    if(ogl::UseDeprecatedPath())
    {
	    piSetActiveTexture(_GunsightFOVTexId, 0);
    }
    else
    {
        basictexture->AddTexture(0, _GunsightFOVTexId);
        basictexture->Apply();
    }

	draw3dTexturedQuad(&V4,&V3,&V2,&V1,colorWHITEALPHA);

    if(ogl::UseDeprecatedPath())
    {
        piDisableTexture();
    }
    else
    {
        basiccolor->Apply();
    }

	V1.x = -10000;
	V1.y = -10000;
	V2.x = -FOVMils;
	V2.y = -10000;
	V3.x = -FOVMils;
	V3.y = 10000;
	V4.x = -10000;
	V4.y = 10000;
	draw3dTexturedQuad(&V4,&V3,&V2,&V1,colorBLACKALPHA);

	V1.x = FOVMils;
	V1.y = -10000;
	V2.x = FOVMils;
	V2.y = 10000;
	V3.x = 10000;
	V3.y = 10000;
	V4.x = 10000;
	V4.y = -10000;
	draw3dTexturedQuad(&V1,&V2,&V3,&V4,colorBLACKALPHA);

	V1.x = -FOVMils;
	V1.y = FOVMils;
	V2.x = FOVMils;
	V2.y = FOVMils;
	V3.x = FOVMils;
	V3.y = 10000;
	V4.x = -FOVMils;
	V4.y = 10000;
	draw3dTexturedQuad(&V4,&V3,&V2,&V1,colorBLACKALPHA);

	V1.x = -FOVMils;
	V1.y = -10000;
	V2.x = FOVMils;
	V2.y = -10000;
	V3.x = FOVMils;
	V3.y = -FOVMils;
	V4.x = -FOVMils;
	V4.y = -FOVMils;
	draw3dTexturedQuad(&V4,&V3,&V2,&V1,colorBLACKALPHA);
#else
    float scale =  90.0f / _BaseFOV;
    _drawscope(_binocs, scale);
#endif

    return TRUE;
}

static LOGICAL _DisplayThinLine(void*  Data)
{
piVECTOR V1,V2;
FLOAT32* Ptr;
FLOAT32 Mils;

	Ptr = (FLOAT32*)Data;
	V1.z = V2.z = 1000.0f;

	V1.x = Ptr[0];
	V1.y = Ptr[1];
	V2.x = Ptr[2];
	V2.y = Ptr[3];

	if (_CurrentRT != nullptr)
	{
		V1.x += _PLPos;
		V2.x += _PLPos;

		Mils = DEG_TO_MIL(_ComputeShotAngle());

		V1.y -= Mils;
		V2.y -= Mils;
	}

	_TranslateCoordinatesByAngle(&V1.x,&V1.y);
	_TranslateCoordinatesByAngle(&V2.x,&V2.y);


	_draw3dLine(&V1,&V2,colorBLACKALPHA);

	return TRUE;
}

static LOGICAL _DisplayTriangle(void*  Data)
{
piVECTOR V1,V2;
FLOAT32* Ptr;
FLOAT32 Mils;

	Ptr = (FLOAT32*)Data;
	V1.z = V2.z = 1000.0f;

	V1.x = Ptr[0];
	V1.y = Ptr[1];
	V2.x = Ptr[2];
	V2.y = Ptr[3];

	_TranslateCoordinatesByAngle(&V1.x,&V1.y);
	_TranslateCoordinatesByAngle(&V2.x,&V2.y);

	if (_CurrentRT != nullptr)
	{
		V1.x += _PLPos;
		V2.x += _PLPos;

		Mils = DEG_TO_MIL(_ComputeShotAngle());

		V1.y -= Mils;
		V2.y -= Mils;
	}

	_draw3dLine(&V1,&V2,colorBLACKALPHA);

	V1.x = Ptr[2];
	V1.y = Ptr[3];
	V2.x = Ptr[4];
	V2.y = Ptr[5];

	_TranslateCoordinatesByAngle(&V1.x,&V1.y);
	_TranslateCoordinatesByAngle(&V2.x,&V2.y);

	if (_CurrentRT != nullptr)
	{
		V1.x += _PLPos;
		V2.x += _PLPos;

		Mils = DEG_TO_MIL(_ComputeShotAngle());

		V1.y -= Mils;
		V2.y -= Mils;
	}

	_draw3dLine(&V1,&V2,colorBLACKALPHA);

	V1.x = Ptr[4];
	V1.y = Ptr[5];
	V2.x = Ptr[0];
	V2.y = Ptr[1];

	_TranslateCoordinatesByAngle(&V1.x,&V1.y);
	_TranslateCoordinatesByAngle(&V2.x,&V2.y);

	if (_CurrentRT != nullptr)
	{
		V1.x += _PLPos;
		V2.x += _PLPos;

		Mils = DEG_TO_MIL(_ComputeShotAngle());

		V1.y -= Mils;
		V2.y -= Mils;
	}

	_draw3dLine(&V1,&V2,colorBLACKALPHA);

	return TRUE;
}

static LOGICAL _DisplayTriangleFill(void*  Data)
{
piVECTOR V1,V2,V3;
FLOAT32* Ptr;
FLOAT32 Mils;

	Ptr = (FLOAT32*)Data;
	V1.z = V2.z = V3.z = 1000.0f;

	V1.x = Ptr[0];
	V1.y = Ptr[1];
	V2.x = Ptr[2];
	V2.y = Ptr[3];
	V3.x = Ptr[4];
	V3.y = Ptr[5];

	_TranslateCoordinatesByAngle(&V1.x,&V1.y);
	_TranslateCoordinatesByAngle(&V2.x,&V2.y);
	_TranslateCoordinatesByAngle(&V3.x,&V3.y);

	if (_CurrentRT != nullptr)
	{
		V1.x += _PLPos;
		V2.x += _PLPos;
		V3.x += _PLPos;

		Mils = DEG_TO_MIL(_ComputeShotAngle());

		V1.y -= Mils;
		V2.y -= Mils;
		V3.y -= Mils;
	}

	V1.x += _ladderOffsetX;
	V1.y += _ladderOffsetY;

	V2.x += _ladderOffsetX;
	V2.y += _ladderOffsetY;

	V3.x += _ladderOffsetX;
	V3.y += _ladderOffsetY;

    if(!ogl::UseDeprecatedPath())
        basiccolor->Apply();

	draw3dTriangle(&V3,&V2,&V1,colorBLACKALPHA,colorBLACKALPHA,colorBLACKALPHA);

	return TRUE;
}

static LOGICAL _NewPitchLadder(void*  Data)
{
	_PLPos = 0.0f;
	_CurrentRT = nullptr;
	_CurrentRange = 0.0f;
	_UseAngleRange = FALSE;
	_PitchScale = 1.0f;
	_AngleOffset = 0.0f;

	return TRUE;
}

static LOGICAL _SetAmmoType(void*  Data)
{
	_CurrentRT = (persGunsightRangeTable*)Data;

	return TRUE;
}

static LOGICAL _SetPLPos(void*  Data)
{
	memcpy(&_PLPos,&Data,sizeof(FLOAT32));

	return TRUE;
}

static LOGICAL _SetPLRange(void*  Data)
{
	memcpy(&_CurrentRange,&Data,sizeof(FLOAT32));

	return TRUE;
}

static LOGICAL _SetPitchScale(void*  Data)
{
	memcpy(&_PitchScale,&Data,sizeof(FLOAT32));

	return TRUE;
}

static LOGICAL _SetTextScaleX(void*  Data)
{
	memcpy(&_TextScaleX,&Data,sizeof(FLOAT32));

	return TRUE;
}

static LOGICAL _SetTextScaleY(void*  Data)
{
	memcpy(&_TextScaleY,&Data,sizeof(FLOAT32));

	return TRUE;
}

static LOGICAL _SetAngleOffset(void*  Data)
{
	memcpy(&_AngleOffset,&Data,sizeof(FLOAT32));

	return TRUE;
}

static LOGICAL _SetRangeAngleMin(void*  Data)
{
	memcpy(&_AdjustMin,&Data,sizeof(FLOAT32));

	return TRUE;
}

static LOGICAL _SetRangeAngleMax(void*  Data)
{
	memcpy(&_AdjustMax,&Data,sizeof(FLOAT32));

	return TRUE;
}

static LOGICAL _SetRangeReadout(void*  Data)
{
UINT32 i;
FLOAT32 Angle;
FLOAT32 Range;


	if (GunSightView.AlignRT != nullptr)
	{
		Angle = _AdjustGunsight * (_AdjustMax - _AdjustMin);

		Range = 0.0f;

		for (i=0;i<RANGETABLE_NUMELEMENTS;i++)
		{
			if (GunSightView.AlignRT->AimAngle[i] > Angle)
			{
				Range = (FLOAT32)(i * RANGETABLE_INCREMENT);
				break;
			}
		}

		_DrawStats = TRUE;
		_StatsRange = Range;
	}

	return TRUE;
}

static LOGICAL _DrawPLLine(void*  Data)
{
piVECTOR V1,V2;
FLOAT32 Mils;
FLOAT32* Ptr;

	if (_CurrentRT == nullptr)
	{
		return FALSE;
	}

	Ptr = (FLOAT32*)Data;
	V1.z = V2.z = 1000.0f;
	V1.y = V2.y = 0.0f;

	V1.x = Ptr[0];
	V2.x = Ptr[1];

	V1.x += _PLPos;
	V2.x += _PLPos;

	Mils = DEG_TO_MIL(_ComputeShotAngle());

	V1.y -= Mils;
	V2.y -= Mils;

	_TranslateCoordinatesByAngle(&V1.x,&V1.y);
	_TranslateCoordinatesByAngle(&V2.x,&V2.y);


	_draw3dLine(&V1,&V2,colorBLACKALPHA);

	return TRUE;
}

static LOGICAL _DrawRangeLine(void*  Data)
{
piVECTOR V1,V2;
FLOAT32 Mils;
FLOAT32* Ptr;
FLOAT32 SaveRange;

	if (_CurrentRT == nullptr)
	{
		return FALSE;
	}

	SaveRange = _CurrentRange;

	Ptr = (FLOAT32*)Data;
	V1.z = V2.z = 1000.0f;

	V1.x = Ptr[0];
	V2.x = Ptr[1];

	V1.x += _PLPos;
	V2.x += _PLPos;

	_CurrentRange = Ptr[2];
	Mils = -DEG_TO_MIL(_ComputeShotAngle());
	V1.y = Mils;

	_CurrentRange = Ptr[3];
	Mils = -DEG_TO_MIL(_ComputeShotAngle());
	V2.y = Mils;

	_CurrentRange = SaveRange;

	_TranslateCoordinatesByAngle(&V1.x,&V1.y);
	_TranslateCoordinatesByAngle(&V2.x,&V2.y);

	_draw3dLine(&V1,&V2,colorBLACKALPHA);

	return TRUE;
}

static LOGICAL _DisplayTextLine(void*  Data)
{
	FLOAT32 X,Y,Size,Rotation;
	FLOAT32* Ptr;
	FLOAT32 Mils;
	static SINT8 TextString[128];

	Ptr = (FLOAT32*)Data;
	X = Ptr[0];
	Y = Ptr[1];
	Size = Ptr[2];
	Rotation = Ptr[3];

	strcpy(TextString,(char*)(&Ptr[4]));

	// check for pitch ladder text
	if (_CurrentRT != nullptr)
	{
		X += _PLPos;

		Mils = DEG_TO_MIL(_ComputeShotAngle());

		Y -= Mils;
	}

	_DrawGunsightText(X,Y,Size,Rotation,uiUTF8toUTF16(TextString));

	return TRUE;
}


static LOGICAL _DisplayThickLine(void*  Data)
{
piVECTOR V1,V2,P1,P2,P3,P4;
FLOAT32 Thickness;
FLOAT32 TempNum;
FLOAT32 Mils;
FLOAT32* Ptr;

	Ptr = (FLOAT32*)Data;
	V1.z = V2.z = 1000.0f;

	V1.x = Ptr[0];
	V1.y = Ptr[1];
	V2.x = Ptr[2];
	V2.y = Ptr[3];
	Thickness = Ptr[4];

	if (_CurrentRT != nullptr)
	{
		V1.x += _PLPos;
		V2.x += _PLPos;

		Mils = DEG_TO_MIL(_ComputeShotAngle());

		V1.y -= Mils;
		V2.y -= Mils;
	}

	P1 = P2 = V1;
	P3 = P4 = V2;

	mathSubVectors(&V1,&V1,&V2);
	TempNum = V1.x;
	V1.x = V1.y;
	V1.y = TempNum;
	math3dNormalize(&V1);

	P1.x += V1.x * Thickness;
	P1.y += V1.y * Thickness;
	P3.x += V1.x * Thickness;
	P3.y += V1.y * Thickness;
	P2.x -= V1.x * Thickness;
	P2.y -= V1.y * Thickness;
	P4.x -= V1.x * Thickness;
	P4.y -= V1.y * Thickness;

	_TranslateCoordinatesByAngle(&P1.x,&P1.y);
	_TranslateCoordinatesByAngle(&P2.x,&P2.y);
	_TranslateCoordinatesByAngle(&P3.x,&P3.y);
	_TranslateCoordinatesByAngle(&P4.x,&P4.y);

    if(ogl::UseDeprecatedPath())
	    piDisableTexture();
    else
        basiccolor->Apply();

    draw3dTexturedQuad(&P2,&P4,&P3,&P1,colorBLACKALPHA);
	draw3dTexturedQuad(&P1,&P3,&P4,&P2,colorBLACKALPHA);

	return TRUE;
}

static LOGICAL _KillGeneric(void*  Data)
{
	if (Data != nullptr)
	{
		memFree(Data);
	}
	return TRUE;
}

LOGICAL persPrepGunsightDraw(void)
{

	_DrawStats = FALSE;
	_ladderOffsetX = 0.0f;
	_ladderOffsetY = 0.0f;
	return TRUE;
}

LOGICAL persGunsightDraw(OBJECT* DOF, UINT32 IsInstrumentView)
{
	GSCommandList * GSCL;
	FLOAT32 SavedFieldOfView;
	SINT32 i;
	piMATRIX SavedViewMat;

	GSCL = (IsInstrumentView ) ? &InstView : &GunSightView;

	if(!GSCL->GECounter) return FALSE;

	_DrawStats = FALSE;

	VertexCount = 0;
	LineVertexCount = 0;

	piSetRenderState(piRENDERSTATE_FOGENABLE, FALSE);
	piSetRenderState(piRENDERSTATE_ZFUNC, piCMP_ALWAYS);
	piSetRenderState(piRENDERSTATE_ZWRITEENABLE, FALSE);
	piSetRenderStageState(0, piTSS_MIPFILTER, piTFP_NONE);

	SavedFieldOfView = FieldOfView;
	SavedViewMat = *piGetViewMatrix();

	piSetViewMatrix(&IdentityMatrix);
	piSetWorldMatrix(&IdentityMatrix);

	// reset pitch ladder stuff
	_NewPitchLadder(nullptr);

	// draw the sight
	for(i = 0; i < GSCL->GECounter; i++)
	{
		if(GSCL->GE[i].CommandCode && Lookup[GSCL->GE[i].CommandCode].DisplayFunction)
			Lookup[GSCL->GE[i].CommandCode].DisplayFunction( GSCL->GE[i].Data);
	}

	// render any text/lines that didn't get rendered in the FOV line
	// or if there is no FOV line
	_RenderLineVertices();
	_RenderTextVertices();

	piSetViewMatrix(&SavedViewMat);

	gmlpHandleFieldOfView();

	piSetRenderStageState(0, piTSS_MIPFILTER, TextureMipFilter);
	piSetRenderState(piRENDERSTATE_ZWRITEENABLE, bZWrite);
	piSetRenderState(piRENDERSTATE_ZFUNC, ZFunction);
	piSetRenderState(piRENDERSTATE_FOGENABLE, piFOGENABLE_DEFAULT);

	return TRUE;
}

static LOGICAL _RenderTextVertices(void)
{
piLVERTEX* Vertex;
SINT32 NumVerts;
SINT32 i;

	// render the text
	if (VertexCount > 0)
	{
        if(ogl::UseDeprecatedPath())
        {
		    piSetActiveTexture(rntextGetTexId(GUNSIGHT_FONT), 0);
        }
        else
        {
            basictexture->AddTexture(0, rntextGetTexId(GUNSIGHT_FONT));
            basictexture->Apply();
        }

		for (i=0;i<=VertexCount / 480;i++)
		{
			Vertex = &(VertexList[i * 480]);
			if (VertexCount / 480 > i)
			{
				NumVerts = 480;
			}
			else
			{
				NumVerts = VertexCount % 480;
			}

			if (NumVerts > 0)
			{
				if (!piRenderScene(piPT_TRIANGLELIST, piVT_LVERTEX,
									Vertex, NumVerts))
				{
					return FALSE;
				}
			}
		}
	}

	VertexCount = 0;

	return TRUE;
}

static LOGICAL _RenderLineVertices(void)
{
piLVERTEX* Vertex;
SINT32 NumVerts;
SINT32 i;

	// render the text
	if (LineVertexCount > 0)
	{
        if(ogl::UseDeprecatedPath())
		    piDisableTexture();
        else
            basiccolor->Apply();

        for (i=0;i<=LineVertexCount / 500;i++)
		{
			Vertex = &(LineList[i * 500]);
			if (LineVertexCount / 500 > i)
			{
				NumVerts = 500;
			}
			else
			{
				NumVerts = LineVertexCount % 500;
			}

			if (NumVerts > 0)
			{
				if (!piRenderScene(piPT_LINELIST, piVT_LVERTEX, Vertex, NumVerts))
				{
					return FALSE;
				}
			}
		}
	}

	LineVertexCount = 0;

	return TRUE;
}

static FLOAT32 _ComputeShotAngle(void)
{
FLOAT32 Angle,Angle1,Angle2;
FLOAT32 Delta;
SINT32 I1,I2;

	I1 = (SINT32)(_CurrentRange / RANGETABLE_INCREMENT);
	I2 = I1 + 1;

	Angle1 = _CurrentRT->AimAngle[I1];
	Angle2 = _CurrentRT->AimAngle[I2];

	Delta = (_CurrentRange - (I1 * RANGETABLE_INCREMENT)) / RANGETABLE_INCREMENT;

	Angle = (Angle2 - Angle1) * Delta + Angle1;

	Angle += _AdjustGunsight * (_AdjustMax - _AdjustMin);

	return Angle;
}

#if(0)
static FLOAT32 _ComputeShotDrop(FLOAT32 Range,persGunsightRangeTable * RT)
{
FLOAT32 Drop,Drop1,Drop2;
FLOAT32 Delta;
SINT32 I1,I2;

	I1 = (SINT32)(Range / RANGETABLE_INCREMENT);
	I2 = I1 + 1;

	Drop1 = RT->ProjectileDrop[I1];
	Drop2 = RT->ProjectileDrop[I2];

	Delta = (Range - (I1 * RANGETABLE_INCREMENT)) / RANGETABLE_INCREMENT;
	Drop = (Drop2 - Drop1) * Delta + Drop1;

	return Drop;
}
#endif

static LOGICAL _DrawGunsightText(FLOAT32 X,
						 FLOAT32 Y,
						 FLOAT32 Size,
						 FLOAT32 Rotation,
						 const WCHAR* TextString)
{
FLOAT32 Width,Height,WMov,HMov,Scale;
FLOAT32 TempNum;
FLOAT32 C,S;
SINT32 NumVerts;
SINT32 i;
LOGICAL DoRotation;
piLVERTEX * Vertex;

	if (wcslen(TextString) > 32)
	{
		Write_Debug("Attempted to display text string that was too long in rendering gunsight");
		Write_Debug(" String = %s",TextString);
		piExitProgram(_FL,"Gunsight render string too long");
	}

	NumVerts = wcslen(TextString) * 6;

	Vertex = &(VertexList[VertexCount]);

	rntext3DString(TextString,
					colorBLACKALPHA,
					GUNSIGHT_FONT,
					Vertex,
					NumVerts);


	VertexCount += NumVerts;

	Width = 0;
	Height = 0;
	for (i=0;i<NumVerts;i++)
	{
		if (Vertex[i].x > Width)
		{
			Width = Vertex[i].x;
		}
		if (Vertex[i].y > Height)
		{
			Height = Vertex[i].y;
		}
	}

	if (Width <= 0.0001 || Height <= 0.0001)
	{
		return FALSE;
	}

	WMov = Width * 0.5f;
	HMov = Height * 0.5f;
	Scale = 1.0f / Height * Size;
#ifdef RNTEXT2
    Scale *= 0.6; // fudge factor
#endif
	if (Rotation > 0.0001 || Rotation < -0.0001)
	{
		DoRotation = TRUE;
		C = (FLOAT32)(cos(Rotation));
		S = (FLOAT32)(sin(Rotation));
	}
	else
	{
		DoRotation = FALSE;
	}

	for (i=0;i<NumVerts;i++)
	{
		// center text
		Vertex[i].x -= WMov;
		Vertex[i].y -= HMov;

		// scale text
		Vertex[i].x *= Scale * _TextScaleX;
		Vertex[i].y *= Scale * _TextScaleY;

		// rotate text
		if (DoRotation == TRUE)
		{
			TempNum = Vertex[i].x;
			Vertex[i].x = C * Vertex[i].x - S * Vertex[i].y;
			Vertex[i].y = S * TempNum + C * Vertex[i].y;
		}

		Vertex[i].z = 1000.0;
		Vertex[i].x += X;
		Vertex[i].y += Y;

		_TranslateCoordinatesByAngle(&Vertex[i].x,&Vertex[i].y);
	}

	return TRUE;
}

static LOGICAL _SetAngleRange(void*  Data)
{
	_UseAngleRange = TRUE;

	return TRUE;
}

static LOGICAL _ResetAngleRange(void*  Data)
{
	_UseAngleRange = FALSE;

	return TRUE;
}

static LOGICAL _TranslateCoordinatesByAngle(FLOAT32 * X,FLOAT32 * Y)
{
FLOAT32 Radius;
FLOAT32 Angle;

	*Y *= _PitchScale;

	if (_UseAngleRange == TRUE)
	{
		Radius = *X;
		Angle = MIL_TO_RAD(*Y) + DEG_TO_RAD(_AngleOffset);

		*X = (FLOAT32)(cos(Angle) * Radius);
		*Y = (FLOAT32)(sin(Angle) * Radius);
	}

	return TRUE;
}

UINT32 persResetGunsightAdjust(FLOAT32 val)
{
	_AdjustGunsight = val;
	return 0;
}

LOGICAL persLoadGunsight(SINT8* FileName)
{
	return _ReadGunfileData(&GunSightView,FileName);
}

LOGICAL persLoadInstGunsight(SINT8* FileName)
{
	return _ReadGunfileData(&InstView,FileName);
}

LOGICAL persClearGunsights(void)
{
	_ClearGunfileData(&GunSightView);
	_ClearGunfileData(&InstView);

	return TRUE;
}

LOGICAL persGunSightSetBaseFOV(FLOAT32 FOV)
{
	_BaseFOV = FOV;

	return TRUE;
}

LOGICAL persAlignGunsight(OBJECT * AlignObj,OBJECT * ViewDOF,FLOAT32 AlignmentRange)
{
	_AlignGS(&GunSightView,AlignObj,ViewDOF,AlignmentRange);

	return TRUE;
}

LOGICAL persAlignInstGunsight(OBJECT * AlignObj,OBJECT * ViewDOF,FLOAT32 AlignmentRange)
{
	_AlignGS(&InstView,AlignObj,ViewDOF,AlignmentRange);

	return TRUE;
}

static LOGICAL _AlignGS(GSCommandList * GSCL,OBJECT * AlignObj,OBJECT * ViewDOF,FLOAT32 AlignmentRange)
{
	piMATRIX TempMat = IdentityMatrix;

	if(!GSCL->AlignRT) return FALSE;

	if(AlignmentRange > 0)
		persGunsightAlignDOF(&TempMat,ViewDOF,AlignObj,AlignmentRange);

	GSCL->AlignmentMat = TempMat;
	GSCL->ViewDOF = ViewDOF;

	return TRUE;
}

// determines the matrix required to align the two DOF's to intersect at the specified distance
LOGICAL persGunsightAlignDOF(piMATRIX * RetMatrix, OBJECT * AlignedObject, OBJECT * AlignToObject, FLOAT32 AlignmentRange)
{
	FLOAT32 UseRange;
	piVECTOR V1;
	piVECTOR Align;
	piMATRIX Mat1;

	if(!(AlignedObject->flags & OBJECT_DOF) || !(AlignToObject->flags & OBJECT_DOF)) return FALSE;
	if(!RetMatrix) return FALSE;

	UseRange = (AlignmentRange > 0.001f) ? AlignmentRange : 400.0f;

	V1.x = V1.y = V1.z = 0.0f;
	V1.y = UseRange;

	math3dVectorMultiply(&V1,&V1,&AlignToObject->dof->dofmatrix2);
	math3dVectorMultiply(&V1,&V1,&AlignToObject->mat);
	math3dInvertMatrix(&Mat1,&AlignedObject->mat);
	math3dVectorMultiply(&V1,&V1,&Mat1);
	math3dVectorMultiply(&V1,&V1,&AlignedObject->dof->dofmatrix1);

	Align.z = (FLOAT32)atan2(V1.x,V1.y);
	Align.x = -(FLOAT32)atan2(V1.z,V1.y);
	Align.y = 0.0f;

	mathEul_ToMatrix(RetMatrix,EulOrdXYZr,&Align);

	return TRUE;
}

LOGICAL persUpdateGunsightView(void)
{
	if(GunSightView.ViewDOF)
		_UpdateView(&GunSightView);

	return TRUE;
}

LOGICAL persUpdateInstGunsightView(void)
{
	if(InstView.ViewDOF)
		_UpdateView(&InstView);

	return TRUE;
}

static LOGICAL _UpdateView(GSCommandList * GSCL)
{

	GSCL->ViewDOF->dof->dofposmatrix = GSCL->ViewUseMat;
	GSCL->ViewDOF->dof->dofflags |= DOF_USE_DOFPOSMATRIX;

	return TRUE;
}

// called when the player's vehicle goes away
LOGICAL persGunsightClearData(void)
{
	GunSightView.ViewDOF = nullptr;
	InstView.ViewDOF = nullptr;

	return TRUE;
}

LOGICAL persGunsightGetInstFieldOfView(FLOAT32* FOV)
{
FLOAT32 TempFloat32;
SINT32 i;

	for(i=0;i<InstView.GECounter;i++)
	{
		if (strcmp(Lookup[InstView.GE[i].CommandCode].CommandName,"mag") == 0)
		{
			memcpy(&TempFloat32,&(InstView.GE[i].Data),sizeof(FLOAT32));

			*FOV = _BaseFOV / TempFloat32;

			return TRUE;
		}
	}

	return FALSE;
}

LOGICAL persGunsightGetGunSightFieldOfView(FLOAT32* FOV)
{
FLOAT32 TempFloat32;
SINT32 i;

	for(i=0;i<GunSightView.GECounter;i++)
	{
		if (strcmp(Lookup[GunSightView.GE[i].CommandCode].CommandName,"mag") == 0)
		{
			memcpy(&TempFloat32,&(GunSightView.GE[i].Data),sizeof(FLOAT32));

			*FOV = _BaseFOV / TempFloat32;

			return TRUE;
		}
	}

	return FALSE;
}

static LOGICAL _draw3dLine(piVECTOR* point1, piVECTOR* point2, UINT32 Color)
{
	LineList[LineVertexCount].x = point1->x;
	LineList[LineVertexCount].y = point1->y;
	LineList[LineVertexCount].z = point1->z;
	LineList[LineVertexCount].color = Color;
	LineList[LineVertexCount].specular = colorALPHA100;
	LineList[LineVertexCount].tu = 0.0f;
	LineList[LineVertexCount++].tv = 0.0f;

	LineList[LineVertexCount].x = point2->x;
	LineList[LineVertexCount].y = point2->y;
	LineList[LineVertexCount].z = point2->z;
	LineList[LineVertexCount].color = Color;
	LineList[LineVertexCount].specular = colorALPHA100;
	LineList[LineVertexCount].tu = 0.0f;
	LineList[LineVertexCount++].tv = 0.0f;

	return FALSE;
}

static FLOAT32 persGunsightCalcDistanceForAngle(weapAmmoType const* AmmoType, FLOAT32 Angle)
{
	if(Angle <= 0 || Angle >=90) return 0.0f;

	FLOAT64 Mach;
	FLOAT64 Speed;
	FLOAT64 Cd;
	FLOAT64 Density;
	FLOAT64VECTOR Accel;
	FLOAT64 DeltaTime;
	FLOAT64 Drag;
	FLOAT64VECTOR AvgAcc;
	weapAmmo Ammo;
	DeltaTime = TIME_INCREMENT;

	memset(&Ammo,0,sizeof(weapAmmo));
	Ammo.Velocity.y =  Ammo.LastVel.y = AmmoType->InitVelocity*cos(Angle*PI/180.0);
	Ammo.Velocity.z =  Ammo.LastVel.z = AmmoType->InitVelocity*sin(Angle*PI/180.0);
	Ammo.AmmoType = (weapAmmoType*)AmmoType;

	for(;;)
	{
	// Get mach number for drag
		Speed = mathExactVectorLength(&Ammo.Velocity);
		mlGetAltSpeedOfSound(&Mach, Ammo.Position.z);
		Mach = Speed / Mach;
		UINT32 MachIdx = (UINT32)(floor(Mach / G1_CHART_MACH_DELTA));

		// Drag Coefficient
		if(MachIdx + 1 < G1_CHART_NUM_DIVISIONS)
		{
			FLOAT64 Delta = (Mach / G1_CHART_MACH_DELTA) - MachIdx;
			mlInterpolate(&Cd ,G1MachChart[MachIdx],G1MachChart[MachIdx+1], Delta);
		}else
		{
			Cd = G1MachChart[G1_CHART_NUM_DIVISIONS - 1];
		}

		mlGetAltDensity(&Density, Ammo.Position.z);

		// NOTE: the 1790 is a constant that makes the Cd table work with
		// G1 ballistic coefficients and this particular formula
		Drag = Density * Cd * Speed / (Ammo.AmmoType->BallisticCoefficient * 1790.0f);
		Accel.x = -Drag * Ammo.Velocity.x;
		Accel.y = -Drag * Ammo.Velocity.y;
		Accel.z = -Drag * Ammo.Velocity.z;

		AvgAcc.x = (Accel.x + Ammo.LastAcc.x) * 0.5;
		AvgAcc.y = (Accel.y + Ammo.LastAcc.y) * 0.5;
		AvgAcc.z = (Accel.z + Ammo.LastAcc.z) * 0.5;

		Ammo.Velocity.x += AvgAcc.x * DeltaTime;
		Ammo.Velocity.y += AvgAcc.y * DeltaTime;
		Ammo.Velocity.z += AvgAcc.z * DeltaTime;

		Ammo.LastAcc = Accel;

		Ammo.Velocity.z -= 9.8 * DeltaTime; // gravity

		// update position
		Ammo.Position.x += Ammo.LastVel.x * DeltaTime + 0.5 * AvgAcc.x * DeltaTime * DeltaTime;
		Ammo.Position.y += Ammo.LastVel.y * DeltaTime + 0.5 * AvgAcc.y * DeltaTime * DeltaTime;
		Ammo.Position.z += Ammo.LastVel.z * DeltaTime + 0.5 * AvgAcc.z * DeltaTime * DeltaTime;

		Ammo.LastVel = Ammo.Velocity;

		if(Ammo.Position.z < 0)
			break;
	}

	return (FLOAT32)Ammo.Position.y;

}

#define MORTAR_ANGLE_START	35
#define MORTAR_ANGLE_END	88
#define MORTAR_ANGLE_INC	1
#define MORTAR_RANGES ((MORTAR_ANGLE_END - MORTAR_ANGLE_START) / MORTAR_ANGLE_INC) + 1

static FLOAT32 persGunsightCalcMortarSightAngle(weapAmmoType const* AmmoType, FLOAT32 Range)
{
	// rudimentary caching
	static weapAmmoType const* LastAmmoType;
	static float Ranges[MORTAR_RANGES];
	FLOAT32 angle;
	if(AmmoType != LastAmmoType)
	{
		angle = MORTAR_ANGLE_START;
		for(int i = 0; i < MORTAR_RANGES; i++, angle += MORTAR_ANGLE_INC)
			Ranges[i] = persGunsightCalcDistanceForAngle(AmmoType, angle);
		LastAmmoType = AmmoType;
	}

	angle = MORTAR_ANGLE_START;
	for(int i = 0; i < MORTAR_RANGES - 1; i++, angle += MORTAR_ANGLE_INC)
	{
		if(Ranges[i] == Ranges[i+1] || Ranges[i] <= 0 || Ranges[i+1] <= 0) break;

		if(Range < Ranges[i] && Range > Ranges[i+1])
			return angle+MORTAR_ANGLE_INC*(Range-Ranges[i+1])/(Ranges[i]-Ranges[i+1]);
	}
	return MORTAR_ANGLE_START;
}

// do* NOT* use this function on a per-frame basis as it's not cheap
// returnangle is in radians
LOGICAL persGunsightCalcSightAngle(FLOAT32 * ReturnAngle,SINT8* AmmoName,FLOAT32 Range)
{
weapAmmoType* AmmoType;
persGunsightRangeTable RT;
FLOAT32 Angle,Angle1,Angle2;
FLOAT32 Delta;
SINT32 I1,I2;

	AmmoType = nullptr;
	weapFindAmmoTypeByName(&AmmoType,(UCHAR*)AmmoName);
	if(AmmoType == nullptr) return FALSE;

	if(AmmoType->SubType == WEAPAMMO_SUBTYPE_MORTAR)
	{
		*ReturnAngle = DEG_TO_RAD(persGunsightCalcMortarSightAngle(AmmoType, Range));
		return TRUE;
	}

	if(Range > RANGETABLE_MAXRANGE - 100.0)
		Range = (FLOAT32)RANGETABLE_MAXRANGE - 100.0f;

	persComputeRangeTable(&RT, AmmoType, Range + 100.0);

	I1 = (SINT32)(Range / RANGETABLE_INCREMENT);
	I2 = I1 + 1;

	Angle1 = RT.AimAngle[I1];
	Angle2 = RT.AimAngle[I2];

	Delta = (Range - (I1 * RANGETABLE_INCREMENT)) / RANGETABLE_INCREMENT;

	Angle = (Angle2 - Angle1) * Delta + Angle1;

	if (Angle > 0.0000001f || Angle < 0.0000001f)
	{
		*ReturnAngle = DEG_TO_RAD(Angle);

		return TRUE;
	}
	else
	{
		return FALSE;
	}


	return TRUE;
}

/************************************************************************************************
	_ReadLadder
*************************************************************************************************/

static LOGICAL _ReadLadder(GSCommandList * GSCL,SINT8 * Params)
{
	UCHAR* Ptr1,*Ptr2;
	GSLadderInfo ladder;

	memset(&ladder, 0x0, sizeof(ladder));

	mlUtilityGetVarnameAndContents1((UCHAR**)&Ptr1,(UCHAR**)&Ptr2,(UCHAR*)Params);
	if (Ptr1 == nullptr || Ptr2 == nullptr || Ptr1[0] == '\0') return FALSE;

	// dir
	ladder.dir = (Ptr1[strlen((char*)Ptr1) - 1] == '-') ? -1 : 1;
	Ptr1[strlen((char*)Ptr1) - 1] = '\0';

	// type
	mlUtilityFindVarname(&ladder.type, Ptr1, (UCHAR*)&_LadderTypeNames[0][0], 32, GL_LADDER_TYPE_MAX);

	mlUtilityGetVarnameAndContents1((UCHAR**)&Ptr1,(UCHAR**)&Ptr2,(UCHAR*)Ptr2);
	if (Ptr2 == nullptr) return FALSE;

	// input
	mlUtilityFindVarname(&ladder.input, Ptr1, (UCHAR*)&_LadderInputNames[0][0], 32, GS_LADDER_INPUT_MAX);

	mlUtilityGetVarnameAndContents1((UCHAR**)&Ptr1,(UCHAR**)&Ptr2,(UCHAR*)Ptr2);
	if (Ptr2 == nullptr) return FALSE;

	// min
	ladder.inputMin = mathUnitAsciiToMetric((char*)Ptr1);

	mlUtilityGetVarnameAndContents1((UCHAR**)&Ptr1,(UCHAR**)&Ptr2,(UCHAR*)Ptr2);
	if (Ptr2 == nullptr) return FALSE;

	// max
	ladder.inputMax = mathUnitAsciiToMetric((char*)Ptr1);

	// len
	ladder.len = (FLOAT32)atof((char*)Ptr2);

	GSCL->GE[GSCL->GECounter].Data = memAlloc(sizeof(ladder));

	if (GSCL->GE[GSCL->GECounter].Data == nullptr)
	{
		Write_Debug("Memory Allocation Error!");
		return FALSE;
	}

	memcpy(GSCL->GE[GSCL->GECounter].Data, &ladder, sizeof(ladder));

	return TRUE;
}

/************************************************************************************************
	_SetLadder
*************************************************************************************************/

static LOGICAL _SetLadder(void* Data)
{
	GSLadderInfo* ladder = (GSLadderInfo*)Data;
	FLOAT32 inputVal = 0.0f;
	FLOAT32* val;

	if(ladder == nullptr) return FALSE;

	switch(ladder->input)
	{
		case GS_LADDER_INPUT_BOMBSIGHT_ALT:
			inputVal = persBombsightAltitude();
		break;

		case GS_LADDER_INPUT_BOMBSIGHT_SPEED:
			inputVal = persBombsightSpeed();
		break;
	}

	_ladderOffsetX = _ladderOffsetY = 0.0f;

	val = (ladder->type == GS_LADDER_HORIZONTAL) ? &_ladderOffsetX : &_ladderOffsetY;

	*val = (inputVal - ladder->inputMin) * ladder->len / (ladder->inputMax - ladder->inputMin);

	if(*val < 0.0f) *val = 0.0f;
	if(*val > ladder->len) *val = ladder->len;

	*val *= ladder->dir;

	return TRUE;
}

/************************************************************************************************
	_KillLadder
*************************************************************************************************/

static LOGICAL _KillLadder(void*  Data)
{
	if(Data)
		memFree(Data);

	return TRUE;
}

/************************************************************************************************
	_DisplayBearingLadder
*************************************************************************************************/

static LOGICAL _DisplayBearingLadder(void* Data)
{
	FLOAT32 verticalPos = *(FLOAT32*)&Data;
	piVECTOR V1,V2;
	FLOAT32 relBearing, bearingStartOffset;
	SINT32 TickDegree;
	static char str[256];
	int i;

	V1.z = V2.z = 1000.0f;
	V1.x = -300; // tie to fov
	V1.y = verticalPos;
	V2.x = 300; // tie to fov
	V2.y = verticalPos;

	_draw3dLine(&V1,&V2,colorBLACKALPHA);

	relBearing = RAD_TO_DEG(_GetCurrentViewBearing());
	bearingStartOffset = relBearing - ((int)relBearing / 10) * 10;
	TickDegree = ((int)relBearing / 10) * 10 - 30;

	V1.x = V2.x = DEG_TO_MIL(-(30 + bearingStartOffset));
	V2.y = verticalPos;

	for(i = 0; i <= 60; i++) // tie 30/60 to FOV
	{
		if(i % 10 == 0)
			V1.y = verticalPos + 20;
		else if (i % 5 == 0)
			V1.y = verticalPos + 10;
		else
			V1.y = verticalPos + 5;

		_draw3dLine(&V1,&V2,colorBLACKALPHA);

		if(i % 10 == 0)
		{
			snprintf(str, 255, "%d", (TickDegree < 0) ? -TickDegree : TickDegree);
			_DrawGunsightText(V1.x, verticalPos - 10, 10, 0, uiUTF8toUTF16(str));
		}

		V1.x += DEG_TO_MIL(1);
		V2.x += DEG_TO_MIL(1);
		TickDegree++;
	}

	return TRUE;
}

/************************************************************************************************
	_DisplayTorpedoSolution
*************************************************************************************************/

static LOGICAL _DisplayTorpedoSolution(void* Data)
{
	FLOAT32 verticalPos = *(FLOAT32*)&Data;
	piVECTOR V1,V2;
	FLOAT32 FireBearing;
	FLOAT32 TargetSpeed, TargetRange, TargetAngleOnBow;
	FLOAT32 TorpSpeed, TorpDepth;
	weapAmmoType* AmmoType = nullptr;

	TargetSpeed = _AdjustSpeed * GUNGISHT_MAX_TARGET_SPEED;
	TargetRange = _AdjustGunsight * GUNGISHT_MAX_TARGET_RANGE;
	TargetAngleOnBow = _AdjustAngle * PI;
	TorpDepth =  _AdjustTorpDepth * (GUNSIGHT_MAX_TORP_DEPTH - 1) + 1;

	V1.z = V2.z = 1000.0f;
	V1.x = -300; // tie to fov
	V1.y = verticalPos;
	V2.x = 300; // tie to fov
	V2.y = verticalPos;
	_draw3dLine(&V1, &V2, colorBLACKALPHA);

	if(_CalcSolution)
	{
		weapFindAmmoTypeByName(&AmmoType,(UCHAR*)_AmmoTypeName);
		TorpSpeed = (AmmoType) ? AmmoType->TorpedoRunSpeed : 20.6f; // 40 kts just default for now

		if(!_ComputeFireControlSolution(&_LeadAngle, &_TravelTime, _TargetBearing, TargetAngleOnBow,
											TargetRange, TargetSpeed, TorpSpeed))
		{
			_TravelTime = -1;
		}

		_CalcSolution = FALSE;
	}

	if(_TargetBearing > 0)
		FireBearing = _TargetBearing + _LeadAngle;
	else
		FireBearing = _TargetBearing - _LeadAngle;

	mlSetTorpedoReadout(_TargetBearing, TargetAngleOnBow, TargetRange, TargetSpeed, TorpDepth,
						FireBearing, _LeadAngle, _TravelTime, FALSE);

	// draw firing bearing
	if(_TravelTime > 0)
	{
		V1.x = V2.x = RAD_TO_MIL(FireBearing - _GetCurrentViewBearing());
		V1.y = -100;
		V2.y = 100;
		_draw3dLine(&V1, &V2, colorBLACKALPHA);


		// draw lead angle
		V1.x = V2.x = -RAD_TO_MIL(_LeadAngle);
		V1.y = -100;
		V2.y = 100;
		_draw3dLine(&V1, &V2, colorBLACKALPHA);
	}

	return TRUE;
}

/************************************************************************************************
	_ComputeFireControlSolution
*************************************************************************************************/

static LOGICAL _ComputeFireControlSolution(FLOAT32* LeadAngle, FLOAT32* TravelTime, FLOAT32 TargetRelBearing, FLOAT32 TargetAngleOnBow,
											FLOAT32 TargetRange, FLOAT32 TargetSpeed, FLOAT32 ProjSpeed)
{
	LOGICAL ValidSolution = FALSE;
	FLOAT32 Side1, Side2;
	FLOAT32 CompAngle;
	FLOAT32 RunTime1 = 0, RunTime2 = 0, RunTime = -1;
	FLOAT32 a, b, c, d, sqrtd; // for quadratic equation
	FLOAT32 Cos, Sin;

	CompAngle = (FLOAT32)(PI - fabs(TargetRelBearing) - fabs(TargetAngleOnBow));
	Sin = (FLOAT32)sin(CompAngle);
	Cos = (FLOAT32)cos(CompAngle);

	Side1 = (FLOAT32)fabs(TargetRange * (FLOAT32)sin(TargetAngleOnBow) / Sin);
	Side2 = (FLOAT32)fabs(TargetRange * (FLOAT32)sin(TargetRelBearing) / Sin);

	a = (ProjSpeed * ProjSpeed - TargetSpeed * TargetSpeed);
	b = 2 * TargetSpeed * (Side2 - Side1 * Cos);
	c = -TargetRange * TargetRange;
	d = b * b - 4 * a * c;
	sqrtd = (FLOAT32)sqrt(d);

	// gotta pick the correct solution no negatives and shortest time
	if(d > 0) // no solution if negative
	{
		RunTime1 = (FLOAT32)(-b - sqrtd) / (2 * a);
		RunTime2 = (FLOAT32)(-b + sqrtd) / (2 * a);
	}

	if(RunTime1 < 0)
		RunTime = RunTime2;
	else if(RunTime2 < 0)
		RunTime = RunTime1;
	else if(RunTime1 < RunTime2)
		RunTime = RunTime1;
	else
		RunTime = RunTime2;

	if(RunTime > 0)
	{
		*LeadAngle = (FLOAT32)fabs(TargetRelBearing) - (FLOAT32)asin(sin(CompAngle) * (Side2 - TargetSpeed * RunTime) / (ProjSpeed * RunTime));
		*TravelTime = RunTime;
		ValidSolution = TRUE;

		// figure out whether to lead or lag
		if(TargetRelBearing * TargetAngleOnBow < 0)
			*LeadAngle = -*LeadAngle;
	}

	return ValidSolution;
}


/************************************************************************************************
	persComputeRangeTable
*************************************************************************************************/

LOGICAL persComputeRangeTable(persGunsightRangeTable* RT, weapAmmoType* AmmoType, FLOAT64 MaxRange)
{
	FLOAT64 Mach,BaseMach;
	FLOAT64 Distance,LastDistance;
	FLOAT64 Time,LastTime;
	FLOAT64 Speed;
	FLOAT64 TempNum;
	SINT32 Index1,Index2;
	FLOAT64 Cd;
	FLOAT64 Delta;
	FLOAT64 Density;
	FLOAT64 NextRange;
	FLOAT64VECTOR Accel;
	FLOAT64VECTOR TempVec2;
	FLOAT64 DeltaTime;
	FLOAT64VECTOR LastPosition;
	weapAmmo Ammo;
	SINT32 i;

	DeltaTime = TIME_INCREMENT;

	mlGetAltSpeedOfSound(&BaseMach,0);
	mlGetAltDensity(&Density,0);

	Distance = LastDistance = 0.0;
	Time = LastTime = 0.0;

	memset(&Ammo,0,sizeof(weapAmmo));
	Ammo.Velocity.y = AmmoType->InitVelocity;

	NextRange = 0.0;
	i = 0;

	while (NextRange <= MaxRange)
	{
		LastDistance = Distance;
		LastTime = Time;

		Time += TIME_INCREMENT;

		Speed = mathExactVectorLength(&(Ammo.Velocity));
		Mach = Speed / BaseMach;

		Index1 = (UINT32)(floor(Mach / G1_CHART_MACH_DELTA));
		Index2 = Index1+1;

		if (Index2 >= G1_CHART_NUM_DIVISIONS)
		{
			Cd = G1MachChart[G1_CHART_NUM_DIVISIONS - 1];
		}
		else
		{
			Delta = (Mach / G1_CHART_MACH_DELTA) - Index1;

			mlInterpolate(&Cd,G1MachChart[Index1],G1MachChart[Index2],Delta);
		}

	//	TempNum = Density / Density0 * 0.5 * Cd / Ammo->AmmoType->BallisticCoefficient;
		// NOTE: the 1790 is a constant that makes the Cd table work with
		// G1 ballistic coefficients and this particular formula
		TempNum = Density * Cd * Speed / (AmmoType->BallisticCoefficient * 1790.0f);
		Accel.x = TempNum * Ammo.Velocity.x;
		Accel.y = TempNum * Ammo.Velocity.y;
		Accel.z = TempNum * Ammo.Velocity.z;

		TempVec2.x = (Accel.x + Ammo.LastAcc.x) * 0.5;
		TempVec2.y = (Accel.y + Ammo.LastAcc.y) * 0.5;
		TempVec2.z = (Accel.z + Ammo.LastAcc.z) * 0.5;

		Ammo.Velocity.x -= TempVec2.x * DeltaTime;
		Ammo.Velocity.y -= TempVec2.y * DeltaTime;
		Ammo.Velocity.z -= TempVec2.z * DeltaTime;

		Ammo.LastAcc = Accel;

		// add in gravity
		Ammo.Velocity.z -= 9.8 * DeltaTime;

		LastPosition = Ammo.Position;
		// now apply velocity vector
		if (fabs(Ammo.LastVel.x) > fabs(Ammo.Velocity.x) ||
			fabs(Ammo.LastVel.y) > fabs(Ammo.Velocity.y))
		{
			Ammo.Position.x += Ammo.LastVel.x * DeltaTime + 0.5 * TempVec2.x * DeltaTime * DeltaTime;
			Ammo.Position.y += Ammo.LastVel.y * DeltaTime + 0.5 * TempVec2.y * DeltaTime * DeltaTime;
			Ammo.Position.z += Ammo.LastVel.z * DeltaTime + 0.5 * TempVec2.z * DeltaTime * DeltaTime;
		}
		else
		{
			Ammo.Position.x += Ammo.Velocity.x * DeltaTime;
			Ammo.Position.y += Ammo.Velocity.y * DeltaTime;
			Ammo.Position.z += Ammo.Velocity.z * DeltaTime;
		}

		Ammo.LastVel = Ammo.Velocity;

		Distance = mathExactVectorLength(&Ammo.Position);

		if (Distance > NextRange)
		{
			if (LastDistance < 0.00001)
			{
				RT->TimeToDist[i] = 0.0f;
				RT->ProjectileDrop[i] = 0.0f;
				RT->AimAngle[i] = 0.0f;
			}
			else
			{
				Delta = ((NextRange - LastDistance) / (Distance - LastDistance));

				RT->TimeToDist[i] = (FLOAT32)(((Time - LastTime) * Delta) + LastTime);
				RT->ProjectileDrop[i] = -(FLOAT32)(((Ammo.Position.z - LastPosition.z) * Delta) + LastPosition.z);
				RT->AimAngle[i] = (FLOAT32)RAD_TO_DEG((atan2((FLOAT32)RT->ProjectileDrop[i],(FLOAT32)Distance)));
			}

			i++;

			if(i >= RANGETABLE_NUMELEMENTS)
				break;

			NextRange += RANGETABLE_INCREMENT;
		}
	}

	return TRUE;
}

/************************************************************************************************
	_GetCurrentViewBearing
*************************************************************************************************/

static FLOAT32 _GetCurrentViewBearing()
{
	mlVehicleData* vehData = persGetPersonaModelDataPtr();
	if(!vehData) return 0;

	FLOAT64VECTOR dirVec = {0,1,0};
	FLOAT32 relBearing;

	mathRotateVectorByAtt(&dirVec, &dirVec, &vehData->VehicleRealAtt);
	relBearing = (FLOAT32)acos(mathVectDotProduct(&dirVec, &playdir));
	if(dirVec.y * playdir.x - dirVec.x * playdir.y < 0)
	{
		relBearing = -relBearing;
	}

	return relBearing;
}

/************************************************************************************************
	persGunsightGetRange
*************************************************************************************************/

FLOAT32 persGunsightGetRange(void)
{
	return _StatsRange;
}

static void _drawQuadDeprecated(float u, float v, float scale, bool hasAlpha)
{
    float w = (float)piGetWidth();
    float h = (float)piGetHeight();

    float x = h/w * scale;
    float y = 1.0f * scale;

    glBegin(GL_QUADS);
    {
        glTexCoord2f(u, 0);
        glVertex2f(-x, y);

        glTexCoord2f(0, 0);
        glVertex2f(x, y);

        glTexCoord2f(0, v);
        glVertex2f(x, -y);

        glTexCoord2f(u, v);
        glVertex2f(-x, -y);
    }
    glEnd();

    piDisableTexture();

	// texture has alpha areas in it so lighting was on, kill it
	if(hasAlpha)
		piSetRenderState(piRENDERSTATE_LIGHTING, FALSE);

	oglSetRenderState(oglRENDERSTATE_COLOR4F, piRGBA(0, 0, 0, 1));

    // Draw border
    glBegin(GL_QUADS);
    {
        glTexCoord2f(1, 0);
        glVertex2f(-1, y);

        glTexCoord2f(0, 0);
        glVertex2f(-x, y);

        glTexCoord2f(0, 1);
        glVertex2f(-x, -y);

        glTexCoord2f(1, 1);
        glVertex2f(-1, -y);
    }
    glEnd();

    // Draw border
    glBegin(GL_QUADS);
    {
        glTexCoord2f(1, 0);
        glVertex2f(x, y);

        glTexCoord2f(0, 0);
        glVertex2f(1, y);

        glTexCoord2f(0, 1);
        glVertex2f(1, -y);

        glTexCoord2f(1, 1);
        glVertex2f(x, -y);
    }
    glEnd();
}

static void _drawQuad(const Vec4f& uv, const Vec4f& pos, unsigned int color)
{
    piLVERTEX v[4];
    float u1, v1, u2, v2;

    u1 = uv.x();
    v1 = uv.y();
    u2 = uv.z();
    v2 = uv.w();

    float x1 = pos.x();
    float y1 = pos.y();

    float x2 = pos.z();
    float y2 = pos.w();

	v[0].x = x1;
	v[0].y = y2;
	v[0].z = zdepthLAYER0;
	v[0].color = color;
	v[0].specular = colorALPHA100;
	v[0].tu = u1;
	v[0].tv = v2;

	v[1].x = x2;
	v[1].y = y2;
	v[1].z = zdepthLAYER0;
	v[1].color = color;
	v[1].specular = colorALPHA100;
	v[1].tu = u2;
	v[1].tv = v2;

	v[2].x = x2;
	v[2].y = y1;
	v[2].z = zdepthLAYER0;
	v[2].color = color;
	v[2].specular = colorALPHA100;
	v[2].tu = u2;
	v[2].tv = v1;

	v[3].x = x1;
	v[3].y = y1;
	v[3].z = zdepthLAYER0;
	v[3].color = color;
	v[3].specular = colorALPHA100;
	v[3].tu = u1;
	v[3].tv = v1;

    piINDICES indices[2];

    indices[0].v1 = 0;
    indices[0].v2 = 1;
    indices[0].v3 = 3;
    indices[1].v1 = 1;
    indices[1].v2 = 2;
    indices[1].v3 = 3;

    oglRenderIndexScene(oglPT_TRIANGLELIST,
                        oglVT_LVERTEX,
                        v,
                        4,
                        (unsigned short*)indices,
                        6);
}

#if USE_STATESET
void _drawscope(SINT32 texid, float scale, bool hasAlpha)
{
    static ref_ptr<RenderStateSet> scopess;

    if(ogl::UseDeprecatedPath())
    {
        static TextureUnit* tu = nullptr;

        if(!scopess.valid())
        {
            scopess = new RenderStateSet;
            scopess->SetRenderState(new Color4F(0, 0, 0, 1.0f));
            scopess->SetRenderState(new TextureUnit(0, texid));
            scopess->SetMode(GL_FOG, 0);
            scopess->SetRenderState(new DepthMask(false));
            scopess->SetMode(GL_BLEND, 1);

            tu = dynamic_cast<TextureUnit*>(scopess->GetRenderState(RS_TEXTURE_UNIT0));
        }

        tu->SetTexture(texid);

        Matrixf oldproj(*oglGetProjectionMatrix());
        Matrixf oldworld(*oglGetWorldMatrix());
        Matrixf oldview(*oglGetViewMatrix());

		// if texture has alpha'ed areas in it, lighting needs to be on
		// otherwise we disable it because some scope images aren't fully black
		// and you see borders...probably should fix that in art
		if(hasAlpha)
            scopess->SetMode(GL_LIGHTING, 1);
		else
            scopess->SetMode(GL_LIGHTING, 0);

        oglSetProjectionMatrix(&IdentityMatrix);
        oglSetWorldMatrix(&IdentityMatrix);
        oglSetViewMatrix(&IdentityMatrix);

        scopess->Apply();

        _drawQuadDeprecated(1, 1, scale, hasAlpha);

        oglSetProjectionMatrix(oldproj);
        oglSetWorldMatrix(oldworld);
        oglSetViewMatrix(oldview);
    }
    else
    {
        static ref_ptr<oglMaterial> texmaterial;
        static ref_ptr<oglMaterial> colmaterial;

        if(!texmaterial.valid())
        {
            texmaterial = new oglMaterial;
            texmaterial->SetShader("basictexture");
        }

        if(!colmaterial.valid())
        {
            colmaterial = new oglMaterial;
            colmaterial->SetShader("basiccolor");
        }

        piSetRenderState(piRENDERSTATE_ZWRITEENABLE, FALSE);

        Matrixf id;

        oglSetProjectionMatrix(id);
        oglSetWorldMatrix(id);
        oglSetViewMatrix(id);

        float w = (float)piGetWidth();
        float h = (float)piGetHeight();

        float x = h/w * scale;
        float y = 1.0f * scale;

        texmaterial->AddTexture(0, texid);
        texmaterial->Apply();

        Vec4f uv(0, 0, 1, 1);
        Vec4f pos(-x, -y, x, y);

        _drawQuad(uv, pos, colorWHITEALPHA);

        colmaterial->Apply();

        Vec4f pos2(-1.0, -y, -x, y);
        _drawQuad(uv, pos2, colorBLACKALPHA);

        pos2.set(x, -y, 1.0, y);
        _drawQuad(uv, pos2, colorBLACKALPHA);
    }
}
#else
void _drawscope(SINT32 texid, float scale, bool hasAlpha)
{
    if(ogl::UseDeprecatedPath())
    {
        piDisableMaterial();

	    oglSetRenderState(oglRENDERSTATE_COLOR4F, piRGBA(0, 0, 0, 1.0f));

        piSetActiveTexture(texid, 0);

        UINT32 lighton = piGetEngineRenderState(piRENDERSTATE_LIGHTING);
        UINT32 fogon = piGetEngineRenderState(piRENDERSTATE_FOGENABLE);
        UINT32 zwrite = piGetEngineRenderState(piRENDERSTATE_ZWRITEENABLE);
        UINT32 blendon = piGetEngineRenderState(piRENDERSTATE_ALPHABLENDENABLE);

		// if texture has alpha'ed areas in it, lighting needs to be on
		// otherwise we disable it because some scope images aren't fully black
		// and you see borders...probably should fix that in art
		if(hasAlpha)
			piSetRenderState(piRENDERSTATE_LIGHTING, TRUE);
		else
			piSetRenderState(piRENDERSTATE_LIGHTING, FALSE);

        piSetRenderState(piRENDERSTATE_FOGENABLE, FALSE);
        piSetRenderState(piRENDERSTATE_ZWRITEENABLE, FALSE);
        piSetRenderState(piRENDERSTATE_ALPHABLENDENABLE, TRUE);

        oglSetMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();

        oglSetMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();

        _drawQuadDeprecated(1, 1, scale, hasAlpha);

        glPopMatrix();
        oglSetMatrixMode(GL_PROJECTION);

        glPopMatrix();
        oglSetMatrixMode(GL_MODELVIEW);

        piSetRenderState(piRENDERSTATE_LIGHTING, lighton);
        piSetRenderState(piRENDERSTATE_FOGENABLE, fogon);
        piSetRenderState(piRENDERSTATE_ZWRITEENABLE, zwrite);
        piSetRenderState(piRENDERSTATE_ALPHABLENDENABLE, blendon);

        piSetRenderStageState(0, piTSS_MINFILTER, TextureMinFilter);
        piSetRenderStageState(0, piTSS_MAGFILTER, TextureMagFilter);

        // Default color
	    oglSetRenderState(oglRENDERSTATE_COLOR4F, piRGBA(1, 1, 1, 1));
    }
    else
    {
        static ref_ptr<oglMaterial> texmaterial;
        static ref_ptr<oglMaterial> colmaterial;

        if(!texmaterial.valid())
        {
            texmaterial = new oglMaterial;
            texmaterial->SetShader("basictexture");
        }

        if(!colmaterial.valid())
        {
            colmaterial = new oglMaterial;
            colmaterial->SetShader("basiccolor");
        }

        piSetRenderState(piRENDERSTATE_ZWRITEENABLE, FALSE);

        Matrixf id;

        oglSetProjectionMatrix(id);
        oglSetWorldMatrix(id);
        oglSetViewMatrix(id);

        float w = (float)piGetWidth();
        float h = (float)piGetHeight();

        float x = h/w * scale;
        float y = 1.0f * scale;

        texmaterial->AddTexture(0, texid);
        texmaterial->Apply();

        Vec4f uv(0, 0, 1, 1);
        Vec4f pos(-x, -y, x, y);

        _drawQuad(uv, pos, colorWHITEALPHA);

        colmaterial->Apply();

        Vec4f pos2(-1.0, -y, -x, y);
        _drawQuad(uv, pos2, colorBLACKALPHA);

        pos2.set(x, -y, 1.0, y);
        _drawQuad(uv, pos2, colorBLACKALPHA);
    }
}
#endif

void persGunsightDrawFPSScope(int type)
{
    // Maintain some of this crap to retrieve scope parameters
    if(InstView.GECounter == 0)
    {
        return;
    }

    // Get FieldOfView param
	for(int i = 0; i < InstView.GECounter; i++)
	{
        if(InstView.GE[i].CommandCode)
        {
            GunFileDisplayFunc* f = Lookup[InstView.GE[i].CommandCode].DisplayFunction;

            if(f && f == _DisplayMagLine)
            {
                f(InstView.GE[i].Data);
                break;
            }
        }
	}

    switch((fpsInfantryTypes)type)
    {
    case BRITISH_INFANTRY:
    case BRITISH_HC:
    case BRITISH_ML:
    case BRITISH_PARA_PACK:
    case BRITISH_PARA_NOPACK:
    case BRITISH_PARA_PACK_SELF:
        {
            _drawscope(_britscope, 1.0f);
            break;
        }
    case AMERICAN_INFANTRY:
        {
			if(fpsTrooper.activeWeaponNumber >= 0 && fpsTrooper.weaponPositionModel[fpsTrooper.activeWeaponNumber] == BAZOOKA)
				_drawscope(_bazookascope, 1.0f, true);
			else
				_drawscope(_yankscope, 1.0f);

            break;
        }
    case FRENCH_INFANTRY:
        {
			if(fpsTrooper.activeWeaponNumber >= 0 && fpsTrooper.weaponPositionModel[fpsTrooper.activeWeaponNumber] == BAZOOKA)
				_drawscope(_bazookascope, 1.0f, true);
			else
				_drawscope(_frenchscope, 1.0f);

            break;
        }
    case FRENCH_HC:
    case FRENCH_ML:
    case FRENCH_PARA_PACK:
    case FRENCH_PARA_NOPACK:
    case FRENCH_PARA_PACK_SELF:
        {
            _drawscope(_frenchscope, 1.0f);
            break;
        }
    case GERMAN_INFANTRY:
    case GERMAN_HC:
    case GERMAN_ML:
    case GERMAN_PARA_PACK:
    case GERMAN_PARA_NOPACK:
    case GERMAN_PARA_PACK_SELF:
        {
            _drawscope(_germanscope, 1.0f);
            break;
        }
    default:
        // shouldn't get here
        break;
    }
}

void persGunsightDrawOverlay()
{
    // Maintain some of this crap to retrieve scope parameters
    if(GunSightView.GECounter == 0)
    {
        return;
    }

    // Get FieldOfView param
	for(int i = 0; i < GunSightView.GECounter; i++)
	{
        if(GunSightView.GE[i].CommandCode)
        {
            GunFileDisplayFunc* f = Lookup[GunSightView.GE[i].CommandCode].DisplayFunction;

            if(f && f == _DisplayMagLine)
            {
                f(GunSightView.GE[i].Data);
                break;
            }
        }
	}

    _drawscope(_binocs, 1.0f);
}
