// vars.h

#ifndef vars_HEADER

	#define vars_HEADER

#ifdef __cplusplus
extern "C"
{
#endif

	#include "resource.h"
	#include "systypes.h"

	#undef EXTERN
	#ifndef  GLOBALS
    #define EXTERN extern
	#else
    #define EXTERN
	#endif

	#define PI    3.14159265359f
	#define PI2   6.28318530718f

	// These are configuration variables that are filled
	// from the .CFG file. Corresponding information
	// should be included in CONFIG.C structures.
	EXTERN LOGICAL          bZEnable;          // Z buffer is on
	EXTERN LOGICAL          bZWrite;           // Z buffer write enable
	EXTERN UINT32           ZFunction;         // Z compare function
	EXTERN LOGICAL          bTexturePerspective;// perspective correction is on
	EXTERN LOGICAL          bMipmapEnable;     // mipmapping?
	EXTERN LOGICAL          bMipmapSoftware;   // mipmap using software?
	EXTERN FLOAT32          MipmapDistance;    // mipmap distance
	EXTERN UINT32           SrcBlend;          // source blend mode
	EXTERN UINT32           DestBlend;         // destination blend mode
	EXTERN LOGICAL          bPerFrameBSP;      // generate BSP sorting tree per frame
	EXTERN SINT32           MinTextureDim;     // minimum texture width/height
	EXTERN UINT32			ShadeMode;         // flat, gouraud, phong?
	EXTERN UINT32			TextureMagFilter;  // linear or bi-linear texture filter
	EXTERN UINT32			TextureMinFilter;  // linear or bi-linear texture filter
	EXTERN UINT32			TextureBlend;      // Use shade mode or copy mode?
	EXTERN UINT32			FillMode;          // solid, lines or points?
	EXTERN LOGICAL          bDithering;        // dithering is on
	EXTERN LOGICAL          bSpecular;         // specular highlights are on
	EXTERN UINT32			Antialiasing;      // antialiasing mode

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
	EXTERN piVECTOR         deltapos;          // player delta position
	EXTERN piVECTOR         deltarot;          // player delta rotation

	EXTERN SINT32           frametime;         // time (in ticks) of start of present frame
	EXTERN SINT32           lastframetime;     // time (in ticks) of start of last frame
	EXTERN SINT32           ft1000mscount;     // number of 1000ms intervals since start
	EXTERN SINT32           ft1000msticks;     // number of 1000ms intervals since last frame
	EXTERN SINT32           ft500mscount;      // number of 500ms intervals since start
	EXTERN SINT32           ft500msticks;      // number of 500ms intervals since last frame
	EXTERN SINT32           ft333mscount;      // number of 333ms intervals since start
	EXTERN SINT32           ft333msticks;      // number of 333ms intervals since last frame
	EXTERN SINT32           ft250mscount;      // number of 250ms intervals since start
	EXTERN SINT32           ft250msticks;      // number of 250ms intervals since last frame
	EXTERN SINT32           ft100mscount;      // number of 100ms intervals since start
	EXTERN SINT32           ft100msticks;      // number of 100ms intervals since last frame
	EXTERN SINT32           ft50mscount;       // number of 50ms intervals since start
	EXTERN SINT32           ft50msticks;       // number of 50ms intervals since last frame
	EXTERN SINT32           ft10mscount;       // number of 10ms intervals since start
	EXTERN SINT32           ft10msticks;       // number of 10ms intervals since last frame

	EXTERN LOGICAL          WantedHelp;			// asked for commandline help
	EXTERN SINT32           WantedWidth;		// desired/actual width
	EXTERN SINT32           WantedHeight;		// desired/actual height
	EXTERN SINT32           WantedBpp;			// desired/actual bpp
	EXTERN SINT32           WantedAlpha;		// desired/actual alpha
	EXTERN LOGICAL          WantedHdw;			// desired/actual hardware
	EXTERN LOGICAL          WantedWindow;		// desired/actual window
	EXTERN SINT32			WantedMode;			// desired/actual mode

	EXTERN LOGICAL			PlayDemo;			// demo mode
	EXTERN LOGICAL			StaticLighting;		// static lighting model
	EXTERN LOGICAL			DynamicLighting;	// dynamic lighting model

	EXTERN FLOAT32			ScaleFactor;
	EXTERN FLOAT32			InvScaleFactor;

	EXTERN LOGICAL			Online; 			// network connection made
	EXTERN LOGICAL			InFlight; 			// no where but in the air

	EXTERN LOGICAL			bAnimatedWater;		// animate water surface with waves

	#ifdef GLOBALS
	EXTERN SINT8			DataPath[_MAX_PATH]=PREPATH;
	#else
	extern SINT8			DataPath[_MAX_PATH];
	#endif

	/**/

	EXTERN SINT32			TexBpp;
	EXTERN SINT32			TexAlpha;

	#ifdef SCREENSAVER
	EXTERN LOGICAL			SS_pwd;
	EXTERN LOGICAL			SS_cfg;
	EXTERN HWND				SS_hwnd;
	EXTERN LOGICAL			SS_run;
	#endif

	#ifdef GLOBALS
	piMATRIX	identity =	{
							1.0f, 0.0f, 0.0f, 0.0f,
							0.0f, 1.0f, 0.0f, 0.0f,
							0.0f, 0.0f, 1.0f, 0.0f,
							0.0f, 0.0f, 0.0f, 1.0f
							};
	#else
	extern piMATRIX	identity;
	#endif

#ifdef __cplusplus
}
#endif

#endif
