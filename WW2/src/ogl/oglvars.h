#pragma once

// Variables
extern UINT32		gMaxUserClipPlanes;
extern FLOAT32		gMaxPointSize;
extern SINT32		gMaxLights;
extern SINT32		gMaxTextureWidth;
extern SINT32		gMaxTextureHeight;
extern UINT32		gSquareTextures;
extern LOGICAL		gRenderToTexture;
extern SINT32		gMaxTextureUnits;
extern LOGICAL		gTextureCompression;
extern LOGICAL		gMipmapEnable;

// Supported blend mode flags
extern UINT32		gSupportedDstBlendModes;
extern UINT32		gSupportedSrcBlendModes;

// Compression function flags
extern LOGICAL		gSupportDXT1;
extern LOGICAL		gSupportDXT1A;
extern LOGICAL		gSupportDXT3;
extern LOGICAL		gSupportDXT5;

// OpenGL major and minor version information
extern UINT32		gOpenGLMajorVersion;
extern UINT32		gOpenGLMinorVersion;

// Swap interval functionality
extern UINT32		gSupportSwapInterval;
