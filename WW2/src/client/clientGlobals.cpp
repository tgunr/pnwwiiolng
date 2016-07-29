
#include "syscfg.h"
#include "pi/pi.h"
#include "include/vars.h"
#include "ddp.h"

SINT32			bZEnable;          // Z buffer is on
LOGICAL          bZWrite;           // Z buffer write enable
UINT32           ZFunction;         // Z compare function
LOGICAL          bMipmapEnable;     // mipmapping?
LOGICAL          bMipmapSoftware;   // mipmap using software?
FLOAT32          MipmapDistance;    // mipmap distance
UINT32           SrcBlend;          // source blend mode
UINT32           DestBlend;         // destination blend mode
LOGICAL          bPerFrameBSP;      // generate BSP sorting tree per frame
SINT32           MinTextureDim;     // minimum texture width/height
UINT32			ShadeMode;         // flat, gouraud, phong?
UINT32			TextureMagFilter;  // texture mag filter
UINT32			TextureMinFilter;  // texture min filter
UINT32			TextureMipFilter;  // texture mipmap filter
UINT32			MaxAnisotropy;     // maximum anisotropic texture filtering
UINT32			FillMode;          // solid, lines or points?
LOGICAL          bDithering;        // dithering is on
LOGICAL          bSpecular;         // specular highlights are on
LOGICAL			MultiTextureEnable;// MultiTexturing

LOGICAL          bFogEnabled;       // fog is on
UINT32           FogColor;          // fog color
UINT32			FogMode;           // linear, exp. etc.
SINT32           FogStart;          // begining depth
SINT32           FogEnd;            // ending depth
SINT32           FogDensity;        // fog density
SINT32           CrossHairs;        // crosshairs type

LOGICAL          bClearViewport;    // clear the viewport each frame
LOGICAL          bClearZBuffer;     // clear the z-buffer each frame
LOGICAL          bShowInfo;         // show window information at the top right
LOGICAL          bResizingDisabled; // do not allow resizing
LOGICAL          bTexturesDisabled; // textures are disabled
LOGICAL          bOnlySystemMemory; // all surfaces forced into system memory
LOGICAL          bOnlyEmulation;    // no hardware devices allowed
LOGICAL          bBackdrop;         // usually need backdrop for foog to look correct
LOGICAL          bSoundEnable;      // sound is enabled
LOGICAL          b3DSoundEnable;    // 3D sound is enabled
FLOAT32			GammaRed;          // gamma for screen
FLOAT32			GammaGreen;        // gamma for screen
FLOAT32			GammaBlue;         // gamma for screen
piVECTOR         playpos;           // player position
piVECTOR         playrot;           // player rotation
piVECTOR         playright;         // player right
piVECTOR         playup;            // player up
piVECTOR         playdir;           // player forward
piVECTOR			playtrans;         // player translation
piVECTOR         deltapos;          // player delta position
piVECTOR         deltarot;          // player delta rotation

FLOAT32			LeftPlaneX;        // Left X plane used for fustrum clipping
FLOAT32			LeftPlaneZ;        // Left Z plane used for fustrum clipping
FLOAT32			TopPlaneY;         // Top Y plane used for fustrum clipping
FLOAT32			TopPlaneZ;         // Top Z plane used for fustrum clipping
FLOAT32			FrontClip;         // Front (near) plane used for fustrum clipping
FLOAT32			BackClip;          // Back (far) plane used for fustrum clipping

SINT32           frametime;         // time (in ticks) of start of present frame
SINT32           lastframetime;     // time (in ticks) of start of last frame
SINT32           ft1000mscount;     // number of 1000ms intervals since start
SINT32           ft1000msticks;     // number of 1000ms intervals since last frame
SINT32           ft500mscount;      // number of 500ms intervals since start
SINT32           ft500msticks;      // number of 500ms intervals since last frame
SINT32           ft200mscount;      // number of 200ms intervals since start
SINT32           ft200msticks;      // number of 200ms intervals since last frame
SINT32           ft100mscount;      // number of 100ms intervals since start
SINT32           ft100msticks;      // number of 100ms intervals since last frame
SINT32           ft50mscount;       // number of 50ms intervals since start
SINT32           ft50msticks;       // number of 50ms intervals since last frame
SINT32           ft10mscount;       // number of 10ms intervals since start
SINT32           ft10msticks;       // number of 10ms intervals since last frame
SINT32           combattime;       // last time a combat event was recorded

UINT32			MaxUserClipPlanes;
UINT32			MaxTextureWidth;
UINT32			MaxTextureHeight;

LOGICAL          WantedHelp;			// asked for commandline help
UINT32			WindowAdapter;		// desired adapter
UINT32			WindowDeviceType;	// desired device type
UINT32			WindowRefreshRate;	// desired refresh rate
UINT32			DefTextureFormat;
UINT32			DefTextureAlphaFormat;

SINT32           WantedX;			// desired/actual x
SINT32           WantedY;			// desired/actual y
SINT32           WantedWidth;		// desired/actual width
SINT32           WantedHeight;		// desired/actual height
SINT32           WantedBpp;			// desired/actual bpp
SINT32          	WantedRefreshRate;
SINT32           WantedAlpha;		// desired/actual alpha
LOGICAL          WantedHdw;			// desired/actual hardware
SINT32           WantedStencil;		// desired/actual hardware
LOGICAL          WantedWindow;		// desired/actual window
SINT32			WantedMode;			// desired/actual mode
SINT32           WantedSide;         // User selected side from webpage.
SINT32			UseStaccato;		// are we using the staccato sound system?
SINT8			TicketFile[_MAX_PATH];
SINT8			GameServerIP[64];	// IP of server to connect to
SINT8			GameServerPort[64];	// Port of server to connect to
SINT8			GameServerOTP[64];	// OTP for connecting to server
SINT8			HackID[128];		// used for hacker detection (comes from playgate only)
LOGICAL			AltFont;            // testing if true use alternate UI font
SINT8			ServerApID[64];		// program apId
SINT8			ServerData[64];		// data passed from auth server
SINT8			ServerProtocol[64]; // connection protocol
SINT8			BypassString[64];
#if defined(DEBUG) || defined(ALPHA_RELEASE)
LOGICAL			WantedUserLogEnabled = TRUE;	// If the user wants the log enabled
#else
LOGICAL			WantedUserLogEnabled = FALSE;	// If the user wants the log enabled
#endif
SCHAR			WantedVideoCard[64];// desired video card name
SCHAR			WantedVideoResolution[32];// desired video resolution
SCHAR			WantedSoundCard[64];// desired sound card name

LOGICAL			UserLogEnabled;		// If the user wants the debug log enabled
LOGICAL			LogEnabled;			// If the log output is currently desired

LOGICAL			PlayDemo;			// demo mode
LOGICAL			StaticLighting;		// static lighting model
LOGICAL			DynamicLighting;	// dynamic lighting model

FLOAT32			ScaleFactor;
FLOAT32			InvScaleFactor;
FLOAT32          g_fRadialDensity;   // 0 - 1 Radial Clutter density
FLOAT32          g_fRadialRadius;    // 0 - 1 Radial clutter distance
LOGICAL          g_bRadialWhenFlying;   // Show radial clutter when flying

LOGICAL			Online; 			// network connection made
LOGICAL			InFlight; 			// no where but in the air

LOGICAL			bAnimatedWater;		 // animate water surface with waves

LOGICAL          g_bBackgroundMusic;  // Enable UI background music.
LOGICAL          g_bCursorOffAtSpawn; // Disable cursor at spawn
LOGICAL          g_bVehicleAttached;  // Received vehicle from host
LOGICAL          g_bTreeShader;       // User shaders for SpeedTree
LOGICAL          g_bRadialClutter;    // Radial clutter enabled/disabled
LOGICAL          g_postRenderFilter;  // Want post render filtering
LOGICAL          g_grassShader;       // Want shader for grass
LOGICAL          g_wantNormalMaps;    // Want normal maps enabled
LOGICAL          g_wantWaterReflections; // Water reflections
LOGICAL          wantWaterRefractions; // Water refractions
UINT32           ShadowMapFlags;    // bitfield for which shadow receivers to use
LOGICAL          g_wantMessageAlert;   // Annoying UI message alert sounds
LOGICAL          g_wantSSE2;          // Run SSE2 exe
LOGICAL          WantBloodEffects;       // Blood hit effects
LOGICAL          WantSpecularMaps;    // Specular mapping
LOGICAL          WantCombatSmoke;    // emersive smoke from damaged buildings and the like

SINT32           g_updateReflectionRate; // 0 - 5 seconds
SINT32           g_iNumBullets;      // Limit to number of displayable bullets.
SINT32           g_iAmbLevel;        // Frequent = 1; Infrequent = 2; Never = 0;
SINT32           g_iVisiblePlayerLimit; // 1 = low 2 = medium 3 = high
SINT32           g_useNetworkRoute;    // 0, 1, 2 (auto/primary/secondary)

SINT32			g_iFSSASamples; 	// 0 = off, 2,4,6,8 (Mac OS X only currently)
LOGICAL			g_bFSSAHighQuality; // enable high quality FSAA hinting (Mac OS X only currently) XXXX

LOGICAL			g_bSoundHighQuality;  // enable high quality 3D audio render (Mac OS X only currently)
LOGICAL			g_bSoundForceStereo;  // force to stereo output regardless of attached sound devices (Mac OS X only currently)
LOGICAL			g_bSoundConvertOnLoad;// convert audio data to best format when loaded, costs memory but saves CPU (Mac OS X only currently)

LOGICAL			g_bCaptuerAllDisplays;// capture all displays or just the game display (Mac OS X only currently)

SINT8			DataPath[_MAX_PATH];//=PREPATH;
SINT8			UserDataPath[_MAX_PATH];

FLOAT32			FieldOfView;		// FOV in degrees
FLOAT32			HorizontalFieldOfView;

LOGICAL			bVehicleMirror;		// Render vehicle mirror(s)

LOGICAL			bNoFrameRunahead;	// Insure only 2 frame 3d render pipeline
LOGICAL			g_enableVSync;	    // Lock the frame rate to refresh rate

SINT32			ShadowSizeSelect;	// A number representing shadow texture size											// (1-8) means "2^n"
											// (1-8) means "2^n"
											// 0 means "No Shadows"
SINT8			Playarc[_MAX_PATH]; // path the archiver, usually just "playarc"
/**/

SINT32           nErrorCode;         // Set to any user specified error code

SINT32			TexBpp;
SINT32			TexAlpha;

LOGICAL			TextureReduction;	// force texture reduction

UINT32			DamageLevel;

FLOAT32			FrameFactor;		// Based on FPS, helper ratio
											// used to adjust object visibility when FPS is low
FLOAT32			InvFrameFactor;		// inverse of framefactor

FLOAT32			FogFactor;			// Based on Height above ground, helper ratio
											// used to adjust object visibility when height is low

SINT32			TotalTextureMemory;

FLOAT32			VolumeMultiplier;	// 0.00001 to 1.0
FLOAT32			MusicMultiplier;	// 0.00001 to 1.0

UINT32			HUDVisible;			// HUD mode; 0 = off, 2 = full on
SINT32			FogScaleSelector;	// FogScaleMultiplier selection; 0 = 1.0
UINT32			MaxNumberOfSounds;	// Maximum number of playing sounds at once

UINT32			SupportedSrcBlendModes;
UINT32			SupportedDestBlendModes;

LOGICAL			SuppressMuzzleSmoke;		// Suppresses smoke from muzzle fire
LOGICAL          SuppressMuzzleLight;        // Suppresses muzzle light
LOGICAL			SuppressTracerSmoke;		// Suppresses smoke from tracers
LOGICAL			SuppressSmallClutter;		// Suppresses creation of objects which are too small for FPS cover
LOGICAL			SuppressPlaneClutter;		// Suppress all clutter if your vehicle is a plane
LOGICAL			LODFieldOfView;				// Evaluates LOD shifts based on FOV
LOGICAL			LODLimitPerFrame;			// Limits number of objects whose LOD is shifted to 'n' per frame

UINT32			gIconFriendlyColor;			// Icon color for friendly players
UINT32			gIconEnemyColor;			// Icon color for enemy players
UINT32			gIconSquadColor;			// Icon color for squad mates
UINT32			gIconMissionColor;          // Icon color for mission mates
UINT32			gIconMissionLeaderColor;    // Icon color for mission leader

UINT32			gLanguage;					// Language used for localization

SINT32 			FoliageMatId;				// Foliage material ID is special; for tree lines

LOGICAL			SupportDXT1;				// DXT1 Opaque / one-bit alpha
LOGICAL			SupportDXT2;				// DXT2 Explicit alpha - Alpha-premultiplied
LOGICAL			SupportDXT3;				// DXT3 Explicit alpha
LOGICAL			SupportDXT4;				// DXT4 Interpolated alpha - Alpha-premultiplied
LOGICAL			SupportDXT5;				// DXT5 Interpolated alpha

piMATRIX			IdentityMatrix;

LOGICAL			VehicleObjectAnalysis;		// "Gizmos and Whizbangs"
LOGICAL			WantDataDir = FALSE;		// Want to run out of data directory
LOGICAL			HasKeyFile;					// Have key file to enable special features
LOGICAL			SuperTroop;					// SuperTroop!
LOGICAL			SuperFloat;					// SuperFloat!
LOGICAL			NoBlitzWorPreload;			// don't preload blitz.wor
LOGICAL			NoInfPreload;				// don't preload infantry
LOGICAL          g_bBuildMDLS;               // Build MDLS for archives.
LOGICAL          g_buildTerrain;             // Build SDMs for archives.

LOGICAL          g_log_comm;					// turn on logging for LOG_COMM
LOGICAL          g_log_fps;					// turn on logging for LOG_FPS
LOGICAL          g_log_weap;					// turn on logging for LOG_WEAP

FLOAT32			gFPS;
FLOAT32			gLastFrameFPS;


LOGICAL			gCellClientActive;
LOGICAL          gWaterShadersEnabled;

LOGICAL			disableButtonInput, disableAxisInput;
#if defined(DEBUG) || defined(ALPHA_RELEASE)
LOGICAL			OpenConsole = TRUE;
#else
LOGICAL			OpenConsole = FALSE;
#endif
LOGICAL			SquadInvites;
UINT32			OverrideSpawner;
UINT32			gRegion; // used for china localizations and other functionality differences
LOGICAL			gRagdolls;
UINT32			gVCache; // client vehicle caching: 0 = off, 1 = troops only, 2 = troops and vehicles (vehicles only on same mission)
LOGICAL			LowMemorySystem;			// user specified low memory
FLOAT32			gTrooperLOD; // LOD scaler for slower systems
LOGICAL          EnableMouseClickToDamageBuildings;
FLOAT32			gTrooperMOD; // LOD scaler mod from preferences
LOGICAL          gCustomTerrain;
UINT32			gEZMode;
UINT32			GameInProgress; // 0 = not running, 1 = running, 2 = quitting

UINT32			Convergence; // Global (default) Airplane gun convergence setting
UINT32			CurrentConv; // Current airplane gun convergence setting

FLOAT32			aimMouse;	// Aiming mouse sensitivity
SINT8            gAuthServer[128]; // Authentication server to be used
SINT8            gArena[8]; // Arena to use, 1 = Live, 22 = Training, 222 = Beta, 250 = Dev
SINT8            gClientV[32]; // Version to submit

#ifdef DEBUG
LOGICAL          DebugBuildingMDLFilesOnly = false;
#endif

#ifdef BETA_RELEASE
	LOGICAL			gCheckPlanarPlacement;
	UINT32			gPersDecalOverride;
	UINT32			gUnitDecalOverride;
	UINT32			gr2DrawFlags;
#endif	// #ifdef BETA_RELEASE
