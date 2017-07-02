#include "syscfg.h" // Path is project specific.

/*
	 Copyright 2008 Playnet Inc, All rights reserved
	 Not to be distributed in API form except as part of a compiled library
	 under License or NDA
*/

#include "include/vars.h"
#include "cfg/cfgw2.h"
#include "cfgcodes.h"
#include "cfg/cfgData.h"
#include "ld/ldpix.h"
#include "pi/pi.h"
#include "pi/piinfo.h"
#include "locl/loclApp.h"

static void*	pGlobalVars[cfgNUM_TOTAL_PARAMS]=
{
	&ShadeMode,                 // cfgTEXTURE_SHADE_MODE
	&bDithering,                // cfgTEXTURE_DITHERING
	&bSpecular,                 // cfgTEXTURE_SPECULAR
	&WantedSoundCard,           // cfgSOUND_CARD
	&bSoundEnable,              // cfgSOUND_ENABLED
	&b3DSoundEnable,            // cfgSOUND_3D_ENABLED
	&WantedVideoCard,           // cfgVIDEO_CARD
	&WantedVideoResolution,     // cfgVIDEO_CARD_RESOLUTION
	&bNoFrameRunahead,          // cfgVIDEO_NO_FRAME_RUN_AHEAD
	&g_enableVSync,             // cfgVIDEO_ENABLE_VSYNC
	&GammaRed,                  // cfgCOLOR_GAMMA_RED
	&GammaGreen,                // cfgCOLOR_GAMMA_GREEN
	&GammaBlue,                 // cfgCOLOR_GAMMA_BLUE
	&bShowInfo,                 // cfgSPECIAL_FX_SHOW_INFO
	&ShadowSizeSelect,          // cfgSHADOW_SIZE_SELECT
	&VolumeMultiplier,          // cfgVOLUME_MULTIPLIER
	&MusicMultiplier,           // cfgMUSIC_MULTIPLIER
	&HUDVisible,                // cfgHUD_VISIBLE
	&FogScaleSelector,          // cfgFOG_SCALE_SELECTOR
	&MaxNumberOfSounds,         // cfgMAX_NUMBER_SOUNDS
	&SuppressMuzzleSmoke,       // cfgSUPPRESS_MUZZLE_SMOKE
	&SuppressMuzzleLight,       // cfgSUPPRESS_MUZZLE_LIGHT
	&SuppressTracerSmoke,       // cfgSUPPRESS_TRACER_SMOKE
	&LODFieldOfView,            // cfgLOD_FIELD_OF_VIEW
	&LODLimitPerFrame,          // cfgLOD_LIMIT_PER_FRAME
	&g_bCursorOffAtSpawn,       // cfgCURSOR_ENABLED
	&gIconFriendlyColor,        // cfgFRIENDLY_COLOR
	&gIconEnemyColor,           // cfgENEMY_COLOR
	&gIconSquadColor,           // cfgSQUAD_COLOR
	&gIconMissionColor,         // cfgMISSION_COLOR
	&gIconMissionLeaderColor,   // cfgMISSIONLEADER_COLOR
	&gLanguage,                 // cfgLANGUAGE_CHOICE
	&g_iAmbLevel,               // cfgSOUND_AMBIENT
	&g_bRadialClutter,          // cfgRADIAL_CLUTTER_ENABLE
	&g_fRadialDensity,          // cfgRADIAL_CLUTTER_DENSITY
	&g_fRadialRadius,           // cfgRADIAL_CLUTTER_RADIUS
	&g_iVisiblePlayerLimit,     // cfgVISIBLE_PLAYER_LIMIT
	&g_iFSSASamples,            // cfgFSSA_SAMPLES
	&g_bFSSAHighQuality,        // cfgFSSA_QUALITY
	&g_bSoundHighQuality,       // cfgSOUND_QUALITY
	&g_bSoundForceStereo,       // cfgSOUND_FORCE_STEREO
	&g_bSoundConvertOnLoad,     // cfgSOUND_CONVERT_ON_LOAD
	&g_bCaptuerAllDisplays,     // cfgCAPTURE_ALL_DISPLAYS
	&gWaterShadersEnabled,      // cfgENABLE_H2O_SHADERS
	&g_postRenderFilter,        // cfgPOST_RENDER_FILTER
	&g_grassShader,             // cfgGRASS_SHADER
	&g_wantNormalMaps,          // cfgNORMAL_MAPS
	&g_wantWaterReflections,    // cfgH2O_REFLECTIONS
	&g_updateReflectionRate,    // cfgH2O_H2O_UPDATE
	&g_wantMessageAlert,        // cfgMESSAGE_ALERT_SOUND
	&g_useNetworkRoute,         // cfgNETWORK_ROUTE
	&g_wantSSE2,                // cfgWANT_SSE2
	&wantWaterRefractions,      // cfgH2O_REFRACTIONS
	&WantBloodEffects,          // cfgWANT_BLOODEFFECTS
	&ShadowMapFlags
};

static int ShadeModeMap[cfgNUM_TEXTURE_SHADE_MODE_VALUES]=
{
	piSHADE_FLAT,	 // cfgSHADE_MODE_FLAT
	piSHADE_GOURAUD, // cfgSHADE_MODE_GOURAUD
	piSHADE_PHONG	 // cfgSHADE_MODE_PHONG
};

static int MagFilterMap[cfgNUM_TEXTURE_MAG_FILTER_VALUES]=
{
	piTFG_NONE,		// cfgTEXTURE_MAG_FILTER_NONE
	piTFG_POINT,	// cfgTEXTURE_MAG_FILTER_POINT
	piTFG_LINEAR	// cfgTEXTURE_MAG_FILTER_LINEAR
};

static int MinFilterMap[cfgNUM_TEXTURE_MIN_FILTER_VALUES]=
{
	piTFN_NONE,			// cfgTEXTURE_MIN_FILTER_NONE
	piTFN_POINT,		// cfgTEXTURE_MIN_FILTER_POINT
	piTFN_LINEAR,		// cfgTEXTURE_MIN_FILTER_LINEAR
};

static int MipFilterMap[cfgNUM_TEXTURE_MIP_FILTER_VALUES]=
{
	piTFP_NONE,		// cfgTEXTURE_MIP_FILTER_NONE
	piTFP_POINT,	// cfgTEXTURE_MIP_FILTER_POINT
	piTFP_LINEAR,	// cfgTEXTURE_MIP_FILTER_LINEAR
};

static int* EnumValueMap[cfgNUM_TYPE_VALUES]=
{
	nullptr,				// cfgTYPE_INTEGER
	nullptr,				// cfgTYPE_FLOAT
	nullptr,				// cfgTYPE_STRING
	ShadeModeMap,		// cfgTYPE_TEXTURE_SHADE_MODE,
	MagFilterMap,		// cfgTYPE_TEXTURE_MAG_FILTER,
	MinFilterMap,		// cfgTYPE_TEXTURE_MIN_FILTER,
	MipFilterMap,		// cfgTYPE_TEXTURE_MIP_FILTER,
	nullptr,				// cfgTYPE_TEXTURE_ANTIALIASING,
	nullptr,				// cfgTYPE_SHADOW_SIZE_SELECT,
	nullptr,				// cfgTYPE_TRUE_FALSE,
};

static void ConvertTypes()
{
	for (int i = 0; i < cfgNUM_TOTAL_PARAMS; ++i)
	{
		int Type = cfgParameters[i].Type;
		if (EnumValueMap[Type] != nullptr)
		{
			*(int*)pGlobalVars[i] = EnumValueMap[Type][*(int*)pGlobalVars[i]];
		}
	}
}

#if platOS == OS_WIN32 && defined(DIRECT3D)
#include "dd/dx3dinfo.h"

extern DWORD				numadapters;
extern ADAPTERMODES*		adaptermodes;

int FindVideoDriverIndexFromString(char* Driver)
{
DWORD		i;
const char* guid = nullptr;

	for (i = 0; i < numadapters; ++i)
	{
		guid = piinfoGetDeviceIDAsString(adaptermodes[i].identifier.DeviceIdentifier);

		if (_stricmp(Driver, guid) == 0)
		{
			return i;
		}
	}

	return 0;
}
#endif

static void SetupVariables()
{
	bZEnable            = piZENABLE_DEFAULT;				// Z buffer enable
	bZWrite             = piZWRITEENABLE_DEFAULT;			// Z buffer write enable
	ZFunction           = piZFUNC_DEFAULT;					// Z compare function
	ShadeMode           = piSHADEMODE_DEFAULT;				// flat, gouraud, phong?
	TextureMagFilter    = piTEXTUREMAG_DEFAULT;				// texture mag filter
	TextureMinFilter    = piTEXTUREMIN_DEFAULT;				// texture min filter
	TextureMipFilter	= piTEXTUREMIP_DEFAULT;				// texture mipmap filter
	FillMode            = piFILLMODE_DEFAULT;				// solid, lines or points?
	bDithering          = piDITHERENABLE_DEFAULT;			// dithering is on
	bSpecular           = piSPECULARENABLE;					// specular highlights are on
	SrcBlend            = piSRCBLEND_DEFAULT;				// source blend mode
	DestBlend           = piDESTBLEND_DEFAULT;				// destination blend mode

	bFogEnabled         = piFOGENABLE_DEFAULT;				// fog is on
	FogColor            = piFOGCOLOR_DEFAULT;				// fog color
	FogMode             = piFOGMODE_DEFAULT;				// linear, exp. etc.
	FogStart            = piFOGSTART_DEFAULT;				// begining depth
	FogEnd              = piFOGEND_DEFAULT;					// ending depth
	FogDensity          = piFOGDENSITY_DEFAULT;				// fog density

	bPerFrameBSP        = TRUE;								// generate BSP sorting tree per frame
	bClearViewport      = FALSE;								// clear the viewport each frame
	bClearZBuffer       = TRUE;								// clear the z-buffer each frame
	bMipmapEnable       = TRUE;								// mipmap enable
	bMipmapSoftware     = FALSE;							// mipmap using software
	MipmapDistance      = 500.0f;							// mipmap distance
	MinTextureDim       = 1;								// minimum texture dimensions
	bShowInfo           = FALSE;							// show window information at the top right
	bResizingDisabled   = FALSE;							// do not allow resizing
	bTexturesDisabled   = FALSE;							// textures are disabled
	bOnlySystemMemory   = FALSE;							// all surfaces forced into system memory
	bOnlyEmulation      = FALSE;							// no hardware DD or D3D devices allowed
	bBackdrop           = TRUE;								// usually need backdrop for fog to look correct
	bSoundEnable        = TRUE;								// most of the time, sound is nice
	b3DSoundEnable      = TRUE;   							// all the time, 3D sound is better
	g_wantMessageAlert  = FALSE;

	GammaRed            = 1.0f;								// gamma for screen
	GammaGreen          = 1.0f;								// gamma for screen
	GammaBlue           = 1.0f;								// gamma for screen

	CrossHairs          = piCROSSHAIRS_DEFAULT;				// crosshairs shape

	StaticLighting		= FALSE;
	DynamicLighting		= TRUE;

	ScaleFactor         = 1.0f;

	bAnimatedWater		= TRUE;

	FieldOfView			= 90.0f;

	bVehicleMirror		= FALSE;

	bNoFrameRunahead	= FALSE;

	ShadowSizeSelect	= 6;

	MultiTextureEnable	= TRUE;

	VolumeMultiplier	= 1.0f;
	MusicMultiplier		= 1.0f; // defaults to low volume

	HUDVisible			= 2;
	FogScaleSelector	= 0;
	MaxNumberOfSounds	= 16;
	g_iNumBullets       = 128;

	SuppressMuzzleSmoke		= FALSE;
	SuppressMuzzleLight     = FALSE;
	SuppressTracerSmoke		= FALSE;
	SuppressSmallClutter	= FALSE;
	SuppressPlaneClutter	= FALSE;
	LODFieldOfView			= FALSE;
	LODLimitPerFrame		= FALSE;

	g_iFSSASamples = 0;
	g_bFSSAHighQuality = FALSE;
	g_bSoundHighQuality = TRUE;
	g_bSoundForceStereo = FALSE;
	g_bSoundConvertOnLoad = FALSE;
	g_bCaptuerAllDisplays = TRUE;

	gIconFriendlyColor = colorBLUE;	// DEFAULT_FRIENDLY_COLOR
	gIconEnemyColor = colorRED;		// DEFAULT_ENEMY_COLOR
	gIconSquadColor = colorWHITE;		// DEFAULT_SQUAD_COLOR

	DamageLevel			= 0;

	FrameFactor			= 1.0f;

	InvScaleFactor      = 1.0f / ScaleFactor;

	piSetMinTextureDim(MinTextureDim);
	MipmapDistance = MipmapDistance * MipmapDistance * ScaleFactor;
//	if (bMipmapEnable && !piIsHdwMipmap())
//	{
//		bMipmapSoftware = TRUE;
//	}

	if (ShadowSizeSelect < 0 || ShadowSizeSelect > 8)
	{
		ShadowSizeSelect = 6;
	}

	SquadInvites = TRUE;
	EnableMouseClickToDamageBuildings = FALSE;
}

LOGICAL cfgLoadGlobals(const char* ConfigName)
{
	SCHAR str[16];
	UINT32 Position = 0;
	UINT32 Length = 0;

	SetupVariables();

	if (!cfgLoadW2(ConfigName, pGlobalVars))
	{
		Write_Debug("Config Value Init Failed");
		return FALSE;
	}

	ConvertTypes();

	// These are permanently disabled, should remove completely from code at some point.
	SuppressPlaneClutter = FALSE;
	SuppressSmallClutter = FALSE;

	// Always use multitexture
	MultiTextureEnable = TRUE;

	TextureMagFilter    = piTEXTUREMAG_DEFAULT;				// texture mag filter
	TextureMinFilter    = piTEXTUREMIN_DEFAULT;				// texture min filter
	TextureMipFilter	= piTEXTUREMIP_DEFAULT;				// texture mipmap filter

#if defined(DIRECT3D) && platOS == OS_WIN32
	WindowAdapter = FindVideoDriverIndexFromString(WantedVideoCard);
#endif

	// These are no longer available in settings.exe.
	bMipmapEnable = TRUE;
	bTexturesDisabled = FALSE;
	g_bCursorOffAtSpawn = TRUE;
	g_iNumBullets = 128;

	g_bTreeShader = TRUE;

#if platOS == OS_MACOS
	g_bTreeShader = FALSE;
#endif

	// Load up the resolution stuff
	// Width
	Length = strcspn(WantedVideoResolution, "x");
	strncpy(str, WantedVideoResolution, Length);
	str[Length] = 0;
	Position += Length + 1;
	WantedWidth = atoi(str);

	// Height
	Length = strcspn(WantedVideoResolution + Position, "x");
	strncpy(str, WantedVideoResolution + Position, Length);
	str[Length] = 0;
	Position += Length + 1;
	WantedHeight = atoi(str);

	// Depth
	Length = strcspn(WantedVideoResolution + Position, "x");
	strncpy(str, WantedVideoResolution + Position, Length);
	str[Length] = 0;
	Position += Length + 1;
	WantedBpp = atoi(str);

	// Refresh Rate
	Length = strcspn(WantedVideoResolution + Position, "x");
	if(Length > 0)
	{
		strncpy(str, WantedVideoResolution + Position, Length);
		str[Length] = 0;
		WantedRefreshRate = atoi(str);
	}

	// Translate ShadowSize from key to value
	if ((ShadowSizeSelect < 0) || (ShadowSizeSelect <= cfgNUM_SPECIAL_FX_SHADOW_SIZE_SELECT_VALUES))
	{
		ShadowSizeSelect = ((int **) Values)[cfgTYPE_SHADOW_SIZE_SELECT][ShadowSizeSelect];
	}

	// Translate FSAA samples from key to value
	if ((g_iFSSASamples > 0) && (g_iFSSASamples <= cfgNUM_FSAA_SAMPLE_VALUES))
	{
		g_iFSSASamples = ((int **) Values)[cfgTYPE_FSAA_SAMPLES][g_iFSSASamples];
	}

	if(g_useNetworkRoute < 0)
	{
		g_useNetworkRoute = 0;
	}

#if platOS == OS_MACOS
  MaxNumberOfSounds = 128; // ignore setting since core audio doesn't care
#endif
	if(MaxNumberOfSounds < 16)
		MaxNumberOfSounds = 16;

	return TRUE;
}
