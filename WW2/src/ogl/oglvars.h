#pragma once

#undef EXTERN
#ifdef GLOBALS
	#define EXTERN
#else
	#define EXTERN	extern
#endif

// Variables
EXTERN UINT32		gMaxUserClipPlanes;
EXTERN FLOAT32		gMaxPointSize;
EXTERN SINT32		gMaxLights;
EXTERN SINT32		gMaxTextureWidth;
EXTERN SINT32		gMaxTextureHeight;
EXTERN UINT32		gSquareTextures;
EXTERN LOGICAL		gRenderToTexture;
EXTERN SINT32		gMaxTextureUnits;
EXTERN LOGICAL		gTextureCompression;
EXTERN LOGICAL		gMipmapEnable;

// Supported blend mode flags
EXTERN UINT32		gSupportedDstBlendModes;
EXTERN UINT32		gSupportedSrcBlendModes;

// Compression function flags
EXTERN LOGICAL		gSupportDXT1;
EXTERN LOGICAL		gSupportDXT1A;
EXTERN LOGICAL		gSupportDXT3;
EXTERN LOGICAL		gSupportDXT5;

// OpenGL major and minor version information
EXTERN UINT32		gOpenGLMajorVersion;
EXTERN UINT32		gOpenGLMinorVersion;

// Swap interval functionality
EXTERN UINT32		gSupportSwapInterval;
