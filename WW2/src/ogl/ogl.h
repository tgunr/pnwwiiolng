#pragma once

	#include "pi/pi.h"
	#include "gl/glew.h"
	#include "gl/glplat.h"
#include "ogl/ogldefs.h"
#include "math/Matrixf.h"

	typedef LOGICAL (*DRAW_OBJECTS)(void);

	// We need version 1.3 or better
	// Settle for 1.1 for now until Apple extensions are working
	#define OPENGL_MAJOR_VERSION	1
	#define OPENGL_MINOR_VERSION	1

	// flags to indicate mode of operation
	#define DDWND_FULLSCREEN    0x00000001
	#define DDWND_WINDOW        0x00000002
	#define DDWND_CAPTION       0x00000004
	#define DDWND_SYSMENU       0x00000008
	#define DDWND_SIZEABLE      0x00000010
	#define DDWND_FULLWINDOW    (DDWND_WINDOW|DDWND_CAPTION|DDWND_SYSMENU|DDWND_SIZEABLE)

#define MAX_TEXTURE_UNITS   7

	// Module specific variables
	#include "ogl/oglvars.h"

	// Prototypes
	extern LOGICAL oglSetupRC(void);
	extern void	oglResize(GLsizei w, GLsizei h);
	//extern LOGICAL oglDrawToScreen(SINT32 texid, LOGICAL stretch);

	extern LOGICAL	oglInit(void);
	extern LOGICAL	oglClearPorts(piRECT rect, UINT32 targets);
	extern LOGICAL	oglClearTargets(UINT32 targets);
	extern LOGICAL	oglIsTexture(SINT32 texid, SINT32 miplevel);
	extern LOGICAL oglIsTexture3D(SINT32 texid);
	extern LOGICAL	oglTextureHasAlpha(SINT32 texid);
	extern LOGICAL	oglAddTexRef(SINT32 texid);
	extern void	oglUpdateTextures(void);
	extern LOGICAL	oglGetTextureMemory(SINT32* total, SINT32* free);
	extern LOGICAL	oglProjectSceneOnTexture(SINT32 texid, SINT32 miplevel, DRAW_OBJECTS pDrawObjects);
	extern LOGICAL	oglReleaseTextureHandles(void);
	extern void oglSetActiveTextureStage(SINT32 stage);
	extern LOGICAL	oglSetActiveTexture(SINT32 stage, SINT32 texid);
	extern LOGICAL oglSetActiveTexture3D(SINT32 stage, SINT32 texid);
	extern void	oglDisableTexture();
    extern void	oglEnableTexture(GLenum tgt = GL_TEXTURE_2D);
	extern void	oglSetMatrixMode(GLenum mode);
	extern LOGICAL oglDisableTexture3D(void);
	extern LOGICAL	oglLoadTexture(SINT32 texid, SINT32 miplevel);
	extern LOGICAL	oglGetTextureFactors(SINT32 texid, SINT32 miplevel, FLOAT32* widthfactor, FLOAT32* heightfactor);
	extern LOGICAL	oglIsMaterial(SINT32 matid);
	extern LOGICAL	oglSetActiveMaterial(SINT32 matid);
	extern LOGICAL oglAddMaterialReference(SINT32 matid);
	extern LOGICAL	oglDisableMaterial(void);
	extern LOGICAL	oglGetMaterialTexName(SINT32 matid, char* texname);
	extern LOGICAL	oglSetFieldOfView(FLOAT32 fov, FLOAT32 zmin, FLOAT32 zmax);
	extern LOGICAL	oglGetViewportDimensions(SINT32* x, SINT32* y, SINT32* width, SINT32* height, FLOAT32* znear, FLOAT32* zfar);
	extern LOGICAL	oglSetViewportDimensions(SINT32 x, SINT32 y, SINT32 width, SINT32 height, FLOAT32 znear, FLOAT32 zfar);
	extern LOGICAL oglGetViewportZ(FLOAT32* znear, FLOAT32* zfar);
	extern LOGICAL oglSetViewportZ(FLOAT32 znear, FLOAT32 zfar);
	extern void oglSetRenderState(oglRENDERSTATETYPES StateType, UINT32 State);
	extern LOGICAL	oglDisplayRenderStates(void);
	extern LOGICAL oglReleaseLight(SINT32 numlight);
	extern LOGICAL	oglSetLightState(SINT32 lightNum, UINT32 State);
	extern LOGICAL	oglBeginScene(void);
	extern LOGICAL	oglEndScene(void);
	extern LOGICAL oglGetTextureMemoryDescriptor(SINT32 texid, SINT32 miplevel, oglSURFACEDESC* lpsd);
	extern LOGICAL	oglUpdateRenderStats(oglPRIMITIVETYPE oglt, UINT32 Count);
	extern LOGICAL	oglRenderScene(oglPRIMITIVETYPE oglt, oglVERTEXTYPE oglvt, LPVOID lpVertices, UINT32 VertexCount);
	extern LOGICAL	oglRenderIndexScene(oglPRIMITIVETYPE oglt, oglVERTEXTYPE oglvt, LPVOID lpVertices, UINT32 VertexCount, void*  lpIndices, UINT32 IndexCount);
	extern LOGICAL	oglUpdateRenderIndexStats(int pipt, UINT32 VertexCount, UINT32 IndexCount);
	extern LOGICAL oglSetCockpitViewMatrix(piMATRIX* lpnewview);
	extern LOGICAL	oglSetRenderStageState(UINT32 stage, oglTEXTURESTAGESTATETYPE state, UINT32 value);
	extern LOGICAL	oglMaterialHasSpecular(SINT32 matid);
	extern LOGICAL	oglGetTextureUVAlpha(UINT32* alpha, SINT32 texid, SINT32 miplevel, FLOAT32 u, FLOAT32 v);
	extern LOGICAL	oglSetUnscaledViewMatrix(piMATRIX* lpNewView);
	extern LOGICAL	oglSetColorAtTextureUV(SINT32 texid, SINT32 miplevel, FLOAT32 u, FLOAT32 v, UINT32 color);

	extern SINT32	oglGetNumberVertices(void);
	extern LOGICAL oglGetTextureMemory(SINT32* total, SINT32* free);
	extern SINT32	oglGetTextureMipmapLevel(SINT32 texid);
	extern SINT32	oglGetNamedTexture(const char* name);
	extern SINT32	oglCreateRawTexture(SINT32 texid, SINT32 miplevel, SINT32 width, SINT32 height, SINT32 bpp, SINT32 alphabits, SINT32 flags);
	extern SINT32 oglCreateRawTexture3D(SINT32 texid, SINT32 miplevel, SINT32 width, SINT32 height, SINT32 depth, SINT32 bpp, SINT32 alphabits, SINT32 flags);
	extern SINT32 oglFindMaterialName(const char* name);
	extern LOGICAL oglAddMatRef(SINT32 texid);
	extern SINT32	oglGetMaterialTexId(SINT32 matid);
	extern SINT32	oglCreateMaterial(LPVOID lpmat);
	extern SINT32	oglGetLights(LIGHT** lplight);
	extern LOGICAL oglSetMaterialName(SINT32 matid, const char* name);
	extern SINT32	oglGetMaterials(MATERIAL** lpmat);
	extern SINT32	oglDuplicateRawTexture(SINT32 texid, SINT32 miplevel);
    extern SINT32 oglGetActiveMaterial();

	extern TEXTURE* oglGetTextureData(SINT32 texid);
	extern TEXTURE* oglGetTextureData2(SINT32 texid);
	extern TEXTURE3D* oglGetTexture3DData(SINT32 texid);

    extern SINT32 oglGetTextureHandle(SINT32 texid, SINT32 miplevel);
    extern SINT32 oglGetTexture3DHandle(SINT32 tex3d);

	extern LOGICAL oglLockTexture(SINT32 texid, SINT32 miplevel, oglSURFACEDESC* lpsd);
	extern LOGICAL oglUnlockTexture(SINT32 texid, SINT32 miplevel);

	extern void oglKill(void);
	extern void oglSetMinTextureDim(SINT32 mintexdim);
    extern void oglDisableTexturesAboveAndIncluding(int unit);
	extern void oglUnloadTexture(SINT32 texid, SINT32 miplevel);
	extern void oglUnloadTexture3D(SINT32 tex3d);
	extern void oglFreeTexture(SINT32 texid, SINT32 miplevel);
	extern void oglFreeTexture3D(SINT32 texid);
	extern void oglFreeMipTextures(SINT32 texid);
	extern void oglFreeMipTexture3Ds(SINT32 texid);
	extern void oglResetRenderStats(void);
	extern void oglGetRenderStats(UINT32* points, UINT32* lines, UINT32* triangles, UINT32* vertices);
	extern void oglGetRenderTypes(UINT32* draw, UINT32* drawindex);
	extern void oglResetRenderBuckets(void);
	extern void oglResetRenderIndexBuckets(void);
	extern void oglUpdateRenderBuckets(UINT32 TriangleCount);
	extern void oglUpdateRenderIndexBuckets(UINT32 TriangleCount);
	extern void oglDisplayRenderBuckets(void);
	extern void oglDisplayRenderIndexBuckets();
	extern void oglFreeMaterial(SINT32 matid);
	extern void oglSetAmbLight(oglCOLOR color);
	extern void* oglGetTextureMemorySurface(SINT32 iTexID, SINT32 iMipLevel);
    extern void oglDisableRenderStates();
    extern void oglTogglePostFilter();
    extern void oglPushRenderState(UINT32 state);
    extern void oglPopRenderState(UINT32 state);
    extern void oglLockState(SINT32 state, LOGICAL lock);
    extern void oglLockTextureState(bool lock);
    extern void oglLockMaterialState(LOGICAL lock);
    extern void oglGetPostRenderFilterTexture(GLuint* texid, GLenum* target);
    extern void oglLockWorldMatrix(LOGICAL lock);
    extern void oglLockViewMatrix(LOGICAL lock);
    extern void oglLockUnscaledViewMatrix(LOGICAL lock);
    extern void oglLockCockpitMatrix(LOGICAL lock);
    extern void oglLockProjectionMatrix(LOGICAL lock);
    extern void oglLockTextureMatrix(LOGICAL lock);
    extern void oglCreateModelView(piMATRIX* returnMatrix,
                            piVECTOR* eye,
                            piVECTOR* center,
                            piVECTOR* up,
                            piQUAT* axisangle);

    extern void oglDrawScene(UINT32 mode,
                      FLOAT32* coords,
                      FLOAT32* normals,
                      FLOAT32* texcoords,
                      UINT32* colors,
                      UINT16* indices,
                      SINT32 stride,
                      SINT32 vertexcount,
                      SINT32 indexcount);

	// Vertex Source
	extern SINT32 oglCreateVertexSource(char* name, UINT32 Length, UINT32 vertexformat);
	extern LOGICAL oglIsVertexSource(SINT32 vertexid);
	extern LOGICAL oglSetVertexSource(SINT32 srcid, UINT32 Stride, UINT32 FVF);
	extern LOGICAL oglResetVertexSource(void);
	extern LOGICAL oglLockVertexSource(SINT32 srcid, UINT32 OffsetToLock, UINT32 SizeToLock, UINT8** ppbData, UINT32 Flags);
	extern LOGICAL oglUnlockVertexSource(SINT32 srcid);
	extern LOGICAL oglReleaseVertexSource(SINT32 srcid);

	// Index Source
	extern SINT32 oglCreateIndexSource(char* name, UINT32 Length, UINT32 indexformat);
	extern LOGICAL oglIsIndexSource(SINT32 srcid);
	extern LOGICAL oglSetIndexSource(SINT32 srcid, UINT32 base);
	extern LOGICAL oglResetIndexSource(void);
	extern LOGICAL oglLockIndexSource(SINT32 srcid, UINT32 OffsetToLock, UINT32 SizeToLock, UINT8** ppbData, UINT32 Flags);
	extern LOGICAL oglUnlockIndexSource(SINT32 srcid);
	extern LOGICAL oglReleaseIndexSource(SINT32 srcid);

	extern UINT32 oglColorToDescColor(LPVOID lpddsd, UINT32 color);
    extern UINT32 oglGetEngineRenderState(UINT32 state);
    extern UINT32 oglGetEngineRenderStageState(UINT32 stage, UINT32 state);

    extern SINT32 oglGetTextureUVColor(UINT32* pColor,
                                SINT32 iTexID,
                                SINT32 iMipLevel,
                                FLOAT32 u,
                                FLOAT32 v);

	extern oglCOLOR	oglGetAmbLight(void);

    extern void oglGetModelViewProjectionMatrix(Matrixf& mvp);

    extern const piMATRIX& oglGetModelViewMatrix();
	extern piMATRIX* oglGetProjectionMatrix(void);
	extern piMATRIX* oglGetWorldMatrix(void);
	extern piMATRIX* oglGetViewMatrix(void);
	extern piMATRIX* oglGetCockpitViewMatrix(void);
	extern piMATRIX* oglGetUnscaledViewMatrix(void);
	extern FLOAT32 oglGetFieldOfViewZMin();
	extern FLOAT32 oglGetFieldOfViewZMax();
	extern LOGICAL oglGetWorldToScreen(double x, double y, double z, double& sx, double& sy, double& sz);
    extern bool oglGetScreenToWorld(int sx, int sy, double& x, double& y, double& z);

	extern piMATRIX& oglGetTextureMatrix(int unit);
	extern LOGICAL oglRenderIndexStridedScene(oglPRIMITIVETYPE oglt, oglVERTEXTYPE oglvt, LPVOID lpStridedData, UINT32 VertexCount, void*  lpIndices, UINT32 IndexCount);
	extern LOGICAL	oglRenderStridedScene(oglPRIMITIVETYPE pipt, oglVERTEXTYPE pivt, LPVOID lpStridedData, UINT32 VertexCount);
	extern LOGICAL oglSetTextureName(SINT32 texid, const SINT8* name);

	extern LOGICAL oglSetLight(SINT32 numlight, LIGHT* light);
	extern SINT32 oglCreateLight(piLIGHT* light);

    extern SINT32 oglGetNewTextureID();
    extern bool oglCanUseTexture2D(int w, int h);
    extern GLenum oglGetMatrixMode();
    extern void oglAddMaterialName(const char* name, int id);
    extern unsigned int oglGetTextureName(int id);
    extern const piMATERIAL& oglGetMaterial(int id);
    extern void oglSetViewMatrix(const piMATRIX* newview);
    extern void oglSetViewMatrix(const Matrixf& newview);
    extern void oglSetWorldMatrix(const piMATRIX* newworld);
    extern void oglSetWorldMatrix(const Matrixf& neworld);
    extern void oglSetProjectionMatrix(const piMATRIX* newproj);
    extern void oglSetProjectionMatrix(const Matrixf& newproj);
    extern void oglSetTextureMatrix(const piMATRIX* newtexmat);
    extern void oglSetTextureMatrix(const Matrixf& newtexmat);
    extern void oglSetOrthoProjectionMatrix(float r, float l, float t, float b, float znear, float zfar);
    extern const Matrixf& oglGetWaterReflectionCamera();
    extern void oglSetWaterReflectionCamera(float height);
    extern void oglSetViewport(int left, int right, int width, int height);
    extern void oglLockViewport(bool lock);

