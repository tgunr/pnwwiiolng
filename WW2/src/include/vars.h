#pragma once

#if platBuild == TEUL_SERVER
static_assert( platBuild != TEUL_SERVER, "Can't include vars.h in a server build." );
#else

#include "pi/pi.h"
#include "include/defs.h"

#undef EXTERN
#ifdef  GLOBALS
	#define EXTERN
#else
	#define EXTERN extern
#endif

#ifdef  GLOBALS
	SINT8 volatile	Copyright[]="Copyright 2012 Playnet, Inc.  All Rights Reserved";
#endif

// These are configuration variables that are filled
// from the .CFG file. Corresponding information
// should be included in CONFIG.C structures.
EXTERN SINT32			bZEnable;          // Z buffer is on
EXTERN LOGICAL          bZWrite;           // Z buffer write enable
EXTERN UINT32           ZFunction;         // Z compare function
EXTERN LOGICAL          bMipmapEnable;     // mipmapping?
EXTERN LOGICAL          bMipmapSoftware;   // mipmap using software?
EXTERN FLOAT32          MipmapDistance;    // mipmap distance
EXTERN UINT32           SrcBlend;          // source blend mode
EXTERN UINT32           DestBlend;         // destination blend mode
EXTERN LOGICAL          bPerFrameBSP;      // generate BSP sorting tree per frame
EXTERN SINT32           MinTextureDim;     // minimum texture width/height
EXTERN UINT32			ShadeMode;         // flat, gouraud, phong?
EXTERN UINT32			TextureMagFilter;  // texture mag filter
EXTERN UINT32			TextureMinFilter;  // texture min filter
EXTERN UINT32			TextureMipFilter;  // texture mipmap filter
EXTERN UINT32			MaxAnisotropy;     // maximum anisotropic texture filtering
EXTERN UINT32			FillMode;          // solid, lines or points?
EXTERN LOGICAL          bDithering;        // dithering is on
EXTERN LOGICAL          bSpecular;         // specular highlights are on
EXTERN LOGICAL			MultiTextureEnable;// MultiTexturing

EXTERN LOGICAL          bFogEnabled;       // fog is on
EXTERN UINT32           FogColor;          // fog color
EXTERN UINT32			FogMode;           // linear, exp. etc.
EXTERN SINT32           FogStart;          // begining depth
EXTERN SINT32           FogEnd;            // ending depth
EXTERN SINT32           FogDensity;        // fog density
EXTERN SINT32           CrossHairs;        // crosshairs type

EXTERN LOGICAL          bClearViewport;    // clear the viewport each frame
EXTERN LOGICAL          bClearZBuffer;     // clear the z-buffer each frame
EXTERN LOGICAL          bShowInfo;         // show window information at the top right
EXTERN LOGICAL          bResizingDisabled; // do not allow resizing
EXTERN LOGICAL          bTexturesDisabled; // textures are disabled
EXTERN LOGICAL          bOnlySystemMemory; // all surfaces forced into system memory
EXTERN LOGICAL          bOnlyEmulation;    // no hardware devices allowed
EXTERN LOGICAL          bBackdrop;         // usually need backdrop for foog to look correct
EXTERN LOGICAL          bSoundEnable;      // sound is enabled
EXTERN LOGICAL          b3DSoundEnable;    // 3D sound is enabled
EXTERN FLOAT32			GammaRed;          // gamma for screen
EXTERN FLOAT32			GammaGreen;        // gamma for screen
EXTERN FLOAT32			GammaBlue;         // gamma for screen
EXTERN piVECTOR         playpos;           // player position
EXTERN piVECTOR         playrot;           // player rotation
EXTERN piVECTOR         playright;         // player right
EXTERN piVECTOR         playup;            // player up
EXTERN piVECTOR         playdir;           // player forward
EXTERN piVECTOR			playtrans;         // player translation
EXTERN piVECTOR         deltapos;          // player delta position
EXTERN piVECTOR         deltarot;          // player delta rotation

EXTERN FLOAT32			LeftPlaneX;        // Left X plane used for fustrum clipping
EXTERN FLOAT32			LeftPlaneZ;        // Left Z plane used for fustrum clipping
EXTERN FLOAT32			TopPlaneY;         // Top Y plane used for fustrum clipping
EXTERN FLOAT32			TopPlaneZ;         // Top Z plane used for fustrum clipping
EXTERN FLOAT32			FrontClip;         // Front (near) plane used for fustrum clipping
EXTERN FLOAT32			BackClip;          // Back (far) plane used for fustrum clipping

EXTERN SINT32           frametime;         // time (in ticks) of start of present frame
EXTERN SINT32           lastframetime;     // time (in ticks) of start of last frame
EXTERN SINT32           ft1000mscount;     // number of 1000ms intervals since start
EXTERN SINT32           ft1000msticks;     // number of 1000ms intervals since last frame
EXTERN SINT32           ft500mscount;      // number of 500ms intervals since start
EXTERN SINT32           ft500msticks;      // number of 500ms intervals since last frame
EXTERN SINT32           ft200mscount;      // number of 200ms intervals since start
EXTERN SINT32           ft200msticks;      // number of 200ms intervals since last frame
EXTERN SINT32           ft100mscount;      // number of 100ms intervals since start
EXTERN SINT32           ft100msticks;      // number of 100ms intervals since last frame
EXTERN SINT32           ft50mscount;       // number of 50ms intervals since start
EXTERN SINT32           ft50msticks;       // number of 50ms intervals since last frame
EXTERN SINT32           ft10mscount;       // number of 10ms intervals since start
EXTERN SINT32           ft10msticks;       // number of 10ms intervals since last frame
EXTERN SINT32           combattime;       // last time a combat event was recorded

EXTERN UINT32			MaxUserClipPlanes;
EXTERN UINT32			MaxTextureWidth;
EXTERN UINT32			MaxTextureHeight;

EXTERN LOGICAL          WantedHelp;			// asked for commandline help
EXTERN UINT32			WindowAdapter;		// desired adapter
EXTERN UINT32			WindowDeviceType;	// desired device type
EXTERN UINT32			WindowRefreshRate;	// desired refresh rate
EXTERN UINT32			DefTextureFormat;
EXTERN UINT32			DefTextureAlphaFormat;

EXTERN SINT32           WantedX;			// desired/actual x
EXTERN SINT32           WantedY;			// desired/actual y
EXTERN SINT32           WantedWidth;		// desired/actual width
EXTERN SINT32           WantedHeight;		// desired/actual height
EXTERN SINT32           WantedBpp;			// desired/actual bpp
EXTERN SINT32          	WantedRefreshRate;
EXTERN SINT32           WantedAlpha;		// desired/actual alpha
EXTERN LOGICAL          WantedHdw;			// desired/actual hardware
EXTERN SINT32           WantedStencil;		// desired/actual hardware
EXTERN LOGICAL          WantedWindow;		// desired/actual window
EXTERN SINT32			WantedMode;			// desired/actual mode
EXTERN SINT32           WantedSide;         // User selected side from webpage.
EXTERN SINT32			UseStaccato;		// are we using the staccato sound system?
EXTERN SINT8			TicketFile[_MAX_PATH];
EXTERN SINT8			GameServerIP[64];	// IP of server to connect to
EXTERN SINT8			GameServerPort[64];	// Port of server to connect to
EXTERN SINT8			GameServerOTP[64];	// OTP for connecting to server
EXTERN SINT8			HackID[128];		// used for hacker detection (comes from playgate only)
EXTERN LOGICAL			AltFont;            // testing if true use alternate UI font
EXTERN SINT8			ServerApID[64];		// program apId
EXTERN SINT8			ServerData[64];		// data passed from auth server
EXTERN SINT8			ServerProtocol[64]; // connection protocol
EXTERN SINT8			BypassString[64];
EXTERN LOGICAL			WantedUserLogEnabled;	// If the user wants the log enabled
EXTERN SCHAR			WantedVideoCard[64];// desired video card name
EXTERN SCHAR			WantedVideoResolution[32];// desired video resolution
EXTERN SCHAR			WantedSoundCard[64];// desired sound card name

EXTERN LOGICAL			UserLogEnabled;		// If the user wants the debug log enabled
EXTERN LOGICAL			LogEnabled;			// If the log output is currently desired

EXTERN LOGICAL			PlayDemo;			// demo mode
EXTERN LOGICAL			StaticLighting;		// static lighting model
EXTERN LOGICAL			DynamicLighting;	// dynamic lighting model

EXTERN FLOAT32			ScaleFactor;
EXTERN FLOAT32			InvScaleFactor;
EXTERN FLOAT32          g_fRadialDensity;   // 0 - 1 Radial Clutter density
EXTERN FLOAT32          g_fRadialRadius;    // 0 - 1 Radial clutter distance
EXTERN LOGICAL          g_bRadialWhenFlying;   // Show radial clutter when flying

EXTERN LOGICAL			Online; 			// network connection made
EXTERN LOGICAL			InFlight; 			// no where but in the air

EXTERN LOGICAL			bAnimatedWater;		 // animate water surface with waves

EXTERN LOGICAL          g_bBackgroundMusic;  // Enable UI background music.
EXTERN LOGICAL          g_bCursorOffAtSpawn; // Disable cursor at spawn
EXTERN LOGICAL          g_bVehicleAttached;  // Received vehicle from host
EXTERN LOGICAL          g_bTreeShader;       // User shaders for SpeedTree
EXTERN LOGICAL          g_bRadialClutter;    // Radial clutter enabled/disabled
EXTERN LOGICAL          g_postRenderFilter;  // Want post render filtering
EXTERN LOGICAL          g_grassShader;       // Want shader for grass
EXTERN LOGICAL          g_wantNormalMaps;    // Want normal maps enabled
EXTERN LOGICAL          g_wantWaterReflections; // Water reflections
EXTERN LOGICAL          wantWaterRefractions; // Water refractions
EXTERN UINT32           ShadowMapFlags;    // bitfield for which shadow receivers to use
EXTERN LOGICAL          g_wantMessageAlert;   // Annoying UI message alert sounds
EXTERN LOGICAL          g_wantSSE2;          // Run SSE2 exe
EXTERN LOGICAL          WantBloodEffects;       // Blood hit effects
EXTERN LOGICAL          WantSpecularMaps;    // Specular mapping
EXTERN LOGICAL          WantCombatSmoke;    // emersive smoke from damaged buildings and the like

EXTERN SINT32           g_updateReflectionRate; // 0 - 5 seconds
EXTERN SINT32           g_iNumBullets;      // Limit to number of displayable bullets.
EXTERN SINT32           g_iAmbLevel;        // Frequent = 1; Infrequent = 2; Never = 0;
EXTERN SINT32           g_iVisiblePlayerLimit; // 1 = low 2 = medium 3 = high
EXTERN SINT32           g_useNetworkRoute;    // 0, 1, 2 (auto/primary/secondary)

EXTERN SINT32			g_iFSSASamples; 	// 0 = off, 2,4,6,8 (Mac OS X only currently)
EXTERN LOGICAL			g_bFSSAHighQuality; // enable high quality FSAA hinting (Mac OS X only currently) XXXX

EXTERN LOGICAL			g_bSoundHighQuality;  // enable high quality 3D audio render (Mac OS X only currently)
EXTERN LOGICAL			g_bSoundForceStereo;  // force to stereo output regardless of attached sound devices (Mac OS X only currently)
EXTERN LOGICAL			g_bSoundConvertOnLoad;// convert audio data to best format when loaded, costs memory but saves CPU (Mac OS X only currently)

EXTERN LOGICAL			g_bCaptuerAllDisplays;// capture all displays or just the game display (Mac OS X only currently)

#ifdef GLOBALS
#if platOS == OS_WIN32
EXTERN SINT8			DataPath[_MAX_PATH]=PREPATH;
#else
EXTERN SINT8			DataPath[_MAX_PATH];//=PREPATH;
#endif
#else
extern SINT8			DataPath[_MAX_PATH];
#endif
EXTERN SINT8			UserDataPath[_MAX_PATH];

EXTERN FLOAT32			FieldOfView;		// FOV in degrees
EXTERN FLOAT32			HorizontalFieldOfView;

EXTERN LOGICAL			bVehicleMirror;		// Render vehicle mirror(s)

EXTERN LOGICAL			bNoFrameRunahead;	// Insure only 2 frame 3d render pipeline
EXTERN LOGICAL			g_enableVSync;	    // Lock the frame rate to refresh rate

EXTERN SINT32			ShadowSizeSelect;	// A number representing shadow texture size											// (1-8) means "2^n"
											// (1-8) means "2^n"
											// 0 means "No Shadows"
EXTERN SINT8			Playarc[_MAX_PATH]; // path the archiver, usually just "playarc"
/**/

EXTERN SINT32           nErrorCode;         // Set to any user specified error code

EXTERN SINT32			TexBpp;
EXTERN SINT32			TexAlpha;

EXTERN LOGICAL			TextureReduction;	// force texture reduction

EXTERN UINT32			DamageLevel;

EXTERN FLOAT32			FrameFactor;		// Based on FPS, helper ratio
											// used to adjust object visibility when FPS is low
EXTERN FLOAT32			InvFrameFactor;		// inverse of framefactor

EXTERN FLOAT32			FogFactor;			// Based on Height above ground, helper ratio
											// used to adjust object visibility when height is low

EXTERN SINT32			TotalTextureMemory;

EXTERN FLOAT32			VolumeMultiplier;	// 0.00001 to 1.0
EXTERN FLOAT32			MusicMultiplier;	// 0.00001 to 1.0

EXTERN UINT32			HUDVisible;			// HUD mode; 0 = off, 2 = full on
EXTERN SINT32			FogScaleSelector;	// FogScaleMultiplier selection; 0 = 1.0
EXTERN UINT32			MaxNumberOfSounds;	// Maximum number of playing sounds at once

EXTERN UINT32			SupportedSrcBlendModes;
EXTERN UINT32			SupportedDestBlendModes;

EXTERN LOGICAL			SuppressMuzzleSmoke;		// Suppresses smoke from muzzle fire
EXTERN LOGICAL          SuppressMuzzleLight;        // Suppresses muzzle light
EXTERN LOGICAL			SuppressTracerSmoke;		// Suppresses smoke from tracers
EXTERN LOGICAL			SuppressSmallClutter;		// Suppresses creation of objects which are too small for FPS cover
EXTERN LOGICAL			SuppressPlaneClutter;		// Suppress all clutter if your vehicle is a plane
EXTERN LOGICAL			LODFieldOfView;				// Evaluates LOD shifts based on FOV
EXTERN LOGICAL			LODLimitPerFrame;			// Limits number of objects whose LOD is shifted to 'n' per frame

EXTERN UINT32			gIconFriendlyColor;			// Icon color for friendly players
EXTERN UINT32			gIconEnemyColor;			// Icon color for enemy players
EXTERN UINT32			gIconSquadColor;			// Icon color for squad mates
EXTERN UINT32			gIconMissionColor;          // Icon color for mission mates
EXTERN UINT32			gIconMissionLeaderColor;    // Icon color for mission leader

EXTERN UINT32			gLanguage;					// Language used for localization

EXTERN SINT32 			FoliageMatId;				// Foliage material ID is special; for tree lines

EXTERN LOGICAL			SupportDXT1;				// DXT1 Opaque / one-bit alpha
EXTERN LOGICAL			SupportDXT2;				// DXT2 Explicit alpha - Alpha-premultiplied
EXTERN LOGICAL			SupportDXT3;				// DXT3 Explicit alpha
EXTERN LOGICAL			SupportDXT4;				// DXT4 Interpolated alpha - Alpha-premultiplied
EXTERN LOGICAL			SupportDXT5;				// DXT5 Interpolated alpha

EXTERN piMATRIX			IdentityMatrix;

EXTERN LOGICAL			VehicleObjectAnalysis;		// "Gizmos and Whizbangs"
EXTERN LOGICAL			WantDataDir;				// Want to run out of data directory
EXTERN LOGICAL			HasKeyFile;					// Have key file to enable special features
EXTERN LOGICAL			SuperTroop;					// SuperTroop!
EXTERN LOGICAL			SuperFloat;					// SuperFloat!
EXTERN LOGICAL			NoBlitzWorPreload;			// don't preload blitz.wor
EXTERN LOGICAL			NoInfPreload;				// don't preload infantry
EXTERN LOGICAL          g_bBuildMDLS;               // Build MDLS for archives.
EXTERN LOGICAL          g_buildTerrain;             // Build SDMs for archives.

EXTERN LOGICAL          g_log_comm;					// turn on logging for LOG_COMM
EXTERN LOGICAL          g_log_fps;					// turn on logging for LOG_FPS
EXTERN LOGICAL          g_log_weap;					// turn on logging for LOG_WEAP

EXTERN FLOAT32			gFPS;
EXTERN FLOAT32			gLastFrameFPS;


EXTERN LOGICAL			gCellClientActive;
EXTERN LOGICAL          gWaterShadersEnabled;

EXTERN LOGICAL			disableButtonInput, disableAxisInput;
EXTERN LOGICAL			OpenConsole;
EXTERN LOGICAL			SquadInvites;
EXTERN UINT32			OverrideSpawner;
EXTERN UINT32			gRegion; // used for china localizations and other functionality differences
EXTERN LOGICAL			gRagdolls;
EXTERN UINT32			gVCache; // client vehicle caching: 0 = off, 1 = troops only, 2 = troops and vehicles (vehicles only on same mission)
EXTERN LOGICAL			LowMemorySystem;			// user specified low memory
EXTERN FLOAT32			gTrooperLOD; // LOD scaler for slower systems
EXTERN LOGICAL          EnableMouseClickToDamageBuildings;
EXTERN FLOAT32			gTrooperMOD; // LOD scaler mod from preferences
EXTERN LOGICAL          gCustomTerrain;
EXTERN UINT32			gEZMode;
EXTERN UINT32			GameInProgress; // 0 = not running, 1 = running, 2 = quitting

EXTERN UINT32			Convergence; // Global (default) Airplane gun convergence setting
EXTERN UINT32			CurrentConv; // Current airplane gun convergence setting

EXTERN FLOAT32			aimMouse;	// Aiming mouse sensitivity

#ifdef BETA_RELEASE
	EXTERN LOGICAL			gCheckPlanarPlacement;
	EXTERN UINT32			gPersDecalOverride;
	EXTERN UINT32			gUnitDecalOverride;
	EXTERN UINT32			gr2DrawFlags;
#endif	// #ifdef BETA_RELEASE

static inline bool IN_COMBAT(SINT32 t) { return (frametime - combattime) <= t; }

namespace WWII
{
	extern const unsigned int svnRevisionNo;
	extern const char* const svnRevisionStr;
}

#endif
