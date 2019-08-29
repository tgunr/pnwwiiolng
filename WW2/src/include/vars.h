#pragma once

#if platBuild == BUILD_SERVER
static_assert( platBuild != BUILD_SERVER, "Can't include vars.h in a server build." );
#else

#include "pi/pi.h"
#include "include/defs.h"
#include "fio/fiostructure.h"

// These are configuration variables that are filled
// from the .CFG file. Corresponding information
// should be included in CONFIG.C structures.
extern SINT32			bZEnable;          // Z buffer is on
extern LOGICAL          bZWrite;           // Z buffer write enable
extern UINT32           ZFunction;         // Z compare function
extern LOGICAL          bMipmapEnable;     // mipmapping?
extern LOGICAL          bMipmapSoftware;   // mipmap using software?
extern FLOAT32          MipmapDistance;    // mipmap distance
extern UINT32           SrcBlend;          // source blend mode
extern UINT32           DestBlend;         // destination blend mode
extern LOGICAL          bPerFrameBSP;      // generate BSP sorting tree per frame
extern SINT32           MinTextureDim;     // minimum texture width/height
extern UINT32			ShadeMode;         // flat, gouraud, phong?
extern UINT32			TextureMagFilter;  // texture mag filter
extern UINT32			TextureMinFilter;  // texture min filter
extern UINT32			TextureMipFilter;  // texture mipmap filter
extern UINT32			MaxAnisotropy;     // maximum anisotropic texture filtering
extern UINT32			FillMode;          // solid, lines or points?
extern LOGICAL          bDithering;        // dithering is on
extern LOGICAL          bSpecular;         // specular highlights are on
extern LOGICAL			MultiTextureEnable;// MultiTexturing

extern LOGICAL          bFogEnabled;       // fog is on
extern UINT32           FogColor;          // fog color
extern UINT32			FogMode;           // linear, exp. etc.
extern SINT32           FogStart;          // begining depth
extern SINT32           FogEnd;            // ending depth
extern SINT32           FogDensity;        // fog density
extern SINT32           CrossHairs;        // crosshairs type

extern LOGICAL          bClearViewport;    // clear the viewport each frame
extern LOGICAL          bClearZBuffer;     // clear the z-buffer each frame
extern LOGICAL          bShowInfo;         // show window information at the top right
extern LOGICAL          bResizingDisabled; // do not allow resizing
extern LOGICAL          bTexturesDisabled; // textures are disabled
extern LOGICAL          bOnlySystemMemory; // all surfaces forced into system memory
extern LOGICAL          bOnlyEmulation;    // no hardware devices allowed
extern LOGICAL          bBackdrop;         // usually need backdrop for foog to look correct
extern LOGICAL          bSoundEnable;      // sound is enabled
extern LOGICAL          b3DSoundEnable;    // 3D sound is enabled
extern FLOAT32			GammaRed;          // gamma for screen
extern FLOAT32			GammaGreen;        // gamma for screen
extern FLOAT32			GammaBlue;         // gamma for screen
extern piVECTOR         playpos;           // player position
extern piVECTOR         playrot;           // player rotation
extern piVECTOR         playright;         // player right
extern piVECTOR         playup;            // player up
extern piVECTOR         playdir;           // player forward
extern piVECTOR			playtrans;         // player translation
extern piVECTOR         deltapos;          // player delta position
extern piVECTOR         deltarot;          // player delta rotation

extern FLOAT32			LeftPlaneX;        // Left X plane used for fustrum clipping
extern FLOAT32			LeftPlaneZ;        // Left Z plane used for fustrum clipping
extern FLOAT32			TopPlaneY;         // Top Y plane used for fustrum clipping
extern FLOAT32			TopPlaneZ;         // Top Z plane used for fustrum clipping
extern FLOAT32			FrontClip;         // Front (near) plane used for fustrum clipping
extern FLOAT32			BackClip;          // Back (far) plane used for fustrum clipping

extern SINT32           frametime;         // time (in ticks) of start of present frame
extern SINT32           lastframetime;     // time (in ticks) of start of last frame
extern SINT32           ft1000mscount;     // number of 1000ms intervals since start
extern SINT32           ft1000msticks;     // number of 1000ms intervals since last frame
extern SINT32           ft500mscount;      // number of 500ms intervals since start
extern SINT32           ft500msticks;      // number of 500ms intervals since last frame
extern SINT32           ft200mscount;      // number of 200ms intervals since start
extern SINT32           ft200msticks;      // number of 200ms intervals since last frame
extern SINT32           ft100mscount;      // number of 100ms intervals since start
extern SINT32           ft100msticks;      // number of 100ms intervals since last frame
extern SINT32           ft50mscount;       // number of 50ms intervals since start
extern SINT32           ft50msticks;       // number of 50ms intervals since last frame
extern SINT32           ft10mscount;       // number of 10ms intervals since start
extern SINT32           ft10msticks;       // number of 10ms intervals since last frame
extern SINT32           combattime;       // last time a combat event was recorded

extern UINT32			MaxUserClipPlanes;
extern UINT32			MaxTextureWidth;
extern UINT32			MaxTextureHeight;

extern UINT32			WindowAdapter;		// desired adapter
extern UINT32			WindowDeviceType;	// desired device type
extern UINT32			WindowRefreshRate;	// desired refresh rate
extern UINT32			DefTextureFormat;
extern UINT32			DefTextureAlphaFormat;

extern SINT32           WantedX;			// desired/actual x
extern SINT32           WantedY;			// desired/actual y
extern SINT32           WantedWidth;		// desired/actual width
extern SINT32           WantedHeight;		// desired/actual height
extern SINT32           WantedBpp;			// desired/actual bpp
extern SINT32          	WantedRefreshRate;
extern SINT32           WantedAlpha;		// desired/actual alpha
extern LOGICAL          WantedHdw;			// desired/actual hardware
extern SINT32           WantedStencil;		// desired/actual hardware
extern LOGICAL          WantedFullscreen;	// desired/actual window
extern LOGICAL          WantedBorderless;	// desired/actual window
extern LOGICAL          WantedWindow;		// desired/actual window
extern SINT32			WantedMode;			// desired/actual mode
extern SINT32           WantedSide;         // User selected side from webpage.
extern SINT32			UseStaccato;		// are we using the staccato sound system?
extern SINT8			TicketFile[_MAX_PATH];
extern SINT8			GameServerIP[64];	// IP of server to connect to
extern SINT8			GameServerPort[64];	// Port of server to connect to
extern SINT8			GameServerOTP[64];	// OTP for connecting to server
extern SINT8			HackID[128];		// used for hacker detection (comes from playgate only)
extern LOGICAL			AltFont;            // testing if true use alternate UI font
extern SINT8			ServerApID[64];		// program apId
extern SINT8			ServerData[64];		// data passed from auth server
extern SINT8			BypassString[64];
extern LOGICAL          LaunchSettingsOnly; // stand-alone settings (for mac initially)
extern LOGICAL			UseSteamLogin;
extern LOGICAL			WantedUserLogEnabled;	// If the user wants the log enabled
extern SCHAR			WantedDisplayDevice[64];// desired video card name
extern SCHAR			WantedVideoResolution[32];// desired video resolution
extern SCHAR			WantedSoundCard[64];// desired sound card name

extern LOGICAL			UserLogEnabled;		// If the user wants the debug log enabled
extern LOGICAL			LogEnabled;			// If the log output is currently desired

extern LOGICAL			PlayDemo;			// demo mode
extern LOGICAL			StaticLighting;		// static lighting model
extern LOGICAL			DynamicLighting;	// dynamic lighting model

extern FLOAT32			ScaleFactor;
extern FLOAT32			InvScaleFactor;
extern FLOAT32          g_fRadialDensity;   // 0 - 1 Radial Clutter density
extern FLOAT32          g_fRadialRadius;    // 0 - 1 Radial clutter distance
extern LOGICAL          g_bRadialWhenFlying;   // Show radial clutter when flying

extern LOGICAL			Online; 			// network connection made
extern LOGICAL			InFlight; 			// no where but in the air

extern LOGICAL			bAnimatedWater;		 // animate water surface with waves

extern LOGICAL          g_bBackgroundMusic;  // Enable UI background music.
extern LOGICAL          g_bCursorOffAtSpawn; // Disable cursor at spawn
extern LOGICAL          g_bVehicleAttached;  // Received vehicle from host
extern LOGICAL          g_bTreeShader;       // User shaders for SpeedTree
extern LOGICAL          g_bRadialClutter;    // Radial clutter enabled/disabled
extern LOGICAL          g_postRenderFilter;  // Want post render filtering
extern LOGICAL          g_grassShader;       // Want shader for grass
extern LOGICAL          g_wantNormalMaps;    // Want normal maps enabled
extern LOGICAL          g_wantWaterReflections; // Water reflections
extern LOGICAL          wantWaterRefractions; // Water refractions
extern UINT32           ShadowMapFlags;    // bitfield for which shadow receivers to use
extern LOGICAL          g_wantMessageAlert;   // Annoying UI message alert sounds
extern LOGICAL          g_wantSSE2;          // Run SSE2 exe
extern LOGICAL          WantBloodEffects;       // Blood hit effects
extern LOGICAL          WantSpecularMaps;    // Specular mapping
extern LOGICAL          WantCombatSmoke;    // emersive smoke from damaged buildings and the like

extern SINT32           g_updateReflectionRate; // 0 - 5 seconds
extern SINT32           g_iNumBullets;      // Limit to number of displayable bullets.
extern SINT32           g_iAmbLevel;        // Frequent = 1; Infrequent = 2; Never = 0;
extern SINT32           g_iVisiblePlayerLimit; // 0 = very low, 1 = low, 2 = medium, 3 = high, 4 = very high
extern SINT32           g_useNetworkRoute;    // 0, 1, 2 (auto/primary/secondary)
extern LOGICAL          g_wantNetcode;        // true = netcode X enabled

extern SINT32			g_iFSSASamples; 	// 0 = off, 2,4,6,8 (Mac OS X only currently)
extern LOGICAL			g_bFSSAHighQuality; // enable high quality FSAA hinting (Mac OS X only currently) XXXX

extern LOGICAL			g_bSoundHighQuality;  // enable high quality 3D audio render (Mac OS X only currently)
extern LOGICAL			g_bSoundForceStereo;  // force to stereo output regardless of attached sound devices (Mac OS X only currently)
extern LOGICAL			g_bSoundConvertOnLoad;// convert audio data to best format when loaded, costs memory but saves CPU (Mac OS X only currently)

extern LOGICAL			g_bCaptuerAllDisplays;// capture all displays or just the game display (Mac OS X only currently)

//  MJH =====
extern LOGICAL				WantedCopyOldSetup;			//  DEF = TRUE -- Look for and copy the old BGE setup files (IF FOUND) to the New WWIIOnline folder.
extern wwiiolpath_string	WantedDataPath;				//  DEF = L"" -- SET by the command line - ENABLED by WantDataDir = True < OR > is a Nul String L"".
extern wwiiolpath_string	WWIIOnlineDirName;			//  DEF = L"WWIIOnline" -- The ROOT folder for the game install - .\WWIIOnline

extern wwiiolpath_string	WW2_exePath;				//	Windows - EXE ROOT folder:  \Program * Files -> CRS -> WWIIOnline == *.exe, WWIIOnline.ico, readme.*, unins*.* (uninstaller), *.tkt, *.script, bin(dir), DATA(dir), docs(dir), playarc(dir).
														//	macOS - EXE ROOT folder:  /Applications -> World War II Online.app == Contents -> _CodeSignature(dir), Frameworks(dir), MacOS(dir -> exefile), Resources(dir) info.plist, PkgInfo.
extern wwiiolpath_string	WW2_exeDataPath;			//	EXE Support Data folder:  \Program * Files -> CRS  -> WWIIOnline -> DATA == DirectXRedist(dir), loc(dir), playgate(dir), RuntimeLibraries(dir). 
extern wwiiolpath_string	WW2_exeDocsPath;			//	Windows - EXE Docs Folder:  \Program * Files -> CRS  -> WWIIOnline -> docs == eula.txt, license.*, regions.txt, tos.txt.
														//	macOS - EXE Docs Folder:  /Applications -> World War II Online.app -> Contents -> Resources -> docs == eula.txt, license.*, regions.txt, tos.txt.
extern wwiiolpath_string	WW2_exePlayarcPath;			//	Windoes - EXE playarc folder:  \Program * Files -> CRS  -> WWIIOnline -> playarc == playarc??.ww2  --OR--  == "-data" command line switch.
														//	macOS - EXE playarc folder:  /Applications -> World War II Online.app -> Contents -> Resources == playarc.ww2.
extern wwiiolpath_string	WW2_exeWorkingPath;			//	Windows - EXE Working folder:  \ProgramData -> WWIIOnline == ftplog.txt, downloaded patches, WWIIOL.log, *.log.
														//	macOS - EXE Working folder:  /Users -> "currentuser" -> Library -> Application Support -> com.corneredrats.wwiionline == ftplog.txt, downloaded patches, WWIIOL.log, *.log.
extern wwiiolpath_string	WW2_userDocumentsPath;		//	Windows - USER Config ROOT folder:  \User -> My Documents -> WWIIOnline == wwiiol.xml, cfml(dir).
														//	macOS - USER Config ROOT folder:  /Users -> "currentuser" -> Documents -> WWIIOnline == wwiiol.xml, cfml(dir).
extern wwiiolpath_string	WW2_userScreenShotsPath;	//	Windows - USER Config ROOT folder:  \User -> My Documents -> WWIIOnline -> screenshots == SShot*.bmp.
														//	macOS - USER Config ROOT folder:  /Users -> "currentuser" -> Documents -> WWIIOnline -> screenshots == SShot*.bmp.
extern wwiiolpath_string	WW2_userWorkingPath;		//	Windows - USER Working folder:  \User -> My Documents -> WWIIOnline -> bin == lg.dat.
														//	macOS - USER Working folder:  /Users -> "currentuser" -> Documents -> WWIIOnline -> bin == lg.dat.
//  =========

extern FLOAT32			FieldOfView;		// FOV in degrees
extern FLOAT32			HorizontalFieldOfView;

extern LOGICAL			bVehicleMirror;		// Render vehicle mirror(s)

extern LOGICAL			bNoFrameRunahead;	// Insure only 2 frame 3d render pipeline
extern LOGICAL			g_enableVSync;	    // Lock the frame rate to refresh rate

extern SINT32			ShadowSizeSelect;	// A number representing shadow texture size											// (1-8) means "2^n"
											// (1-8) means "2^n"
											// 0 means "No Shadows"
/**/

extern SINT32           nErrorCode;         // Set to any user specified error code

extern SINT32			TexBpp;
extern SINT32			TexAlpha;

extern UINT32			DamageLevel;

extern FLOAT32			FrameFactor;		// Based on FPS, helper ratio
											// used to adjust object visibility when FPS is low
extern FLOAT32			InvFrameFactor;		// inverse of framefactor

extern FLOAT32			FogFactor;			// Based on Height above ground, helper ratio
											// used to adjust object visibility when height is low

extern SINT32			TotalTextureMemory;

extern FLOAT32			VolumeMultiplier;	// 0.00001 to 1.0
extern FLOAT32			MusicMultiplier;	// 0.00001 to 1.0

extern UINT32			HUDVisible;			// HUD mode; 0 = off, 2 = full on
extern SINT32			FogScaleSelector;	// FogScaleMultiplier selection; 0 = 1.0
extern UINT32			MaxNumberOfSounds;	// Maximum number of playing sounds at once

extern UINT32			SupportedSrcBlendModes;
extern UINT32			SupportedDestBlendModes;

extern LOGICAL			SuppressMuzzleSmoke;		// Suppresses smoke from muzzle fire
extern LOGICAL          SuppressMuzzleLight;        // Suppresses muzzle light
extern LOGICAL			SuppressTracerSmoke;		// Suppresses smoke from tracers
extern LOGICAL			SuppressSmallClutter;		// Suppresses creation of objects which are too small for FPS cover
extern LOGICAL			SuppressPlaneClutter;		// Suppress all clutter if your vehicle is a plane
extern LOGICAL			LODFieldOfView;				// Evaluates LOD shifts based on FOV
extern LOGICAL			LODLimitPerFrame;			// Limits number of objects whose LOD is shifted to 'n' per frame

extern UINT32			gIconFriendlyColor;			// Icon color for friendly players
extern UINT32			gIconEnemyColor;			// Icon color for enemy players
extern UINT32			gIconSquadColor;			// Icon color for squad mates
extern UINT32			gIconMissionColor;          // Icon color for mission mates
extern UINT32			gIconMissionLeaderColor;    // Icon color for mission leader
extern UINT32			gIconNoobColor;             // Icon color for new players

extern UINT32			gLanguage;					// Language used for localization

extern SINT32 			FoliageMatId;				// Foliage material ID is special; for tree lines

extern LOGICAL			SupportDXT1;				// DXT1 Opaque / one-bit alpha
extern LOGICAL			SupportDXT2;				// DXT2 Explicit alpha - Alpha-premultiplied
extern LOGICAL			SupportDXT3;				// DXT3 Explicit alpha
extern LOGICAL			SupportDXT4;				// DXT4 Interpolated alpha - Alpha-premultiplied
extern LOGICAL			SupportDXT5;				// DXT5 Interpolated alpha

extern piMATRIX			IdentityMatrix;

extern LOGICAL			VehicleObjectAnalysis;		// "Gizmos and Whizbangs"
extern LOGICAL			WantDataDir;				// Want to run out of data directory
extern LOGICAL			HasKeyFile;					// Have key file to enable special features
extern LOGICAL			SuperTroop;					// SuperTroop!
extern LOGICAL			SuperFloat;					// SuperFloat!
extern LOGICAL			NoBlitzWorPreload;			// don't preload blitz.wor
extern LOGICAL			NoInfPreload;				// don't preload infantry
extern LOGICAL          g_bBuildMDLS;               // Build MDLS for archives.
extern LOGICAL          g_buildTerrain;             // Build SDMs for archives.
extern LOGICAL          WantRequiredTraining;		// Want The Basic Tutorials to be Required - MJH (DEV-1999: Remove basic graduation tutorial restriction.)

extern LOGICAL          g_log_comm;					// turn on logging for LOG_COMM
extern LOGICAL          g_log_fps;					// turn on logging for LOG_FPS
extern LOGICAL          g_log_weap;					// turn on logging for LOG_WEAP

extern FLOAT32			gFPS;
extern FLOAT32			gLastFrameFPS;


extern LOGICAL			gCellClientActive;
extern LOGICAL          gWaterShadersEnabled;

extern LOGICAL			disableButtonInput, disableAxisInput;
extern LOGICAL			OpenConsole;
extern LOGICAL			SquadInvites;
extern UINT32			OverrideSpawner;
extern UINT32			gRegion; // used for china localizations and other functionality differences
extern LOGICAL			gRagdolls;
extern UINT32			gVCache; // client vehicle caching: 0 = off, 1 = troops only, 2 = troops and vehicles (vehicles only on same mission)
extern LOGICAL			LowMemorySystem;			// user specified low memory
extern FLOAT32			gTrooperLOD; // LOD scaler for slower systems
extern LOGICAL          EnableMouseClickToDamageBuildings;
extern FLOAT32			gTrooperMOD; // LOD scaler mod from preferences
extern LOGICAL          gCustomTerrain;
extern UINT32			gEZMode;

extern UINT32			Convergence; // Global (default) Airplane gun convergence setting
extern UINT32			CurrentConv; // Current airplane gun convergence setting

extern FLOAT32			aimMouse;	// Aiming mouse sensitivity
extern SINT8            gAuthServer[128]; // Authentication server to be used
extern SINT8            gAuthServerPort[64];
extern char             gAuthRelayURL[128]; // Auth relay URL override
extern SINT8            gArena[8]; // Arena to use, 1 = Live, 22 = Training, 222 = Beta, 250 = Dev
extern SINT8            gClientV[32]; // Version to submit

#ifdef DEBUG_WW2
extern LOGICAL          DebugBuildingMDLFilesOnly;
extern LOGICAL          gDisableMouseClipping; // No ClipCursor() on Windows. Mac doesn't do this.
extern LOGICAL          gDisableMouseHiding; // Don't hide the mouse while the game is frontmost. Mac doesn't need this (LLDB is smarter than VS)
extern LOGICAL          gDisableGameTopmost; // Use HWND_TOP instead of HWND_TOPMOST in SetWindowPos() on Windows. On by default when debugging.
#endif

#if platOS == OS_MACOS
extern LOGICAL          MacNoKeychainUsage;
#endif

#ifdef BETA_RELEASE
extern LOGICAL			gCheckPlanarPlacement;
extern UINT32			gPersDecalOverride;
extern UINT32			gUnitDecalOverride;
extern UINT32			gr2DrawFlags;
#endif	// #ifdef BETA_RELEASE

static inline bool IN_COMBAT(SINT32 t) { return (frametime - combattime) <= t; }

#endif
